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

#include <ev.h>

#include "event.h"
#include "env.h"
#include "socket.h"
#include "error.h"
#include "memproto.h"

// external globals
volatile sig_atomic_t SigCatched;

// external functions
extern inline bool neoagent_memproto_is_request_divided (int req_cnt);

// private functions
inline static void neoagent_spare_free (neoagent_client_t *client);
inline static void neoagent_event_switch (EV_P_ struct ev_io *old, ev_io *new, int fd, int revent);

static void neoagent_client_close (neoagent_client_t *client, neoagent_env_t *env);
static int  neoagent_remaining_size (int fd);
static void neoagent_make_spare(neoagent_client_t *client, neoagent_env_t *env);
static void neoagent_health_check_callback (EV_P_ ev_timer *w, int revents);
static void neoagent_stat_callback (EV_P_ struct ev_io *w, int revents);
static void neoagent_target_server_callback (EV_P_ struct ev_io *w, int revents);
static void neoagent_client_callback (EV_P_ struct ev_io *w, int revents);
static void neoagent_front_server_callback (EV_P_ struct ev_io *w, int revents);

inline static void neoagent_spare_free (neoagent_client_t *client)
{
    neoagent_spare_buf_t *p, *pp;
    p = client->head_spare;
    while (p != NULL) {
        pp = p->next;
        free(p->buf);
        free(p);
        p = NULL;
        p = pp;
    }
    client->head_spare    = NULL;
    client->current_spare = NULL;
}

inline static void neoagent_event_switch (EV_P_ struct ev_io *old, struct ev_io *new, int fd, int revent)
{
    ev_io_stop(EV_A_ old);
    ev_io_set(new, fd, revent);
    ev_io_start(EV_A_ new);
}

static void neoagent_client_close (neoagent_client_t *client, neoagent_env_t *env)
{
    int tsfd, size, cur_pool;
    neoagent_connpool_t *connpool;
    bool is_use_connpool;

    tsfd            = client->tsfd;
    cur_pool        = client->cur_pool;
    is_use_connpool = client->is_use_connpool;

    // release client resources
    close(client->cfd);
    client->cfd = -1;
    neoagent_spare_free(client);
    free(client->buf);
    client->buf = NULL;
    free(client);
    client = NULL;

    if ((size = neoagent_remaining_size(tsfd)) > 0) {
        NEOAGENT_STDERR_MESSAGE(NEOAGENT_ERROR_REMAIN_DATA);
        env->error_count++;
        close(tsfd);
        goto finish;
    }
    
    connpool = &env->connpool_active;

    if (is_use_connpool) {
        connpool->fd_pool[cur_pool] = tsfd;
        connpool->mark[cur_pool]    = 0;
    
        if (connpool->cur == connpool->max) {
        
            if (!connpool->is_full) {
                connpool->is_full = true;
            }
        
            connpool->cur = 0;
        }
    } else {
        close(tsfd);
    }
    
 finish:
    // update environment
    env->current_conn--;

}

static int neoagent_remaining_size (int fd)
{
    int size;
    if (ioctl(fd, FIONREAD, &size) != 0) {
        return 0;
    }
    return size;
}

static void neoagent_make_spare(neoagent_client_t *client, neoagent_env_t *env)
{
    int c = client->req_cnt - 1;
    neoagent_spare_buf_t *p = NULL;
    for (int i=0;i<c;++i) {
        if (i == 0) {
            client->current_spare = (neoagent_spare_buf_t *)malloc(sizeof(neoagent_spare_buf_t));
            memset(client->current_spare, 0, sizeof(neoagent_spare_buf_t));
            client->current_spare->buf = (char *)malloc(env->bufsize + 1);
            client->head_spare = client->current_spare;
            p                  = client->current_spare;
        } else {
            p->next = (neoagent_spare_buf_t *)malloc(sizeof(neoagent_spare_buf_t));
            memset(p->next, 0, sizeof(neoagent_spare_buf_t));
            p->next->buf = (char *)malloc(env->bufsize + 1);
            p = p->next;
        }
    }
}

