/**
   In short, neoagent is distributed under so called "BSD license",
   
   Copyright (c) 2011-2012 Tatsuhiko Kubo <cubicdaiya@gmail.com>
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
#include <time.h>
#include <pthread.h>

#include <ev.h>

#include "error.h"
#include "time.h"
#include "conf.h"
#include "stat.h"
#include "version.h"

// constants
static const int NA_STAT_BUF_MAX  = 8192;

// external globals
time_t StartTimestamp;
volatile sig_atomic_t SigExit;

void na_env_set_jbuf(char *buf, int bufsize, na_env_t *env)
{
    na_connpool_t *connpool;
    struct json_object *stat_obj;
    struct json_object *connpoolmap_obj;
    time_t up_diff;
    char start_dt[NA_DATETIME_BUF_MAX];
    char up_time[NA_DATETIME_BUF_MAX];

    connpool        = env->is_refused_active ? &env->connpool_backup : &env->connpool_active;
    stat_obj        = json_object_new_object();
    connpoolmap_obj = na_connpoolmap_array_json(connpool);
    up_diff         = time(NULL) - StartTimestamp;

    na_ts2dt(&StartTimestamp, "%Y-%m-%d %H:%M:%S", start_dt, NA_DATETIME_BUF_MAX);
    na_elapsed_time(up_diff, up_time, NA_DATETIME_BUF_MAX);

    json_object_object_add(stat_obj, "name",                 json_object_new_string(NA_NAME));
    json_object_object_add(stat_obj, "version",              json_object_new_string(NA_VERSION));
    json_object_object_add(stat_obj, "environment_name",     json_object_new_string(env->name));
    json_object_object_add(stat_obj, "event_model",          json_object_new_string(na_event_model_name(env->event_model)));
    json_object_object_add(stat_obj, "start_time",           json_object_new_string(start_dt));
    json_object_object_add(stat_obj, "up_time",              json_object_new_string(up_time));
    json_object_object_add(stat_obj, "fsport",               json_object_new_int(env->fsport));
    json_object_object_add(stat_obj, "fssockpath",           json_object_new_string(env->fssockpath));
    json_object_object_add(stat_obj, "target_host",          json_object_new_string(env->target_server.host.ipaddr));
    json_object_object_add(stat_obj, "target_port",          json_object_new_int(env->target_server.host.port));
    json_object_object_add(stat_obj, "backup_host",          json_object_new_string(env->backup_server.host.ipaddr));
    json_object_object_add(stat_obj, "backup_port",          json_object_new_int(env->backup_server.host.port));
    json_object_object_add(stat_obj, "current_target_host",  json_object_new_string(env->is_refused_active ? 
                                                                                    env->backup_server.host.ipaddr : env->target_server.host.ipaddr));
    json_object_object_add(stat_obj, "current_target_port",  json_object_new_int(env->is_refused_active ?
                                                                                 env->backup_server.host.port : env->target_server.host.port));
    json_object_object_add(stat_obj, "error_count",          json_object_new_int(env->error_count));
    json_object_object_add(stat_obj, "error_count_max",      json_object_new_int(env->error_count_max));
    json_object_object_add(stat_obj, "conn_max",             json_object_new_int(env->conn_max));
    json_object_object_add(stat_obj, "connpool_max",         json_object_new_int(env->connpool_max));
    json_object_object_add(stat_obj, "is_connpool_only",     json_object_new_string(env->is_connpool_only ?  "true" : "false"));
    json_object_object_add(stat_obj, "is_refused_active",    json_object_new_string(env->is_refused_active ? "true" : "false"));
    json_object_object_add(stat_obj, "request_bufsize",      json_object_new_int(env->request_bufsize));
    json_object_object_add(stat_obj, "request_bufsize_max",  json_object_new_int(env->request_bufsize_max));
    json_object_object_add(stat_obj, "response_bufsize",     json_object_new_int(env->response_bufsize));
    json_object_object_add(stat_obj, "response_bufsize_max", json_object_new_int(env->response_bufsize_max));
    json_object_object_add(stat_obj, "current_conn",         json_object_new_int(env->current_conn));
    json_object_object_add(stat_obj, "available_conn",       json_object_new_int(na_available_conn(connpool)));
    json_object_object_add(stat_obj, "current_conn_max",     json_object_new_int(env->current_conn_max));
    json_object_object_add(stat_obj, "connpool_map",         connpoolmap_obj);

    snprintf(buf, bufsize, "%s", json_object_to_json_string(stat_obj));

    json_object_put(stat_obj);
}    

int na_available_conn (na_connpool_t *connpool)
{
    int available_conn;

    available_conn = 0;

    for (int i=0;i<connpool->max;++i) {
        if (connpool->mark[i] == 0) {
            ++available_conn;
        }
    }

    return available_conn;
}

struct json_object *na_connpoolmap_array_json(na_connpool_t *connpool)
{
    struct json_object *connpoolmap_obj;
    connpoolmap_obj = json_object_new_array();
    for (int i=0;i<connpool->max;++i) {
        json_object_array_add(connpoolmap_obj, json_object_new_int(connpool->mark[i]));
    }
    return connpoolmap_obj;
}

void na_stat_callback (EV_P_ struct ev_io *w, int revents)
{
    int cfd, stfd, th_ret;
    int size;
    na_env_t *env;
    char buf[NA_STAT_BUF_MAX + 1];
    
    th_ret = 0;
    stfd   = w->fd;
    env    = (na_env_t *)w->data;

    if (SigExit == 1) {
        pthread_exit(&th_ret);
        return;
    }
    
    if (env->error_count_max > 0 && (env->error_count > env->error_count_max)) {
        env->error_count = 0;
        return;
    }

    if ((cfd = na_server_accept(stfd)) < 0) {
        NA_STDERR("accept()");
        return;
    }

    na_env_set_jbuf(buf, NA_STAT_BUF_MAX, env);

    // send statictics of environment to client
    if ((size = write(cfd, buf, strlen(buf))) < 0) {
        NA_STDERR("failed to return stat response");
        close(cfd);
        return;
    }

    close(cfd);
}
