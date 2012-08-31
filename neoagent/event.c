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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>

#include <ev.h>

#include "event.h"
#include "connpool.h"
#include "env.h"
#include "socket.h"
#include "error.h"
#include "memproto.h"
#include "stat.h"
#include "hc.h"
#include "util.h"
#include "queue.h"

#define NA_EVENT_FAIL(na_error, loop, w, client, env) do {  \
        na_event_stop(loop, w, client, env);                \
        na_error_count_up(env);                             \
        NA_STDERR_MESSAGE(na_error);                        \
    } while(false)
 
// globals
static na_client_t *ClientPool;
static na_event_queue_t *EventQueue = NULL;

// external globals
volatile sig_atomic_t SigExit;
volatile sig_atomic_t SigClear;

// private functions
inline static void na_event_stop (EV_P_ struct ev_io *w, na_client_t *client, na_env_t *env);
inline static void na_event_switch (EV_P_ struct ev_io *old, ev_io *new, int fd, int revent);

static struct ev_loop *na_event_loop_create (na_event_model_t model);
static int na_client_assign (na_env_t *env);
static void na_client_close (EV_P_ na_client_t *client, na_env_t *env);
static void na_target_server_callback (EV_P_ struct ev_io *w, int revents);
static void na_client_callback (EV_P_ struct ev_io *w, int revents);
static void na_front_server_callback (EV_P_ struct ev_io *w, int revents);
static bool na_is_worker_busy(na_env_t *env);
static void *na_event_observer(void *args);
static void *na_support_loop (void *args);

inline static void na_event_stop (EV_P_ struct ev_io *w, na_client_t *client, na_env_t *env)
{
    ev_io_stop(EV_A_ w);
    na_client_close(EV_A_ client, env);
}

inline static void na_event_switch (EV_P_ struct ev_io *old, struct ev_io *new, int fd, int revent)
{
    ev_io_stop(EV_A_ old);
    ev_io_set(new, fd, revent);
    ev_io_start(EV_A_ new);
}

static struct ev_loop *na_event_loop_create(na_event_model_t model)
{
    struct ev_loop *loop;
    switch (model) {
    case NA_EVENT_MODEL_AUTO:
        loop = ev_loop_new(EVFLAG_AUTO);
        break;
    case NA_EVENT_MODEL_SELECT:
        loop = ev_loop_new(EVBACKEND_SELECT);
        break;
    case NA_EVENT_MODEL_EPOLL:
        loop = ev_loop_new(EVBACKEND_EPOLL);
        break;
    case NA_EVENT_MODEL_KQUEUE:
        loop = ev_loop_new(EVBACKEND_KQUEUE);
        break;
    default:
        // no through
        assert(false);
        break;
    }
    return loop;
}

static int na_client_assign (na_env_t *env)
{
    int ri;

    ri = rand() % env->client_pool_max;
    if (ClientPool[ri].is_used == false) {
        ClientPool[ri].is_used = true;
        return ri;
    }

    switch (rand() % 2) {
    case 0:
        for (int i=env->client_pool_max-1;i>=0;--i) {
            if (ClientPool[i].is_used == false) {
                ClientPool[i].is_used = true;
                return i;
            }
        }
        break;
    default:
        for (int i=0;i<env->client_pool_max;++i) {
            if (ClientPool[i].is_used == false) {
                ClientPool[i].is_used = true;
                return i;
            }
        }
        break;
    }
    return -1;
}

static void na_client_close (EV_P_ na_client_t *client, na_env_t *env)
{
    close(client->cfd);
    ev_io_stop(EV_A_ &client->c_watcher);
    ev_io_stop(EV_A_ &client->ts_watcher);
    client->cfd = -1;
    pthread_mutex_lock(&env->lock_connpool);
    if (client->is_use_connpool) {
        if (client->connpool->mark[client->cur_pool] == 0) {
            close(client->connpool->fd_pool[client->cur_pool]);
        }
        client->tsfd = -1;
        client->connpool->mark[client->cur_pool] = 0;
    } else {
        close(client->tsfd);
        client->tsfd = -1;
    }
    pthread_mutex_unlock(&env->lock_connpool);

    if (client->is_use_client_pool) {
        client->is_used = false;
    } else {
        NA_FREE(client->crbuf);
        NA_FREE(client->srbuf);
        NA_FREE(client);
    }

    pthread_mutex_lock(&env->lock_current_conn);
    if (env->current_conn > 0) {
        --env->current_conn;
    }
    pthread_mutex_unlock(&env->lock_current_conn);
}