static void neoagent_health_check_callback (EV_P_ ev_timer *w, int revents)
{
    int tsfd;
    neoagent_env_t *env;

    env = (neoagent_env_t *)w->data;

    tsfd = neoagent_target_server_tcpsock_init();

    if (tsfd <= 0) {
        NEOAGENT_STDERR_MESSAGE(NEOAGENT_ERROR_INVALID_FD);
        env->error_count++;
        return;
    }

    neoagent_target_server_healthchecksock_setup(tsfd);

    if (!neoagent_server_connect(tsfd, &env->target_server.addr)) {
        if (errno != EINPROGRESS && errno != EALREADY) {
            if (!env->is_refused_active) {
                env->is_refused_active = true;
                neoagent_connpool_switch(env, env->connpool_max);
            }
        } else {
            if (env->is_refused_active) {
                env->is_refused_active = false;
                neoagent_connpool_switch(env, env->connpool_max);
            }
        }
    } else {
        if (env->is_refused_active) {
            env->is_refused_active = false;
            neoagent_connpool_switch(env, env->connpool_max);
        }
    }

    close(tsfd);

    ev_timer_stop (loop, w);
    ev_timer_set (w, 5., 0.);
    ev_timer_start (loop, w);
}

static void neoagent_stat_callback (EV_P_ struct ev_io *w, int revents)
{
    int cfd, stfd;
    int size, available_conn;
    neoagent_env_t *env;
    char buf[BUFSIZ + 1];

    stfd           = w->fd;
    env            = (neoagent_env_t *)w->data;
    available_conn = 0;

    if ((cfd = neoagent_server_accept(stfd)) < 0) {
        NEOAGENT_STDERR("accept()");
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

    if ((size = write(cfd, buf, strlen(buf))) < 0) {
        close(cfd);
        return;
    }

    close(cfd);
}

void neoagent_target_server_callback (EV_P_ struct ev_io *w, int revents)
{
    int cfd, tsfd, size, req_cnt;
    neoagent_client_t *client;
    neoagent_env_t * env;

    tsfd   = w->fd;
    client = (neoagent_client_t *)w->data;
    cfd    = client->cfd;

    if (client == NULL) {
        return;
    }

    env = client->env;

    if (client->is_refused_active != env->is_refused_active) {
        ev_io_stop(EV_A_ w);
        close(cfd);
        return;
    }

    if (revents & EV_WRITE) {

        size = write(tsfd, client->buf, client->bufsize);
        if (size <= 0) {
            ev_io_stop(EV_A_ w);
            neoagent_client_close(client, env);
            return;
        }

        neoagent_event_switch(EV_A_ w, &client->ts_watcher, tsfd, EV_READ);

    } else if (revents & EV_READ) {

        // use spare buffer
        if (client->cmd == NEOAGENT_MEMPROTO_CMD_GET && neoagent_memproto_is_request_divided(client->req_cnt) && client->current_req_cnt > 0) {
            size = neoagent_remaining_size(tsfd);
            if (size == 0) {
                neoagent_event_switch(EV_A_ w, &client->ts_watcher, tsfd, EV_READ);
                return;
            }

            size = read(tsfd, client->current_spare->buf + client->current_spare->ts_pos, size);
            if (size <= 0) {
                neoagent_event_switch(EV_A_ w, &client->ts_watcher, tsfd, EV_READ);
                return;
            }
            client->current_spare->ts_pos                             += size;
            client->current_spare->buf[client->current_spare->ts_pos]  = '\0';
            client->current_spare->bufsize                             = client->ts_pos;

            req_cnt = neoagent_memproto_count_response(client->current_spare->buf, client->current_spare->bufsize);
            if (req_cnt == 0) {
                return;
            }

            client->current_req_cnt += req_cnt;
            client->current_spare    = client->current_spare->next;

            if (client->current_req_cnt >= client->req_cnt) {
                neoagent_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_WRITE);
            }

            return;
        }

        size = neoagent_remaining_size(tsfd);
        if (size == 0) {
            neoagent_event_switch(EV_A_ w, &client->ts_watcher, tsfd, EV_READ);
            return;            
        }

        size = read(tsfd, client->buf + client->ts_pos, size);
        if (size <= 0) {
            neoagent_event_switch(EV_A_ w, &client->ts_watcher, tsfd, EV_READ);
            return;
        }
        client->ts_pos              += size;
        client->buf[client->ts_pos]  = '\0';
        client->bufsize              = client->ts_pos;

        if (client->cmd == NEOAGENT_MEMPROTO_CMD_GET && neoagent_memproto_is_request_divided(client->req_cnt)) {
            client->current_req_cnt += neoagent_memproto_count_response(client->buf, client->bufsize);
            if (client->current_req_cnt >= client->req_cnt) {
                neoagent_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_WRITE);
            }
            return;
        } else {
            if (client->cmd == NEOAGENT_MEMPROTO_CMD_GET && neoagent_memproto_count_response(client->buf, client->bufsize) > 0) {
                neoagent_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_WRITE);
            } else {
                if (client->cmd == NEOAGENT_MEMPROTO_CMD_GET) {
                    neoagent_event_switch(EV_A_ w, &client->ts_watcher, tsfd, EV_READ);
                } else {
                    neoagent_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_WRITE);
                }
            }
        }
    }

}

