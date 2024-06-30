/*
 * Author: younes.behloul@technicolor.com
 *
 * Based on the ipchains code by Paul Russell and Michael Neuling
 *
 *  This is a deamonized version of iptables that execute commands
 *  it receives over a Unix socket. This daemon is needed by critical tasks
 *  that need run-time iptable changes but cannot afford the cost of calling
 *  fork()/sustem()
 *
 *	iptables -- IP firewall administration for kernels with
 *	firewall table (aimed for the 2.3 kernels)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/file.h>
#include <assert.h>
#include <syslog.h>
#include <iptables.h>
/*########################################################################
#                                                                        #
#  DEFINES                                                               #
#                                                                        #
########################################################################*/
#define SERVER_SOCK_PATH    "/tmp/mmpbx-fw-server"
#define SERVER_LOCK_PATH    "/tmp/mmpbx-fw-server.lock"

/*########################################################################
#                                                                        #
#  PRIVATE DATA MEMBERS                                                  #
#                                                                        #
########################################################################*/
bool static _daemonize = false;

/*########################################################################
#                                                                        #
#  PRIVATE FUNCTION PROTOTYPES                                           #
#                                                                        #
########################################################################*/
static int daemonize(char *name,
                     char *path,
                     char *outFile,
                     char *errFile,
                     char *inFile);
static int prepareSocket();
static void help();
static void handle_options(int  argc,
                           char **argv);

/*########################################################################
#                                                                        #
#  EXTERNAL FUNCTION PROTOTYPES                                          #
#                                                                        #
########################################################################*/
extern void libxt_CT_init(void);
extern void libxt_MARK_init(void);
extern void libxt_SET_init(void);
extern void libxt_TCPMSS_init(void);
extern void libxt_comment_init(void);
extern void libxt_conntrack_init(void);
extern void libxt_limit_init(void);
extern void libxt_mac_init(void);
extern void libxt_mark_init(void);
extern void libxt_multiport_init(void);
extern void libxt_set_init(void);
extern void libxt_standard_init(void);
extern void libxt_tcp_init(void);
extern void libxt_time_init(void);
extern void libxt_udp_init(void);

/*########################################################################
#                                                                        #
#  MAIN FUNCTION                                                         #
#                                                                        #
########################################################################*/
int main(int argc, char *argv[])
{
    int ret;
    int socketFd = -1;

    handle_options(argc, argv);

    /* daemonize task */
    if (_daemonize) {
        if ((ret = daemonize("mmpbxfwctl", "/tmp", NULL, NULL, NULL)) != 0) {
            fprintf(stderr, "error: daemonize failed\n");
            exit(EXIT_FAILURE);
        }
    }


    openlog("mmpbxfwctl", LOG_PID, LOG_DAEMON);


    syslog(LOG_NOTICE, "mmpbxfwctl daemon started\n");

    socketFd = prepareSocket(SERVER_SOCK_PATH);
    if (socketFd == -1) {
        syslog(LOG_ERR, "mmpbxfwctl daemon exiting\n");
        exit(EXIT_FAILURE);
    }

    iptables_globals.program_name = "iptables";
    ret = xtables_init_all(&iptables_globals, NFPROTO_IPV4);
    if (ret < 0) {
        syslog(LOG_CRIT, "%s/%s Failed to initialize xtables\n",
               iptables_globals.program_name,
               iptables_globals.program_version);
        exit(1);
    }

    libxt_CT_init();
    libxt_MARK_init();
    libxt_SET_init();
    libxt_comment_init();
    libxt_multiport_init();
    libxt_set_init();
    libxt_standard_init();
    libxt_tcp_init();
    libxt_time_init();
    libxt_udp_init();

    do {
        char              *table    = "filter";
        struct xtc_handle *handle   = NULL;
        char              cmd[512]  = {};
        int               rc;
        enum {
            kMaxArgs = 64
        };
        int   nargc = 0;
        char  *nargv[kMaxArgs];

        memset(cmd, 0, sizeof(cmd));
        syslog(LOG_DEBUG, "Waiting for cmd...\n");
        rc = recv(socketFd, cmd, sizeof(cmd), 0);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            else {
                syslog(LOG_ERR, "recv() failed with:%s\n", strerror(errno));
                break;
            }
        }
        syslog(LOG_DEBUG, "received cmd:%s", cmd);

        /* prase the recv command into argc/argv like couple */
        char *p2 = strtok(cmd, " ");
        while (p2 && nargc < kMaxArgs - 1) {
            nargv[nargc++]  = p2;
            p2              = strtok(NULL, " ");
        }

        /* make sure that nargv is termintaed with a NULL element*/
        if (nargv[nargc - 1] != NULL) {
            nargc++;
            nargv[nargc - 1] = NULL;
        }

        ret = do_command4(nargc - 1, nargv, &table, &handle, false);
        if (ret) {
            ret = iptc_commit(handle);
            iptc_free(handle);
        }

        if (!ret) {
            if (errno == EINVAL) {
                syslog(LOG_ERR, "%s. Run `dmesg' for more information.\n", iptc_strerror(errno));
            }
            else {
                syslog(LOG_ERR, "%s.\n", iptc_strerror(errno));
            }
            if (errno == EAGAIN) {
                syslog(LOG_ERR, "Ressource problem.\n");
            }
        }
    } while (true);

    close(socketFd);
    unlink(argv[1]);

    exit(!ret);
}

