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

#include <ev.h>

#include "event.h"
#include "env.h"
#include "socket.h"
#include "error.h"
#include "memproto.h"
#include "util.h"

// external globals
volatile sig_atomic_t SigExit;

// external functions
extern inline bool na_memproto_is_request_divided (int req_cnt);

// private functions
inline static void na_event_switch (EV_P_ struct ev_io *old, ev_io *new, int fd, int revent);
inline static void na_error_count_up (na_env_t *env);

static void na_client_close (na_client_t *client, na_env_t *env);
static void na_health_check_callback (EV_P_ ev_timer *w, int revents);
static void na_stat_callback (EV_P_ struct ev_io *w, int revents);
static void na_target_server_callback (EV_P_ struct ev_io *w, int revents);
static void na_client_callback (EV_P_ struct ev_io *w, int revents);
static void na_front_server_callback (EV_P_ struct ev_io *w, int revents);
static void *na_support_loop (void *args);

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

static void na_client_close (na_client_t *client, na_env_t *env)
{
    close(client->cfd);
    client->cfd = -1;

    NA_FREE(client->client_rbuf);
    NA_FREE(client->client_wbuf);
    NA_FREE(client->server_rbuf);
    NA_FREE(client->server_wbuf);
    NA_FREE(client);

    --env->current_conn;
}

void na_target_server_callback (EV_P_ struct ev_io *w, int revents)
{
    int cfd, tsfd, size;
    na_client_t *client;
    na_env_t *env;

    tsfd   = w->fd;
    client = (na_client_t *)w->data;
    env    = client->env;
    cfd    = client->cfd;

    if (client->loop_cnt++ > env->loop_max) {
        ev_io_stop(EV_A_ w);
        na_client_close(client, env);
        return; // request fail
    }

    if (revents & EV_READ) {

        if (client->server_rbufsize >= env->bufsize) {
            ev_io_stop(EV_A_ w);
            na_client_close(client, env);
            NA_STDERR_MESSAGE(NA_ERROR_OUTOF_BUFFER);
            return; // request fail
        }

        size = read(tsfd, 
                    client->server_rbuf + client->server_rbufsize,
                    env->bufsize - client->server_rbufsize);

        if (size == 0) {
            ev_io_stop(EV_A_ w);
            na_client_close(client, env);
            return; // request success
        } else if (size < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return; // not ready yet
            }
            ev_io_stop(EV_A_ w);
            na_client_close(client, env); // request fail
            return;
        }

        client->server_rbufsize                      += size;
        client->server_rbuf[client->server_rbufsize]  = '\0';

        client->res_cnt = na_memproto_count_response(client->server_rbuf, client->server_rbufsize);
        if (client->res_cnt >= client->req_cnt) {
            na_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_WRITE);
            return;
        }

    } else if (revents & EV_WRITE) {

        size = write(tsfd, 
                     client->client_rbuf + client->server_wbufsize, 
                     client->client_rbufsize - client->server_wbufsize);

        if (size < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return; // not ready yet
            }
            ev_io_stop(EV_A_ w);
            na_client_close(client, env);
            return; // request fail
        }

        client->server_wbufsize                      += size;
        client->server_wbuf[client->server_wbufsize]  = '\0';

        if (client->server_wbufsize < client->client_rbufsize) {
            na_event_switch(EV_A_ w, &client->ts_watcher, tsfd, EV_WRITE);
        } else {
            na_event_switch(EV_A_ w, &client->ts_watcher, tsfd, EV_READ);
        }
        return;
    }

}

void na_client_callback(EV_P_ struct ev_io *w, int revents)
{
    int cfd, tsfd, size;
    na_client_t *client;
    na_env_t *env;

    cfd    = w->fd;
    client = (na_client_t *)w->data;
    env    = client->env;
    tsfd   = client->tsfd;

    if (client->loop_cnt++ > env->loop_max) {
        ev_io_stop(EV_A_ w);
        na_client_close(client, env);
        return; // request fail
    }

    if (revents & EV_READ) {

        size = read(cfd, client->client_rbuf, env->bufsize);

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

        client->client_rbufsize                      += size;
        client->client_rbuf[client->client_rbufsize]  = '\0';

        client->cmd = na_memproto_detect_command(client->client_rbuf);

        if (client->cmd == NA_MEMPROTO_CMD_QUIT    ||
            client->cmd == NA_MEMPROTO_CMD_UNKNOWN)
        {
            ev_io_stop(EV_A_ w);
            na_client_close(client, env);
            return; // request exit
        } else if (client->cmd == NA_MEMPROTO_CMD_GET) {
            client->req_cnt = na_memproto_count_request(client->client_rbuf, client->client_rbufsize);
        }

        switch (client->event_state) {
        case NA_EVENT_STATE_CLIENT_READ:
            // start target server event
            client->event_state = NA_EVENT_STATE_TARGET_WRITE;
            ev_io_stop(EV_A_ w);
            ev_io_init(&client->ts_watcher, na_target_server_callback, tsfd, EV_WRITE);
            ev_io_start(EV_A_ &client->ts_watcher);
            break;
        default:
            // not through
            assert(false);
            break;
        }

    } else if (revents & EV_WRITE) {

        size = write(cfd,
                     client->server_rbuf + client->client_wbufsize,
                     client->server_rbufsize - client->client_wbufsize);

        if (size < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return; // not ready yet
            }
            ev_io_stop(EV_A_ w);
            na_client_close(client, env);
            return; // request fail
        }

        client->client_wbufsize += size;
        if (client->client_wbufsize < client->server_rbufsize) {
            na_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_WRITE);
        } else {
            client->client_rbufsize = 0;
            client->client_wbufsize = 0;
            client->server_rbufsize = 0;
            client->server_wbufsize = 0;
            client->event_state     = NA_EVENT_STATE_CLIENT_READ;
            client->req_cnt         = 0;
            client->res_cnt         = 0;
            na_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_READ);
        }
        
    }
}

