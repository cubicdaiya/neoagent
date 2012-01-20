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

#ifndef NA_SOCKET_H
#define NA_SOCKET_H

#include <sys/types.h>
#include <sys/stat.h>

#define NA_HOSTNAME_MAX 256

typedef struct na_host_t {
    char ipaddr[NA_HOSTNAME_MAX + 1];
    uint16_t port;
} na_host_t;

int na_target_server_tcpsock_init (void);
void na_target_server_tcpsock_setup (int tsfd, bool is_keepalive);
void na_target_server_hcsock_setup (int tsfd);
void na_set_sockaddr (na_host_t *host, struct sockaddr_in *addr);
na_host_t na_create_host(char *host);
int na_front_server_tcpsock_init (uint16_t port);
int na_front_server_unixsock_init (char *sockpath, mode_t mask);
int na_stat_server_tcpsock_init (uint16_t port);
bool na_server_connect (int tsfd, struct sockaddr_in *tsaddr);
int na_server_accept (int sfd);

#endif // NA_SOCKET_H
