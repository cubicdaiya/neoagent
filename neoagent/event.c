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
#include "env.h"
#include "socket.h"
#include "error.h"
#include "memproto.h"
#include "util.h"

// constants
static const int NA_STAT_BUF_MAX  = 8192;
static const int NA_STAT_MBUF_MAX = 4096;

// external globals
volatile sig_atomic_t SigExit;

// external functions
extern inline bool na_memproto_is_request_divided (int req_cnt);

// private functions
inline static void na_event_switch (EV_P_ struct ev_io *old, ev_io *new, int fd, int revent);
inline static void na_error_count_up (na_env_t *env);

static void na_connpool_deactivate (na_connpool_t *connpool);
static bool na_connpool_assign (na_env_t *env, int *cur, int *fd);
static void na_connpool_init (na_env_t *env);
static void na_connpool_switch (na_env_t *env);
static void na_client_close (na_client_t *client, na_env_t *env);
static void na_health_check_callback (EV_P_ ev_timer *w, int revents);
static void na_stat_callback (EV_P_ struct ev_io *w, int revents);
static void na_target_server_callback (EV_P_ struct ev_io *w, int revents);
static void na_client_callback (EV_P_ struct ev_io *w, int revents);
static void na_front_server_callback (EV_P_ struct ev_io *w, int revents);
static void *na_support_loop (void *args);
static void na_connpoolmap_get(char *buf, int bufsize, na_connpool_t *connpool);

inline static void na_event_switch (EV_P_ struct ev_io *old, struct ev_io *new, int fd, int revent)
{
    ev_io_stop(EV_A_ old);
    ev_io_set(new, fd, revent);
    ev_io_start(EV_A_ new);
}

inline static void na_error_count_up (na_env_t *env)
{
    if (env->error_count_max > 0) {
        env->error_count++;
    }
}

static void na_connpool_deactivate (na_connpool_t *connpool)
{
    for (int i=0;i<connpool->max;++i) {
        if (connpool->fd_pool[i] > 0) {
            close(connpool->fd_pool[i]);
            connpool->fd_pool[i] = 0;
            connpool->mark[i]    = 0;
        }
    }
}

static bool na_connpool_assign (na_env_t *env, int *cur, int *fd)
{
    na_connpool_t *connpool;

    connpool = env->is_refused_active ? &env->connpool_backup : &env->connpool_active;

    switch (rand() % 2) {
    case 0:
        for (int i=env->connpool_max-1;i>=0;--i) {
            if (connpool->mark[i] == 0) {
                connpool->mark[i] = 1;
                *fd  = connpool->fd_pool[i];
                *cur = i;
                return true;
            }
        }
        break;
    default:
        for (int i=0;i<env->connpool_max;++i) {
            if (connpool->mark[i] == 0) {
                connpool->mark[i] = 1;
                *fd  = connpool->fd_pool[i];
                *cur = i;
                return true;
            }
        }
        break;
    }
    return false;
}

static void na_connpool_init (na_env_t *env)
{
    for (int i=0;i<env->connpool_max;++i) {
        env->connpool_active.fd_pool[i] = na_target_server_tcpsock_init();
        na_target_server_tcpsock_setup(env->connpool_active.fd_pool[i], true);
        if (env->connpool_active.fd_pool[i] <= 0) {
            NA_DIE_WITH_ERROR(NA_ERROR_INVALID_FD);
        }
        
        if (!na_server_connect(env->connpool_active.fd_pool[i], &env->target_server.addr)) {
            if (errno != EINPROGRESS && errno != EALREADY) {
                na_error_count_up(env);
                NA_DIE_WITH_ERROR(NA_ERROR_CONNECTION_FAILED);
            }
        }
    }
}

static void na_connpool_switch (na_env_t *env)
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
                na_error_count_up(env);
                NA_DIE_WITH_ERROR(NA_ERROR_CONNECTION_FAILED);
            }
        }
    }
}

