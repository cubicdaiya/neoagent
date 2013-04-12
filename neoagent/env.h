/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#ifndef NA_ENV_H
#define NA_ENV_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

#include <ev.h>

#include "ext/fnv.h"
#include "socket.h"
#include "memproto.h"

#define NA_NAME_MAX 64
#define NA_PATH_MAX 256

typedef enum na_event_state_t {
    NA_EVENT_STATE_CLIENT_READ,
    NA_EVENT_STATE_CLIENT_WRITE,
    NA_EVENT_STATE_TARGET_READ,
    NA_EVENT_STATE_TARGET_WRITE,
    NA_EVENT_STATE_COMPLETE,
    NA_EVENT_STATE_MAX // Always add new codes to the end before this one
} na_event_state_t;

typedef enum na_event_model_t {
    NA_EVENT_MODEL_SELECT,
    NA_EVENT_MODEL_EPOLL,
    NA_EVENT_MODEL_KQUEUE,
    NA_EVENT_MODEL_AUTO,
    NA_EVENT_MODEL_UNKNOWN,
    NA_EVENT_MODEL_MAX // Always add new codes to the end before this one
} na_event_model_t;

typedef enum na_log_format_t {
    NA_LOG_FORMAT_PLAIN,
    NA_LOG_FORMAT_JSON,
    NA_LOG_FORMAT_UNKNOWN,
    NA_LOG_FORMAT_MAX // Always add new codes to the end before this one
} na_log_format_t;

typedef struct na_server_t {
    na_host_t host;
    struct sockaddr_in addr;
} na_server_t;

typedef struct na_connpool_t {
    int *fd_pool;
    int *mark;
    int *used_cnt;
    int max;
} na_connpool_t;

typedef struct na_ctl_env_t {
    int        fd;
    char       sockpath[NA_PATH_MAX + 1];
    mode_t     access_mask;
    fnv_tbl_t *tbl_env;
    ev_io      watcher;
    char*      restart_envname;
    pthread_mutex_t lock_restart;
} na_ctl_env_t;

typedef struct na_env_t {
    char name[NA_NAME_MAX + 1];
    int fsfd;
    uint16_t fsport;
    int stfd;
    uint16_t stport;
    char fssockpath[NA_PATH_MAX + 1];
    char stsockpath[NA_PATH_MAX + 1];
    mode_t access_mask;
    na_server_t target_server;
    na_server_t backup_server;
    int current_conn;
    int current_conn_max;
    int request_bufsize;
    int response_bufsize;
    ev_io fs_watcher;
    bool is_use_backup;
    bool is_refused_active;
    bool is_refused_accept;
    bool *is_worker_busy;
    na_connpool_t connpool_active;
    na_connpool_t connpool_backup;
    pthread_mutex_t lock_connpool;
    pthread_mutex_t lock_current_conn;
    pthread_mutex_t lock_tid;
    pthread_mutex_t lock_loop;
    pthread_rwlock_t lock_refused;
    pthread_rwlock_t *lock_worker_busy;
    na_event_model_t event_model;
    int worker_max;
    int conn_max;
    int connpool_max;
    int connpool_use_max;
    int client_pool_max;
    int loop_max;
    struct timespec slow_query_sec;
    char slow_query_log_path[NA_PATH_MAX + 1];
    FILE *slow_query_fp;
    na_log_format_t slow_query_log_format;
    mode_t slow_query_log_access_mask;
} na_env_t;

typedef struct na_client_t {
    int cfd;
    int tsfd;
    char *crbuf;
    char *srbuf;
    int crbufsize;
    int cwbufsize;
    int srbufsize;
    int swbufsize;
    int request_bufsize;
    int response_bufsize;
    na_memproto_cmd_t cmd;
    bool is_refused_active;
    bool is_use_connpool;
    bool is_use_client_pool;
    bool is_used;
    na_env_t *env;
    na_event_state_t event_state;
    na_connpool_t *connpool;
    int req_cnt;
    int res_cnt;
    int loop_cnt;
    int cur_pool;
    ev_io c_watcher;
    ev_io ts_watcher;
    pthread_mutex_t lock_use;
    struct timespec na_from_ts_time_begin;
    struct timespec na_from_ts_time_end;
    struct timespec na_to_ts_time_begin;
    struct timespec na_to_ts_time_end;
    struct timespec na_to_client_time_begin;
    struct timespec na_to_client_time_end;
} na_client_t;

void na_ctl_env_setup_default(na_ctl_env_t *ctl_env);
void na_env_setup_default(na_env_t *env, int idx);
void na_env_init(na_env_t *env);
void na_env_clear (na_env_t *env);

void na_connpool_create (na_connpool_t *connpool, int c);
void na_connpool_destroy (na_connpool_t *connpool);

#endif // NA_ENV_H
