/************* COPYRIGHT AND CONFIDENTIALITY INFORMATION *********************
 **                                                                          **
 ** Copyright (c) 2013 Technicolor                                           **
 ** All Rights Reserved                                                      **
 **
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **                                                                         **
**                                                                          **
 ******************************************************************************/

/**
 * UCI-based Broadcom Switch configuration
 *
 * @file bcmswconfig.c
 * @author Thierry Du Tre
 */

#include "string.h"
#include "stdarg.h"
#include "stdlib.h"
#ifndef HAVE_STRLCPY
#include "strl.h"
#endif

#include "uci.h"

/* uncomment to enable debug stuff */
#define BCMSWCONFIG_DEBUG

#define TRACE_ERR(fmt, ...)	trace(1, "bcmswconfig", "ERROR: " fmt, ## __VA_ARGS__)

#ifdef BCMSWCONFIG_DEBUG
#define TRACE_DBG(fmt, ...)	trace(3, "bcmswconfig", fmt, ## __VA_ARGS__)
#else
#define TRACE_DBG(...)
#endif

#define NUM_SWITCH	4	/* # of switch instances */
#define NUM_PORTS	9	/* # of switch ports */

#define SWITCH_NAME_LENGTH	32
#define SWITCH_TYPE_LENGTH	32

static const char *const bcm_mdk_shell = "/usr/bin/mdkshell";

struct uci_switch_cfg {
	char name[SWITCH_NAME_LENGTH + 1];
	char type[SWITCH_TYPE_LENGTH +1];
	int enable_vlan;
	int unit;
};

static const struct uci_switch_cfg def_bcmsw_cfg = { .enable_vlan = 0 , .unit = 0 };

struct uci_switch_vlan_cfg {
	char		device[SWITCH_NAME_LENGTH + 1];
	int		vid;
	unsigned	members;
	unsigned	untagged;
	unsigned	defvid;
};

static struct uci_switch_cfg bcmsw_cfg[NUM_SWITCH];

static struct uci_context *bcmswitch_uci_ctx;
static struct uci_package *bcmswitch_uci_pkg;

int trace (unsigned level, const char *name,  const char *fmt, ...);

#ifdef BCMSWCONFIG_DEBUG
static int debug;

static void check_debug (const char *cmd)
{
	debug = ((cmd = strrchr(cmd, '_')) != NULL && !strcmp(cmd, "_dbg"));
	TRACE_DBG ("debug enabled");
}
#endif

int trace (unsigned level, const char *name,  const char *fmt, ...)
{
	va_list ap;
	char pref[32];
	char buf[256];

	static const char *const trc_types[] = { "?", "err", "warn", "dbg", "info"};
	static const char tmpl[] = "%s %s\r\n";

	if (level > sizeof(trc_types) / sizeof(trc_types[0]))
		level = 0;

#ifdef BCMSWCONFIG_DEBUG
	if (level >= 3 && !debug)
		return 0;
#endif

	snprintf(pref, sizeof(pref), "[%s] (%s)", name, trc_types[level]);

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	printf(tmpl, pref, buf);

	return 1;
}

static struct uci_switch_cfg * switch_cfg_new (const char *name)
{
	int i;
	struct uci_switch_cfg *sw_cfg;

	for (i = 0; i < NUM_SWITCH; i++) {
		sw_cfg = &bcmsw_cfg[i];
		if (sw_cfg->name[0] == 0) {
			*sw_cfg = def_bcmsw_cfg;
			strlcpy(sw_cfg->name, name, sizeof(sw_cfg->name));
			strlcpy(sw_cfg->type, name, sizeof(sw_cfg->type));
			return sw_cfg;
		}
		if (!strncmp(sw_cfg->name, name, sizeof(sw_cfg->name)))
			break;
	}

	return NULL;
}

static void switch_cfg_free (struct uci_switch_cfg *switch_cfg)
{
	memset(switch_cfg, 0, sizeof(*switch_cfg));
}

static struct uci_switch_cfg * switch_cfg_get (const char *name)
{
	int i;

	if (name[0] != '\0') {
		for (i = 0; i < NUM_SWITCH; i++) {
			if (!strncmp(bcmsw_cfg[i].name, name, sizeof(bcmsw_cfg[i].name)))
				return &bcmsw_cfg[i];
		}
	}

	return NULL;
}

static int executeShellCmd (const char *fmt, ...)
{
	char shell_cmd[256];
	va_list ap;
	int rv;

	va_start(ap, fmt);
	rv = vsnprintf(shell_cmd, sizeof(shell_cmd), fmt, ap);
	va_end(ap);

	if (rv < 0 || rv > sizeof(shell_cmd) - 20) {
		TRACE_ERR ("shell cmd exceeds buffer");
		return -1;
	}

#ifdef BCMSWCONFIG_DEBUG
	if (!debug)
#endif
	{
		sprintf(shell_cmd + rv, " >/dev/null 2>&1");
	}
	TRACE_DBG ("shell_cmd : %s", shell_cmd);

	return system(shell_cmd);
}

