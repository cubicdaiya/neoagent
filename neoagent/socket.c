/**
   In short, neoagent is distributed under so called "BSD license",
   
   Copyright (c) 2011 Tatsuhiko Kubo <cubicdaiya@gmail.com>
   All rights reserved.
   
   Redistribution and use in source and binary forms, with or without modification, 
   are permitted provided that the following conditions are met:
   
   * Redistributions of source code must retain the above copyright notice, 
   this list of conditions and the following disclaimer.
   
   * Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.
   
   * Neither the name of the authors nor the names of its contributors 
   may be used to endorse or promote products derived from this software 
   without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>

#include <errno.h>
#include <assert.h>

#include "error.h"
#include "socket.h"

const int NEOAGENT_BACKLOG_MAX = 1024;
const int NEOAGENT_IPADDR_MAX  = 15;

inline static bool neoagent_is_ipaddr (const char *ipaddr);
static void neoagent_set_nonblock (int fd);

inline static bool neoagent_is_ipaddr (const char *ipaddr)
{
    return inet_addr(ipaddr) != INADDR_NONE;
}

static void neoagent_set_nonblock (int fd)
{
    if (fd > 0) {
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);
    } else {
        neoagent_die_with_error(NEOAGENT_ERROR_INVALID_FD);
    }
}

static void neoagent_set_sockopt(int fd, int optname)
{
    if (fd <= 0) {
        neoagent_die_with_error(NEOAGENT_ERROR_INVALID_FD);
    }

    switch (optname) {
    case SO_KEEPALIVE:
    case SO_REUSEADDR:
        {
            int flags = 1;
            setsockopt(fd, SOL_SOCKET, optname, (void *)&flags, sizeof(flags));
        }
        break;
    case SO_LINGER:
        {
            struct linger ling = {0, 0};
            setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
        }
        break;
    default :
        // no through
        assert(false);
        break;
    }
}

bool neoagent_server_connect (int tsfd, struct sockaddr_in *tsaddr)
{
    if (connect(tsfd, (struct sockaddr *)tsaddr, sizeof(*tsaddr)) == -1) {
        return false;
    }
    return true;
}

int neoagent_server_accept (int sfd)
{
    int cfd;
    struct sockaddr_in caddr;
    socklen_t clen = sizeof(caddr);
    
    if ((cfd = accept(sfd, (struct sockaddr *) &caddr, &clen)) < 0) {
        return -1;
    }
    return cfd;
}

int neoagent_target_server_tcpsock_init (void)
{
    int tsfd;
    if ((tsfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        NEOAGENT_STDERR("socket()");
        return -1;
    }
    return tsfd;
}

void neoagent_target_server_tcpsock_setup (int tsfd)
{
    neoagent_set_nonblock(tsfd);
    neoagent_set_sockopt(tsfd, SO_KEEPALIVE);
    neoagent_set_sockopt(tsfd, SO_REUSEADDR);
    neoagent_set_sockopt(tsfd, SO_LINGER);
}

void neoagent_target_server_healthchecksock_setup (int tsfd)
{
    neoagent_set_sockopt(tsfd, SO_REUSEADDR);
    neoagent_set_sockopt(tsfd, SO_LINGER);
}

neoagent_host_t neoagent_create_host(char *host)
{
    // hostname example
    // 192.168.0.11:11211, example.com:20001
    neoagent_host_t host_info;
    char *p;
    int16_t port;
    int hostname_len = 0;

    p = host;
    while (p != NULL) {
        if (*p == ':') {
            break;
        }
        hostname_len++;
        p++;
    }

    if (p == NULL || hostname_len <= 0 || hostname_len > NEOAGENT_HOSTNAME_MAX) {
        neoagent_die_with_error(NEOAGENT_ERROR_INVALID_HOSTNAME);
    }

    strncpy(host_info.ipaddr, host, hostname_len);
    host_info.ipaddr[hostname_len] = '\0';

    p++;

    port = atoi(p);

    if (port <= 0) {
        neoagent_die_with_error(NEOAGENT_ERROR_INVALID_PORT);
    }

    host_info.port = port;

    return host_info;
}

void neoagent_set_sockaddr (neoagent_host_t *host, struct sockaddr_in *addr)
{
    memset(addr, 0, sizeof(*addr));

    if (neoagent_is_ipaddr(host->ipaddr)) {
        addr->sin_family      = AF_INET;
        addr->sin_addr.s_addr = inet_addr(host->ipaddr);
        addr->sin_port        = htons(host->port);
    } else {
        struct addrinfo hints, *ais, *ai;
        char port_buf[NI_MAXSERV];
        int ai_res, sock;

        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_family   = AF_INET;
        snprintf(port_buf, sizeof(port_buf), "%d", host->port);

        if ((ai_res = getaddrinfo(host->ipaddr, port_buf, &hints, &ais)) != 0) {
            NEOAGENT_STDERR(gai_strerror(ai_res));
            neoagent_die_with_error(NEOAGENT_ERROR_INVALID_HOSTNAME);
        }

        for (ai=ais;ai!=NULL;ai=ai->ai_next) {
            sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if (sock < 0) {
                continue;
            }
 
            if (connect(sock, ai->ai_addr, ai->ai_addrlen) != 0) {
                close(sock);
                continue;
            }

            addr->sin_family      = ai->ai_family;
            addr->sin_addr.s_addr = ((struct sockaddr_in*)(ai->ai_addr))->sin_addr.s_addr;
            addr->sin_port        = htons(host->port);

            close(sock);

            break;
        }

        if (ai == NULL) {
            neoagent_die_with_error(NEOAGENT_ERROR_INVALID_HOSTNAME);
        }

        freeaddrinfo(ais);
    }
}

int neoagent_front_server_tcpsock_init (uint16_t port)
{
    int fsfd;
    struct sockaddr_in iaddr;
    
    if ((fsfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        NEOAGENT_STDERR("socket()");
        return -1;
    }

    neoagent_set_nonblock(fsfd);
    neoagent_set_sockopt(fsfd, SO_KEEPALIVE);
    neoagent_set_sockopt(fsfd, SO_REUSEADDR);
    neoagent_set_sockopt(fsfd, SO_LINGER);

    if (port > 0) {
        memset(&iaddr, 0, sizeof(iaddr));
        iaddr.sin_family      = AF_INET;
        iaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        iaddr.sin_port        = htons(port);
    } else {
        close(fsfd);
        neoagent_die_with_error(NEOAGENT_ERROR_INVALID_PORT);
    }

    if (bind(fsfd, (struct sockaddr *)&iaddr, sizeof(iaddr)) < 0) {
        close(fsfd);
        NEOAGENT_STDERR("bind()");
        return -1;
    }

    if (listen(fsfd, NEOAGENT_BACKLOG_MAX) == -1) {
        close(fsfd);
        NEOAGENT_STDERR("listen()");
        return -1;
    }

    return fsfd;
}

int neoagent_front_server_unixsock_init (char *sockpath, mode_t mask)
{
    int fsfd;
    mode_t old_umask;
    struct stat tstat;
    struct sockaddr_un uaddr;

    if ((fsfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        NEOAGENT_STDERR("socket()");
        return -1;
    }

    if (sockpath != NULL &&
        lstat(sockpath, &tstat) == 0 &&
        S_ISSOCK(tstat.st_mode))
    {
        unlink(sockpath);
    }

    neoagent_set_nonblock(fsfd);
    neoagent_set_sockopt(fsfd, SO_KEEPALIVE);
    neoagent_set_sockopt(fsfd, SO_REUSEADDR);
    neoagent_set_sockopt(fsfd, SO_LINGER);

    if (sockpath != NULL) {
        memset(&uaddr, 0, sizeof(uaddr));
        uaddr.sun_family = AF_UNIX;
        strncpy(uaddr.sun_path, sockpath, sizeof(uaddr.sun_path) - 1);
    }  else {
        close(fsfd);
        neoagent_die_with_error(NEOAGENT_ERROR_INVALID_SOCKPATH);
    }

    old_umask = umask(~(mask & 0777));

    if (bind(fsfd, (struct sockaddr *)&uaddr, sizeof(uaddr)) < 0) {
        close(fsfd);
        umask(old_umask);
        NEOAGENT_STDERR("bind()");
        return -1;
    }

    umask(old_umask);

    if (listen(fsfd, NEOAGENT_BACKLOG_MAX) == -1) {
        close(fsfd);
        NEOAGENT_STDERR("listen()");
        return -1;
    }

    return fsfd;
}
