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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>

#include "preload.h"
#include "mwan.h"

#define PRELOAD_CONFIG  "/var/etc/mwan.config"

#define MWAN_NF_MASK    0xF0000000

static int hook=0;
static int fwmark= 0;

/**
 * @brief Verify if sockets should be updated after creation
 *
 * Check the configuration file for an extry specifying this executable.
 * If a match is found, the command line arguments are string matched with
 * the config file, if an argument match is specified for the executable.
 *
 * If all the tests are positive, indicate that redirection is required.
 *
 * @return true if socket update is required.
 */
static int should_update_socket(const char *qualified_path, const char *args)
{
    int ret= -1;
    FILE *config= NULL;
    char *env, cmd[256];

    // check if an environment variable SO_MARK is defined and that
    // we can convert the first field to a hex number
    env = getenv("SO_MARK");
    if (env) {
        if (sscanf(env, "%x", &fwmark) == 1)
            return 1;
    }

    // quick check for config file
    if (access(PRELOAD_CONFIG, R_OK) != 0)
        goto err;

    // qualified_path contains the executable we need to check
    // now check versus local db.
    config= fopen(PRELOAD_CONFIG, "r");
    if (!config)
        goto err;

    while(!feof(config)) {
        int matchlen;
        char *p_match;
        char match[256];

        // read exe name and mark number
        if (fscanf(config, "%255s %x", cmd, &fwmark) != 2)
            goto err;

        // read rest of the line
        if (fgets(match, sizeof(match), config) == NULL)
            goto err;

        // strip trailing CR and whitespaces
        matchlen = strlen(match);
        while (matchlen && (match[matchlen-1] == '\n' || match[matchlen-1] == '\r' || match[matchlen-1] == ' '))
            match[--matchlen] = '\0';

        // skip leading spaces;
        p_match= match;
        while(p_match && *p_match == ' ') {
            p_match++;
            matchlen--;
        }

        // does command match what is specified in the config?
        if (!strcmp(qualified_path, cmd)) {

            // are the arguments also a match?
            if ( matchlen == 0 || (matchlen && strstr(args, p_match))) {
                 ret= 0;
                 break;
            }
        }
    }

err:
    if (config)
        fclose(config);

    return ret == 0;
}

/**
 * @brief socket - create an endpoint for communication
 *
 * This function is called after a the preloader intercepted a call
 * to the real libc() socket() function and a new socket was created.
 * It checks if for the current executable a firewall mark needs to be
 * set on the socket.
 */
int preloader_mwan_socket(int s, int domain, int type, int protocol)
{
    unsigned val;
    socklen_t len;

    if (!hook || (domain != AF_INET && domain != AF_INET6))
        return 0;

    len = sizeof(val);
    if (getsockopt(s, SOL_SOCKET, SO_MARK, (char*) &val, &len) < 0 || len != sizeof(val))
        perror("getsockopt SO_MARK");
    val &= ~MWAN_NF_MASK;
    val |= fwmark & MWAN_NF_MASK;
    if (setsockopt(s, SOL_SOCKET, SO_MARK, (char*) &val, sizeof(val)) < 0)
        perror("setsockopt SO_MARK");

    return 1;
}

/**
 * @brief init - check if mwan mark should be applied
 *
 * */
void preloader_mwan_init(const char *qualified_path, const char *args)
{
    hook = should_update_socket(qualified_path, args);
}

