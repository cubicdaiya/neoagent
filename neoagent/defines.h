/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#ifndef NA_DEFINES_H
#define NA_DEFINES_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>

#include <ev.h>
#include <pthread.h>
#include <json.h>

#include "ext/fnv.h"

#define NA_DATETIME_BUF_MAX  32
#define NA_HOSTNAME_MAX     256
#define NA_NAME_MAX          64
#define NA_PATH_MAX         256
#define NA_BM_SKIP_SIZE     256

/**
 * time
 */
void na_ts2dt(time_t time, char *format, char *buf, size_t bufsize);
void na_elapsed_time(time_t time, char *buf, size_t bufsize);
void na_difftime(struct timespec *ret, struct timespec *start, struct timespec *end);
void na_addtime(struct timespec *ret, struct timespec *t1, struct timespec *t2);

/**
 * socket
 */
typedef struct na_host_t {
    char ipaddr[NA_HOSTNAME_MAX + 1];
    uint16_t port;
} na_host_t;

void na_set_nonblock (int fd);
int na_target_server_tcpsock_init (void);
void na_target_server_tcpsock_setup (int tsfd, bool is_keepalive);
void na_target_server_hcsock_setup (int tsfd);
void na_set_sockaddr (na_host_t *host, struct sockaddr_in *addr);
na_host_t na_create_host(char *host);
int na_front_server_tcpsock_init (uint16_t port, int conn_max);
int na_front_server_unixsock_init (char *sockpath, mode_t mask, int conn_max);
int na_stat_server_unixsock_init (char *sockpath, mode_t mask);
int na_stat_server_tcpsock_init (uint16_t port);
bool na_server_connect (int tsfd, struct sockaddr_in *tsaddr);
int na_server_accept (int sfd);

/**
 * memproto
 */
typedef enum na_memproto_cmd_t {
    NA_MEMPROTO_CMD_GET,
    NA_MEMPROTO_CMD_SET,
    NA_MEMPROTO_CMD_INCR,
    NA_MEMPROTO_CMD_DECR,
    NA_MEMPROTO_CMD_ADD,
    NA_MEMPROTO_CMD_DELETE,
    NA_MEMPROTO_CMD_QUIT,
    NA_MEMPROTO_CMD_UNKNOWN,
    NA_MEMPROTO_CMD_NOT_DETECTED,
    NA_MEMPROTO_CMD_MAX // Always add new codes to the end before this one
} na_memproto_cmd_t;

void na_memproto_bm_skip_init (void);
na_memproto_cmd_t na_memproto_detect_command (char *buf);
int na_memproto_count_request_get(char *buf, int bufsize);
int na_memproto_count_response_get(char *buf, int bufsize);

/**
 * env
 */
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
    int *active;
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

void na_connpool_create (na_connpool_t *connpool, int c);
void na_connpool_destroy (na_connpool_t *connpool);

/**
 * conf
 */
const char *na_event_model_name (na_event_model_t model);
const char *na_log_format_name (na_log_format_t format);
struct json_object *na_get_conf(const char *conf_file_json);
struct json_object *na_get_ctl(struct json_object *conf_obj);
struct json_object *na_get_environments(struct json_object *conf_obj, int *env_cnt);
void na_conf_ctl_init(struct json_object *ctl_obj, na_ctl_env_t *na_ctl_env);
int na_conf_get_environment_idx(struct json_object *environments_obj, char *envname);
void na_conf_env_init(struct json_object *environments_obj, na_env_t *na_env,
                      int idx, bool reconf);

/**
 * error
 */