static void na_client_close (na_client_t *client, na_env_t *env)
{
    close(client->cfd);
    client->cfd = -1;
    if (client->is_use_connpool) {
        client->connpool->mark[client->cur_pool] = 0;
    } else {
        close(client->tsfd);
    }

    NA_FREE(client->crbuf);
    NA_FREE(client->cwbuf);
    NA_FREE(client->srbuf);
    NA_FREE(client->swbuf);
    NA_FREE(client);

    --env->current_conn;
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

    if (client->is_refused_active != env->is_refused_active) {
        ev_io_stop(EV_A_ w);
        na_client_close(client, env);
        return; // request fail
    }

    if (env->loop_max > 0 && client->loop_cnt++ > env->loop_max) {
        ev_io_stop(EV_A_ w);
        na_client_close(client, env);
        return; // request fail
    }

    if (revents & EV_READ) {

        if (client->srbufsize >= env->bufsize) {
            ev_io_stop(EV_A_ w);
            na_client_close(client, env);
            NA_STDERR_MESSAGE(NA_ERROR_OUTOF_BUFFER);
            return; // request fail
        }

        size = read(tsfd, 
                    client->srbuf + client->srbufsize,
                    env->bufsize - client->srbufsize);

        if (size == 0) {
            ev_io_stop(EV_A_ w);
            na_client_close(client, env);
            return; // request fail
        } else if (size < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return; // not ready yet
            }
            ev_io_stop(EV_A_ w);
            na_client_close(client, env); // request fail
            return;
        }

        client->srbufsize                += size;
        client->srbuf[client->srbufsize]  = '\0';

        if (client->cmd == NA_MEMPROTO_CMD_GET) {
            client->res_cnt = na_memproto_count_response_get(client->srbuf, client->srbufsize);
            if (client->res_cnt >= client->req_cnt) {
                client->event_state = NA_EVENT_STATE_CLIENT_WRITE;
                na_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_WRITE);
                return;
            }
        } else if (client->cmd == NA_MEMPROTO_CMD_SET) {
            client->res_cnt = na_memproto_count_response_set(client->srbuf, client->srbufsize);
            if (client->res_cnt >= client->req_cnt) {
                client->event_state = NA_EVENT_STATE_CLIENT_WRITE;
                na_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_WRITE);
                return;
            }
        } else {
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
            }
            ev_io_stop(EV_A_ w);
            na_client_close(client, env);
            return; // request fail
        }

        client->swbufsize                += size;
        client->swbuf[client->swbufsize]  = '\0';

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

    if (client->is_refused_active != env->is_refused_active) {
        ev_io_stop(EV_A_ w);
        na_client_close(client, env);
        return; // request fail
    }

    if (env->loop_max > 0 && client->loop_cnt++ > env->loop_max) {
        ev_io_stop(EV_A_ w);
        na_client_close(client, env);
        return; // request fail
    }

    if (revents & EV_READ) {

        size = read(cfd, 
                    client->crbuf + client->crbufsize,
                    env->bufsize - client->crbufsize);

        if (size == 0) {
            ev_io_stop(EV_A_ w);
            na_client_close(client, env);
            return; // request success
        } else if (size < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return; // not ready yet
            }
            ev_io_stop(EV_A_ w);
            na_client_close(client, env);
            return; // request fail
        }

        client->crbufsize                += size;
        client->crbuf[client->crbufsize]  = '\0';

        client->cmd = na_memproto_detect_command(client->crbuf);

        if (client->cmd == NA_MEMPROTO_CMD_QUIT) {
            ev_io_stop(EV_A_ w);
            na_client_close(client, env);
            return; // request success
        } else if (client->cmd == NA_MEMPROTO_CMD_GET) {
            client->req_cnt = na_memproto_count_request_get(client->crbuf, client->crbufsize);
        } else if (client->cmd == NA_MEMPROTO_CMD_SET) {
            client->req_cnt = na_memproto_count_request_set(client->crbuf, client->crbufsize);
        }

        if (client->crbufsize < 2) {
            ev_io_stop(EV_A_ w);
            na_client_close(client, env);
            return; // request fail
        } else if (client->crbuf[client->crbufsize - 2] == '\r' &&
                   client->crbuf[client->crbufsize - 1] == '\n')
        {
            client->event_state = NA_EVENT_STATE_TARGET_WRITE;
            ev_io_stop(EV_A_ w);
            ev_io_init(&client->ts_watcher, na_target_server_callback, tsfd, EV_WRITE);
            ev_io_start(EV_A_ &client->ts_watcher);
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
            ev_io_stop(EV_A_ w);
            na_client_close(client, env);
            return; // request fail
        }

        client->cwbufsize += size;
        if (client->cwbufsize < client->srbufsize) {
            na_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_WRITE);
            return;
        } else {
            client->crbufsize   = 0;
            client->cwbufsize   = 0;
            client->srbufsize   = 0;
            client->swbufsize   = 0;
            client->event_state = NA_EVENT_STATE_CLIENT_READ;
            client->req_cnt     = 0;
            client->res_cnt     = 0;
            na_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_READ);
            return;
        }
        
    }
}

