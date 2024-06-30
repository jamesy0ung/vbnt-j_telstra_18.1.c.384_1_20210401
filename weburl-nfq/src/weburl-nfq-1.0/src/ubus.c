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

#include <syslog.h>
#include <libubox/blobmsg.h>
#include <libubus.h>
#include "urlfilterd.h"
#include "config.h"
#include "ubus.h"

static int handle_reload(struct ubus_context *ctx, struct ubus_object *obj,
			 struct ubus_request_data *req, const char *method,
			 struct blob_attr *msg);

static struct ubus_context *ubus_ctx = NULL;

static struct ubus_method main_object_methods[] = {
{ .name = "reload", .handler = handle_reload },
};

static struct ubus_object_type main_object_type =
		UBUS_OBJECT_TYPE("urlfilterd", main_object_methods);

static struct ubus_object main_object = {
	.name = "urlfilterd",
	.type = &main_object_type,
	.methods = main_object_methods,
	.n_methods = ARRAY_SIZE(main_object_methods),
};

static int handle_reload(struct ubus_context *ctx, struct ubus_object *obj,
			 struct ubus_request_data *req, const char *method,
			 struct blob_attr *msg)
{
	(void)ctx;
	(void)obj;
	(void)req;
	(void)method;
	(void)msg;
	syslog(LOG_DEBUG, "ubus reload called");
	config_reload();
	return 0;
}

int ubus_init()
{
	int ret;

	if ((ubus_ctx = ubus_connect(NULL)) == NULL) {
		syslog(LOG_ERR, "Unable to connect to ubus: %s", strerror(errno));
		return -1;
	}
	ubus_add_uloop(ubus_ctx);

	ret = ubus_add_object(ubus_ctx, &main_object);
	if (ret != 0) {
		syslog(LOG_ERR, "Failed to publish object '%s': %s", main_object.name, ubus_strerror(ret));
		return -1;
	}

	return 0;
}
