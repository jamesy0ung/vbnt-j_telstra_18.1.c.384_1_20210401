/*
 * weburl-nfq - url filter daemon
 * 
 * Copyright (C) 2017 Technicolor Delivery Technologies, SAS
 * ** All Rights Reserved                                                  
 *                                                                      
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation
 *
 */
#define _LGPL_SOURCE

#include <string.h>
#include <sys/wait.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <libubus.h>
#include <urcu.h>
#include "urlfilterd.h"
#include "ubus.h"
#include "config.h"
#include "nfq.h"

static void reload_cb(struct uloop_fd *u, _unused unsigned int events);

unsigned int debug_level = 0;
static char applname[32];
static int reload_pipe[2];
static struct uloop_fd reload_fd = { .cb = reload_cb };

static const char *remove_path(const char *argv)
{
	const char *applname = NULL;

	if (!argv)
		return NULL;

	applname = strrchr(argv, '/');

	/* if no '/' found, return original string */
	if (!applname)
		return argv;

	return ++applname;
}

static void sighandler(int signal)
{
	switch (signal) {
	case SIGHUP: {
			char b = 0;
			if (write(reload_pipe[1], &b, sizeof(b)) < 0) {}
			break;
		}

	default:
		uloop_end();
		break;
	}
}

static void reload_cb(struct uloop_fd *u, _unused unsigned int events)
{
	char b[512];

	if (read(u->fd, b, sizeof(b)) < 0) {}

	config_reload();
}

static int usage(const char *progname)
{
	fprintf(stderr, "Usage: %s [options]\n"
		"Options:\n"
		" -u <url>:		Redirect URL (default: http://dsldevice.lan/parental-block.lp)\n"
		" -d <level>:		Set debug level\n"
		"\n", progname);

	return 1;
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	const char *app;
	uint32_t queue;
	int ch;

	app = remove_path(argv[0]);
	memset(applname, 0, sizeof(applname));
	strncpy(applname, app, sizeof(applname) -1);

	while((ch = getopt(argc, argv, "u:d:")) != -1) {
		switch(ch) {
		case 'u':
			redirect_url = optarg;
			break;
		case 'd':
			debug_level = atoi(optarg);
			break;
		default:
			return usage(applname);
		}
	}

	openlog(applname, LOG_PID, LOG_DAEMON);
	setlogmask(LOG_UPTO(LOG_NOTICE));
	setlogmask(LOG_UPTO(LOG_DEBUG));

	rcu_init();
	uloop_init();

	signal(SIGUSR1, SIG_IGN);
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	if (pipe2(reload_pipe, O_NONBLOCK | O_CLOEXEC) < 0)
		return 1;
	reload_fd.fd = reload_pipe[0];
	uloop_fd_add(&reload_fd, ULOOP_READ);
	signal(SIGHUP, sighandler);

	if (ubus_init() < 0)
		return 3;

	/* load initial configuration */
	config_reload();

	/* grab NFQUEUE queue number from config */
	rcu_read_lock();
	struct config *cfg = rcu_dereference(active_config);
	queue = cfg->queue;
	rcu_read_unlock();

	if (nfq_init(queue) < 0)
		return 4;

	uloop_run();
	nfq_end();

	uloop_done();
	return 0;
}
