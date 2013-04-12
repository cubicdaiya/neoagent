/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#ifndef NA_SOCKET_H
#define NA_SOCKET_H

#include <sys/types.h>
#include <sys/stat.h>

#define NA_HOSTNAME_MAX 256

typedef struct na_host_t {
    char ipaddr[NA_HOSTNAME_MAX + 1];
    uint16_t port;
} na_host_t;

void na_set_nonblock (int fd);
int na_target_server_tcpsock_init (void);
void na_target_server_tcpsock_setup (int tsfd, bool is_keepalive);
void na_target_server_hcsock_setup (int tsfd);
void na_set_sockaddr (na_host_t *host, struct sockaddr_in *addr);
na_host_t na_create_host(char *host);
int na_front_server_tcpsock_init (uint16_t port, int conn_max);
int na_front_server_unixsock_init (char *sockpath, mode_t mask, int conn_max);
int na_stat_server_unixsock_init (char *sockpath, mode_t mask);
int na_stat_server_tcpsock_init (uint16_t port);
bool na_server_connect (int tsfd, struct sockaddr_in *tsaddr);
int na_server_accept (int sfd);

#endif // NA_SOCKET_H