void neoagent_client_callback(EV_P_ struct ev_io *w, int revents)
{
    int cfd, tsfd, size;
    bool is_use_pool;
    neoagent_client_t *client;
    neoagent_spare_buf_t *spare;
    neoagent_env_t *env;
    neoagent_connpool_t *connpool;
    neoagent_server_t *server;

    cfd    = w->fd;
    client = (neoagent_client_t *)w->data;
    env    = client->env;
    tsfd   = client->tsfd;
    is_use_pool = false;

    if (client == NULL) {
        return;
    }

    if (client->is_refused_active != env->is_refused_active) {
        ev_io_stop(EV_A_ w);
        close(cfd);
        return;
    }

    if (revents & EV_READ) {

        size = neoagent_remaining_size(cfd);

        if (size == 0)  {
            ev_io_stop(EV_A_ w);
            neoagent_client_close(client, env);
            return;
        }

        size = read(cfd, client->buf, size);
        if (size <= 0) {
            ev_io_stop(EV_A_ w);
            neoagent_client_close(client, env);
            return;
        }
        client->buf[size] = '\0';
        client->bufsize   = size;

        if (client->tsfd > 0) {
            neoagent_spare_free(client);
        }

        // for get command specific(request piplining)
        client->cmd = neoagent_memproto_detect_command(client->buf);
        if (client->cmd == NEOAGENT_MEMPROTO_CMD_GET) {
            client->req_cnt = neoagent_memproto_count_request(client->buf, client->bufsize);
            if (neoagent_memproto_is_request_divided(client->req_cnt)) {
                neoagent_make_spare(client, env);
            }
        } else if (client->cmd == NEOAGENT_MEMPROTO_CMD_SET) {
            client->req_cnt++;
        }

        connpool = &env->connpool_active;

        if (client->is_use_connpool) {

            // connection pool is not full
            if (connpool->is_full) {
                client->tsfd = connpool->fd_pool[client->cur_pool];
                tsfd         = client->tsfd;
            }
            
            if (tsfd <= 0 || !connpool->is_full) {
                if (client->cmd == NEOAGENT_MEMPROTO_CMD_SET) {
                    if (tsfd <= 0) {
                        client->tsfd                        = neoagent_target_server_tcpsock_init();
                        tsfd                                = client->tsfd;
                        connpool->fd_pool[client->cur_pool] = tsfd;
                        connpool->mark[client->cur_pool]    = 1;
                    } else {
                        is_use_pool = true;
                    }
                } else {
                    if (tsfd <= 0) {
                        client->tsfd = neoagent_target_server_tcpsock_init();
                        tsfd         = client->tsfd;
                    } else {
                        is_use_pool = true;
                    }
                }
                
                if (tsfd <= 0) {
                    NEOAGENT_STDERR_MESSAGE(NEOAGENT_ERROR_INVALID_FD);
                    env->error_count++;
                    ev_io_stop(EV_A_ w);
                    neoagent_client_close(client, env);
                    return;
                }
                
                neoagent_target_server_tcpsock_setup(tsfd);
                
                if (client->is_refused_active) {
                    server = &env->backup_server;
                } else {
                    server = &env->target_server;
                }
                
                if (!is_use_pool && !neoagent_server_connect(tsfd, &server->addr)) {
                    if (errno != EINPROGRESS && errno != EALREADY) {
                        NEOAGENT_STDERR_MESSAGE(NEOAGENT_ERROR_CONNECTION_FAILED);
                        env->error_count++;
                        ev_io_stop(EV_A_ w);
                        neoagent_client_close(client, env);
                        return;
                    }
                }
            }
            
        } else {

            bool is_connected = false;

            if (client->tsfd <= 0) {
                client->tsfd = neoagent_target_server_tcpsock_init();
                tsfd         = client->tsfd;
            } else {
                is_connected = true;
            }

            if (tsfd <= 0) {
                NEOAGENT_STDERR_MESSAGE(NEOAGENT_ERROR_INVALID_FD);
                env->error_count++;
                ev_io_stop(EV_A_ w);
                neoagent_client_close(client, env);
                return;
            } 

            neoagent_target_server_tcpsock_setup(tsfd);

            if (client->is_refused_active) {
                server = &env->backup_server;
            } else {
                server = &env->target_server;
            }

            if (!is_connected && !neoagent_server_connect(tsfd, &server->addr)) {
                if (errno != EINPROGRESS && errno != EALREADY) {
                    NEOAGENT_STDERR_MESSAGE(NEOAGENT_ERROR_CONNECTION_FAILED);
                    env->error_count++;
                    ev_io_stop(EV_A_ w);
                    neoagent_client_close(client, env);
                    return;
                }
            }
             
        }
        
        ev_io_stop(EV_A_ w);
        ev_io_init(&client->ts_watcher, neoagent_target_server_callback, tsfd, EV_WRITE);
        ev_io_start(EV_A_ &client->ts_watcher);

    } else if (revents & EV_WRITE) {

        size = write(cfd, client->buf, client->bufsize);
        if (size <= 0) {
            ev_io_stop(EV_A_ w);
            neoagent_client_close(client, env);
            return;
        }

        client->ts_pos = 0;

        if (client->head_spare != NULL) {
            spare = client->head_spare;
            while (spare != NULL) {
                size = write(cfd, spare->buf, spare->bufsize);
                if (size < 0) {
                    ev_io_stop(EV_A_ w);
                    neoagent_client_close(client, env);
                    return;
                }
                spare->ts_pos = 0;
                spare = spare->next;
            }
            client->current_req_cnt = 0;
        }

        tsfd = client->tsfd;
        size = neoagent_remaining_size(tsfd);

        if (size > 0) {
            neoagent_event_switch(EV_A_ w, &client->ts_watcher, tsfd, EV_READ);
            return;
        }

        neoagent_event_switch(EV_A_ w, &client->c_watcher, cfd, EV_READ);

    }

}

