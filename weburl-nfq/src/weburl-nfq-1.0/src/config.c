/*
 * weburl-nfq - url filter daemon
 * Copyright (C) 2017 Technicolor Delivery Technologies, SAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define _LGPL_SOURCE

#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <net/if.h>
#include <netdb.h>
#include <fcntl.h>
#include <syslog.h>

#include <urcu.h>
#include <libubox/blob.h>
#include <libubox/blobmsg.h>
#include <uci.h>
#include <uci_blob.h>

#include "urlfilterd.h"
#include "config.h"

const char *redirect_url = NULL;
struct config *active_config = NULL;
static struct blob_buf b;

enum {
	GENERAL_ATTR_ENABLE,
	GENERAL_ATTR_KEYWORDFILTER_ENABLE,
	GENERAL_ATTR_EXCLUDE,
	GENERAL_ATTR_IPV6_ENABLE,
	GENERAL_ATTR_LAN_INTF,
	GENERAL_ATTR_BLOCKED_PAGE_REDIRECT,
	GENERAL_ATTR_QUEUE,
	GENERAL_ATTR_REDIRECTED_MARK,
	GENERAL_ATTR_STOP_PROCESSING_MARK,
	GENERAL_ATTR_MAX
};

static const struct blobmsg_policy general_attrs[GENERAL_ATTR_MAX] = {
	[GENERAL_ATTR_ENABLE] = { .name = "enable", .type = BLOBMSG_TYPE_BOOL },
	[GENERAL_ATTR_KEYWORDFILTER_ENABLE] = { .name = "keywordfilter_enable", .type = BLOBMSG_TYPE_BOOL },
	[GENERAL_ATTR_EXCLUDE] = { .name = "exclude", .type = BLOBMSG_TYPE_BOOL },
	[GENERAL_ATTR_IPV6_ENABLE] = { .name = "ipv6_enable", .type = BLOBMSG_TYPE_BOOL },
	[GENERAL_ATTR_LAN_INTF] = { .name = "lan_intf", .type = BLOBMSG_TYPE_ARRAY },
	[GENERAL_ATTR_BLOCKED_PAGE_REDIRECT] = { .name = "blocked_page_redirect", .type = BLOBMSG_TYPE_STRING },
	[GENERAL_ATTR_QUEUE] = { .name = "queue", .type = BLOBMSG_TYPE_INT32 },
	[GENERAL_ATTR_REDIRECTED_MARK] = { .name = "redirected_mark", .type = BLOBMSG_TYPE_STRING },
	[GENERAL_ATTR_STOP_PROCESSING_MARK] = { .name = "stop_processing_mark", .type = BLOBMSG_TYPE_STRING },
};

static const struct uci_blob_param_info general_attr_info[GENERAL_ATTR_MAX] = {
	[GENERAL_ATTR_LAN_INTF] = { .type = BLOBMSG_TYPE_STRING },
};

static const struct uci_blob_param_list general_attr_list = {
	.n_params = GENERAL_ATTR_MAX,
	.params = general_attrs,
	.info = general_attr_info
};

enum {
	URLFILTER_ATTR_DEVICE,
	URLFILTER_ATTR_SITE,
	URLFILTER_ATTR_MACADDRESS,
	URLFILTER_ATTR_ACTION,
	URLFILTER_ATTR_MAX
};

static const struct blobmsg_policy urlfilter_attrs[URLFILTER_ATTR_MAX] = {
	[URLFILTER_ATTR_DEVICE] = { .name = "device", .type = BLOBMSG_TYPE_STRING },
	[URLFILTER_ATTR_SITE] = { .name = "site", .type = BLOBMSG_TYPE_STRING },
	[URLFILTER_ATTR_MACADDRESS] = { .name = "mac", .type = BLOBMSG_TYPE_STRING },
	[URLFILTER_ATTR_ACTION] = { .name = "action", .type = BLOBMSG_TYPE_STRING },
};

static const struct uci_blob_param_list urlfilter_attr_list = {
	.n_params = URLFILTER_ATTR_MAX,
	.params = urlfilter_attrs,
};


enum {
	FILTERKEYWORD_ATTR_KEYWORD,
	FILTERKEYWORD_ATTR_KEYWORD_FILTER_ENABLE,
	FILTERKEYWORD_ATTR_MAX
};

static const struct blobmsg_policy filterkeyword_attrs[FILTERKEYWORD_ATTR_MAX] = {
	[FILTERKEYWORD_ATTR_KEYWORD] = { .name = "keyword", .type = BLOBMSG_TYPE_STRING },
	[FILTERKEYWORD_ATTR_KEYWORD_FILTER_ENABLE] = { .name = "keywordfilter_enable", .type = BLOBMSG_TYPE_BOOL },
};

static const struct uci_blob_param_list filterkeyword_attr_list = {
	.n_params = FILTERKEYWORD_ATTR_MAX,
	.params = filterkeyword_attrs,
};



static struct config *alloc_config()
{
	struct config *c = calloc(1, sizeof(*c));
	int len;
	const char *template_header = "HTTP/1.1 307 Temporary Redirect\n"
				      "Content-Length: 0\n"
				      "Location: %s\n"
				      "Cache-Control: no-store,no-cache\n"
				      "Pragma: no-cache\n"
				      "Content-Type: text/html\n"
				      "P3P: CP=\"NOI ADM DEV PSAi COM NAV OUR OTR STP IND DEM\"\n"
				      "Set-Cookie: _sm_au_d=1;\n\n";
	const char *template_url = "http://dsldevice.lan/parental-block.lp";
	char *template_buffer;

	INIT_LIST_HEAD(&c->filters);
	INIT_LIST_HEAD(&c->keywordfilters);
	c->stop_processing_mark = 0x2000000;
	c->redirected_mark = 0x4000000;

	/* NFQUEUE quenum */
	c->queue = 2;

	if (redirect_url) {
		template_url = redirect_url;
	}
	template_buffer = calloc(1, strlen(template_header) + strlen(template_url) + 1);
	sprintf(template_buffer, template_header, template_url);

	c->redirect = template_buffer;
	c->redirect_len = strlen(c->redirect);

	c->enabled = true;
	c->keywordfilter_enabled = false;
	c->exclude = false;
	c->ipv6_enabled = false;

