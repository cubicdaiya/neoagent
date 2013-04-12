/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#ifndef NA_CONF_H
#define NA_CONF_H

#include "env.h"

typedef enum na_ctl_param_t {
    NA_CTL_PARAM_SOCKPATH,
    NA_CTL_PARAM_ACCESS_MASK,
    NA_CTL_PARAM_MAX
} na_ctl_param_t;

typedef enum na_param_t {
    NA_PARAM_NAME,
    NA_PARAM_PORT,
    NA_PARAM_SOCKPATH,
    NA_PARAM_ACCESS_MASK,
    NA_PARAM_TARGET_SERVER,
    NA_PARAM_BACKUP_SERVER,
    NA_PARAM_STPORT,
    NA_PARAM_STSOCKPATH,
    NA_PARAM_WORKER_MAX,
    NA_PARAM_CONN_MAX,
    NA_PARAM_CONNPOOL_MAX,
    NA_PARAM_CONNPOOL_USE_MAX,
    NA_PARAM_CLIENT_POOL_MAX,
    NA_PARAM_LOOP_MAX,
    NA_PARAM_EVENT_MODEL,
    NA_PARAM_REQUEST_BUFSIZE,
    NA_PARAM_RESPONSE_BUFSIZE,
    NA_PARAM_SLOW_QUERY_SEC,
    NA_PARAM_SLOW_QUERY_LOG_PATH,
    NA_PARAM_SLOW_QUERY_LOG_FORMAT,
    NA_PARAM_SLOW_QUERY_LOG_ACCESS_MASK,
    NA_PARAM_MAX // Always add new codes to the end before this one
} na_param_t;

const char *na_event_model_name (na_event_model_t model);
const char *na_log_format_name (na_log_format_t format);
struct json_object *na_get_conf(const char *conf_file_json);
struct json_object *na_get_ctl(struct json_object *conf_obj);
struct json_object *na_get_environments(struct json_object *conf_obj, int *env_cnt);
void na_conf_ctl_init(struct json_object *ctl_obj, na_ctl_env_t *na_ctl_env);
int na_conf_get_environment_idx(struct json_object *environments_obj, char *envname);
void na_conf_env_init(struct json_object *environments_obj, na_env_t *na_env,
                      int idx, bool reconf);

#endif // NA_CONF_H