static void na_target_server_callback (EV_P_ struct ev_io *w, int revents)
{
    int cfd, tsfd, size;
    na_client_t *client;
    na_env_t *env;

    tsfd   = w->fd;
    client = (na_client_t *)w->data;
    env    = client->env;
    cfd    = client->cfd;

    pthread_rwlock_rdlock(&env->lock_refused);
    if ((client->is_refused_active != env->is_refused_active) || env->is_refused_accept) {
        pthread_rwlock_unlock(&env->lock_refused);
        NA_EVENT_FAIL(NA_ERROR_INVALID_CONNPOOL, EV_A, w, client, env);
        return; // request fail
    }
    pthread_rwlock_unlock(&env->lock_refused);

    if (env->loop_max > 0 && client->loop_cnt++ > env->loop_max) {
        NA_EVENT_FAIL(NA_ERROR_OUTOF_LOOP, EV_A, w, client, env);
        return; // request fail
    }

    if (revents & EV_READ) {

        if (!env->is_extensible_response_buf) {
            if (client->srbufsize >= client->response_bufsize) {
                NA_EVENT_FAIL(NA_ERROR_OUTOF_BUFFER, EV_A, w, client, env);
                return; // request fail
            }
        } else if (client->response_bufsize > env->response_bufsize_max) {
            NA_EVENT_FAIL(NA_ERROR_OUTOF_BUFFER, EV_A, w, client, env);
            return; // request fail
        } else if (client->srbufsize >= client->response_bufsize) {
            size_t es;
            es = (client->response_bufsize - 1) * 2;
            if (es >= env->response_bufsize_max) {
                es = env->response_bufsize_max;
            }
            client->srbuf = (char *)realloc(client->srbuf, es + 1);
            client->response_bufsize = es;
        }

        size = read(tsfd, 
                    client->srbuf + client->srbufsize,
                    client->response_bufsize - client->srbufsize);

        if (size <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return; // not ready yet
            }
            NA_EVENT_FAIL(NA_ERROR_FAILED_READ, EV_A, w, client, env);
            return; // request fail
        }

        client->srbufsize                += size;
        client->srbuf[client->srbufsize]  = '\0';

        if (client->srbufsize > env->response_bufsize_current_max) {
            env->response_bufsize_current_max = client->srbufsize;
        }

        if (client->cmd == NA_MEMPROTO_CMD_GET) {
            client->res_cnt = na_memproto_count_response_get(client->srbuf, client->srbufsize);
            if (client->res_cnt >= client->req_cnt) {
                client->event_state = NA_EVENT_STATE_CLIENT_WRITE;
                na_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_WRITE);
                return;
            }
        } else if (client->srbufsize > 2 &&
                   client->srbuf[client->srbufsize - 2] == '\r' &&
                   client->srbuf[client->srbufsize - 1] == '\n')
        {
            client->event_state = NA_EVENT_STATE_CLIENT_WRITE;
            na_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_WRITE);
        }

    } else if (revents & EV_WRITE) {

        size = write(tsfd, 
                     client->crbuf + client->swbufsize, 
                     client->crbufsize - client->swbufsize);

        if (size < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return; // not ready yet
            } else if (client->is_use_connpool) {
                int i = client->cur_pool;
                na_server_t *server;
                pthread_mutex_lock(&env->lock_connpool);
                if (client->connpool->fd_pool[i] > 0) {
                    close(client->connpool->fd_pool[i]);
                }
                client->connpool->fd_pool[i] = na_target_server_tcpsock_init();
                if (env->is_use_backup) {
                    pthread_rwlock_rdlock(&env->lock_refused);
                    server = env->is_refused_active ? &env->backup_server : &env->target_server;
                    pthread_rwlock_unlock(&env->lock_refused);
                } else {
                    server = &env->target_server;
                }
                na_target_server_tcpsock_setup(client->connpool->fd_pool[i], true);
                if (client->connpool->fd_pool[i] <= 0) {
                    pthread_mutex_unlock(&env->lock_connpool);
                    NA_DIE_WITH_ERROR(NA_ERROR_INVALID_FD);
                }

                if (!na_server_connect(client->connpool->fd_pool[i], &server->addr)) {
                    if (errno != EINPROGRESS && errno != EALREADY) {
                        pthread_mutex_unlock(&env->lock_connpool);
                        na_error_count_up(env);
                        NA_DIE_WITH_ERROR(NA_ERROR_CONNECTION_FAILED);
                    }
                }
                pthread_mutex_unlock(&env->lock_connpool);
            }

            if (errno == EPIPE) {
                NA_EVENT_FAIL(NA_ERROR_BROKEN_PIPE, EV_A, w, client, env);
            } else {
                NA_EVENT_FAIL(NA_ERROR_FAILED_WRITE, EV_A, w, client, env);
            }
            return; // request fail
        }

        client->swbufsize += size;

        if (client->swbufsize < client->crbufsize) {
            na_event_switch(EV_A_ w, &client->ts_watcher, tsfd, EV_WRITE);
        } else {
            client->event_state = NA_EVENT_STATE_TARGET_READ;
            na_event_switch(EV_A_ w, &client->ts_watcher, tsfd, EV_READ);
        }
        return;
    }

}

