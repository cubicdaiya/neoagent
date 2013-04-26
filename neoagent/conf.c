/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "defines.h"

typedef enum na_ctl_param_t {
    NA_CTL_PARAM_SOCKPATH,
    NA_CTL_PARAM_ACCESS_MASK,
    NA_CTL_PARAM_LOGPATH,
    NA_CTL_PARAM_MAX // Always add new codes to the end before this one
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

#define NA_PARAM_TYPE_CHECK(param_obj, expected_type) do {            \
        if (!json_object_is_type(param_obj, expected_type)) {         \
            NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);          \
        }                                                             \
    } while(false)

static const int NA_JSON_BUF_MAX = 65536;

const char *na_ctl_params[NA_PARAM_MAX] = {
    [NA_CTL_PARAM_SOCKPATH]    = "sockpath",
    [NA_CTL_PARAM_ACCESS_MASK] = "access_mask",
    [NA_CTL_PARAM_LOGPATH]     = "logpath",
};

const char *na_params[NA_PARAM_MAX]     = {
    [NA_PARAM_NAME]                     = "name",
    [NA_PARAM_PORT]                     = "port",
    [NA_PARAM_SOCKPATH]                 = "sockpath",
    [NA_PARAM_ACCESS_MASK]              = "access_mask",
    [NA_PARAM_TARGET_SERVER]            = "target_server",
    [NA_PARAM_BACKUP_SERVER]            = "backup_server",
    [NA_PARAM_STPORT]                   = "stport",
    [NA_PARAM_STSOCKPATH]               = "stsockpath",
    [NA_PARAM_WORKER_MAX]               = "worker_max",
    [NA_PARAM_CONN_MAX]                 = "conn_max",
    [NA_PARAM_CONNPOOL_MAX]             = "connpool_max",
    [NA_PARAM_CONNPOOL_USE_MAX]         = "connpool_use_max",
    [NA_PARAM_CLIENT_POOL_MAX]          = "client_pool_max",
    [NA_PARAM_LOOP_MAX]                 = "loop_max",
    [NA_PARAM_EVENT_MODEL]              = "event_model",
    [NA_PARAM_REQUEST_BUFSIZE]          = "request_bufsize",
    [NA_PARAM_RESPONSE_BUFSIZE]         = "response_bufsize",
    [NA_PARAM_SLOW_QUERY_SEC]           = "slow_query_sec",
    [NA_PARAM_SLOW_QUERY_LOG_PATH]      = "slow_query_log_path",
    [NA_PARAM_SLOW_QUERY_LOG_FORMAT]    = "slow_query_log_format",
    [NA_PARAM_SLOW_QUERY_LOG_ACCESS_MASK] = "slow_query_log_access_mask"
};

static const char *na_event_models[NA_EVENT_MODEL_MAX] = {
    [NA_EVENT_MODEL_SELECT] = "select",
    [NA_EVENT_MODEL_EPOLL]  = "epoll",
    [NA_EVENT_MODEL_KQUEUE] = "kqueue",
    [NA_EVENT_MODEL_AUTO]   = "auto",
};

static const char *na_log_formats[NA_LOG_FORMAT_MAX] = {
    [NA_LOG_FORMAT_PLAIN] = "text",
    [NA_LOG_FORMAT_JSON]  = "json"
};

static const char *na_ctl_param_name (na_ctl_param_t param);
static const char *na_param_name (na_param_t param);
static na_event_model_t na_detect_event_model (const char *model_str);

static const char *na_ctl_param_name (na_ctl_param_t param)
{
    return na_ctl_params[param];
}

static const char *na_param_name (na_param_t param)
{
    return na_params[param];
}

static na_event_model_t na_detect_event_model (const char *model_str)
{
    na_event_model_t model;
    if (strcmp(model_str, na_event_models[NA_EVENT_MODEL_SELECT]) == 0) {
        model = NA_EVENT_MODEL_SELECT;
    } else if (strcmp(model_str, na_event_models[NA_EVENT_MODEL_EPOLL]) == 0) {
        model = NA_EVENT_MODEL_EPOLL;
    } else if (strcmp(model_str, na_event_models[NA_EVENT_MODEL_KQUEUE]) == 0) {
        model = NA_EVENT_MODEL_KQUEUE;
    } else if (strcmp(model_str, na_event_models[NA_EVENT_MODEL_AUTO]) == 0) {
        model = NA_EVENT_MODEL_AUTO;
    } else {
        model = NA_EVENT_MODEL_UNKNOWN;
    }
    return model;
}