static void applySwitchConfig (const struct uci_switch_cfg *switch_cfg)
{
	executeShellCmd("%s %d:switch vlan %s", bcm_mdk_shell, switch_cfg->unit, (switch_cfg->enable_vlan != 1)? "disable" : "enable");
	executeShellCmd("%s %d:switch learning shared", bcm_mdk_shell, switch_cfg->unit);
}

static void applySwitchDefaultVidConfig (const struct uci_switch_cfg *switch_cfg, int vid, unsigned ports)
{
	int i;

	for (i = 0; i < NUM_PORTS; i++) {
		if (ports & (1 << i)) {
			executeShellCmd("%s %d:pvlan %d %d", bcm_mdk_shell, switch_cfg->unit, i, vid);
		}
	}
}

static void applySwitchVlanConfig (const struct uci_switch_cfg *switch_cfg, const struct uci_switch_vlan_cfg *vlan_cfg)
{
	int i;

	executeShellCmd("%s %d:vlan destroy %d", bcm_mdk_shell, switch_cfg->unit, vlan_cfg->vid);
	if (!vlan_cfg->members)
		return;

	executeShellCmd("%s %d:vlan create %d", bcm_mdk_shell, switch_cfg->unit, vlan_cfg->vid);

	for (i = 0; i < NUM_PORTS; i++) {
		if (vlan_cfg->members & (1 << i)) {
			executeShellCmd("%s %d:vlan add %d %d %s", bcm_mdk_shell, switch_cfg->unit, vlan_cfg->vid, i, (vlan_cfg->untagged & (1 << i))? "untag":"");
		}
	}

	if (vlan_cfg->defvid) {
		applySwitchDefaultVidConfig(switch_cfg, vlan_cfg->vid, vlan_cfg->defvid);
	}
}

static int parseSwitchVlanOptionPorts (const char *ports, struct uci_switch_vlan_cfg *vlan_cfg)
{
	const char *head = ports;
	char *rest;
	int port;
	unsigned tagged = 0;

	while (*head != '\0') {
		port = strtol(head, &rest, 10);
		if (head == rest || port < 0 || port >= NUM_PORTS)
			return -1;

		vlan_cfg->members |= 1 << port;
		switch (*rest) {
			case 't' :
				tagged |= (1 << port);
				rest++;
				break;
			case '*' :
				vlan_cfg->defvid |= 1 << port;
			case 'u' :
				rest++;
				break;
		}
		vlan_cfg->untagged = vlan_cfg->members & ~tagged;

		TRACE_DBG ("port present : %d (%s%s", port,
				(vlan_cfg->untagged & (1 << port))? "untagged":"tagged",
				(vlan_cfg->defvid & (1 << port))? " defvid)":")");
		head = rest;
	}

	return 0;
}

static int parseSwitchVlanOptionString (struct uci_option *o, struct uci_switch_vlan_cfg *vlan_cfg)
{
	char *rest;

	if (!strcmp(o->e.name, "vlan")) {
		vlan_cfg->vid = strtol(o->v.string, &rest, 10);
		return (*rest != '\0' || vlan_cfg->vid < 0 || vlan_cfg->vid > 4094)? -1 : 0;
	}

	if (!strcmp(o->e.name, "device")) {
		strlcpy(vlan_cfg->device, o->v.string, sizeof(vlan_cfg->device));
		return 0;
	}

	if (!strcmp(o->e.name, "ports")) {
		return parseSwitchVlanOptionPorts(o->v.string, vlan_cfg);
	}

	return -1;
}

static int parseSectionSwitchVlan (struct uci_section *s)
{
	struct uci_switch_vlan_cfg vlan_cfg;
	struct uci_switch_cfg *switch_cfg;
	struct uci_element *e;
	struct uci_option *o;

	memset(&vlan_cfg, 0, sizeof(vlan_cfg));
	uci_foreach_element(&s->options, e) {
		o = uci_to_option(e);
		if (o->type == UCI_TYPE_STRING) {
			TRACE_DBG ("switch_vlan option string '%s'='%s'", e->name, o->v.string);
			if (parseSwitchVlanOptionString(o, &vlan_cfg) < 0)
				return -1;
		} else {
			TRACE_DBG ("switch_vlan option list '%s'", e->name);
		}
	}

	switch_cfg = switch_cfg_get(vlan_cfg.device);
	if (switch_cfg == NULL) {
		return -1;
	}
	applySwitchVlanConfig(switch_cfg, &vlan_cfg);
	return 0;
}

static int parseSwitchOptionString (struct uci_option *o, struct uci_switch_cfg *switch_cfg)
{
	char *rest;

	if (!strcmp(o->e.name, "type")) {
		strlcpy(switch_cfg->type, o->v.string, sizeof(switch_cfg->type));
		return 0;
	}

	if (!strcmp(o->e.name, "enable_vlan")) {
		switch_cfg->enable_vlan = (strtol(o->v.string, &rest, 10) == 1);
		return (*rest != '\0')? -1 : 0;
	}

	if (!strcmp(o->e.name, "unit")) {
		switch_cfg->unit = strtol(o->v.string, &rest, 10);
		return (*rest != '\0' || switch_cfg->unit < 0)? -1 : 0;
	}

	return 0;
}