static void na_client_callback(EV_P_ struct ev_io *w, int revents)
{
    int cfd, tsfd, size;
    na_client_t *client;
    na_env_t *env;

    cfd    = w->fd;
    client = (na_client_t *)w->data;
    env    = client->env;
    tsfd   = client->tsfd;

    pthread_rwlock_rdlock(&env->lock_refused);
    if ((client->is_refused_active != env->is_refused_active) || env->is_refused_accept) {
        pthread_rwlock_unlock(&env->lock_refused);
        NA_EVENT_FAIL(NA_ERROR_INVALID_CONNPOOL, EV_A, w, client, env);
        return; // request fail
    }
    pthread_rwlock_unlock(&env->lock_refused);

    if (env->loop_max > 0 && client->loop_cnt++ > env->loop_max) {
        NA_EVENT_FAIL(NA_ERROR_OUTOF_LOOP, EV_A, w, client, env);
        return; // request fail
    }

    if (revents & EV_READ) {

        if (!env->is_extensible_request_buf) {
            if (client->crbufsize >= client->request_bufsize) {
                NA_EVENT_FAIL(NA_ERROR_OUTOF_BUFFER, EV_A, w, client, env);
                return; // request fail
            }
        } else if (client->request_bufsize >= env->request_bufsize_max) {
            NA_EVENT_FAIL(NA_ERROR_OUTOF_BUFFER, EV_A, w, client, env);
            return; // request fail
        } else if (client->crbufsize >= client->request_bufsize) {
            size_t es;
            es = (client->request_bufsize - 1) * 2;
            if (es >= env->request_bufsize_max) {
                es = env->request_bufsize_max;
            }
            client->crbuf = (char *)realloc(client->crbuf, es + 1);
            client->request_bufsize = es;
        }

        size = read(cfd, 
                    client->crbuf + client->crbufsize,
                    client->request_bufsize - client->crbufsize);

        if (size == 0) {
            na_event_stop(EV_A_ w, client, env);
            return; // request success
        } else if (size < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return; // not ready yet
            }
            NA_EVENT_FAIL(NA_ERROR_FAILED_READ, EV_A, w, client, env);
            return; // request fail
        }

        client->crbufsize                += size;
        client->crbuf[client->crbufsize]  = '\0';

        if (client->crbufsize > env->request_bufsize_current_max) {
            env->request_bufsize_current_max = client->crbufsize;
        }

        client->cmd = na_memproto_detect_command(client->crbuf);

        if (client->cmd == NA_MEMPROTO_CMD_QUIT) {
            na_event_stop(EV_A_ w, client, env);
            return; // request success
        } else if (client->cmd == NA_MEMPROTO_CMD_GET) {
            client->req_cnt = na_memproto_count_request_get(client->crbuf, client->crbufsize);
        }

        if (client->crbufsize < 2) {
            return; // not ready yet
        } else if (client->crbuf[client->crbufsize - 2] == '\r' &&
                   client->crbuf[client->crbufsize - 1] == '\n')
        {
            if (client->cmd == NA_MEMPROTO_CMD_UNKNOWN) {
                na_event_stop(EV_A_ w, client, env);
                return; // request fail
            }
            client->event_state = NA_EVENT_STATE_TARGET_WRITE;
            na_event_switch(EV_A_ w, &client->ts_watcher, tsfd, EV_WRITE);
            return;
        }

    } else if (revents & EV_WRITE) {

        size = write(cfd,
                     client->srbuf + client->cwbufsize,
                     client->srbufsize - client->cwbufsize);

        if (size < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return; // not ready yet
            }
            if (errno == EPIPE) {
                NA_EVENT_FAIL(NA_ERROR_BROKEN_PIPE, EV_A, w, client, env);
            } else {
                NA_EVENT_FAIL(NA_ERROR_FAILED_WRITE, EV_A, w, client, env);
            }
            return; // request fail
        }

        client->cwbufsize += size;
        if (client->cwbufsize < client->srbufsize) {
            na_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_WRITE);
            return;
        } else {
            client->crbufsize        = 0;
            client->cwbufsize        = 0;
            client->srbufsize        = 0;
            client->swbufsize        = 0;
            client->request_bufsize  = env->request_bufsize;
            client->response_bufsize = env->response_bufsize;
            client->event_state      = NA_EVENT_STATE_CLIENT_READ;
            client->req_cnt          = 0;
            client->res_cnt          = 0;
            na_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_READ);
            return;
        }
        
    }
}