static na_log_format_t na_detect_log_format (const char *format_str)
{
    na_log_format_t format;
    if (strcmp(format_str, na_log_formats[NA_LOG_FORMAT_JSON]) == 0) {
        format = NA_LOG_FORMAT_JSON;
    } else if (strcmp(format_str, na_log_formats[NA_LOG_FORMAT_PLAIN]) == 0) {
        format = NA_LOG_FORMAT_PLAIN;
    } else {
        format = NA_LOG_FORMAT_UNKNOWN;
    }
    return format;
}

const char *na_event_model_name (na_event_model_t model)
{
    return na_event_models[model];
}

const char *na_log_format_name (na_log_format_t format)
{
    return na_log_formats[format];
}

struct json_object *na_get_conf (const char *conf_file_json)
{
    struct json_object *conf_obj;

    char json_buf[NA_JSON_BUF_MAX + 1];
    int json_fd, size;

    if ((json_fd = open(conf_file_json, O_RDONLY)) < 0) {
        NA_DIE_WITH_ERROR(NA_ERROR_INVALID_CONFPATH);
    }

    memset(json_buf, 0, NA_JSON_BUF_MAX + 1);
    if ((size = read(json_fd, json_buf, NA_JSON_BUF_MAX)) < 0) {
        NA_DIE_WITH_ERROR(NA_ERROR_INVALID_CONFPATH);
    }

    conf_obj = json_tokener_parse(json_buf);
    if (is_error(conf_obj)) {
        NA_DIE_WITH_ERROR(NA_ERROR_PARSE_JSON_CONFIG);
    }

    close(json_fd);

    return conf_obj;
}

struct json_object *na_get_environments(struct json_object *conf_obj, int *env_cnt)
{
    struct json_object *environments_obj;

    environments_obj = json_object_object_get(conf_obj, "environments");
    *env_cnt         = json_object_array_length(environments_obj);

    return environments_obj;
}

struct json_object *na_get_ctl(struct json_object *conf_obj)
{
    return json_object_object_get(conf_obj, "ctl");
}

void na_conf_ctl_init(struct json_object *ctl_obj, na_ctl_env_t *na_ctl_env)
{
    char *e;
    struct json_object *param_obj;

    for (int i=0;i<NA_CTL_PARAM_MAX;++i) {
        param_obj = json_object_object_get(ctl_obj, na_ctl_param_name(i));

        if (param_obj == NULL) {
            continue;
        }

        switch (i) {
        case NA_CTL_PARAM_SOCKPATH:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
            strncpy(na_ctl_env->sockpath, json_object_get_string(param_obj), NA_PATH_MAX);
            break;
        case NA_CTL_PARAM_ACCESS_MASK:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
            na_ctl_env->access_mask = (mode_t)strtol(json_object_get_string(param_obj), &e, 8);
            break;
        case NA_CTL_PARAM_LOGPATH:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
            strncpy(na_ctl_env->logpath, json_object_get_string(param_obj), NA_PATH_MAX);
            break;
        }
    }

}

int na_conf_get_environment_idx(struct json_object *environments_obj, char *envname)
{
    struct json_object *environment_obj;
    struct json_object *param_obj;
    int l;

    l = json_object_array_length(environments_obj);

    for (int i=0;i<l;++i) {
        environment_obj = json_object_array_get_idx(environments_obj, i);
        param_obj       = json_object_object_get(environment_obj, "name");
        if (strcmp(envname, json_object_get_string(param_obj)) == 0) {
            return i;
        }
    }
    return -1;
}

