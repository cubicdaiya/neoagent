/**
   In short, neoagent is distributed under so called "BSD license",
   
   Copyright (c) 2012 Tatsuhiko Kubo <cubicdaiya@gmail.com>
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

#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "error.h"
#include "connpool.h"
#include "util.h"

// private functions
static void na_connpool_deactivate (na_connpool_t *connpool);

static void na_connpool_deactivate (na_connpool_t *connpool)
{
    for (int i=0;i<connpool->max;++i) {
        if (connpool->fd_pool[i] > 0) {
            if (connpool->mark[i] == 0) {
                close(connpool->fd_pool[i]);
                connpool->fd_pool[i]  = 0;
                connpool->used_cnt[i] = 0;
            }
            connpool->mark[i] = 0;
        }
    }
}

void na_connpool_create (na_connpool_t *connpool, int c)
{
    connpool->fd_pool  = calloc(sizeof(int), c);
    connpool->mark     = calloc(sizeof(int), c);
    connpool->used_cnt = calloc(sizeof(int), c);
    connpool->max      = c;
}

void na_connpool_destroy (na_connpool_t *connpool)
{
    NA_FREE(connpool->fd_pool);
    NA_FREE(connpool->mark);
    NA_FREE(connpool->used_cnt);
}

void na_connpool_reconnect (na_env_t *env, int i)
{
    na_server_t *server;
    na_connpool_t *connpool;
    if (env->is_use_backup) {
        server = env->is_refused_active ? &env->backup_server : &env->target_server;
    } else {
        server = &env->target_server;
    }
    connpool = na_connpool_select(env);
    close(connpool->fd_pool[i]);
    connpool->fd_pool[i] = na_target_server_tcpsock_init();
    na_target_server_tcpsock_setup(connpool->fd_pool[i], true);
    if (connpool->fd_pool[i] <= 0) {
        NA_DIE_WITH_ERROR(NA_ERROR_INVALID_FD);
    }

    if (!na_server_connect(connpool->fd_pool[i], &server->addr)) {
        if (errno != EINPROGRESS && errno != EALREADY) {
            NA_DIE_WITH_ERROR(NA_ERROR_CONNECTION_FAILED);
        }
    }
}

bool na_connpool_assign (na_env_t *env, int *cur, int *fd)
{
    na_connpool_t *connpool;
    int ri;

    connpool = na_connpool_select(env);

    pthread_mutex_lock(&env->lock_connpool);

    ri = rand() % env->connpool_max;
    if (connpool->mark[ri] == 0) {
        connpool->mark[ri] = 1;
        if (++connpool->used_cnt[ri] > env->connpool_use_max) {
            na_connpool_reconnect(env, ri);
            connpool->used_cnt[ri] = 0;
        }
        *fd  = connpool->fd_pool[ri];
        *cur = ri;
        pthread_mutex_unlock(&env->lock_connpool);
        return true;
    }


    switch (rand() % 2) {
    case 0:
        for (int i=env->connpool_max-1;i>=0;--i) {
            if (connpool->mark[i] == 0) {
                connpool->mark[i] = 1;
                if (++connpool->used_cnt[i] > env->connpool_use_max) {
                    na_connpool_reconnect(env, i);
                    connpool->used_cnt[i] = 0;
                }
                *fd  = connpool->fd_pool[i];
                *cur = i;
                pthread_mutex_unlock(&env->lock_connpool);
                return true;
            }
        }
        break;
    default:
        for (int i=0;i<env->connpool_max;++i) {
            if (connpool->mark[i] == 0) {
                connpool->mark[i] = 1;
                if (++connpool->used_cnt[i] > env->connpool_use_max) {
                    na_connpool_reconnect(env, i);
                    connpool->used_cnt[i] = 0;
                }
                *fd  = connpool->fd_pool[i];
                *cur = i;
                pthread_mutex_unlock(&env->lock_connpool);
                return true;
            }
        }
        break;
    }
    pthread_mutex_unlock(&env->lock_connpool);
    return false;
}

void na_connpool_init (na_env_t *env)
{
    for (int i=0;i<env->connpool_max;++i) {
        env->connpool_active.fd_pool[i] = na_target_server_tcpsock_init();
        na_target_server_tcpsock_setup(env->connpool_active.fd_pool[i], true);
        if (env->connpool_active.fd_pool[i] <= 0) {
            NA_DIE_WITH_ERROR(NA_ERROR_INVALID_FD);
        }
        
        if (!na_server_connect(env->connpool_active.fd_pool[i], &env->target_server.addr)) {
            if (errno != EINPROGRESS && errno != EALREADY) {
                NA_DIE_WITH_ERROR(NA_ERROR_CONNECTION_FAILED);
            }
        }
    }
}

na_connpool_t *na_connpool_select(na_env_t *env)
{
    pthread_rwlock_rdlock(&env->lock_refused);
    if (env->is_refused_active) {
        pthread_rwlock_unlock(&env->lock_refused);
        return &env->connpool_backup;
    }
    pthread_rwlock_unlock(&env->lock_refused);
    return &env->connpool_active;
}

void na_connpool_switch (na_env_t *env)
{
    na_connpool_t *connpool;
    na_server_t   *server;
    if (env->is_refused_active) {
        na_connpool_deactivate(&env->connpool_active);
        connpool = &env->connpool_backup;
        server   = &env->backup_server;
    } else {
        na_connpool_deactivate(&env->connpool_backup);
        connpool = &env->connpool_active;
        server   = &env->target_server;
    }

    for (int i=0;i<env->connpool_max;++i) {
        connpool->fd_pool[i] = na_target_server_tcpsock_init();
        na_target_server_tcpsock_setup(connpool->fd_pool[i], true);
        if (connpool->fd_pool[i] <= 0) {
            NA_DIE_WITH_ERROR(NA_ERROR_INVALID_FD);
        }
        
        if (!na_server_connect(connpool->fd_pool[i], &server->addr)) {
            if (errno != EINPROGRESS && errno != EALREADY) {
                NA_DIE_WITH_ERROR(NA_ERROR_CONNECTION_FAILED);
            }
        }
    }
}
