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

#ifndef NEOAGENT_SOCKET_H
#define NEOAGENT_SOCKET_H

#include <sys/types.h>
#include <sys/stat.h>

#define NEOAGENT_HOSTNAME_MAX 256

typedef struct neoagent_host_t {
    char ipaddr[NEOAGENT_HOSTNAME_MAX + 1];
    uint16_t port;
} neoagent_host_t;

int neoagent_target_server_tcpsock_init (void);
void neoagent_target_server_tcpsock_setup (int tsfd);
void neoagent_target_server_healthchecksock_setup (int tsfd);
void neoagent_set_sockaddr (neoagent_host_t *host, struct sockaddr_in *addr);
neoagent_host_t neoagent_create_host(char *host);
int neoagent_front_server_tcpsock_init (uint16_t port);
int neoagent_front_server_unixsock_init (char *sockpath, mode_t mask);
int neoagent_stat_server_tcpsock_init (uint16_t port);
bool neoagent_server_connect (int tsfd, struct sockaddr_in *tsaddr);
int neoagent_server_accept (int sfd);


#endif // NEOAGENT_SOCKET_H
