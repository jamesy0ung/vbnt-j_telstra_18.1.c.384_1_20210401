/*
 * The source code form of this Open Source Project components is
 * subject to the terms of the Clear BSD license.
 * You can redistribute it and/or modify it under the terms of the
 * Clear BSD License (http://directory.fsf.org/wiki/License:ClearBSD)
 * See COPYING file/LICENSE file for more details.
 * This software project does also include third party Open Source
 * Software: See COPYING file/LICENSE file for more details.
 */

#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>

#include "preload.h"
#include "qos.h"

static const char PRELOAD_CONFIG[] = "/var/etc/preload-qos.cfg";

static int hook = 0;
static int env_appid = 0;
static struct {
  unsigned nf_mark;     // contains traffic id
  unsigned nf_mask;
  unsigned tc_class;    // contains traffic id, dscp/tos and pcp
} qos;


/**
 * @brief Verify if socket() should be redirected.
 *
 * Find the executable that is currently running and expand it's path, then
 * check the configuration file for an extry specifying this executable.
 * If a match is found, the command line arguments are string matched with
 * the config file, if an argument match is specified for the executable.
 *
 * If all the tests are positive, indicate that redirection is required.
 *
 * @return true if redirection is required.
 */
static int should_update_socket(const char *qualified_path, const char *cmd_args)
{
  FILE *config;
  char *env, *buf;
  size_t size;
  int line, n, match, rv;
  unsigned appid;
  char *path, *args;

  /* quick check for config file presence */
  if (access(PRELOAD_CONFIG, R_OK) != 0) {
    debug("qos config file not present (%s)", PRELOAD_CONFIG);
    return 0;
  }

  /* check if an environment variable QOS_APP_ID is set */
  env = getenv("QOS_APP_ID");
  if (env && sscanf(env, "%x", &env_appid) == 1) {
    debug("QOS_APP_ID set in environment (0x%08x)", env_appid);
  }

  /* check config file presence */
  config = fopen(PRELOAD_CONFIG, "r");
  if (config == NULL) {
    debug("qos config file failed to open (%s)", PRELOAD_CONFIG);
    return 0;
  }

  /* process config file line per line, return on first match */
  line = match = size = 0;
  buf = NULL;

  while (!match && !feof(config)) {
    line++;
    // read line, skip if empty
    if (getline(&buf, &size, config) <= 0)
      continue;

    rv = sscanf(buf, "%x/%x %x %n", &qos.nf_mark, &qos.nf_mask, &qos.tc_class, &n);
    if (rv < 3) {
      debug("qos skip invalid config line %d", line);
      continue;
    }
    rv = sscanf(buf + n, "APPID=%x", &appid);
    if (rv > 0) {
      if (appid == env_appid) {
        debug("qos line %d match on APPID=0x%04x (0x%08x/0x%08x 0x%08x)", line, appid, qos.nf_mark, qos.nf_mask, qos.tc_class);
        match = 1;
      }
    } else {
      path = args = NULL;
      rv = sscanf(buf + n, "'%m[^']' '%m[^']'", &path, &args);
      if (rv > 0 && !strcmp(path, qualified_path)) {
        if (rv == 1 // no arguments
            || strstr(cmd_args, args) != NULL) {
          debug("qos line %d match on path='%s' and args='%s' (0x%08x/0x%08x 0x%08x)", line, path, (args != NULL ? args : ""),
              qos.nf_mark, qos.nf_mask, qos.tc_class);
          match = 1;
        }
      }
      free(path);
      free(args);
    }
  }

  free(buf);
  fclose(config);

  return match;
}

/**
 * @brief socket - create an endpoint for communication
 *
 * This function is called after a the preloader intercepted a call
 * to the real libc() socket() function and a new socket was created.
 * It checks if for the current executable some qos options need to be
 * set on the socket.
 */
int preloader_qos_socket(int s, int domain, int type, int protocol)
{
  unsigned val;
  socklen_t len;

  if (!hook)
    return 0;

  /* traffic id */
  if (qos.nf_mark > 0) {
    debug("qos set nf_mark=0x%08x/0x%08x", qos.nf_mark, qos.nf_mask);
    len = sizeof(val);
    if (getsockopt(s, SOL_SOCKET, SO_MARK, (char*) &val, &len) < 0 || len != sizeof(val)) {
      perror("getsockopt SO_MARK");
    }
    val &= ~qos.nf_mask;
    val |= qos.nf_mark & qos.nf_mask;
    if (setsockopt(s, SOL_SOCKET, SO_MARK, (char*) &val, sizeof(val)) < 0) {
      perror("setsockopt SO_MARK");
    }
  }

  /* tc class */
  if (qos.tc_class > 0) {
      debug("qos set tc_class=0x%08x", qos.tc_class);
      if (setsockopt(s, SOL_SOCKET, SO_PRIORITY, (char*) &qos.tc_class, sizeof(qos.tc_class)) < 0)
          perror("setsockopt SO_PRIORITY");
  }

  return 0;
}

/**
 * @brief init - check if qos settings need to be applied
 *
 * */
void preloader_qos_init(const char *qualified_path, const char *cmd_args)
{
  hook = should_update_socket(qualified_path, cmd_args);
}