void na_conf_env_init(struct json_object *environments_obj, na_env_t *na_env,
                      int idx, bool reconf)
{
    char *e;
    char host_buf[NA_HOSTNAME_MAX + 1];
    na_host_t host;
    struct json_object *environment_obj;
    struct json_object *param_obj;
    bool have_slow_query_log_path_opt = false;

    environment_obj = json_object_array_get_idx(environments_obj, idx);

    for (int i=0;i<NA_PARAM_MAX;++i) {
        double slow_query_sec;

        param_obj = json_object_object_get(environment_obj, na_param_name(i));

        if (param_obj == NULL) {
            continue;
        }

        // runtime-reconfigurable parameters
        switch (i) {
        case NA_PARAM_REQUEST_BUFSIZE:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
            na_env->request_bufsize = json_object_get_int(param_obj);
            continue;
        case NA_PARAM_RESPONSE_BUFSIZE:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
            na_env->response_bufsize = json_object_get_int(param_obj);
            continue;
        case NA_PARAM_CONN_MAX:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
            na_env->conn_max = json_object_get_int(param_obj);
            continue;
        case NA_PARAM_SLOW_QUERY_SEC:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_double);
            slow_query_sec = json_object_get_double(param_obj);
            na_env->slow_query_sec.tv_sec = slow_query_sec;
            na_env->slow_query_sec.tv_nsec = 1000000000L * (slow_query_sec - (long)slow_query_sec);
            continue;
        case NA_PARAM_SLOW_QUERY_LOG_PATH:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
            strncpy(na_env->slow_query_log_path, json_object_get_string(param_obj), NA_PATH_MAX);
            have_slow_query_log_path_opt = true;
            continue;
        default:
            break;
        }

        // if we didn't find a reconfigurable parameter, try the others
        if (!reconf) {
            switch (i) {
            case NA_PARAM_NAME:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
                strncpy(na_env->name, json_object_get_string(param_obj), NA_NAME_MAX);
                break;
            case NA_PARAM_SOCKPATH:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
                strncpy(na_env->fssockpath, json_object_get_string(param_obj), NA_PATH_MAX);
                break;
            case NA_PARAM_TARGET_SERVER:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
                strncpy(host_buf, json_object_get_string(param_obj), NA_HOSTNAME_MAX);
                host = na_create_host(host_buf);
                memcpy(&na_env->target_server.host, &host, sizeof(host));
                na_set_sockaddr(&host, &na_env->target_server.addr);
                break;
            case NA_PARAM_BACKUP_SERVER:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
                strncpy(host_buf, json_object_get_string(param_obj), NA_HOSTNAME_MAX);
                host = na_create_host(host_buf);
                memcpy(&na_env->backup_server.host, &host, sizeof(host));
                na_set_sockaddr(&host, &na_env->backup_server.addr);
                na_env->is_use_backup = true;
                break;
            case NA_PARAM_PORT:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
                na_env->fsport = json_object_get_int(param_obj);
                break;
            case NA_PARAM_STPORT:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
                na_env->stport = json_object_get_int(param_obj);
                break;
            case NA_PARAM_STSOCKPATH:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
                strncpy(na_env->stsockpath, json_object_get_string(param_obj), NA_PATH_MAX);
                break;
            case NA_PARAM_ACCESS_MASK:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
                na_env->access_mask = (mode_t)strtol(json_object_get_string(param_obj), &e, 8);
                break;
            case NA_PARAM_WORKER_MAX:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
                na_env->worker_max = json_object_get_int(param_obj);
                break;
            case NA_PARAM_CONNPOOL_MAX:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
                na_env->connpool_max = json_object_get_int(param_obj);
                break;
            case NA_PARAM_CONNPOOL_USE_MAX:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
                na_env->connpool_use_max = json_object_get_int(param_obj);
                break;
            case NA_PARAM_CLIENT_POOL_MAX:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
                na_env->client_pool_max = json_object_get_int(param_obj);
                break;
            case NA_PARAM_LOOP_MAX:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
                na_env->loop_max = json_object_get_int(param_obj);
                break;
            case NA_PARAM_EVENT_MODEL:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
                na_env->event_model = na_detect_event_model(json_object_get_string(param_obj));
                if (na_env->event_model == NA_EVENT_MODEL_UNKNOWN) {
                    NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
                }
                break;
            case NA_PARAM_SLOW_QUERY_LOG_FORMAT:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
                na_env->slow_query_log_format = na_detect_log_format(json_object_get_string(param_obj));
                if (na_env->slow_query_log_format == NA_LOG_FORMAT_UNKNOWN) {
                    NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
                }
                break;
            case NA_PARAM_SLOW_QUERY_LOG_ACCESS_MASK:
                NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
                na_env->slow_query_log_access_mask = (mode_t)strtol(json_object_get_string(param_obj), &e, 8);
                break;
            default:
                // no through
                assert(false);
                break;
            }
        }
    }

    // open slow query log, if enabled
    if (((na_env->slow_query_sec.tv_sec  != 0)  ||
         (na_env->slow_query_sec.tv_nsec != 0))) {
        if (!have_slow_query_log_path_opt) {
            NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
        } else {
            na_slow_query_open(na_env);
        }
    }
}
