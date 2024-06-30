/*
 * weburl-nfq - url filter daemon
 ********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
 * Copyright (C) 2017 Technicolor Delivery Technologies, SAS
 * ** All Rights Reserved   
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation
 *
 */
#define _LGPL_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <errno.h>
#include <linux/netfilter.h>
#include <linux/netfilter/nf_conntrack_tcp.h>
#include <libmnl/libmnl.h>
#include <libnetfilter_conntrack/libnetfilter_conntrack.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <libnetfilter_queue/pktbuff.h>
#include <libnetfilter_queue/libnetfilter_queue_tcp.h>
#include <libnetfilter_queue/libnetfilter_queue_ipv4.h>
#include <libnetfilter_queue/libnetfilter_queue_ipv6.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <urcu.h>
#if PCAP_DEBUG
#include <pcap.h>
#endif

#include "urlfilterd.h"
#include "config.h"
#include "nfq.h"

#define BUFSIZE 2048

static pthread_t nfq = 0;
void *nfq_thread(void *);
int spawn_nfq_thread();
static struct nfq_handle *h = NULL;
static struct nfq_q_handle *qh = NULL;
static struct mnl_socket *nl = NULL;
unsigned int portid;
static int exit_pipe[2];
enum decision {
	PKT_FLOW_IGNORE,
	PKT_FLOW_MORE,
	PKT_REDIRECT
};


#if PCAP_DEBUG
static void dump_pcap(char *packet, int packet_len, int flush)
{
	struct pcap_pkthdr hdr;
	struct timespec time;

	hdr.caplen = hdr.len = packet_len;
	clock_gettime(CLOCK_REALTIME, &time);
	hdr.ts.tv_sec = time.tv_sec;
	hdr.ts.tv_usec = time.tv_nsec / 1000;
	rcu_read_lock();
	struct config *cfg = rcu_dereference(active_config);
	pcap_dump((u_char *)cfg->pd, &hdr, (const unsigned char *)packet);
	if (flush)
		pcap_dump_flush(cfg->pd);
	rcu_read_unlock();
}
#else
static inline void dump_pcap(char _unused *packet, int _unused packet_len, int _unused flush)
{
}
#endif


static int get_payload_offset(int is_v4, const char *pkt, const struct tcphdr const **r_tcp)
{
	int payload_offset;
	const struct tcphdr *tcp;

	if (is_v4) {
		const struct iphdr *iph;

		iph = (const struct iphdr *)pkt;
		tcp = (const struct tcphdr *)(pkt + (iph->ihl<<2));
		payload_offset = ((iph->ihl)<<2) + (tcp->doff<<2);
		*r_tcp = tcp;
		return payload_offset;
	} else {
		const struct ip6_hdr *ip6h;
		const struct ip6_ext *ip_ext_p;
		uint8_t nextHdr;
		int count = 8;

		ip6h = (const struct ip6_hdr *)pkt;
		nextHdr = ip6h->ip6_nxt;
		ip_ext_p = (const struct ip6_ext *)(ip6h + 1);
		payload_offset = sizeof(struct ip6_hdr);

		do
		{
			if ( nextHdr == IPPROTO_TCP )
			{
				tcp = (struct tcphdr *)ip_ext_p;
				*r_tcp = tcp;
				payload_offset += tcp->doff << 2;
				return payload_offset;
			}
			if ( nextHdr == IPPROTO_NONE )
				break;

			payload_offset += (ip_ext_p->ip6e_len + 1) << 3;
			nextHdr = ip_ext_p->ip6e_nxt;
			ip_ext_p = (struct ip6_ext *)(pkt + payload_offset);
			count--; /* at most 8 extension headers */
		} while(count);

	}
	return -1;
}

