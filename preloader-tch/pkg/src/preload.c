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
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>

#include "preload.h"
#include "mwan.h"
#include "qos.h"

static void init() __attribute__ ((constructor));
static int (*realsocket)(int domain, int type, int protocol) = NULL;

int tch_ld_debug = 0;

static char cmdline[256];
static char qualified_path[PATH_MAX];
static char *args; /* points to cmdline */

void __debug(const char* format, ...)
{
  va_list argptr;

  fprintf(stderr, "[preload debug] ");
  va_start(argptr, format);
  vfprintf(stderr, format, argptr);
  va_end(argptr);
  fprintf(stderr, "\n");
}

/**
 * Turn the source path into an absolute path.
 *
 * @param srcpath unqualified path that needs expanding
 * @param maxsize maximum size of the qualified path buffer
 * @param qualified_path resulting fully qualified path
 *
 * @return 0 on success
 */
static int expand_path(const char *srcpath, int maxsize, char *qualified_path)
{
    char path[PATH_MAX];
    char *sptr;
    int ret= -1;
    char *dptr;
    char *maxdptr = &qualified_path[maxsize];

    // check for relative path
    if (srcpath[0] != '/') {
        if (!getcwd(path, sizeof(path)-1))
            goto err;

        sptr = &path[strlen(path)];
        if (sptr[-1] != '/')
            *sptr++ = '/';

    } else {
        sptr= path;
    }

    // check for possible overrun
    if (strlen(srcpath) + (sptr-path) >= sizeof(path))
        goto err;

    // append user specified path
    strcpy(sptr, srcpath);

    sptr = path;
    dptr = qualified_path;

    *dptr++ = '/';
    while(*sptr != '\0') {
        if (*sptr == '/') {
            sptr++;
            continue;
        }

        if (*sptr == '.') {

            // skip single dot
            if (sptr[1] == '\0' || sptr[1] == '/') {
                sptr++;
                continue;
            }

            // ..
            if (sptr[1] == '.') {

                // dot dot or dot dot slash
                if (sptr[2] == '\0' || sptr[2] == '/') {
                    sptr += 2;

                    // don't move up when at root
                    if (dptr == &qualified_path[1])
                        continue;

                    // move up a directory, ie. strip last added component
                    while((--dptr)[-1] != '/');
                    continue;
                }
            }
        }

        // copy current path component to destination
        while(*sptr != '\0' && *sptr != '/') {

            // prevent buffer overrun
            if (dptr == maxdptr)
                goto err;

            *dptr++ = *sptr++;
        }

        *dptr++ = '/';
    }

    // strip last slash, unless we're at the root
    if (dptr != &qualified_path[1] && *(dptr-1) == '/')
        dptr--;

    // null terminate the path
    *dptr = '\0';

    ret= 0;

err:
    return ret;
}

/**
 * @brief Parse cmdline and resolve executable path
 *
 * Find the executable that is currently running and expand it's path
 * Store full path and cmd arguments
 *
 * @return true at success, false otherwise
 */
static int parse_cmdline(void)
{
    int cmdlen;
    int fd= -1;
    int ret= -1;
    int len;
    char current[PATH_MAX];
    struct stat st;

    // get ARGV[0] from proc
    fd= open("/proc/self/cmdline", O_RDONLY);
    if (fd == -1) {
        perror("open");
        goto err;
    }

    cmdlen= read(fd, cmdline, sizeof(cmdline)-1);
    close(fd); fd= -1;
    if (cmdlen <= 0) {
        perror("read");
        goto err;
    }
    cmdline[cmdlen] = 0; // NULL terminate
    len = strlen(cmdline);
    if (len+1 > cmdlen)
        args= NULL;
    else
        args= &cmdline[len+1];

    /* Is there a path component specified?
     * /xxxx
     * ../xxx
     * ./xx
     */
    if (cmdline[0] == '/' ||
            (cmdline[0] == '.' && (
                (len >= 2 && cmdline[1] == '.' && cmdline[2] == '/') ||
                (len >= 1 && cmdline[1] == '/'))
             )
            ) {
        ret= expand_path(cmdline, sizeof(qualified_path), qualified_path);
    } else {
        // need to resolve via PATH
        char *path= getenv("PATH");
        char *pe, *ce;

        if (!path)
            goto err;

        while(path && path != '\0') {
            ce = current;
            pe = strchr(path, ':');
            if (pe == NULL)
                pe = path + strlen(path);

            if (pe - path >= sizeof(current))
                goto err;

            memcpy(ce, path, pe-path);
            ce += pe-path;

            if (*pe == '\0')
                path = NULL;
            else
                path = pe+1;

            // skip empty PATH part, ie. ::
            if (current == ce)
                continue;

            // append command to path component
            if (ce - current + 1 + len >= sizeof(current))
                goto err;
            *ce++ = '/';
            strcpy(ce, cmdline);

            ret= expand_path(current, sizeof(qualified_path), qualified_path);
            if (ret == 0 && !access(qualified_path, X_OK) &&
                !stat(qualified_path, &st) && S_ISREG(st.st_mode)) {

                // executable file found in the path
                ret= 0;
                break;
            }
            ret= -1;
        }
    }
    // nothing found or expansion failed
    if (ret)
        goto err;

    // rebuild commandline of executable by replacing the NUL bytes
    // by spaces
    if (args) {
        char *ptr = strchr(args, 0);
        while(ptr < &cmdline[cmdlen]) {
            *ptr= ' ';
            ptr = strchr(ptr, 0);
        }
    }

err:
    return ret == 0;
}



/**
 * @brief socket - create an endpoint for communication
 *
 * This function intercepts the real libc() socket() function and
 * checks if for the current executable a firewall mark needs to be
 * set on the socket.
 *
 */
int socket(int domain, int type, int protocol)
{
    int s;

    if (!realsocket) {
        errno = ENFILE;
        return -1;
    }


    s = realsocket(domain, type, protocol);
    debug("socket created: %d (type=%u, protocol=%u) = %d", s, type, protocol);

    if (s >= 0) {
        preloader_mwan_socket(s, domain, type, protocol);
        preloader_qos_socket(s, domain, type, protocol);
    }

    return s;
}

/**
 * @brief init - get real socket() address and init mwan + qos
 *
 * This needs to happen in the constructor, because we can't do it in the
 * actual socket() call as that might cause issues in multi threaded apps
 * where socket() calls are run in parallel. To prevent threading issues,
 * we'd have to use pthread_mutex, but then every app we preload to will
 * include pthreads, which we don't want either.
 */
static void init()
{
    tch_ld_debug = (getenv("TCH_LD_DEBUG") != NULL);

    if (!realsocket) {
        realsocket = dlsym(RTLD_NEXT, "socket");

        if (parse_cmdline()) {
            preloader_mwan_init(qualified_path, args);
            preloader_qos_init(qualified_path, args);
        }
    }
}
