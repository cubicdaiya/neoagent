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

#ifndef NA_ENV_H
#define NA_ENV_H

#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ev.h>

#include "mpool.h"
#include "socket.h"
#include "memproto.h"

#define NA_NAME_MAX     64
#define NA_SOCKPATH_MAX 256

typedef struct na_server_t {
    na_host_t host;
    struct sockaddr_in addr;
} na_server_t;

typedef struct na_connpool_t {
    int *fd_pool;
    int *mark;
    int cur;
    int max;
} na_connpool_t;

typedef enum na_event_state_t {
    NA_EVENT_STATE_CLIENT_READ,
    NA_EVENT_STATE_CLIENT_WRITE,
    NA_EVENT_STATE_TARGET_READ,
    NA_EVENT_STATE_TARGET_WRITE,
    NA_EVENT_STATE_COMPLETE,
    NA_EVENT_STATE_MAX // Always add new codes to the end before this one
} na_event_state_t;

typedef struct na_env_t {
    char name[NA_NAME_MAX + 1];
    int fsfd;
    uint16_t fsport;
    int stfd;
    uint16_t stport;
    char fssockpath[NA_SOCKPATH_MAX + 1];
    mode_t access_mask;
    na_server_t target_server;
    na_server_t backup_server;
    int current_conn;
    int bufsize;
    ev_io fs_watcher;
    bool is_refused_active;
    bool is_connpool_only;
    na_connpool_t connpool_active;
    na_connpool_t connpool_backup;
    int error_count;
    int conn_max;
    int connpool_max;
    int loop_max;
    int error_count_max;
} na_env_t;

typedef struct na_client_t {
    int cfd;
    int tsfd;
    char *crbuf;
    char *cwbuf;
    char *srbuf;
    char *swbuf;
    int crbufsize;
    int cwbufsize;
    int srbufsize;
    int swbufsize;
    na_memproto_cmd_t cmd;
    bool is_refused_active;
    bool is_use_connpool;
    na_env_t *env;
    na_event_state_t event_state;
    int req_cnt;
    int res_cnt;
    int loop_cnt;
    int cur_pool;
    ev_io c_watcher;
    ev_io ts_watcher;
} na_client_t;

mpool_t *na_pool_create (int size);
void na_pool_destroy (mpool_t *pool);
na_env_t *na_env_add (mpool_t **env_pool);
void na_env_setup_default(na_env_t *env, int idx);

void na_connpool_create (na_connpool_t *connpool, int c);
void na_connpool_switch (na_env_t *env, int c);

#endif // NA_ENV_H
