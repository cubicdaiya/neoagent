/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "defines.h"

static const int  NA_PORT_DEFAULT             = 30001;
static const int  NA_STPORT_DEFAULT           = 30011;
static const int  NA_CONN_MAX_DEFAULT         = 1000;
static const int  NA_CONNPOOL_MAX_DEFAULT     = 20;
static const int  NA_CONNPOOL_USE_MAX_DEFAULT = 10000;
static const int  NA_CLIENT_POOL_MAX_DEFAULT  = 20;
static const int  NA_ACCESS_MASK_DEFAULT      = 0664;
static const int  NA_BUFSIZE_DEFAULT          = 65536;
static const int  NA_WORKER_MAX_DEFAULT       = 1;

// refs to external globals
extern pthread_rwlock_t LockReconf;

void na_ctl_env_setup_default(na_ctl_env_t *ctl_env)
{
    ctl_env->access_mask = NA_ACCESS_MASK_DEFAULT;
}

void na_env_setup_default(na_env_t *env, int idx)
{
    char *target_server_s = "127.0.0.1:11211";
    na_host_t host;
    sprintf(env->name, "env%d", idx);
    env->fsport                  = NA_PORT_DEFAULT + idx;
    env->access_mask             = NA_ACCESS_MASK_DEFAULT;
    host                         = na_create_host(target_server_s);
    memcpy(&env->target_server.host, &host, sizeof(host));
    na_set_sockaddr(&host, &env->target_server.addr);
    env->stport                  = NA_STPORT_DEFAULT + idx;
    env->worker_max              = NA_WORKER_MAX_DEFAULT;
    env->conn_max                = NA_CONN_MAX_DEFAULT;
    env->connpool_max            = NA_CONNPOOL_MAX_DEFAULT;
    env->connpool_use_max        = NA_CONNPOOL_USE_MAX_DEFAULT;
    env->client_pool_max         = NA_CLIENT_POOL_MAX_DEFAULT;
    env->is_use_backup           = false;
    env->request_bufsize         = NA_BUFSIZE_DEFAULT;
    env->response_bufsize        = NA_BUFSIZE_DEFAULT;
    memset(&env->slow_query_sec, 0, sizeof(struct timespec));
    env->slow_query_fp = NULL;
    env->slow_query_log_format   = NA_LOG_FORMAT_PLAIN;
    env->slow_query_log_access_mask = NA_ACCESS_MASK_DEFAULT;
}

void na_env_init(na_env_t *env)
{
    env->current_conn      = 0;
    env->is_refused_active = false;
    env->is_refused_accept = false;
    env->is_worker_busy    = calloc(sizeof(bool), env->worker_max);
    for (int j=0;j<env->worker_max;++j) {
        env->is_worker_busy[j] = false;
    }
    env->current_conn_max = 0;
    pthread_mutex_init(&env->lock_connpool,     NULL);
    pthread_mutex_init(&env->lock_current_conn, NULL);
    pthread_mutex_init(&env->lock_tid,          NULL);
    pthread_mutex_init(&env->lock_loop,         NULL);
    pthread_rwlock_init(&env->lock_refused, NULL);
    pthread_rwlock_init(&LockReconf,        NULL);
    env->lock_worker_busy = calloc(sizeof(pthread_rwlock_t), env->worker_max);
    for (int j=0;j<env->worker_max;++j) {
        pthread_rwlock_init(&env->lock_worker_busy[j], NULL);
    }
    na_connpool_create(&env->connpool_active, env->connpool_max);
    if (env->is_use_backup) {
        na_connpool_create(&env->connpool_backup, env->connpool_max);
    }
}
