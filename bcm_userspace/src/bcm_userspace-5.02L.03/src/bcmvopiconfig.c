/************* COPYRIGHT AND CONFIDENTIALITY INFORMATION *********************
 **                                                                          **
 ** Copyright (c) 2013 Technicolor                                           **
 ** All Rights Reserved                                                      **
 **
 ** This program is free software; you can redistribute it and/or modify **
 ** it under the terms of the GNU General Public License version 2 as    **
 ** published by the Free Software Foundation.                           **                                                                          **
 **                                                                          **
 ******************************************************************************/

/**
 * UCI-based Broadcom VOPI configuration (VLAN Operations Interface)
 *
 * @file bcmvopiconfig.c
 * @author Thierry Du Tre
 */

#include "string.h"
#include "stdarg.h"
#include "stdlib.h"
#ifndef HAVE_STRLCPY
#include "strl.h"
#endif

#include "uci.h"

#define MAX_IFNAME_LEN		16
#define MAX_VOPINAME_LEN	(MAX_IFNAME_LEN + 10)

/* uncomment to enable debug stuff */
#define BCMVOPICONFIG_DEBUG

#define TRACE_ERR(fmt, ...)	trace(1, "bcmvopiconfig", "ERROR: " fmt, ## __VA_ARGS__)

#ifdef BCMVOPICONFIG_DEBUG
#define TRACE_DBG(fmt, ...)	trace(3, "bcmvopiconfig", fmt, ## __VA_ARGS__)
#else
#define TRACE_DBG(...)
#endif

static const char default_uci_package[] = "network";
static const char savedir_state[] = "/var/state/";

static const char bcm_vlanctl_bin[] = "/usr/bin/vlanctl";
static const char ifconfig_bin[] = "/sbin/ifconfig";
static const char uci_bin[] = "/sbin/uci";

static struct uci_context *bcmvopi_uci_ctx;
static struct uci_package *bcmvopi_uci_pkg;

enum { ACTION_UNKNOWN = 0, ACTION_LOAD, ACTION_UNLOAD };
static int bcmvopi_action;

struct uci_bcmvopi_cfg {
	char		if_name[MAX_IFNAME_LEN+1];
	int		vid;
	int		pbits;
	int		defvid;
	int		dscp2pbits;
	char		vopi_name[MAX_VOPINAME_LEN+1];
};

static const struct uci_bcmvopi_cfg def_bcmvopi_cfg = { .if_name = "", .pbits = 0, .vopi_name = "", .dscp2pbits = 0 , .vid = -1 };

#ifdef BCMVOPICONFIG_DEBUG
static int debug;

int trace (unsigned level, const char *name,  const char *fmt, ...);

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

#ifdef BCMVOPICONFIG_DEBUG
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

#ifdef BCMVOPICONFIG_DEBUG
	if (!debug)
#endif
	{
		sprintf(shell_cmd + rv, " >/dev/null 2>&1");
	}
	TRACE_DBG ("shell_cmd : %s", shell_cmd);

	return system(shell_cmd);
}

