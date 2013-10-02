/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "defines.h"

const int NA_STAT_BACKLOG_MAX = 32;
const int NA_IPADDR_MAX       = 15;

inline static bool na_is_ipaddr (const char *ipaddr);
static void na_set_sockopt(int fd, int optname);

inline static bool na_is_ipaddr (const char *ipaddr)
{
    return inet_addr(ipaddr) != INADDR_NONE;
}

static void na_set_sockopt(int fd, int optname)
{
    if (fd <= 0) {
        NA_DIE_WITH_ERROR(NULL, NA_ERROR_INVALID_FD);
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

void na_set_nonblock (int fd)
{
    if (fd > 0) {
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);
    } else {
        NA_DIE_WITH_ERROR(NULL, NA_ERROR_INVALID_FD);
    }
}

bool na_server_connect (int tsfd, struct sockaddr_in *tsaddr)
{
    if (connect(tsfd, (struct sockaddr *)tsaddr, sizeof(*tsaddr)) == -1) {
        return false;
    }
    return true;
}

int na_server_accept (int sfd)
{
    int cfd;
    struct sockaddr_in caddr;
    socklen_t clen = sizeof(caddr);

    if ((cfd = accept(sfd, (struct sockaddr *) &caddr, &clen)) < 0) {
        return -1;
    }
    return cfd;
}

int na_target_server_tcpsock_init (void)
{
    int tsfd;
    if ((tsfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        NA_ERROR_OUTPUT(NULL, "socket()");
        return -1;
    }
    return tsfd;
}

void na_target_server_tcpsock_setup (int tsfd, bool is_keepalive)
{
    na_set_nonblock(tsfd);
    if (is_keepalive) {
        na_set_sockopt(tsfd, SO_KEEPALIVE);
    }
    na_set_sockopt(tsfd, SO_REUSEADDR);
    na_set_sockopt(tsfd, SO_LINGER);
}

void na_target_server_hcsock_setup (int tsfd)
{
    struct timeval tv_recv, tv_send;
    na_set_sockopt(tsfd, SO_KEEPALIVE);
    na_set_sockopt(tsfd, SO_REUSEADDR);
    na_set_sockopt(tsfd, SO_LINGER);

    tv_recv.tv_sec  = 4;
    tv_recv.tv_usec = 0;
    tv_send.tv_sec  = 4;
    tv_send.tv_usec = 0;
    setsockopt(tsfd, SOL_SOCKET, SO_RCVTIMEO, &tv_recv, sizeof(struct timeval));
    setsockopt(tsfd, SOL_SOCKET, SO_SNDTIMEO, &tv_send, sizeof(struct timeval));
}

na_host_t na_create_host(char *host)
{
    // hostname example
    // 192.168.0.11:11211, example.com:20001
    na_host_t host_info;
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

    if (p == NULL || hostname_len <= 0 || hostname_len > NA_HOSTNAME_MAX) {
        NA_DIE_WITH_ERROR(NULL, NA_ERROR_INVALID_HOSTNAME);
    }

    strncpy(host_info.ipaddr, host, hostname_len);
    host_info.ipaddr[hostname_len] = '\0';

    p++;

    port = atoi(p);

    if (port <= 0) {
        NA_DIE_WITH_ERROR(NULL, NA_ERROR_INVALID_PORT);
    }

    host_info.port = port;

    return host_info;
}

void na_set_sockaddr (na_host_t *host, struct sockaddr_in *addr)
{
    memset(addr, 0, sizeof(*addr));

    if (na_is_ipaddr(host->ipaddr)) {
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
            NA_ERROR_OUTPUT(NULL, gai_strerror(ai_res));
            NA_DIE_WITH_ERROR(NULL, NA_ERROR_INVALID_HOSTNAME);
        }

        for (ai=ais;ai!=NULL;ai=ai->ai_next) {
            sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if (sock < 0) {
                continue;
            }

            na_set_sockopt(sock, SO_REUSEADDR);
            na_set_sockopt(sock, SO_LINGER);

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
            NA_DIE_WITH_ERROR(NULL, NA_ERROR_INVALID_HOSTNAME);
        }

        freeaddrinfo(ais);
    }
}

int na_front_server_tcpsock_init (uint16_t port, int conn_max)
{
    int fsfd;
    struct sockaddr_in iaddr;

    if ((fsfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        NA_ERROR_OUTPUT(NULL, "socket()");
        return -1;
    }

    na_set_nonblock(fsfd);
    na_set_sockopt(fsfd, SO_KEEPALIVE);
    na_set_sockopt(fsfd, SO_REUSEADDR);
    na_set_sockopt(fsfd, SO_LINGER);

    if (port > 0) {
        memset(&iaddr, 0, sizeof(iaddr));
        iaddr.sin_family      = AF_INET;
        iaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        iaddr.sin_port        = htons(port);
    } else {
        close(fsfd);
        NA_DIE_WITH_ERROR(NULL, NA_ERROR_INVALID_PORT);
    }

    if (bind(fsfd, (struct sockaddr *)&iaddr, sizeof(iaddr)) < 0) {
        close(fsfd);
        NA_ERROR_OUTPUT(NULL, "bind()");
        return -1;
    }

    if (listen(fsfd, conn_max) == -1) {
        close(fsfd);
        NA_ERROR_OUTPUT(NULL, "listen()");
        return -1;
    }

    return fsfd;
}

int na_front_server_unixsock_init (char *sockpath, mode_t mask, int conn_max)
{
    int fsfd;
    mode_t old_umask;
    struct stat tstat;
    struct sockaddr_un uaddr;

    if ((fsfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        NA_ERROR_OUTPUT(NULL, "socket()");
        return -1;
    }

    if (sockpath != NULL &&
        lstat(sockpath, &tstat) == 0 &&
        S_ISSOCK(tstat.st_mode))
    {
        unlink(sockpath);
    }

    na_set_nonblock(fsfd);
    na_set_sockopt(fsfd, SO_KEEPALIVE);
    na_set_sockopt(fsfd, SO_REUSEADDR);
    na_set_sockopt(fsfd, SO_LINGER);

    if (sockpath != NULL) {
        memset(&uaddr, 0, sizeof(uaddr));
        uaddr.sun_family = AF_UNIX;
        strncpy(uaddr.sun_path, sockpath, sizeof(uaddr.sun_path) - 1);
    }  else {
        close(fsfd);
        NA_DIE_WITH_ERROR(NULL, NA_ERROR_INVALID_SOCKPATH);
    }

    old_umask = umask(~(mask & 0777));

    if (bind(fsfd, (struct sockaddr *)&uaddr, sizeof(uaddr)) < 0) {
        close(fsfd);
        umask(old_umask);
        NA_ERROR_OUTPUT(NULL, "bind()");
        return -1;
    }

    umask(old_umask);

    if (listen(fsfd, conn_max) == -1) {
        close(fsfd);
        NA_ERROR_OUTPUT(NULL, "listen()");
        return -1;
    }

    return fsfd;
}

int na_stat_server_unixsock_init (char *sockpath, mode_t mask)
{
    int stfd;
    mode_t old_umask;
    struct stat tstat;
    struct sockaddr_un uaddr;

    if ((stfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        NA_ERROR_OUTPUT(NULL, "socket()");
        return -1;
    }

    if (sockpath != NULL &&
        lstat(sockpath, &tstat) == 0 &&
        S_ISSOCK(tstat.st_mode))
    {
        unlink(sockpath);
    }

    na_set_sockopt(stfd, SO_KEEPALIVE);
    na_set_sockopt(stfd, SO_REUSEADDR);
    na_set_sockopt(stfd, SO_LINGER);

    if (sockpath != NULL) {
        memset(&uaddr, 0, sizeof(uaddr));
        uaddr.sun_family = AF_UNIX;
        strncpy(uaddr.sun_path, sockpath, sizeof(uaddr.sun_path) - 1);
    }  else {
        close(stfd);
        NA_DIE_WITH_ERROR(NULL, NA_ERROR_INVALID_SOCKPATH);
    }

    old_umask = umask(~(mask & 0777));

    if (bind(stfd, (struct sockaddr *)&uaddr, sizeof(uaddr)) < 0) {
        close(stfd);
        umask(old_umask);
        NA_ERROR_OUTPUT(NULL, "bind()");
        return -1;
    }

    umask(old_umask);

    if (listen(stfd, NA_STAT_BACKLOG_MAX) == -1) {
        close(stfd);
        NA_ERROR_OUTPUT(NULL, "listen()");
        return -1;
    }

    return stfd;
}

int na_stat_server_tcpsock_init (uint16_t port)
{
    int stfd;
    struct sockaddr_in iaddr;

    if ((stfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        NA_ERROR_OUTPUT(NULL, "socket()");
        return -1;
    }

    na_set_sockopt(stfd, SO_REUSEADDR);
    na_set_sockopt(stfd, SO_LINGER);

    if (port > 0) {
        memset(&iaddr, 0, sizeof(iaddr));
        iaddr.sin_family      = AF_INET;
        iaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        iaddr.sin_port        = htons(port);
    } else {
        close(stfd);
        NA_DIE_WITH_ERROR(NULL, NA_ERROR_INVALID_PORT);
    }

    if (bind(stfd, (struct sockaddr *)&iaddr, sizeof(iaddr)) < 0) {
        close(stfd);
        NA_ERROR_OUTPUT(NULL, "bind()");
        return -1;
    }

    if (listen(stfd, NA_STAT_BACKLOG_MAX) == -1) {
        close(stfd);
        NA_ERROR_OUTPUT(NULL, "listen()");
        return -1;
    }

    return stfd;
}
