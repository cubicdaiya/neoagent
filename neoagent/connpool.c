/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <unistd.h>
#include <errno.h>

#include "defines.h"

// private functions
static void na_connpool_deactivate (na_connpool_t *connpool);

static void na_connpool_deactivate (na_connpool_t *connpool)
{
    for (int i=0;i<connpool->max;++i) {
        if (connpool->fd_pool[i] > 0) {
            if (connpool->mark[i] == 0) {
                close(connpool->fd_pool[i]);
                connpool->fd_pool[i] = 0;
            }
            connpool->mark[i]   = 0;
            connpool->active[i] = 0;
        }
    }
}

void na_connpool_create (na_connpool_t *connpool, int c)
{
    connpool->fd_pool  = calloc(sizeof(int), c);
    connpool->mark     = calloc(sizeof(int), c);
    connpool->active   = calloc(sizeof(int), c);
    connpool->max      = c;
}

void na_connpool_destroy (na_connpool_t *connpool)
{
    NA_FREE(connpool->fd_pool);
    NA_FREE(connpool->mark);
}

void na_connpool_assign_internal (na_env_t *env, na_connpool_t *connpool, int i, int *cur, int *fd, na_server_t *server)
{
    if (connpool->active[i] == 0) {
        connpool->active[i] = 1;
        if (!na_server_connect(connpool->fd_pool[i], &server->addr)) {
            if (errno != EINPROGRESS && errno != EALREADY) {
                NA_DIE_WITH_ERROR(env, NA_ERROR_CONNECTION_FAILED);
            }
        }
    }
    connpool->mark[i] = 1;
    *fd  = connpool->fd_pool[i];
    *cur = i;
}

bool na_connpool_assign (na_env_t *env, na_connpool_t *connpool, int *cur, int *fd, na_server_t *server)
{
    int ri;

    pthread_mutex_lock(&env->lock_connpool);

    ri = rand() % env->connpool_max;
    if (connpool->mark[ri] == 0) {
        na_connpool_assign_internal(env, connpool, ri, cur, fd, server);
        pthread_mutex_unlock(&env->lock_connpool);
        return true;
    }


    switch (rand() % 2) {
    case 0:
        for (int i=env->connpool_max-1;i>=0;--i) {
            if (connpool->mark[i] == 0) {
                na_connpool_assign_internal(env, connpool, i, cur, fd, server);
                pthread_mutex_unlock(&env->lock_connpool);
                return true;
            }
        }
        break;
    default:
        for (int i=0;i<env->connpool_max;++i) {
            if (connpool->mark[i] == 0) {
                na_connpool_assign_internal(env, connpool, i, cur, fd, server);
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
            NA_DIE_WITH_ERROR(env, NA_ERROR_INVALID_FD);
        }
    }
}

na_connpool_t *na_connpool_select(na_env_t *env)
{
    if (env->is_refused_active) {
        return &env->connpool_backup;
    }
    return &env->connpool_active;
}

void na_connpool_switch (na_env_t *env)
{
    na_connpool_t *connpool;
    if (env->is_refused_active) {
        na_connpool_deactivate(&env->connpool_active);
        connpool = &env->connpool_backup;
    } else {
        na_connpool_deactivate(&env->connpool_backup);
        connpool = &env->connpool_active;
    }

    for (int i=0;i<env->connpool_max;++i) {
        connpool->fd_pool[i] = na_target_server_tcpsock_init();
        na_target_server_tcpsock_setup(connpool->fd_pool[i], true);
        if (connpool->fd_pool[i] <= 0) {
            NA_DIE_WITH_ERROR(env, NA_ERROR_INVALID_FD);
        }
    }
}