static void createBcmVopi (struct uci_bcmvopi_cfg *bcmvopi_cfg)
{
	if (bcmvopi_cfg->vid < 0) {
		snprintf(bcmvopi_cfg->vopi_name, MAX_VOPINAME_LEN, "%s_untagged", bcmvopi_cfg->if_name);
	} else {
		snprintf(bcmvopi_cfg->vopi_name, MAX_VOPINAME_LEN, "%s_v%d", bcmvopi_cfg->if_name, bcmvopi_cfg->vid);
	}

	/* make sure the real interface is up */
	executeShellCmd("%s %s up", ifconfig_bin, bcmvopi_cfg->if_name);

	/* create the virtual interface */
	executeShellCmd("%s --if %s --if-create-name %s %s --set-if-mode-rg",
			bcm_vlanctl_bin, bcmvopi_cfg->if_name, bcmvopi_cfg->if_name, bcmvopi_cfg->vopi_name);

	/* set filters and header manipulations */
	if (bcmvopi_cfg->vid >= 0) {
		/* RX */
		executeShellCmd("%s --if %s --rx --tags 1 --filter-vid %d 0 --pop-tag --set-rxif %s --rule-append",
				bcm_vlanctl_bin, bcmvopi_cfg->if_name, bcmvopi_cfg->vid, bcmvopi_cfg->vopi_name);
		/* TX */
		if(bcmvopi_cfg->dscp2pbits) {
			executeShellCmd("%s --if %s --tx --tags 0 --filter-txif %s --push-tag --set-vid %d 0 --dscp2pbits 0 --rule-append",
				bcm_vlanctl_bin, bcmvopi_cfg->if_name, bcmvopi_cfg->vopi_name, bcmvopi_cfg->vid);
		}
		else {
			executeShellCmd("%s --if %s --tx --tags 0 --filter-txif %s --push-tag --set-vid %d 0 --set-pbits %d 0 --rule-append",
				bcm_vlanctl_bin, bcmvopi_cfg->if_name, bcmvopi_cfg->vopi_name, bcmvopi_cfg->vid, bcmvopi_cfg->pbits);
		}
	}

	if (bcmvopi_cfg->defvid) {
		/* fix for untagged frames when vopi vid equal to interface default vid */
		executeShellCmd("%s --if %s --rx --tags 0 --default-miss-accept %s",
				bcm_vlanctl_bin, bcmvopi_cfg->if_name, bcmvopi_cfg->vopi_name);
	}
}

static void removeBcmVopi (const struct uci_bcmvopi_cfg *bcmvopi_cfg)
{
	/* remove the virtual interface */
	executeShellCmd("%s --if-delete %s", bcm_vlanctl_bin, bcmvopi_cfg->vopi_name);
}

static void saveUciState (const char* name, const struct uci_bcmvopi_cfg *bcmvopi_cfg)
{
	executeShellCmd("%s -P %s set %s.%s.bcmvopi=%s", uci_bin, savedir_state,
			bcmvopi_uci_pkg->e.name, name, bcmvopi_cfg->vopi_name);
}

static void clearUciState (const char *name)
{
	executeShellCmd("%s -P %s revert %s.%s.bcmvopi", uci_bin, savedir_state,
			bcmvopi_uci_pkg->e.name, name);
}

static int parseBcmVopiOptionString (struct uci_option *o, struct uci_bcmvopi_cfg *bcmvopi_cfg)
{
	int rv = 0;
	char *rest;

	do {
		if (!strcmp(o->e.name, "if")) {
			rv = (strlen(o->v.string) > MAX_IFNAME_LEN)? -1 : 0;
			if (!rv) {
				strncpy(bcmvopi_cfg->if_name, o->v.string, MAX_IFNAME_LEN);
			}
			break;
		}

		if (!strcmp(o->e.name, "vid")) {
			bcmvopi_cfg->vid = strtol(o->v.string, &rest, 10);
			rv = (*rest != '\0' || bcmvopi_cfg->vid < 0 || bcmvopi_cfg->vid > 4094)? -1 : 0;
			break;
		}

		if (!strcmp(o->e.name, "pbits")) {
			bcmvopi_cfg->pbits = strtol(o->v.string, &rest, 10);
			rv = (*rest != '\0' || bcmvopi_cfg->pbits < 0 || bcmvopi_cfg->pbits > 7)? -1 : 0;
			break;
		}

		if (!strcmp(o->e.name, "defvid")) {
			bcmvopi_cfg->defvid = (strcmp(o->v.string, "yes"))? 0 : 1;
			break;
		}

		/* bcmvopi name is currently automatically determined, but saved in UCI-state for unloading */
		if (!strcmp(o->e.name, "bcmvopi")) {
			rv = (strlen(o->v.string) > MAX_VOPINAME_LEN)? -1 : 0;
			if (!rv) {
				strncpy(bcmvopi_cfg->vopi_name, o->v.string, MAX_VOPINAME_LEN);
			}
			break;
		}

		if (!strcmp(o->e.name, "dscp2pbits")) {
			bcmvopi_cfg->dscp2pbits = (strcmp(o->v.string, "yes"))? 0 : 1;
			break;
		}
		/* unknown option */
	} while (0);

	return rv;
}