void na_front_server_callback (EV_P_ struct ev_io *w, int revents)
{
    int fsfd, cfd, tsfd, cur_pool;
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

    if (SigExit == 1 && env->current_conn == 0) {
        pthread_exit(&th_ret);
        return;
    }
    
    if (env->error_count_max > 0 && (env->error_count > env->error_count_max)) {
        pthread_exit(&th_ret);
        return;
    }
    
    if (env->current_conn >= env->conn_max) {
        return;
    }

    connpool = env->is_refused_active ? &env->connpool_backup : &env->connpool_active;

    if (env->is_connpool_only) {
        if (env->current_conn >= connpool->max) {
            return;
        }
        if (!na_connpool_assign(env, &cur_pool, &tsfd)) {
            NA_STDERR("failed assign connection from connpool.");
            return;
        }
    } else {
        if (env->current_conn >= connpool->max) {
            na_server_t *server;
            tsfd = na_target_server_tcpsock_init();
            if (tsfd < 0) {
                na_error_count_up(env);
                NA_STDERR_MESSAGE(NA_ERROR_INVALID_FD);
                return;
            }
            na_target_server_tcpsock_setup(tsfd, true);

            server = env->is_refused_active ? &env->backup_server : &env->target_server;

            if (!na_server_connect(tsfd, &server->addr)) {
                if (errno != EINPROGRESS && errno != EALREADY) {
                    close(tsfd);
                    na_error_count_up(env);
                    NA_STDERR_MESSAGE(NA_ERROR_CONNECTION_FAILED);
                    return;
                }
            }
        } else {
            if (!na_connpool_assign(env, &cur_pool, &tsfd)) {
                NA_STDERR("failed assign connection from connpool.");
                return;
            }
        }
    }
        
    if ((cfd = na_server_accept(fsfd)) < 0) {
        NA_STDERR("accept()");
        return;
    }

    client = (na_client_t *)malloc(sizeof(na_client_t));
    if (client == NULL) {
        close(cfd);
        NA_STDERR_MESSAGE(NA_ERROR_OUTOF_MEMORY);
        return;
    }
    memset(client, 0, sizeof(*client));
    client->crbuf = (char *)malloc(env->bufsize + 1);
    client->cwbuf = (char *)malloc(env->bufsize + 1);
    client->srbuf = (char *)malloc(env->bufsize + 1);
    client->swbuf = (char *)malloc(env->bufsize + 1);
    if (client->crbuf == NULL ||
        client->cwbuf == NULL ||
        client->srbuf == NULL ||
        client->swbuf == NULL) {
        NA_FREE(client->crbuf);
        NA_FREE(client->cwbuf);
        NA_FREE(client->srbuf);
        NA_FREE(client->swbuf);
        NA_FREE(client);
        close(cfd);
        NA_STDERR_MESSAGE(NA_ERROR_OUTOF_MEMORY);
        return;
    }
    client->cfd               = cfd;
    client->tsfd              = tsfd;
    client->env               = env;
    client->c_watcher.data    = client;
    client->ts_watcher.data   = client;
    client->is_refused_active = env->is_refused_active;
    client->is_use_connpool   = cur_pool != -1 ? true : false;
    client->cur_pool          = cur_pool;
    client->crbufsize         = 0;
    client->cwbufsize         = 0;
    client->srbufsize         = 0;
    client->swbufsize         = 0;
    client->event_state       = NA_EVENT_STATE_CLIENT_READ;
    client->req_cnt           = 0;
    client->res_cnt           = 0;
    client->loop_cnt          = 0;
    client->cmd               = NA_MEMPROTO_CMD_NOT_DETECTED;
    client->connpool          = env->is_refused_active ? &env->connpool_backup : &env->connpool_active;

    env->current_conn++;

    ev_io_init(&client->c_watcher, na_client_callback, cfd, EV_READ);
    ev_io_start(EV_A_ &client->c_watcher);
}

static void *na_support_loop (void *args)
{
    struct ev_loop *loop;
    na_env_t *env;
    ev_timer hc_watcher;
    ev_io    st_watcher;

    env  = (na_env_t *)args;
    loop = ev_loop_new(0);

    // health check event
    hc_watcher.data = env;
    ev_timer_init(&hc_watcher, na_health_check_callback, 5., 0.);
    ev_timer_start(loop, &hc_watcher);

    // stat event
    st_watcher.data = env;
    ev_io_init(&st_watcher, na_stat_callback, env->stfd, EV_READ);
    ev_io_start(loop, &st_watcher);
    ev_loop(loop, 0);

    return NULL;
}

void *na_event_loop (void *args)
{
    struct ev_loop *loop;
    na_env_t *env;
    pthread_t th_support;

    env = (na_env_t *)args;

    if (strlen(env->fssockpath) > 0) {
        env->fsfd = na_front_server_unixsock_init(env->fssockpath, env->access_mask);
    } else {
        env->fsfd = na_front_server_tcpsock_init(env->fsport);
    }

    if (env->fsfd < 0) {
        NA_DIE_WITH_ERROR(NA_ERROR_INVALID_FD);
    }

    na_connpool_init(env);

    env->stfd = na_stat_server_tcpsock_init(env->stport);
    pthread_create(&th_support, NULL, na_support_loop, env);

    // for assign connection from connpool directional-ramdomly
    srand(time(NULL));

    loop = ev_loop_new(0);
    env->fs_watcher.data = env;
    ev_io_init(&env->fs_watcher, na_front_server_callback, env->fsfd, EV_READ);
    ev_io_start(loop, &env->fs_watcher);
    ev_loop(loop, 0);

    return NULL;
}