#if PCAP_DEBUG
	c->pcap = pcap_open_dead(DLT_IPV4, 7000);
	c->pd = pcap_dump_open(c->pcap, "/tmp/urlfilter.pcap");
#endif
	return c;
}

static void free_urlfilter(struct urlfilter *filter)
{
	free((void *)filter->site);
}

static void free_keywordfilter(struct keywordfilter *filter)
{
	free((void *)filter->keyword);
}

static void free_config(struct config *c)
{
	struct urlfilter *filter, *n;
	struct keywordfilter *kfilter, *kn;
	list_for_each_entry_safe(filter, n, &c->filters, head) {
		free_urlfilter(filter);
		free(filter);
	}

	list_for_each_entry_safe(kfilter, kn, &c->keywordfilters, head) {
		free_keywordfilter(kfilter);
		free(kfilter);
	}

#if PCAP_DEBUG
	if (c->pd) pcap_dump_close(c->pd);
	if (c->pcap) pcap_close(c->pcap);
#endif

	free((char *)c->redirect);
	free(c);
}

static void dump_urlfilter(const struct urlfilter *filter)
{
	char mac[32];
	char addrstr[NI_MAXHOST];

	if (filter->macaddress_check) {
		sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
			filter->hw_addr[0], filter->hw_addr[1], filter->hw_addr[2],
			filter->hw_addr[3], filter->hw_addr[4], filter->hw_addr[5]);
	} else {
		strcpy(mac, "*");
	}

	if (getnameinfo((struct sockaddr *)&filter->device, sizeof(filter->device), addrstr, sizeof(addrstr),
			NULL, 0, NI_NUMERICHOST) != 0)
			addrstr[0] = '\0';

	syslog(LOG_DEBUG, "Device: '%s', Site: '%s', Mac: '%s', Action: %s",
	       addrstr,
	       filter->site,
	       mac,
	       filter->action == ACTION_DEFAULT ? "DEFAULT" : (filter->action == ACTION_ACCEPT ? "ACCEPT" : "DROP"));
}