typedef enum na_error_t {
    NA_ERROR_INVALID_FD,
    NA_ERROR_INVALID_CMD,
    NA_ERROR_INVALID_HOSTNAME,
    NA_ERROR_INVALID_PORT,
    NA_ERROR_INVALID_SOCKPATH,
    NA_ERROR_INVALID_CONFPATH,
    NA_ERROR_INVALID_JSON_CONFIG,
    NA_ERROR_INVALID_CONNPOOL,
    NA_ERROR_CONNECTION_FAILED,
    NA_ERROR_OUTOF_MEMORY,
    NA_ERROR_OUTOF_BUFFER,
    NA_ERROR_OUTOF_LOOP,
    NA_ERROR_PARSE_JSON_CONFIG,
    NA_ERROR_FAILED_SETUP_SIGNAL,
    NA_ERROR_FAILED_IGNORE_SIGNAL,
    NA_ERROR_FAILED_DAEMONIZE,
    NA_ERROR_FAILED_READ,
    NA_ERROR_FAILED_WRITE,
    NA_ERROR_BROKEN_PIPE,
    NA_ERROR_TOO_MANY_ENVIRONMENTS,
    NA_ERROR_CANT_OPEN_SLOWLOG,
    NA_ERROR_FAILED_CREATE_PROCESS,
    NA_ERROR_INVALID_CTL_CMD,
    NA_ERROR_FAILED_EXECUTE_CTM_CMD,
    NA_ERROR_UNKNOWN,
    NA_ERROR_MAX // Always add new codes to the end before this one
} na_error_t;

const char *na_error_message (na_error_t error);

/**
 * event
 */
void *na_event_loop (void *args);

/**
 * bm
 */
void na_bm_create_table (char *pattern, int *skip);
int na_bm_search (char *haystack, char *pattern, int *skip, int hlen, int plen);

/**
 * connpool
 */
void na_connpool_create (na_connpool_t *connpool, int c);
void na_connpool_destroy (na_connpool_t *connpool);
bool na_connpool_assign (na_env_t *env, na_connpool_t *connpool, int *cur, int *fd, na_server_t *server);
void na_connpool_init (na_env_t *env);
na_connpool_t *na_connpool_select(na_env_t *env);
void na_connpool_switch (na_env_t *env);

/**
 * queue
 */
typedef struct na_event_queue_t {
    na_client_t **queue;
    size_t top;
    size_t bot;
    size_t cnt;
    size_t max;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
} na_event_queue_t;

na_event_queue_t *na_event_queue_create(int c);
void na_event_queue_destroy(na_event_queue_t *q);
bool na_event_queue_push(na_event_queue_t *q, na_client_t *e);
na_client_t *na_event_queue_pop(na_event_queue_t *q);

/**
 * ctl
 */
void *na_ctl_loop (void *args);

/**
 * hc
 */
void na_hc_callback (EV_P_ ev_timer *w, int revents);

/**
 * slowlog
 */
void na_slow_query_gettime(na_env_t *env, struct timespec *time);
void na_slow_query_check(na_client_t *client);
void na_slow_query_open(na_env_t *env);

/**
 * stat
 */
void na_stat_callback (EV_P_ struct ev_io *w, int revents);

/**
 * util
 */
#define NA_FREE(p)                                      \
    do {                                                \
        if (p != NULL) {                                \
            free(p);                                    \
            (p) = NULL;                                 \
        }                                               \
    } while(false)

#define NA_STDERR(s) do {                                               \
        char buf_dt[NA_DATETIME_BUF_MAX];                               \
        time_t cts = time(NULL);                                        \
        na_ts2dt(cts, "%Y-%m-%d %H:%M:%S", buf_dt, NA_DATETIME_BUF_MAX); \
        fprintf(stderr, "%s, %s: %s %d\n", buf_dt, s, __FILE__, __LINE__); \
    } while (false)

#define NA_STDERR_MESSAGE(na_error) do {                                \
        char buf_dt[NA_DATETIME_BUF_MAX];                               \
        time_t cts = time(NULL);                                        \
        na_ts2dt(cts, "%Y-%m-%d %H:%M:%S", buf_dt, NA_DATETIME_BUF_MAX); \
        fprintf(stderr, "%s %s: %s %d\n", buf_dt, na_error_message(na_error), __FILE__, __LINE__); \
    } while (false)

#define NA_DIE_WITH_ERROR(na_error)                                     \
    do {                                                                \
        char buf_dt[NA_DATETIME_BUF_MAX];                               \
        time_t cts = time(NULL);                                        \
        na_ts2dt(cts, "%Y-%m-%d %H:%M:%S", buf_dt, NA_DATETIME_BUF_MAX); \
        fprintf(stderr, "%s %s: %s, %s %d\n", buf_dt, na_error_message(na_error), __FILE__, __FUNCTION__, __LINE__); \
        exit(1);                                                        \
    } while (false)

#endif // NA_DEFINES_H