static char *craft_reply_v4(struct config *cfg, int use_redirect, char *packet, int packet_len, int *new_len)
{
	struct iphdr *iph;
	struct tcphdr *tcp;
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	struct nfgenmsg *nfh;
	struct nf_conntrack *ct;

	*new_len = (use_redirect ? cfg->redirect_len : 0) + sizeof(struct iphdr) + sizeof(struct tcphdr);
	char *newpacket = malloc(*new_len);
	unsigned int ihl = ((struct iphdr *)packet)->ihl<<2;
	memcpy(newpacket, packet, ihl + sizeof(struct tcphdr));

	if (use_redirect)
		memcpy(&newpacket[ihl + sizeof(struct tcphdr)], cfg->redirect, cfg->redirect_len);

	iph = (struct iphdr *)newpacket;
	tcp = (struct tcphdr *)(newpacket + (iph->ihl<<2));

	/* clear conntrack entry as this reply packet would be dropped
	 * otherwise */
	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type = (NFNL_SUBSYS_CTNETLINK << 8) | IPCTNL_MSG_CT_DELETE;
	nlh->nlmsg_flags = NLM_F_REQUEST /* |NLM_F_ACK */;
	nlh->nlmsg_seq = time(NULL);

	nfh = mnl_nlmsg_put_extra_header(nlh, sizeof(struct nfgenmsg));
	nfh->nfgen_family = AF_INET;
	nfh->version = NFNETLINK_V0;
	nfh->res_id = 0;

	ct = nfct_new();
	if (ct != NULL) {
		nfct_set_attr_u8(ct, ATTR_L3PROTO, AF_INET);
		nfct_set_attr_u32(ct, ATTR_IPV4_SRC, iph->saddr);
		nfct_set_attr_u32(ct, ATTR_IPV4_DST, iph->daddr);

		nfct_set_attr_u8(ct, ATTR_L4PROTO, IPPROTO_TCP);
		nfct_set_attr_u16(ct, ATTR_PORT_SRC, tcp->source);
		nfct_set_attr_u16(ct, ATTR_PORT_DST, tcp->dest);

		nfct_nlmsg_build(nlh, ct);

		if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) == -1) {
			syslog(LOG_DEBUG, "nfct DEL failed: %s", strerror(errno));
		}
	}

	iph->id = htons(ntohs(iph->id) + 1);
	uint32_t saddr = iph->saddr;

	iph->saddr = iph->daddr;
	iph->daddr = saddr;
	iph->tot_len = htons(*new_len);
	nfq_ip_set_checksum(iph);

	/* TCP data offset, fixed since we don't want TCP Options to be taken
	 * from the original packet */

	/* swap ports around */
	uint16_t source = tcp->source;
	tcp->source = tcp->dest;
	tcp->dest = source;

	/* swap sequence numbers */
	uint32_t old_seq = ntohl(tcp->seq);
	tcp->seq = tcp->ack_seq;
	tcp->ack_seq = htonl( old_seq + (packet_len - ihl - (tcp->doff << 2)) );
	tcp->doff = 5;
	tcp->res1 = 0;
	tcp->psh = 1;
	tcp->ack = 1;
	tcp->fin = 1;

	nfq_tcp_compute_checksum_ipv4(tcp, iph);

	return newpacket;
}

static uint16_t _checksum(uint32_t sum, uint16_t *buf, int size)
{
	while (size > 1) {
		sum += *buf++;
		size -= sizeof(uint16_t);
	}
	if (size) {
#if __BYTE_ORDER == __BIG_ENDIAN
		sum += (uint16_t)*(uint8_t *)buf << 8;
#else
		sum += (uint16_t)*(uint8_t *)buf;
#endif
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >>16);

	return (uint16_t)(~sum);
}


/*
 * Temporary : libnetfilter_queue's implementation of checksum_tcpudp_ipv6 is
 * completely broken. This is the fixed version.
 * Should make a patch for libnetfilter_queue and send upstream.
 */
static uint16_t _checksum_tcpudp_ipv6(struct ip6_hdr *ip6h, void *transport_hdr, uint16_t protocol_id)
{
	uint32_t sum = 0;
	uint32_t hdr_len = (uint8_t *)transport_hdr - (uint8_t *)ip6h;
	uint32_t len = ntohs(ip6h->ip6_plen);
	uint8_t *payload = (uint8_t *)ip6h + hdr_len;
	int i;

	for (i=0; i<8; i++) {
		sum += ip6h->ip6_src.s6_addr16[i];
		sum += ip6h->ip6_dst.s6_addr16[i];
	}
	sum += htons(protocol_id);
	sum += ip6h->ip6_plen;

	return _checksum(sum, (uint16_t *)payload, len);
}