static void dump_keywordfilter(const struct keywordfilter *filter)
{
	syslog(LOG_DEBUG, "keyword: '%s'", filter->keyword);
}

static void dump_config(struct config *c)
{
	struct urlfilter *filter;
	struct keywordfilter *kfilter;

	syslog(LOG_DEBUG, "-- BEGIN config");
	syslog(LOG_DEBUG, "keywordfilter_enabled: %d", c->keywordfilter_enabled);
	list_for_each_entry(filter, &c->filters, head)
		dump_urlfilter(filter);
	list_for_each_entry(kfilter, &c->keywordfilters, head)
		dump_keywordfilter(kfilter);
	syslog(LOG_DEBUG, "-- END config");
}



static void config_set_general(struct config *cfg, struct uci_section *s)
{
	struct blob_attr *attr[GENERAL_ATTR_MAX], *c;
	unsigned rem;

	syslog(LOG_DEBUG, "config_set_general");
	blob_buf_init(&b, 0);
	uci_to_blob(&b, s, &general_attr_list);
	blobmsg_parse(general_attrs, GENERAL_ATTR_MAX, attr, blob_data(b.head), blob_len(b.head));

	if (!attr[GENERAL_ATTR_LAN_INTF]) {
		syslog(LOG_DEBUG, "lan_intf string missing");
		return;
	}

	if ((c = attr[GENERAL_ATTR_ENABLE])) {
		cfg->enabled =  blobmsg_get_bool(c);
	}

	if ((c = attr[GENERAL_ATTR_KEYWORDFILTER_ENABLE])) {
		cfg->keywordfilter_enabled =  blobmsg_get_bool(c);
	}

	if ((c = attr[GENERAL_ATTR_EXCLUDE])) {
		cfg->exclude =  blobmsg_get_bool(c);
	}

	if ((c = attr[GENERAL_ATTR_IPV6_ENABLE])) {
		cfg->ipv6_enabled =  blobmsg_get_bool(c);
	}

	if ((c = attr[GENERAL_ATTR_BLOCKED_PAGE_REDIRECT])) {
		char *buf = NULL;
		syslog(LOG_DEBUG, "blockedpage");
		int fd = open("/etc/weburl_cfg_tmpl", O_RDONLY);
		if (fd) do {
			struct stat st;
			unsigned long url_len = strlen(blobmsg_get_string(c));
			if (fstat(fd, &st) == -1)
				break;

			/* a valid header requires at least this many bytes */
			if (st.st_size < 32)
				break;

			buf = calloc(1, st.st_size + url_len);
			if (!buf)
				break;

			if (read(fd, buf, st.st_size) != st.st_size)
				break;

			/* don't count last 0x0a, we know there's at least
			 * 32 or more bytes, so no need to check for reaching
			 * 0 here.
			 */
			st.st_size--;
			char *p = strstr(buf, "__WEBREDIR_URL__");
			if (p == NULL)
				break;

			/* Replace marker by user specified URL */
			unsigned long tmpl_len = strlen("__WEBREDIR_URL__");
			unsigned long tmpl_off = p-buf;
			memmove(p+url_len, p+tmpl_len, st.st_size-tmpl_off-tmpl_len);
			memcpy(p, blobmsg_get_string(c), url_len);
			if (cfg->redirect) {
				free((void *)cfg->redirect);
				cfg->redirect_len = 0;
			}
			cfg->redirect = strdup(buf);
			cfg->redirect_len = strlen(cfg->redirect);

		} while(0);
		if (buf)
			free(buf);
		if (fd)
			close(fd);
	}

	if ((c = attr[GENERAL_ATTR_QUEUE])) {
		cfg->queue =  blobmsg_get_u32(c);
	}

	if (attr[GENERAL_ATTR_LAN_INTF] ) {
		blobmsg_for_each_attr(c, attr[GENERAL_ATTR_LAN_INTF], rem) {
			if (blobmsg_type(c) != BLOBMSG_TYPE_STRING)
				continue;
			/* actual rules related to the interface are done
			 * via scripting. Nothing needs to happen in the
			 * daemon code for it.
			 */
		}
	}

	if ((c = attr[GENERAL_ATTR_REDIRECTED_MARK])) {
		unsigned int value;

		if (sscanf(blobmsg_get_string(c), "%x", &value) == 1)
			cfg->redirected_mark = value;
	}

	if ((c = attr[GENERAL_ATTR_STOP_PROCESSING_MARK])) {
		unsigned int value;

		if (sscanf(blobmsg_get_string(c), "%x", &value) == 1)
			cfg->stop_processing_mark = value;
	}

}