static void na_health_check_callback (EV_P_ ev_timer *w, int revents)
{
    int tsfd;
    int th_ret;
    na_env_t *env;

    env = (na_env_t *)w->data;

    if (SigExit == 1 && env->current_conn == 0) {
        pthread_exit(&th_ret);
        return;
    }
    
    if (env->error_count_max > 0 && (env->error_count > env->error_count_max)) {
        pthread_exit(&th_ret);
        return;
    }

    tsfd = na_target_server_tcpsock_init();

    if (tsfd <= 0) {
        na_error_count_up(env);
        NA_STDERR_MESSAGE(NA_ERROR_INVALID_FD);
        return;
    }

    na_target_server_hcsock_setup(tsfd);

    // health check
    if (!na_server_connect(tsfd, &env->target_server.addr)) {
        if (!env->is_refused_active) {
            if (errno != EINPROGRESS && errno != EALREADY) {
                env->is_refused_active = true;
                na_connpool_switch(env);
            }
        }
    } else {
        if (env->is_refused_active) {
            env->is_refused_active = false;
            na_connpool_switch(env);
        }
    }

    close(tsfd);

    ev_timer_stop(loop, w);
    ev_timer_set(w, 5., 0.);
    ev_timer_start(loop, w);
}

static void na_stat_callback (EV_P_ struct ev_io *w, int revents)
{
    int cfd, stfd, th_ret;
    int size, available_conn;
    na_env_t *env;
    char buf[NA_STAT_BUF_MAX + 1];
    char mbuf[NA_STAT_MBUF_MAX + 1];
    na_connpool_t *connpool;

    stfd           = w->fd;
    env            = (na_env_t *)w->data;
    available_conn = 0;

    if (SigExit == 1 && env->current_conn == 0) {
        pthread_exit(&th_ret);
        return;
    }
    
    if (env->error_count_max > 0 && (env->error_count > env->error_count_max)) {
        pthread_exit(&th_ret);
        return;
    }

    if ((cfd = na_server_accept(stfd)) < 0) {
        NA_STDERR("accept()");
        return;
    }

    connpool = env->is_refused_active ? &env->connpool_backup : &env->connpool_active;

    for (int i=0;i<env->connpool_max;++i) {
        if (connpool->mark[i] == 0) {
            ++available_conn;
        }
    }

    na_connpoolmap_get(mbuf, NA_STAT_MBUF_MAX, connpool);

    snprintf(buf, 
             NA_STAT_BUF_MAX,
             "environment stats\n\n"
             "name               :%s\n"
             "fsfd               :%d\n"
             "fsport             :%d\n"
             "fssockpath         :%s\n"
             "target host        :%s\n"
             "target port        :%d\n"
             "backup host        :%s\n"
             "backup port        :%d\n"
             "current target host:%s\n"
             "current target port:%d\n"
             "error count        :%d\n"
             "error count max    :%d\n"
             "conn max           :%d\n"
             "connpool max       :%d\n"
             "is_connpool_only   :%s\n"
             "is_refused_active  :%s\n"
             "bufsize            :%d\n"
             "current conn       :%d\n"
             "available conn     :%d\n"
             "connpool_map       :%s\n"
             ,
             env->name, env->fsfd, env->fsport, env->fssockpath, 
             env->target_server.host.ipaddr, env->target_server.host.port,
             env->backup_server.host.ipaddr, env->backup_server.host.port,
             env->is_refused_active ? env->backup_server.host.ipaddr : env->target_server.host.ipaddr,
             env->is_refused_active ? env->backup_server.host.port   : env->target_server.host.port,
             env->error_count, env->error_count_max, env->conn_max, env->connpool_max, 
             env->is_connpool_only  ? "true" : "false",
             env->is_refused_active ? "true" : "false",
             env->bufsize, env->current_conn, available_conn, mbuf
             );

    // send stat to client
    if ((size = write(cfd, buf, strlen(buf))) < 0) {
        close(cfd);
        return;
    }

    close(cfd);
}

static void na_connpoolmap_get(char *buf, int bufsize, na_connpool_t *connpool)
{
    for (int i=0;i<connpool->max;++i) {
        snprintf(buf + i * 2, bufsize - i * 2, "%d ", connpool->mark[i]);
    }
    buf[connpool->max * 2] = '\0';
}