void neoagent_front_server_callback (EV_P_ struct ev_io *w, int revents)
{
    int fsfd, cfd;
    int cur;
    int th_ret;
    neoagent_env_t *env;
    neoagent_client_t *client;
    neoagent_connpool_t *connpool;
    bool is_connpool_full;

    th_ret           = 0;
    fsfd             = w->fd;
    env              = (neoagent_env_t *)w->data;
    is_connpool_full = true;
    cur              = -1;

    // wait until connections are empty, after signal is catched
    if (SigCatched == 1 && env->current_conn == 0) {
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

    if (connpool->cur == connpool->max) {
        connpool->cur = 0;
    }

    for (int i=0;i<connpool->max;++i) {
        if (connpool->mark[i] == 0) {
            connpool->mark[i] = 1;
            is_connpool_full  = false;
            cur               = i;
            break;
        }
    }

    if (is_connpool_full && env->is_connpool_only) {
        return;
    }

    if (env->current_conn >= env->conn_max) {
        return;
    }

    if ((cfd = neoagent_server_accept(fsfd)) < 0) {
        NEOAGENT_STDERR("accept()");
        return;
    }

    env->current_conn++;

    client                    = (neoagent_client_t *)malloc(sizeof(neoagent_client_t));
    memset(client, 0, sizeof(*client));
    client->buf               = (char *)malloc(env->bufsize + 1);
    client->cfd               = cfd;
    client->req_cnt           = 0;
    client->ts_pos            = 0;
    client->current_req_cnt   = 0;
    client->env               = env;
    client->c_watcher.data    = client;
    client->ts_watcher.data   = client;
    client->cur_pool          = is_connpool_full ? -1 : cur;
    client->is_refused_active = env->is_refused_active;
    client->is_use_connpool   = is_connpool_full ? false : true;

    connpool->cur++;

    ev_io_init(&client->c_watcher, neoagent_client_callback, cfd, EV_READ);
    ev_io_start(EV_A_ &client->c_watcher);

}

void *neoagent_event_loop (void *args)
{
    struct ev_loop *loop;
    ev_timer hc_watcher;
    ev_io    st_watcher;
    neoagent_env_t *env;

    env = (neoagent_env_t *)args;

    if (strlen(env->fssockpath) > 0) {
        env->fsfd = neoagent_front_server_unixsock_init(env->fssockpath, env->access_mask);
    } else {
        env->fsfd = neoagent_front_server_tcpsock_init(env->fsport);
    }

    env->stfd = neoagent_stat_server_tcpsock_init(env->stport);
 

    loop = ev_loop_new(0);
    env->fs_watcher.data = loop;

    if (env->fsfd < 0) {
        neoagent_die_with_error(NEOAGENT_ERROR_INVALID_FD);
    }

    hc_watcher.data = env;
    ev_timer_init (&hc_watcher, neoagent_health_check_callback, 5., 0.);
    ev_timer_start (loop, &hc_watcher);

    st_watcher.data = env;
    ev_io_init(&st_watcher, neoagent_stat_callback, env->stfd, EV_READ);
    ev_io_start(loop, &st_watcher);
 
    env->fs_watcher.data = env;
    ev_io_init(&env->fs_watcher, neoagent_front_server_callback, env->fsfd, EV_READ);
    ev_io_start(loop, &env->fs_watcher);
    ev_loop(loop, 0);

    return NULL;
}