static char *craft_reply_v6(struct config *cfg, int use_redirect, char *packet, int packet_len, int *new_len)
{
	struct ip6_hdr *ip6h;

	struct tcphdr *tcp;
	char buf[MNL_SOCKET_BUFFER_SIZE*2];
	struct nlmsghdr *nlh;
	struct nfgenmsg *nfh;
	struct nf_conntrack *ct;
	const struct ip6_ext *ip_ext_p;
	int payload_offset;
	int payload_len;

	*new_len = (use_redirect ? cfg->redirect_len : 0) + sizeof(struct ip6_hdr) + sizeof(struct tcphdr);
	char *newpacket = malloc(*new_len);
	unsigned int ihl = sizeof(struct ip6_hdr);

	/* copy IPv6 Header */
	memcpy(newpacket, packet, sizeof(struct ip6_hdr));
	ip6h = (struct ip6_hdr *)newpacket;

	/* copy TCP header */
	payload_offset = get_payload_offset(false, packet, (const struct tcphdr **)&tcp);
	memcpy(&newpacket[sizeof(struct ip6_hdr)], tcp, sizeof(struct tcphdr));
	ip6h->ip6_nxt = IPPROTO_TCP;
	ip_ext_p = (const struct ip6_ext *)(ip6h + 1);
	tcp = (struct tcphdr *)ip_ext_p;

	if (use_redirect)
		memcpy(&newpacket[ihl + sizeof(struct tcphdr)], cfg->redirect, cfg->redirect_len);

	/* clear conntrack entry as this reply packet would be dropped
	 * otherwise */
	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type = (NFNL_SUBSYS_CTNETLINK << 8) | IPCTNL_MSG_CT_DELETE;
	nlh->nlmsg_flags = NLM_F_REQUEST /* |NLM_F_ACK */;
	nlh->nlmsg_seq = time(NULL);

	nfh = mnl_nlmsg_put_extra_header(nlh, sizeof(struct nfgenmsg));
	nfh->nfgen_family = AF_INET6;
	nfh->version = NFNETLINK_V0;
	nfh->res_id = 0;

	ct = nfct_new();
	if (ct != NULL) {
		nfct_set_attr_u8(ct, ATTR_L3PROTO, AF_INET6);
		nfct_set_attr(ct, ATTR_ORIG_IPV6_SRC, &ip6h->ip6_src);
		nfct_set_attr(ct, ATTR_ORIG_IPV6_DST, &ip6h->ip6_dst);

		nfct_set_attr_u8(ct, ATTR_L4PROTO, IPPROTO_TCP);
		nfct_set_attr_u16(ct, ATTR_PORT_SRC, tcp->source);
		nfct_set_attr_u16(ct, ATTR_PORT_DST, tcp->dest);

		nfct_nlmsg_build(nlh, ct);

		if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) == -1) {
			syslog(LOG_DEBUG, "nfct DEL failed: %s", strerror(errno));
		}
	}

	struct in6_addr ip6_src = ip6h->ip6_src;

	ip6h->ip6_src = ip6h->ip6_dst;
	ip6h->ip6_dst = ip6_src;
	ip6h->ip6_plen = htons(*new_len - sizeof(struct ip6_hdr));

	/* clear flowlabel, this variable is
	 * 4 bits version, 8 bits TC, 20 bits flow-ID */
	ip6h->ip6_flow &= htonl(~0xFFFFF);

	/* TCP data offset, fixed since we don't want TCP Options to be taken
	 * from the original packet */

	/* swap ports around */
	uint16_t source = tcp->source;
	tcp->source = tcp->dest;
	tcp->dest = source;

	payload_len = packet_len - payload_offset;

	/* swap sequence numbers */
	uint32_t old_seq = ntohl(tcp->seq);
	tcp->seq = tcp->ack_seq;
	tcp->ack_seq = htonl( old_seq + payload_len );
	tcp->doff = 5;
	tcp->res1 = 0;
	tcp->psh = 1;
	tcp->ack = 1;
	tcp->fin = 1;

	tcp->check = 0;
	tcp->check = _checksum_tcpudp_ipv6(ip6h, tcp, IPPROTO_TCP);
	return newpacket;
}

/*
 * Find the first occurrence of find in s, where the search is limited to the
 * first slen characters of s.
 */
