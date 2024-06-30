/*
 * weburl-nfq - url filter daemon
 ************ COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
 * Copyright (C) 2017 Technicolor Delivery Technologies, SAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation
 *
 */
#include <stdint.h>
#include <sys/socket.h>
#include <libubox/list.h>
#if PCAP_DEBUG
#include <pcap.h>
#endif

enum urlfilter_action {
	ACTION_DEFAULT,
	ACTION_ACCEPT,
	ACTION_DROP
};

struct urlfilter {
	struct list_head head;

	struct sockaddr_storage device;
	const char *site;
	size_t site_len;
	bool macaddress_check;
	uint8_t hw_addr[8];
	enum urlfilter_action action;


};

struct keywordfilter {
	struct list_head head;
	const char *keyword;
	size_t keyword_len;
};

struct config {
	struct list_head filters;
	struct list_head keywordfilters;
	const char *redirect;
	size_t redirect_len;
	uint32_t stop_processing_mark;
	uint32_t redirected_mark;

	uint32_t queue;
	bool enabled;
	bool exclude;
	bool keywordfilter_enabled;
	bool ipv4_enabled;
	bool ipv6_enabled;
#if PCAP_DEBUG
	pcap_t *pcap;
	pcap_dumper_t *pd;
#endif
};

extern struct config *active_config;
extern const char *redirect_url;

void config_reload(void);
