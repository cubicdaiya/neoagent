/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <string.h>
#include <unistd.h>

#include "defines.h"
#include "version.h"

// constants
static const int   NA_STAT_BUF_MAX   = 8192;
static const char *NA_BOOL_STR_TRUE  = "true";
static const char *NA_BOOL_STR_FALSE = "false";

// external globals
time_t StartTimestamp;
volatile sig_atomic_t SigExit;

// private functions
static inline const char *na_bool2str(bool b);
static inline char *na_active_host_select(na_env_t *env);
static inline uint16_t na_active_port_select(na_env_t *env);

static void na_env_set_jbuf(char *buf, int bufsize, na_env_t *env);
static int na_available_conn (na_connpool_t *connpool);
static struct json_object *na_connpoolmap_array_json(na_connpool_t *connpool);
static struct json_object *na_workermap_array_json(na_env_t *env);

static inline const char *na_bool2str(bool b)
{
    return b == true ? NA_BOOL_STR_TRUE : NA_BOOL_STR_FALSE;
}

static inline char *na_active_host_select(na_env_t *env)
{
    return env->is_refused_active ? env->backup_server.host.ipaddr : env->target_server.host.ipaddr;
}

static inline uint16_t na_active_port_select(na_env_t *env)
{
    return env->is_refused_active ? env->backup_server.host.port : env->target_server.host.port;
}

static void na_env_set_jbuf(char *buf, int bufsize, na_env_t *env)
{
    na_connpool_t *connpool;
    struct json_object *stat_obj;
    struct json_object *connpoolmap_obj;
    struct json_object *workermap_obj;
    time_t up_diff;
    char start_dt[NA_DATETIME_BUF_MAX];
    char up_time[NA_DATETIME_BUF_MAX];

    connpool        = env->is_refused_active ? &env->connpool_backup : &env->connpool_active;
    stat_obj        = json_object_new_object();
    connpoolmap_obj = na_connpoolmap_array_json(connpool);
    workermap_obj   = na_workermap_array_json(env);
    up_diff         = time(NULL) - StartTimestamp;

    na_ts2dt(StartTimestamp, "%Y-%m-%d %H:%M:%S", start_dt, NA_DATETIME_BUF_MAX);
    na_elapsed_time(up_diff, up_time, NA_DATETIME_BUF_MAX);

    json_object_object_add(stat_obj, "name",                         json_object_new_string(NA_NAME));
    json_object_object_add(stat_obj, "version",                      json_object_new_string(NA_VERSION));
    json_object_object_add(stat_obj, "environment_name",             json_object_new_string(env->name));
    json_object_object_add(stat_obj, "event_model",                  json_object_new_string(na_event_model_name(env->event_model)));
    json_object_object_add(stat_obj, "start_time",                   json_object_new_string(start_dt));
    json_object_object_add(stat_obj, "up_time",                      json_object_new_string(up_time));
    json_object_object_add(stat_obj, "fsport",                       json_object_new_int(env->fsport));
    json_object_object_add(stat_obj, "fssockpath",                   json_object_new_string(env->fssockpath));
    json_object_object_add(stat_obj, "target_host",                  json_object_new_string(env->target_server.host.ipaddr));
    json_object_object_add(stat_obj, "target_port",                  json_object_new_int(env->target_server.host.port));
    json_object_object_add(stat_obj, "backup_host",                  json_object_new_string(env->backup_server.host.ipaddr));
    json_object_object_add(stat_obj, "backup_port",                  json_object_new_int(env->backup_server.host.port));
    json_object_object_add(stat_obj, "current_target_host",          json_object_new_string(na_active_host_select(env)));
    json_object_object_add(stat_obj, "current_target_port",          json_object_new_int(na_active_port_select(env)));
    json_object_object_add(stat_obj, "worker_max",                   json_object_new_int(env->worker_max));
    json_object_object_add(stat_obj, "conn_max",                     json_object_new_int(env->conn_max));
    json_object_object_add(stat_obj, "connpool_max",                 json_object_new_int(env->connpool_max));
    json_object_object_add(stat_obj, "is_refused_active",            json_object_new_string(na_bool2str(env->is_refused_active)));
    json_object_object_add(stat_obj, "request_bufsize",              json_object_new_int(env->request_bufsize));
    json_object_object_add(stat_obj, "response_bufsize",             json_object_new_int(env->response_bufsize));
    json_object_object_add(stat_obj, "current_conn",                 json_object_new_int(env->current_conn));
    json_object_object_add(stat_obj, "available_conn",               json_object_new_int(na_available_conn(connpool)));
    json_object_object_add(stat_obj, "current_conn_max",             json_object_new_int(env->current_conn_max));
    json_object_object_add(stat_obj, "slow_query_sec",               json_object_new_double((double)((double)env->slow_query_sec.tv_sec +
                                                                                                     (double)env->slow_query_sec.tv_nsec /
                                                                                                     1000000000L)));
    json_object_object_add(stat_obj, "slow_query_log_format",        json_object_new_string(na_log_format_name(env->slow_query_log_format)));
    json_object_object_add(stat_obj, "worker_map",                   workermap_obj);
    json_object_object_add(stat_obj, "connpool_map",                 connpoolmap_obj);

    snprintf(buf, bufsize, "%s", json_object_to_json_string(stat_obj));

    json_object_put(stat_obj);
}

static int na_available_conn (na_connpool_t *connpool)
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

static struct json_object *na_connpoolmap_array_json(na_connpool_t *connpool)
{
    struct json_object *connpoolmap_obj;
    connpoolmap_obj = json_object_new_array();
    for (int i=0;i<connpool->max;++i) {
        json_object_array_add(connpoolmap_obj, json_object_new_int(connpool->mark[i]));
    }
    return connpoolmap_obj;
}

static struct json_object *na_workermap_array_json(na_env_t *env)
{
    struct json_object *workermap_obj;
    workermap_obj = json_object_new_array();
    for (int i=0;i<env->worker_max;++i) {
        json_object_array_add(workermap_obj, json_object_new_boolean(env->is_worker_busy[i]));
    }
    return workermap_obj;
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

    if ((cfd = na_server_accept(stfd)) < 0) {
        NA_ERROR_OUTPUT(env, "accept()");
        return;
    }

    na_env_set_jbuf(buf, NA_STAT_BUF_MAX, env);

    // send statictics of environment to client
    if ((size = write(cfd, buf, strlen(buf))) < 0) {
        NA_ERROR_OUTPUT(env, "failed to return stat response");
        close(cfd);
        return;
    }

    close(cfd);
}
