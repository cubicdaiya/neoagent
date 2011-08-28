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

#ifndef NEOAGENT_ENV_H
#define NEOAGENT_ENV_H

#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ev.h>

#include "mpool.h"
#include "socket.h"
#include "memproto.h"

#define NEOAGENT_BUF_MAX      (512 * 1024)
#define NEOAGENT_NAME_MAX     64
#define NEOAGENT_SOCKPATH_MAX 256

typedef struct neoagent_server_t {
    neoagent_host_t host;
    struct sockaddr_in addr;
} neoagent_server_t;

typedef struct neoagent_connpool_t {
    int *fd_pool;
    int *mark;
    int cur;
    int max;
    bool is_full;
} neoagent_connpool_t;

typedef struct neoagent_env_t {
    char name[NEOAGENT_NAME_MAX + 1];
    int fsfd;
    uint16_t fsport;
    char fssockpath[NEOAGENT_SOCKPATH_MAX + 1];
    mode_t access_mask;
    neoagent_server_t target_server;
    neoagent_server_t backup_server;
    int current_conn;
    ev_io fs_watcher;
    bool is_refused_active;
    bool is_connpool_only;
    neoagent_connpool_t connpool_active;
    int error_count;
    int conn_max;
    int connpool_max;
    int error_count_max;
} neoagent_env_t;

typedef struct neoagent_spare_buf_t {
    char buf[NEOAGENT_BUF_MAX + 1];
    int bufsize;
    int ts_pos;
    struct neoagent_spare_buf_t *next;
} neoagent_spare_buf_t;

typedef struct neoagent_client_t {
    int cfd;
    int tsfd;
    char buf[NEOAGENT_BUF_MAX + 1];
    int bufsize;
    int ts_pos;
    int req_cnt;
    neoagent_memproto_cmd_t cmd;
    int current_req_cnt;
    int cur_pool;
    bool is_refused_active;
    bool is_use_connpool;
    neoagent_spare_buf_t *head_spare;
    neoagent_spare_buf_t *current_spare;
    neoagent_env_t *env;
    ev_io c_watcher;
    ev_io ts_watcher;
} neoagent_client_t;

mpool_t *neoagent_pool_create (int size);
void neoagent_pool_destroy (mpool_t *pool);
neoagent_env_t *neoagent_env_add (mpool_t **env_pool);
void neoagent_env_setup_default(neoagent_env_t *env, int idx);

void neoagent_connpool_create (neoagent_connpool_t *connpool, int c);
void neoagent_connpool_switch (neoagent_env_t *env, int c);

#endif // NEOAGENT_ENV_H