static int daemonize(char *name, char *path, char *outFile, char *errFile, char *inFile)
{
    if (!path) {
        path = "/";
    }
    if (!name) {
        name = "daemonexample";
    }
    if (!inFile) {
        inFile = "/dev/null";
    }
    if (!outFile) {
        outFile = "/dev/null";
    }
    if (!errFile) {
        errFile = "/dev/null";
    }

    pid_t child;
    //fork, detach from process group leader
    if ((child = fork()) < 0) { //failed fork
        fprintf(stderr, "error: failed fork\n");
        exit(EXIT_FAILURE);
    }
    if (child > 0) { //parent
        exit(EXIT_SUCCESS);
    }
    if (setsid() < 0) { //failed to become session leader
        fprintf(stderr, "error: failed setsid\n");
        exit(EXIT_FAILURE);
    }

    //catch/ignore signals
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    //fork second time
    if ((child = fork()) < 0) { //failed fork
        fprintf(stderr, "error: failed fork\n");
        exit(EXIT_FAILURE);
    }
    if (child > 0) { //parent
        exit(EXIT_SUCCESS);
    }

    //new file permissions
    umask(0);
    //change to path directory
    if (chdir(path) != 0) {
        fprintf(stderr, "chdir to:%s failed\n", path);
    }

    //Close all open file descriptors
    int fd;
    for (fd = sysconf(_SC_OPEN_MAX); fd > 0; --fd) {
        close(fd);
    }

    //reopen stdin, stdout, stderr
    stdin   = fopen(inFile, "r");   //fd=0
    stdout  = fopen(outFile, "w+"); //fd=1
    stderr  = fopen(errFile, "w+"); //fd=2

    //open syslog
    openlog(name, LOG_PID, LOG_DAEMON);
    return 0;
}

static int prepareSocket()
{
    struct sockaddr_un  serverAddr = {};
    int                 socketFd  = -1;
    int                 lockFd    = -1;
    int                 rc        = 0;

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    strncpy(serverAddr.sun_path, SERVER_SOCK_PATH, 104); // Linux limit is 108 see /usr/include/sys/un.h

    do {
        // create socket
        if ((socketFd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
            syslog(LOG_CRIT, "socket() failed with:%s\n", strerror(errno));
            break;
        }

        // open lock file
        lockFd = open(SERVER_LOCK_PATH, O_RDONLY | O_CREAT, 0600);
        if (lockFd == -1) {
            syslog(LOG_CRIT, "open(%s) failed with:%s\n", SERVER_LOCK_PATH, strerror(errno));
            break;
        }

        // try to acquire lock
        rc = flock(lockFd, LOCK_EX | LOCK_NB);
        if (rc != 0) {
            syslog(LOG_CRIT, "lock(%s) failed with:%s\n", SERVER_LOCK_PATH, strerror(errno));
            close(socketFd);
            socketFd = -1;
            break;
        }

        // remove socket file
        unlink(SERVER_SOCK_PATH);

        // bind client to client_filename
        if ((bind(socketFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr))) != 0) {
            syslog(LOG_CRIT, "bind() failed with:%s\n", strerror(errno));
            close(socketFd);
            unlink(SERVER_SOCK_PATH);
            socketFd = -1;
            break;
        }
    } while (false);

    return socketFd;
}

/****************************************************************************
 * help....
 */
static void help()
{
    printf("\n");
    printf("Usage: mmpbxfwctl [-d -h]\n");
    printf("Options are as follows:\n");
    printf(" -d            Run as a daemon (no CLI available also)\n");
    printf(" -h            Print this help text\n");
    printf("\n");
}

/****************************************************************************
 * Handle commandline options, uses the getopt tool
 * here we parse our optional commandline parameters into local data
 *
 * Parameters:
 *  - argc: see definition of main
 *  - argv: see definition of main.
 *          Remark that argvs are reshuffeld afterwards, so that all non
 *          recognised options are put at the end!
 *
 * Return: nothing
 */
static void handle_options(int  argc,
                           char **argv)
{
    int opt = 0;

    /* let getopt not print an error message */
    opterr = 0;

    /*
     * Parsing of optional parameters using getopt (man 3 getopt).
     * In short:
     * - pass main parameters
     * - pass string with:
     *   - "x:" if x is an option with argument, example:
     *          "-x filename"
     *          the optional filename will be returned by "char *optarg"
     *   - "x" if x is an option as such, example:
     *          "-x"
     * If options are obligatory, use a flag to detect if option is handled.
     * If getopt detects an invalid option, it will return '?' so you
     * can print the help message then.
     * In the example below we give an example of an obligatory filename an
     * optional daemonize and the optional help...
     */
    while ((opt = getopt(argc, argv, "dDhH")) != -1) {
        switch (opt) {
            case 'd':
            case 'D':
                _daemonize = true;
                break;

            case 'h':
            case 'H':
                help();
                exit(EXIT_SUCCESS);
                break;

            default:
                fprintf(stderr, "Invalid option given...\n");
                help();
                exit(EXIT_FAILURE);
                exit(1);
                break;
        }
    }

    return;
}