void na_front_server_callback (EV_P_ struct ev_io *w, int revents)
{
    int fsfd, cfd, tsfd, cur_pool, cur_cli;
    int th_ret;
    na_env_t *env;
    na_client_t *client;
    na_connpool_t *connpool;

    th_ret   = 0;
    fsfd     = w->fd;
    env      = (na_env_t *)w->data;
    cfd      = -1;
    tsfd     = -1;
    cur_pool = -1;
    cur_cli  = -1;

    if (SigExit == 1) {
        pthread_exit(&th_ret);
        return;
    }

    if (SigClear == 1) {
        na_env_clear(env);
        SigClear = 0;
    }

    pthread_rwlock_rdlock(&env->lock_refused);
    if (env->is_refused_accept) {
        pthread_rwlock_unlock(&env->lock_refused);
        return;
    }
    pthread_rwlock_unlock(&env->lock_refused);
    
    if (env->error_count_max > 0 && (env->error_count > env->error_count_max)) {
        env->error_count = 0;
        return;
    }
    
    pthread_mutex_lock(&env->lock_current_conn);
    if (env->current_conn >= env->conn_max) {
        pthread_mutex_unlock(&env->lock_current_conn);
        return;
    }
    pthread_mutex_unlock(&env->lock_current_conn);

    connpool = na_connpool_select(env);

    if (env->is_connpool_only) {
        pthread_mutex_lock(&env->lock_current_conn);
        if (env->current_conn >= connpool->max) {
            pthread_mutex_unlock(&env->lock_current_conn);
            return;
        }
        pthread_mutex_unlock(&env->lock_current_conn);
        if (!na_connpool_assign(env, &cur_pool, &tsfd)) {
            na_error_count_up(env);
            NA_STDERR("failed assign connection from connpool.");
            return;
        }
    } else {
        if (!na_connpool_assign(env, &cur_pool, &tsfd)) {
            na_server_t *server;
            tsfd = na_target_server_tcpsock_init();
            if (tsfd < 0) {
                na_error_count_up(env);
                NA_STDERR_MESSAGE(NA_ERROR_INVALID_FD);
                return;
            }
            na_target_server_tcpsock_setup(tsfd, true);

            if (env->is_use_backup) {
                pthread_rwlock_rdlock(&env->lock_refused);
                server = env->is_refused_active ? &env->backup_server : &env->target_server;
                pthread_rwlock_unlock(&env->lock_refused);
            } else {
                server = &env->target_server;
            }

            if (!na_server_connect(tsfd, &server->addr)) {
                if (errno != EINPROGRESS && errno != EALREADY) {
                    close(tsfd);
                    na_error_count_up(env);
                    NA_STDERR_MESSAGE(NA_ERROR_CONNECTION_FAILED);
                    return;
                }
            }
        }
    }

    if ((cfd = na_server_accept(fsfd)) < 0) {
        if (cur_pool == -1) {
            close(tsfd);
        } else {
            pthread_mutex_lock(&env->lock_connpool);
            connpool->mark[cur_pool] = 0;
            pthread_mutex_unlock(&env->lock_connpool);
        }
        NA_STDERR_MESSAGE(NA_ERROR_INVALID_FD);
        return;
    }

    na_set_nonblock(cfd);

    cur_cli = na_client_assign(env);

    if (cur_cli >= 0) {
        client = &ClientPool[cur_cli];
        if (client->tsfd > 0) {
            close(client->tsfd);
        }
    } else {
        client = (na_client_t *)malloc(sizeof(na_client_t));
        if (client == NULL) {
            close(cfd);
            if (cur_pool == -1) {
                close(tsfd);
            } else {
                pthread_mutex_lock(&env->lock_connpool);
                connpool->mark[cur_pool] = 0;
                pthread_mutex_unlock(&env->lock_connpool);
            }
            na_error_count_up(env);
            NA_STDERR_MESSAGE(NA_ERROR_OUTOF_MEMORY);
            return;
        }
        memset(client, 0, sizeof(*client));
        client->crbuf = (char *)malloc(env->request_bufsize + 1);
        client->srbuf = (char *)malloc(env->response_bufsize + 1);
        if (client->crbuf == NULL ||
            client->srbuf == NULL) {
            NA_FREE(client->crbuf);
            NA_FREE(client->srbuf);
            NA_FREE(client);
            close(cfd);
            if (cur_pool == -1) {
                close(tsfd);
            } else {
                pthread_mutex_lock(&env->lock_connpool);
                connpool->mark[cur_pool] = 0;
                pthread_mutex_unlock(&env->lock_connpool);
            }
            na_error_count_up(env);
            NA_STDERR_MESSAGE(NA_ERROR_OUTOF_MEMORY);
            return;
        }
    }

    client->cfd               = cfd;
    client->tsfd              = tsfd;
    client->env               = env;
    client->c_watcher.data    = client;
    client->ts_watcher.data   = client;
    pthread_rwlock_rdlock(&env->lock_refused);
    client->is_refused_active = env->is_refused_active;
    pthread_rwlock_unlock(&env->lock_refused);
    client->is_use_connpool   = cur_pool != -1 ? true : false;
    client->is_use_client_pool = cur_cli != -1 ? true : false;
    client->cur_pool          = cur_pool;
    client->crbufsize         = 0;
    client->cwbufsize         = 0;
    client->srbufsize         = 0;
    client->swbufsize         = 0;
    client->request_bufsize   = env->request_bufsize;
    client->response_bufsize  = env->response_bufsize;
    client->event_state       = NA_EVENT_STATE_CLIENT_READ;
    client->req_cnt           = 0;
    client->res_cnt           = 0;
    client->loop_cnt          = 0;
    client->cmd               = NA_MEMPROTO_CMD_NOT_DETECTED;
    client->connpool          = na_connpool_select(env);

    pthread_mutex_lock(&env->lock_current_conn);
    ++env->current_conn;

    if (env->current_conn > env->current_conn_max) {
        env->current_conn_max = env->current_conn;
    }
    pthread_mutex_unlock(&env->lock_current_conn);

    if (!na_is_worker_busy(env)) {
        if (!na_event_queue_push(EventQueue, client)) {
            na_error_count_up(env);
            NA_STDERR("Too Many Connections!");
            ev_io_init(&client->c_watcher,  na_client_callback,        client->cfd,  EV_READ);
            ev_io_init(&client->ts_watcher, na_target_server_callback, client->tsfd, EV_NONE);
            ev_io_start(EV_A_ &client->c_watcher);
        }
    } else {
        ev_io_init(&client->c_watcher,  na_client_callback,        client->cfd,  EV_READ);
        ev_io_init(&client->ts_watcher, na_target_server_callback, client->tsfd, EV_NONE);
        ev_io_start(EV_A_ &client->c_watcher);
    }
}