static void config_set_urlfilter(struct config *config, struct uci_section *s)
{
	struct blob_attr *attr[URLFILTER_ATTR_MAX], *c;
	struct urlfilter *filter;

	blob_buf_init(&b, 0);
	uci_to_blob(&b, s, &urlfilter_attr_list);
	blobmsg_parse(urlfilter_attrs, URLFILTER_ATTR_MAX, attr, blob_data(b.head), blob_len(b.head));

	if (!attr[URLFILTER_ATTR_SITE])
		return;

	filter = calloc(1, sizeof(*filter));
	if (!filter)
		return;

	list_add_tail(&filter->head, &config->filters);
	filter->action = config->exclude ? ACTION_DROP : ACTION_ACCEPT;
	filter->macaddress_check = false;

	if ((c = attr[URLFILTER_ATTR_DEVICE])) do {
		struct addrinfo hints, *res;
		int status;
		const char *content = blobmsg_get_string(c);

		if (!content[0])
			break;

		if (strcmp(content, "All") == 0)
			break;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_NUMERICHOST;
		if ((status = getaddrinfo(content, 0, &hints, &res)) != 0)
			break;

		memcpy(&filter->device, res->ai_addr, res->ai_addrlen);
		freeaddrinfo(res);
	} while(0);

	if ((c = attr[URLFILTER_ATTR_SITE])) {
		const char *content = blobmsg_get_string(c);

		if (strncmp(content, "http://", 7) == 0)
			content += 7;
		if (strncmp(content, "www.", 4) == 0)
			content += 4;
		else if (content[0] == '.')
			content++;
		filter->site_len = strlen(content);
		if (filter->site_len > 0 && content[filter->site_len - 1] == '.')
			filter->site_len--; /* remove trailing dot from the site */
		filter->site = strndup(content, filter->site_len);
	}

	if ((c = attr[URLFILTER_ATTR_MACADDRESS])) {
		const char *content = blobmsg_get_string(c);
		if (content[0]) {
			int m[6];
			int res = sscanf(content, "%02x:%02x:%02x:%02x:%02x:%02x",
					&m[0], &m[1], &m[2], &m[3], &m[4], &m[5]);
			if (res == 6) {
				filter->macaddress_check = true;
				filter->hw_addr[0] = (uint8_t)m[0];
				filter->hw_addr[1] = (uint8_t)m[1];
				filter->hw_addr[2] = (uint8_t)m[2];
				filter->hw_addr[3] = (uint8_t)m[3];
				filter->hw_addr[4] = (uint8_t)m[4];
				filter->hw_addr[5] = (uint8_t)m[5];
			} else {
				syslog(LOG_ERR, "unable to parse mac address '%s'", content);
			}
		}
	}

	if ((c = attr[URLFILTER_ATTR_ACTION])) {
		const char *content = blobmsg_get_string(c);
		if (!strcasecmp(content, "DROP"))
			filter->action = ACTION_DROP;
		else if (!strcasecmp(content, "ACCEPT"))
			filter->action = ACTION_ACCEPT;
	}
}