static int parseSectionBcmVopi (struct uci_section *s)
{
	struct uci_bcmvopi_cfg bcmvopi_cfg;
	struct uci_element *e, *e_tmp;
	struct uci_option *o;

	bcmvopi_cfg = def_bcmvopi_cfg;

	uci_foreach_element_safe(&s->options, e_tmp, e) {
		o = uci_to_option(e);
		if (o->type == UCI_TYPE_STRING) {
			TRACE_DBG ("bcmvopi option string '%s'='%s'", e->name, o->v.string);
			if (parseBcmVopiOptionString(o, &bcmvopi_cfg) < 0)
				return -1;
		} else {
			TRACE_DBG ("bcmvopi option list '%s'", e->name);
		}
	}

	/* check params */
	if (*bcmvopi_cfg.if_name == '\0')
		return -1;

	switch (bcmvopi_action) {
		case ACTION_LOAD:
			if (*bcmvopi_cfg.vopi_name == '\0') {
				createBcmVopi(&bcmvopi_cfg);
				/* save state for cleanup */
				saveUciState(s->e.name, &bcmvopi_cfg);
			}
			break;
		case ACTION_UNLOAD:
			if (*bcmvopi_cfg.vopi_name != '\0') {
				removeBcmVopi(&bcmvopi_cfg);
				/* clear state info */
				clearUciState(s->e.name);
			}
			break;
	}
	return 0;
}

static int bcmvopiconfig_init (const char *name)
{
	int rv;

	bcmvopi_uci_ctx = uci_alloc_context();
	uci_set_savedir(bcmvopi_uci_ctx, savedir_state);

	rv = (uci_load(bcmvopi_uci_ctx, name, &bcmvopi_uci_pkg) == UCI_OK)? 0  : -1;
	if (rv) {
		TRACE_ERR ("Loading the UCI configuration from file '%s' failed.", name);
	}
	return rv;
}

static int bcmvopiconfig_load (const char *name)
{
	struct uci_element *e;
	struct uci_section *s;

	if (bcmvopiconfig_init(name))
		return 1;

	/* Iterate over all UCI configuration sections */
	uci_foreach_element(&bcmvopi_uci_pkg->sections, e) {

		s = uci_to_section(e);
		TRACE_DBG ("UCI section '%s' (anon=%d, name=%s)", s->type, s->anonymous, (s->anonymous)? "-" : e->name);

		if (!strcmp(s->type, "bcmvopi")) {
			parseSectionBcmVopi(s);
		}
	}

	uci_free_context(bcmvopi_uci_ctx);
	return 0;
}

static void bcmvopiconfig_help (void)
{
	puts("\n"
	     "Broadcom VOPI (VLAN Operations Interface) configuration\n"
	     "\n"
	     "bcmvopiconfig load <name>   : load VOPI configuration from UCI (create interfaces)\n"
	     "bcmvopiconfig unload <name> : unload VOPI configuration (remove interfaces)\n"
	     "bcmvopiconfig help          : display this info\n");
}

static int bcmvopiconfig_cmd (int argc, char **argv)
{
	do {
		if (argc > 3)
			break;

		if (argc == 2 && !strcmp(argv[1], "help"))
			return 1;

		if (!strcmp(argv[1], "load")) {
			bcmvopi_action = ACTION_LOAD;
		} else if (!strcmp(argv[1], "unload")) {
			bcmvopi_action = ACTION_UNLOAD;
		}

		if (bcmvopi_action != ACTION_UNKNOWN
		    && !(bcmvopiconfig_load((argc == 2)? default_uci_package : argv[2])))
			return 0;
	} while (0);

	TRACE_ERR("invalid syntax");
	return -1;
}

int main (int argc, char **argv)
{
	int ret;

#ifdef BCMVOPICONFIG_DEBUG
	check_debug(argv[0]);
#endif

	ret = (argc > 1)? bcmvopiconfig_cmd(argc, argv) : 1;

	if (ret) {
		bcmvopiconfig_help();
	}
	return ret;
}