void na_front_server_callback (EV_P_ struct ev_io *w, int revents)
{
    int fsfd, cfd;
    int th_ret;
    na_env_t *env;
    na_client_t *client;
    na_connpool_t *connpool;

    th_ret = 0;
    fsfd   = w->fd;
    env    = (na_env_t *)w->data;

    // wait until connections are empty, after signal is catched
    if (SigExit == 1 && env->current_conn == 0) {
        pthread_exit(&th_ret);
        return;
    }
    
    if (env->error_count_max > 0 && (env->error_count > env->error_count_max)) {
        pthread_exit(&th_ret);
        return;
    }
    
    connpool = &env->connpool_active;

    if (env->current_conn >= connpool->max && env->is_connpool_only) {
        return;
    }

    if (env->current_conn >= env->conn_max) {
        return;
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
    client->client_rbuf = (char *)malloc(env->bufsize + 1);
    client->client_wbuf = (char *)malloc(env->bufsize + 1);
    client->server_rbuf = (char *)malloc(env->bufsize + 1);
    client->server_wbuf = (char *)malloc(env->bufsize + 1);
    if (client->client_rbuf == NULL ||
        client->client_wbuf == NULL ||
        client->server_rbuf == NULL ||
        client->server_wbuf == NULL) {
        NA_FREE(client->client_rbuf);
        NA_FREE(client->client_wbuf);
        NA_FREE(client->server_rbuf);
        NA_FREE(client->server_wbuf);
        NA_FREE(client);
        close(cfd);
        NA_STDERR_MESSAGE(NA_ERROR_OUTOF_MEMORY);
        return;
    }
    client->cfd               = cfd;
    client->env               = env;
    client->c_watcher.data    = client;
    client->ts_watcher.data   = client;
    client->is_refused_active = env->is_refused_active;
    client->client_rbufsize   = 0;
    client->client_wbufsize   = 0;
    client->server_rbufsize   = 0;
    client->server_wbufsize   = 0;
    client->event_state       = NA_EVENT_STATE_CLIENT_READ;
    client->req_cnt           = 0;
    client->res_cnt           = 0;
    client->loop_cnt          = 0;

    env->current_conn++;

    client->tsfd = na_target_server_tcpsock_init();
    if (client->tsfd <= 0) {
        close(cfd);
        na_error_count_up(env);
        NA_STDERR_MESSAGE(NA_ERROR_INVALID_FD);
        return;
    }

    na_target_server_tcpsock_setup(client->tsfd, true);
    if (!na_server_connect(client->tsfd, &env->target_server.addr)) {
        if (errno != EINPROGRESS && errno != EALREADY) {
            na_error_count_up(env);
            NA_STDERR_MESSAGE(NA_ERROR_CONNECTION_FAILED);
            return;
        }
    }

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

    hc_watcher.data = env;
    ev_timer_init(&hc_watcher, na_health_check_callback, 5., 0.);
    ev_timer_start(loop, &hc_watcher);

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

    env->stfd = na_stat_server_tcpsock_init(env->stport);

    loop = ev_loop_new(0);

    if (env->fsfd < 0) {
        NA_DIE_WITH_ERROR(NA_ERROR_INVALID_FD);
    }

    pthread_create(&th_support, NULL, na_support_loop, env);

    env->fs_watcher.data = env;
    ev_io_init(&env->fs_watcher, na_front_server_callback, env->fsfd, EV_READ);
    ev_io_start(loop, &env->fs_watcher);
    ev_loop(loop, 0);

    return NULL;
}

static void na_health_check_callback (EV_P_ ev_timer *w, int revents)
{
    int tsfd;
    na_env_t *env;

    env = (na_env_t *)w->data;

    tsfd = na_target_server_tcpsock_init();

    if (tsfd <= 0) {
        na_error_count_up(env);
        NA_STDERR_MESSAGE(NA_ERROR_INVALID_FD);
        return;
    }

    na_target_server_healthchecksock_setup(tsfd);

    // health check

    close(tsfd);

    ev_timer_stop(loop, w);
    ev_timer_set(w, 5., 0.);
    ev_timer_start(loop, w);
}

static void na_stat_callback (EV_P_ struct ev_io *w, int revents)
{
    int cfd, stfd;
    int size, available_conn;
    na_env_t *env;
    char buf[BUFSIZ + 1];

    stfd           = w->fd;
    env            = (na_env_t *)w->data;
    available_conn = 0;

    if ((cfd = na_server_accept(stfd)) < 0) {
        NA_STDERR("accept()");
        return;
    }

    for (int i=0;i<env->connpool_max;++i) {
        if (env->connpool_active.mark[i] == 0) {
            ++available_conn;
        }
    }

    snprintf(buf, 
             BUFSIZ, 
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
             "bufsize            :%d\n"
             "current conn       :%d\n"
             "available conn     :%d\n"
             ,
             env->name, env->fsfd, env->fsport, env->fssockpath, 
             env->target_server.host.ipaddr, env->target_server.host.port,
             env->backup_server.host.ipaddr, env->backup_server.host.port,
             env->is_refused_active ? env->backup_server.host.ipaddr : env->target_server.host.ipaddr,
             env->is_refused_active ? env->backup_server.host.port   : env->target_server.host.port,
             env->error_count, env->error_count_max, env->conn_max, env->connpool_max, 
             env->is_connpool_only ? "true" : "false",
             env->bufsize, env->current_conn, available_conn
             );

    // send stat to client
    if ((size = write(cfd, buf, strlen(buf))) < 0) {
        close(cfd);
        return;
    }

    close(cfd);
}