static void config_set_filterkeyword(struct config *config, struct uci_section *s)
{
	struct blob_attr *attr[FILTERKEYWORD_ATTR_MAX], *c;
	struct keywordfilter *filter;
	int len;
	int pos;
	char *buffer = NULL;
	const char *ptr;

	blob_buf_init(&b, 0);
	uci_to_blob(&b, s, &filterkeyword_attr_list);
	blobmsg_parse(filterkeyword_attrs, FILTERKEYWORD_ATTR_MAX, attr, blob_data(b.head), blob_len(b.head));

	if ((c = attr[FILTERKEYWORD_ATTR_KEYWORD_FILTER_ENABLE])) {
		/* just don't load disabled keywords */
		if (!blobmsg_get_bool(c))
			return;
	}

	if (!(c = attr[FILTERKEYWORD_ATTR_KEYWORD])) {
		syslog(LOG_DEBUG, "%s: keyword missing", __func__);
		return;
	}

	const char *content = blobmsg_get_string(c);
	if (!content[0])
		return;

	/* convert hex bytes to string */
	ptr = content;
	buffer = calloc(1, strlen(content)+1);
	pos = 0;
	while(true) {
		unsigned int ch;
		while( !(*ptr >= '0' && *ptr <= '9') &&
				!(*ptr >= 'a' && *ptr <= 'f') &&
				!(*ptr >= 'A' && *ptr <= 'F') &&
				(*ptr != '\0'))
			ptr++;

			if (sscanf(ptr, "%02x", &ch) != 1)
				break;
			ptr += 2;
			buffer[pos++] = (char)ch;
			if (pos >= strlen(content))
				break;
	}
	buffer[pos] = '\0';

	/* if conversion failed or doesn't contain characters, skip */
	if (!strlen(buffer))
		return;

	filter = calloc(1, sizeof(*filter));
	if (!filter)
		return;

	list_add_tail(&filter->head, &config->keywordfilters);
	filter->keyword = strdup(buffer);
	filter->keyword_len = strlen(buffer);
	free(buffer);
}

/*#######################################################################
 #                                                                      #
 #  EXTERNAL FUNCTIONS                                                  #
 #                                                                      #
 ###################################################################### */
void config_reload(void)
{
	struct uci_context *uci = uci_alloc_context();

	if (!uci)
		return;

	struct uci_package *package = NULL;
	struct config *new_config = alloc_config();
	if (!new_config)
		return;

	syslog(LOG_DEBUG, "parsing configuration");
	if (!uci_load(uci, "parental", &package)) {
		struct uci_element *e;

		uci_foreach_element(&package->sections, e) {
			struct uci_section *s = uci_to_section(e);

			if (!strcmp(s->type, "parental")) {
				if (!strcmp(s->e.name, "general"))
					config_set_general(new_config, s);
			} else if (!strcmp(s->type, "URLfilter"))
				config_set_urlfilter(new_config, s);
			else if (!strcmp(s->type, "filterkeyword"))
				config_set_filterkeyword(new_config, s);
		}
		uci_unload(uci, package);
	}

	/* if no keywords are loaded, then disable the keyword filtering */
	if (list_empty(&new_config->keywordfilters))
		new_config->keywordfilter_enabled = false;

	if (debug_level > 0)
		dump_config(new_config);

	struct config **current_config = &active_config;
	struct config *old_config = rcu_xchg_pointer(current_config, new_config);
	synchronize_rcu();

	if (old_config)
		free_config(old_config);
	uci_free_context(uci);
}