static char *strnstr(const char *s, const char *find,
		     size_t slen, size_t findlen)
{
	char c, sc;

	if (find == NULL || findlen <= 0)
		return (NULL);

	if ((c = *find++) != '\0') {
		findlen--;
		do {
			do {
				if (slen-- <= 0 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (findlen == 0)
				break;
			else if (findlen > slen)
				return (NULL);
		} while (strncmp(s, find, findlen) != 0);
		s--;
	}
	return ((char *)s);
}

/*
 * Find the first occurrence of find in s, where the search is limited to the
 * first slen characters of s.
 */
static char *strncasestr(const char *s, const char *find,
			 size_t slen, size_t findlen)
{
	char c, sc;

	if (find == NULL || findlen <= 0)
		return (NULL);

	if ((c = *find++) != '\0') {
		findlen--;
		do {
			do {
				if (slen-- <= 0 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (findlen == 0)
				break;
			else if (findlen > slen)
				return (NULL);
		} while (strncasecmp(s, find, findlen) != 0);
		s--;
	}
	return ((char *)s);
}

/*
 * Examine packet and return a decision. This function runs with a read lock
 * on the config structure, so is free to access the config.
 */
static enum decision pkt_decision(struct config *cfg, struct nfq_q_handle *qh,
				  struct nfq_data *nfa, uint32_t id,
				  int *verdict_result)
{
	int packet_len;
	char *packet;
	struct iphdr *iph;
	int is_v4, is_tls = 0;
	int payload_offset;
	int payload_len;
	int host_len;
	const char *match;
	const char *host, *host_end;
	struct urlfilter *filter;
	enum decision rv = PKT_FLOW_IGNORE;
	uint32_t mark;
	char *newpacket;
	int new_len;
	int i;
	const struct tcphdr *tcp;

	packet_len = nfq_get_payload(nfa, (unsigned char **)&packet);
	if (packet_len < 0)
		return PKT_FLOW_MORE;

	dump_pcap(packet, packet_len, 0);

	iph = (struct iphdr *)packet;
	is_v4 = (iph->version == 4)?1:0;

	/* IPv6 packet and IPv6 is disabled */
	if (is_v4 == 0 && cfg->ipv6_enabled == 0)
		return PKT_FLOW_IGNORE;

	payload_offset = get_payload_offset(is_v4, packet, &tcp);
	if (payload_offset < 0)
		return PKT_FLOW_MORE;

	payload_len = packet_len - payload_offset;
	if (payload_len == 0)
		return PKT_FLOW_MORE;

	match = (const char *)(packet + payload_offset);

	/* packet from HTTP server to client */
	if (tcp->source == htons(80)) {
		struct keywordfilter *filter;

		/* no keyword filter, means we're not interested in looking at it */
		if (!cfg->keywordfilter_enabled)
			return PKT_FLOW_MORE;

		/* check keywords */
		list_for_each_entry(filter, &cfg->keywordfilters, head) {
			if (strncasestr(match, filter->keyword,
					(size_t)payload_len, filter->keyword_len)) {
				mark = nfq_get_nfmark(nfa);
				mark |=	cfg->redirected_mark;
				*verdict_result = nfq_set_verdict2(qh, id, NF_REPEAT, mark, 0, NULL);
				return PKT_FLOW_IGNORE;
			}
		}

		return PKT_FLOW_MORE;
	}

	if (payload_len > 5 /* longer than a TLS record header */ && match[0] == 22 /* ContentType == handshake */) {
		unsigned off, len, field_type, field_len;

		is_tls = 1;

		/* filter out non-TLS packets and other records than Client Hello handshake */
		len = ((unsigned)match[3] << 8) + (unsigned)match[4];
		off = 5;
		if (len > payload_len - off)
			len = payload_len - off;
		if (match[off] != 1 /* HandshakeType != client_hello */ ||
			len <= 1 /* handshake type */ + 3 /* length */ + 2 /* version */ + 32 /* random struct */)
			return PKT_FLOW_MORE;
		len = ((unsigned)match[off + 1] << 16) + ((unsigned)match[off + 2] << 8) + (unsigned)match[off + 3];
		off += 1 /* handshake type */ + 3 /* length */;
		if (len > payload_len - off)
			len = payload_len - off;

		/* skip version and random struct */
		if (len <= 2 /* version */ + 32 /* random struct */)
			return PKT_FLOW_MORE;
		len -= 2 /* version */ + 32 /* random struct */;
	        off += 2 /* version */ + 32 /* random struct */;

		/* skip session_id */
		field_len = (unsigned)match[off];
		if (len < 1 + field_len + 2 /* cipher_suites length */)
			return PKT_FLOW_MORE;
		len -= 1 + field_len;
		off += 1 + field_len;

		/* skip cipher_suites */
		field_len = ((unsigned)match[off] << 8) + (unsigned)match[off + 1];
		if (len < 2 + field_len + 1 /* compression_methods length */)
			return PKT_FLOW_MORE;
		len -= 2 + field_len;
		off += 2 + field_len;

		/* skip compression_methods */
		field_len = (unsigned)match[off];
		if (len < 1 + field_len + 2 /* extensions length */)
			return PKT_FLOW_MORE;
		len -= 1 + field_len;
		off += 1 + field_len;

		/* jump to extensions beginning */
		len = ((unsigned)match[off] << 8) + (unsigned)match[off + 1];
		off += 2;
		if (len > payload_len - off)
			len = payload_len - off;

		/* parse extensions until Server Name Indication (RFC 6066 section 3) is found */
		host = NULL;
		host_len = 0;
		while (len > 4 /* extension type and length */) {
			field_type = ((unsigned)match[off] << 8) + (unsigned)match[off + 1];
			field_len = ((unsigned)match[off + 2] << 8) + (unsigned)match[off + 3];
			len -= 4;
			off += 4;

			if (len < field_len)
				break;

			if (field_type == 0 /* type == server_name */) {
				len = field_len;
				if (len <= 2 /* list length */ + 3 /* server name type and length */)
					break;

				field_len = ((unsigned)match[off] << 8) + (unsigned)match[off + 1];
				if (len < 2 + field_len)
					break;
				len = field_len;
				off += 2;

				while (len > 3 /* server name type and length */) {
					field_type = (unsigned)match[off];
					field_len = ((unsigned)match[off + 1] << 8) + (unsigned)match[off + 2];
					len -= 3;
					off += 3;

					if (len < field_len)
						break;

					if (field_type == 0 /* type == host_name */) {
						host = match + off;
						host_len = field_len;
						break;
					}

					len -= field_len;
					off += field_len;
				}
				break;
			}

			len -= field_len;
			off += field_len;
		}

		/* if Server Name Indication not found, bail out */
		if (!host_len)
			return PKT_FLOW_MORE;
	} else {
		/* filter out non-HTTP packets and other methods than GET/POST/HEAD */
		if (strnstr(match, "GET ", payload_len, 4) == NULL &&
				strnstr(match, "POST ", payload_len, 5) == NULL &&
				strnstr(match, "HEAD ", payload_len, 5) == NULL)
			return PKT_FLOW_MORE;

		/* find host field in the HTTP header */
		host = strncasestr(match, "\r\nHost:", payload_len, 7);
		if (host == NULL)
			return PKT_FLOW_MORE;
		host += 7;
		while (host - match < payload_len && (host[0] == ' ' || host[0] == '\t'))
			host++;
		host_end = strnstr(host, "\r\n", payload_len - (host - match), 2);
		if (host_end == NULL)
			return PKT_FLOW_MORE;
		host_len = host_end - host;
		while (host_len > 0 && (host[host_len - 1] == ' ' || host[host_len - 1] == '\t'))
			host_len--;
	}

	/* remove trailing dot from the host name */
	if (host_len > 0 && host[host_len - 1] == '.')
		host_len--;

	/* HTTP Request */
	bool found = false;
	struct nfqnl_msg_packet_hw *hw = nfq_get_packet_hw(nfa);
	unsigned int indev = nfq_get_indev(nfa);

	/* check configured filters */
	list_for_each_entry(filter, &cfg->filters, head) {
		const struct iphdr *iph = (const struct iphdr *)packet;
		const struct ip6_hdr *ip6h = (const struct ip6_hdr *)packet;
		const char* site_match;

		/* matching IPv4 or IPv6 source address */
		if ( !(  filter->device.ss_family == AF_UNSPEC
			 ||
			 (is_v4 && filter->device.ss_family == AF_INET &&
			  memcmp( &((struct sockaddr_in *)&filter->device)->sin_addr,
				  &iph->saddr, sizeof(struct in_addr)) == 0)
			 ||
			 (!is_v4 && filter->device.ss_family == AF_INET6 &&
			  memcmp( &((struct sockaddr_in6 *)&filter->device)->sin6_addr,
				  &ip6h->ip6_src, sizeof(struct in6_addr)) == 0)
			 ))
			continue;

		if (filter->macaddress_check) {
			bool check = true;

			for(i = 0; i < ntohs(hw->hw_addrlen); i++) {
				if (filter->hw_addr[i] != hw->hw_addr[i]) {
					check = false;
					break;
				}
			}
			if (check == false)
				continue;
		}

		/* site attribute must be equal with host name or host must be a subdomain of it
		 * (e.g site=google.com should match good.google.com, but not badgoogle.com) */
		site_match = strncasestr(host, filter->site, host_len, filter->site_len);
		if (site_match != NULL &&
			(site_match - host) + filter->site_len == host_len &&
			(site_match == host || *(site_match - 1) == '.')) {
			found = true;
			break;
		}

	}

	/* no match found */
	if (!found) {
		/*
		 * exclude mode.
		 * when this option is 1, the given URL website is inaccessible (black list mode).    -> default ACCEPT
		 * when this option is 0, only the given URL website is accessible (white list mode). -> default REDIRECT
		 */

		/* ignore rest of the stream */
		if (cfg->exclude)
			return PKT_FLOW_IGNORE;

	} else if (filter->action == ACTION_ACCEPT)
		return PKT_FLOW_IGNORE;

	/* craft a redirect reply */
	newpacket = is_v4 ? craft_reply_v4(cfg, !is_tls, packet, packet_len, &new_len) :
			    craft_reply_v6(cfg, !is_tls, packet, packet_len, &new_len);

	if (newpacket) {
		dump_pcap(newpacket, new_len, 1);
		mark = nfq_get_nfmark(nfa);
		mark |=	cfg->redirected_mark;
		mark |=	cfg->stop_processing_mark;
		*verdict_result = nfq_set_verdict2(qh, id, NF_REPEAT, mark, new_len, (const unsigned char *)newpacket);
		free(newpacket);
	} else {
		/* Something went wrong, drop packet instead of redirecting */
		syslog(LOG_DEBUG, "dropping packet, unable to craft reply");
		*verdict_result = nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
	}
	return PKT_REDIRECT;
}

/*
 * This function is TIME CRITICAL! Care should be taken to not perform time
 * and compute intensive operations!
 */
static int handle_packet(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
			 struct nfq_data *nfa, void *data)
{
	struct nfqnl_msg_packet_hdr *ph;
	uint32_t id = 0;
	uint32_t mark;
	int verdict_result;
	int rv;

	(void)nfmsg;
	(void)data;

	ph = nfq_get_msg_packet_hdr(nfa);
	if (ph)	{
		id = ntohl(ph->packet_id);
	}

	rcu_read_lock();
	struct config *cfg = rcu_dereference(active_config);

	switch(pkt_decision(cfg, qh, nfa, id, &verdict_result))
	{
	case PKT_FLOW_IGNORE:
		/* Will re-run the packet through the PREROUTING chain, but
		 * with the NFMARK adjusted. The rules in the chain need to
		 * take care to not present the packet to NFQUEUE again else
		 * this will create an endless loop.
		 */
		mark = nfq_get_nfmark(nfa);
		mark |= cfg->stop_processing_mark;
		rv = nfq_set_verdict2(qh, id, NF_REPEAT, mark, 0, NULL);
		break;

	case PKT_FLOW_MORE:
		/* Accept packet, will skip rest of the PREROUTING chain */
		rv = nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
		break;

	case PKT_REDIRECT:
		rv= verdict_result;
		break;
	}
	rcu_read_unlock();
	return rv;
}

static int cb_err(const struct nlmsghdr *nlh, void *data)
{
	if (debug_level > 0) {
		/* for debugging purposes mostly, will only be called when a netlink
		 * message has 'nlmsg_flags'  with NLM_F_ACK set and an error
		 * happens. Normally we don't ask for an ACK as there's nothing we can
		 * really do anyway.
		 */
		struct nlmsgerr *err = mnl_nlmsg_get_payload(nlh);
		if (err->error != 0)
			syslog(LOG_DEBUG, "message with seq %u has failed: %s\n",
			       nlh->nlmsg_seq, strerror(-err->error));
	}
	return MNL_CB_OK;
}

static mnl_cb_t cb_ctl_array[NLMSG_MIN_TYPE] = {
	[NLMSG_ERROR] = cb_err,
};

void *nfq_thread(void *data)
{
	struct nfq_handle *h = (struct nfq_handle *)data;
	struct nfnl_handle *nh = NULL;
	int fd, mfd;
	char buf[MNL_SOCKET_BUFFER_SIZE];
	int rv;
	fd_set rfds, rfds_init;
	int maxfd=0;

	rcu_register_thread();

	nh = nfq_nfnlh(h);
	fd = nfnl_fd(nh);
	if (fd > maxfd)
		maxfd = fd;

	mfd = mnl_socket_get_fd(nl);
	if (mfd > maxfd)
		maxfd = mfd;

	if (exit_pipe[0] > maxfd)
		maxfd = exit_pipe[0];

	maxfd++;
	FD_ZERO(&rfds_init);
	FD_SET((unsigned int)fd, &rfds_init);
	FD_SET((unsigned int)mfd, &rfds_init);
	FD_SET((unsigned int)exit_pipe[0], &rfds_init);
	syslog(LOG_DEBUG, "nfq_thread entering main loop");
	while(1) {
		rfds = rfds_init;
		rv = select(maxfd, &rfds, NULL, NULL, NULL);
		if (rv <= 0)
			continue;

		/* nfq handler */
		if (FD_ISSET((unsigned int)fd, &rfds)) {
			rv = recv(fd, buf, sizeof(buf), 0);
			if (rv >= 0) {
				// Will do a callback to handle_packet
				nfq_handle_packet(h, buf, rv);
			}
		}

		/* netlink conntrack socket */
		if (FD_ISSET((unsigned int)mfd, &rfds)) {
			rv = mnl_socket_recvfrom(nl, buf, sizeof(buf));
			if (rv >= 0) {
				rv = mnl_cb_run2(buf, rv, 0, portid,
						 NULL , NULL, cb_ctl_array,
						 MNL_ARRAY_SIZE(cb_ctl_array));
			}
		}

		/* signal handler */
		if (FD_ISSET((unsigned int)exit_pipe[0], &rfds))
			break;

	}
	syslog(LOG_DEBUG, "Exiting nfq thread");
	rcu_unregister_thread();
	pthread_exit((void *)0);
	return NULL;
}

int nfq_init(int quenum)
{
	pthread_attr_t attr;
	struct sched_param param = {
		.sched_priority = 1
	};
	int rc= -1;

	/* open a netlink connection to get packet from kernel */
	h = nfq_open();
	if (!h) {
		syslog(LOG_ERR, "Failed to open nfq channel");
		goto out;
	}

	if (nfq_unbind_pf(h, AF_INET) < 0 || nfq_unbind_pf(h, AF_INET6) < 0) {
		syslog(LOG_ERR, "error during nfq_unbind_pf()");
		goto out;
	}

	if (nfq_bind_pf(h, AF_INET) < 0 || nfq_bind_pf(h, AF_INET6) < 0) {
		syslog(LOG_ERR, "error during nfq_bind_pf()");
		goto out;
	}

	qh = nfq_create_queue(h, quenum, &handle_packet, NULL);
	if (!qh) {
		syslog(LOG_ERR, "error during nfq_create_queue()");
		goto out;
	}

	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
		syslog(LOG_ERR, "can't set packet_copy mode");
		goto out;
	}

	/*
	 * TODO: should rework the nfq-based API to also use
	 * the mnl API. The we only need 1 socket and we can
	 * pass the NFQUEUE verdict and CONNTRACK update in
	 * 1 kernel send.
	 *
	 * See https://git.netfilter.org/libnetfilter_queue/tree/examples/nf-queue.c
	 * as example of the newer NFQ API.
	 */
	nl = mnl_socket_open(NETLINK_NETFILTER);
	if (nl == NULL) {
		syslog(LOG_ERR, "netlink socket open");
		goto out;
	}

	if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
		syslog(LOG_ERR, "netlink socket bind");
		goto out;
	}
	portid = mnl_socket_get_portid(nl);

	if (pipe2(exit_pipe, O_NONBLOCK | O_CLOEXEC) < 0)
		goto out;

	pthread_attr_init(&attr);
	pthread_attr_setschedparam(&attr, &param);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	if (pthread_create(&nfq, &attr, &nfq_thread, h)) {
		syslog(LOG_ERR, "Failed to start nfq thread");
		goto out;
	}
	rc = 0;
	syslog(LOG_INFO, "nfq_init done");

out:
	return rc;
}

void nfq_end()
{
	char b = 0;

	if (write(exit_pipe[1], &b, sizeof(b)) < 0) {}
	pthread_join(nfq, NULL);

	if (nl) {
		mnl_socket_close(nl);
		nl = NULL;
	}
	if (qh) {
		nfq_destroy_queue(qh);
		qh = NULL;
	}
	if (h) {
		nfq_close(h);
		h = NULL;
	}
}