static bool na_is_worker_busy(na_env_t *env)
{
    int count_is_woker_busy = 0;
    for (int i=0;i<env->worker_max;++i) {
        pthread_rwlock_rdlock(&env->lock_worker_busy[i]);
        if (env->is_worker_busy[i]) {
            ++count_is_woker_busy;
        }
        pthread_rwlock_unlock(&env->lock_worker_busy[i]);
    }

    if (count_is_woker_busy == env->worker_max) {
        return true;
    }

    return false;
}

static void *na_event_observer(void *args)
{
    struct ev_loop *loop;
    na_env_t *env;
    na_client_t *client;
    static int tid_s = 0;
    int tid;

    env  = (na_env_t *)args;
    pthread_mutex_lock(&env->lock_loop);
    loop = na_event_loop_create(env->event_model);
    pthread_mutex_unlock(&env->lock_loop);

    pthread_mutex_lock(&env->lock_tid);
    tid = tid_s++;
    pthread_mutex_unlock(&env->lock_tid);

    while (true) {
        client = na_event_queue_pop(EventQueue);

        if (client == NULL) {
            pthread_mutex_lock(&EventQueue->lock);
            if (EventQueue->cnt == 0) {
                pthread_cond_wait(&EventQueue->cond, &EventQueue->lock);
            }
            pthread_mutex_unlock(&EventQueue->lock);
            continue;
        }

        ev_io_init(&client->c_watcher,  na_client_callback,        client->cfd,  EV_READ);
        ev_io_init(&client->ts_watcher, na_target_server_callback, client->tsfd, EV_NONE);
        ev_io_start(EV_A_ &client->c_watcher);
        pthread_rwlock_wrlock(&env->lock_worker_busy[tid]);
        env->is_worker_busy[tid] = true;
        pthread_rwlock_unlock(&env->lock_worker_busy[tid]);
        ev_loop(EV_A_ 0);
        pthread_rwlock_wrlock(&env->lock_worker_busy[tid]);
        env->is_worker_busy[tid] = false;
        pthread_rwlock_unlock(&env->lock_worker_busy[tid]);
    }

    return NULL;
}