static int parseSectionSwitch (struct uci_section *s)
{
	struct uci_switch_cfg *switch_cfg;
	struct uci_element *e;
	struct uci_option *o;

	if (s->anonymous || (switch_cfg = switch_cfg_new(s->e.name)) == NULL)
		return -1;

	uci_foreach_element(&s->options, e) {
		o = uci_to_option(e);
		if (o->type == UCI_TYPE_STRING) {
			TRACE_DBG ("switch option string '%s'='%s'", e->name, o->v.string);
			if (parseSwitchOptionString(o, switch_cfg) < 0)
				return -1;
		} else {
			TRACE_DBG ("switch option list '%s'", e->name);
		}
	}

	if (strcmp(switch_cfg->type, "bcmsw")) {
		switch_cfg_free(switch_cfg);
		return -1;
	}
	applySwitchConfig(switch_cfg);
	return 0;
}

static int bcmswconfig_init (const char *name)
{
	int rv;

	bcmswitch_uci_ctx = uci_alloc_context();

	rv = (uci_load(bcmswitch_uci_ctx, name, &bcmswitch_uci_pkg) == UCI_OK)? 0  : -1;
	if (rv) {
		TRACE_ERR ("Loading the UCI configuration from file '%s' failed.", name);
		exit(-1);
	}
	return rv;
}

static void bcmswconfig_process (const char *type, int (*fParse)(struct uci_section *s))
{
	struct uci_element *e;
	struct uci_section *s;

	/* Iterate over all UCI configuration sections */
	uci_foreach_element(&bcmswitch_uci_pkg->sections, e) {

		s = uci_to_section(e);
		TRACE_DBG ("UCI section '%s' (anon=%d, name=%s)", s->type, s->anonymous, (s->anonymous)? "-" : e->name);

		if (!strcmp(s->type, type)) {
			fParse(s);
		}
	}
}

static void bcmswconfig_cleanup (void)
{
	uci_unload(bcmswitch_uci_ctx, bcmswitch_uci_pkg);
	uci_free_context(bcmswitch_uci_ctx);
}

static int bcmswconfig_load (const char *name)
{
	bcmswconfig_init(name);

	bcmswconfig_process("switch", &parseSectionSwitch);
	bcmswconfig_process("switch_vlan", &parseSectionSwitchVlan);

	bcmswconfig_cleanup();
	return 0;
}

static int bcmswconfig_reset_vlan (const struct uci_switch_cfg *switch_cfg, const char *scope)
{
	int scope_num;
	char *rest;
	struct uci_switch_vlan_cfg vlan_cfg;

	if (scope == NULL) {
		scope_num = 1;
	} else if (!strcmp(scope, "all")) {
		scope_num = 4094;
	} else {
		if ((scope_num = strtol(scope, &rest, 10)) < 0 || *rest != '\0')
			return -1;
		if (scope_num > 4094)
			scope_num = 4094;
	}

	if (scope_num) {
		applySwitchDefaultVidConfig(switch_cfg, 1, ~0);

		memset(&vlan_cfg, 0, sizeof(vlan_cfg));
		for (vlan_cfg.vid = 1; vlan_cfg.vid <= scope_num; vlan_cfg.vid++) {
			applySwitchVlanConfig(switch_cfg, &vlan_cfg);
		}
	}
	return 0;
}

static int bcmswconfig_reset (const char *scope)
{
	applySwitchConfig(&def_bcmsw_cfg);

	return bcmswconfig_reset_vlan(&def_bcmsw_cfg, scope);
}

static void bcmswconfig_help (void)
{
	puts("\n"
	     "Broadcom switch configuration\n"
	     "\n"
	     "bcmswconfig load <name>   : load switch configuration from UCI\n"
	     "bcmswconfig reset [N|all] : reset switch configuration (N = # vlan to clear)\n"
	     "bcmswconfig help          : display this info\n");
}

static int bcmswconfig_cmd (int argc, char **argv)
{
	if (argc <= 3 && !strcmp(argv[1], "reset")) {
		if (!bcmswconfig_reset((argc == 3)? argv[2] : NULL))
			return 0;
	}

	if (argc == 3 && !strcmp(argv[1], "load")) {
		return bcmswconfig_load(argv[2]);
	}

	TRACE_ERR("invalid syntax");
	return -1;
}

int main (int argc, char **argv)
{
	int ret;

#ifdef BCMSWCONFIG_DEBUG
	check_debug(argv[0]);
#endif

	if (argc > 1 && strcmp(argv[1], "help")) {
		ret = bcmswconfig_cmd(argc, argv);
	} else {
		ret = 1;
	}

	if (ret) {
		bcmswconfig_help();
	}
	return ret;
}