static void *na_support_loop (void *args)
{
    struct ev_loop *loop;
    na_env_t *env;
    ev_timer hc_watcher;
    ev_io    st_watcher;

    env  = (na_env_t *)args;
    pthread_mutex_lock(&env->lock_loop);
    loop = ev_loop_new(EVFLAG_AUTO);
    pthread_mutex_unlock(&env->lock_loop);

    // health check event
    if (env->is_use_backup) {
        hc_watcher.data = env;
        ev_timer_init(&hc_watcher, na_hc_callback, 3., 0.);
        ev_timer_start(EV_A_ &hc_watcher);
    }

    // stat event
    st_watcher.data = env;
    ev_io_init(&st_watcher, na_stat_callback, env->stfd, EV_READ);
    ev_io_start(EV_A_ &st_watcher);
    ev_loop(EV_A_ 0);

    return NULL;
}

void *na_event_loop (void *args)
{
    struct ev_loop *loop;
    na_env_t  *env;
    pthread_t  th_support;
    pthread_t *th_workers;

    env = (na_env_t *)args;

    if (strlen(env->fssockpath) > 0) {
        env->fsfd = na_front_server_unixsock_init(env->fssockpath, env->access_mask, env->conn_max);
    } else {
        env->fsfd = na_front_server_tcpsock_init(env->fsport, env->conn_max);
    }

    if (env->fsfd < 0) {
        NA_DIE_WITH_ERROR(NA_ERROR_INVALID_FD);
    }

    na_connpool_init(env);

    ClientPool = calloc(sizeof(na_client_t), env->client_pool_max);
    memset(ClientPool, 0, sizeof(na_client_t) * env->client_pool_max);
    for (int i=0;i<env->client_pool_max;++i) {
        ClientPool[i].crbuf   = (char *)malloc(env->request_bufsize + 1);
        ClientPool[i].srbuf   = (char *)malloc(env->response_bufsize + 1);
        ClientPool[i].is_used = false;
    }

    if (EventQueue == NULL) {
        EventQueue = na_event_queue_create(env->conn_max);
    }
    th_workers = calloc(sizeof(pthread_t), env->worker_max);
    for (int i=0;i<env->worker_max;++i) {
        pthread_create(&th_workers[i], NULL, na_event_observer, env);
    }

    if (strlen(env->stsockpath) > 0) {
        env->stfd = na_stat_server_unixsock_init(env->stsockpath, env->access_mask);
    } else {
        env->stfd = na_stat_server_tcpsock_init(env->stport);
    }
    pthread_create(&th_support, NULL, na_support_loop, env);

    // for assign connection from connpool directional-ramdomly
    srand(time(NULL));

    pthread_mutex_lock(&env->lock_loop);
    loop = na_event_loop_create(env->event_model);
    pthread_mutex_unlock(&env->lock_loop);
    env->fs_watcher.data = env;
    ev_io_init(&env->fs_watcher, na_front_server_callback, env->fsfd, EV_READ);
    ev_io_start(EV_A_ &env->fs_watcher);
    ev_loop(EV_A_ 0);

    for (int i=0;i<env->client_pool_max;++i) {
        NA_FREE(ClientPool[i].crbuf);
        NA_FREE(ClientPool[i].srbuf);
    }
    NA_FREE(ClientPool);
    na_event_queue_destroy(EventQueue);

    return NULL;
}
