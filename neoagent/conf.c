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

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include <json/json.h>

#include "conf.h"
#include "env.h"
#include "socket.h"
#include "error.h"

#define NA_PARAM_TYPE_CHECK(param_obj, expected_type) do {            \
        if (!json_object_is_type(param_obj, expected_type)) {         \
            NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);          \
        }                                                             \
    } while(false)

static const int NA_JSON_BUF_MAX = 65536;

const char *na_params[NA_PARAM_MAX] = {
    [NA_PARAM_NAME]                 = "name",
    [NA_PARAM_PORT]                 = "port",
    [NA_PARAM_SOCKPATH]             = "sockpath",
    [NA_PARAM_ACCESS_MASK]          = "access_mask",
    [NA_PARAM_TARGET_SERVER]        = "target_server",
    [NA_PARAM_BACKUP_SERVER]        = "backup_server",
    [NA_PARAM_STPORT]               = "stport",
    [NA_PARAM_STSOCKPATH]           = "stsockpath",
    [NA_PARAM_WORKER_MAX]           = "worker_max",
    [NA_PARAM_CONN_MAX]             = "conn_max",
    [NA_PARAM_CONNPOOL_MAX]         = "connpool_max",
    [NA_PARAM_CONNPOOL_USE_MAX]     = "connpool_use_max",
    [NA_PARAM_CLIENT_POOL_MAX]      = "client_pool_max",
    [NA_PARAM_LOOP_MAX]             = "loop_max",
    [NA_PARAM_EVENT_MODEL]          = "event_model",
    [NA_PARAM_ERROR_COUNT_MAX]      = "error_count_max",
    [NA_PARAM_IS_CONNPOOL_ONLY]     = "is_connpool_only",
    [NA_PARAM_REQUEST_BUFSIZE]      = "request_bufsize",
    [NA_PARAM_REQUEST_BUFSIZE_MAX]  = "request_bufsize_max",
    [NA_PARAM_RESPONSE_BUFSIZE]     = "response_bufsize",
    [NA_PARAM_RESPONSE_BUFSIZE_MAX] = "response_bufsize_max",
};

const char *na_event_models[NA_EVENT_MODEL_MAX] = {
    [NA_EVENT_MODEL_SELECT] = "select",
    [NA_EVENT_MODEL_EPOLL]  = "epoll",
    [NA_EVENT_MODEL_KQUEUE] = "kqueue",
    [NA_EVENT_MODEL_AUTO]   = "auto",

};

static const char *na_param_name (na_param_t param);
static na_event_model_t na_detect_event_model (const char *model_str);

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

const char *na_event_model_name (na_event_model_t model)
{
    return na_event_models[model];
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

void na_conf_env_init(struct json_object *environments_obj, na_env_t *na_env, int idx)
{
    char *e;
    char host_buf[NA_HOSTNAME_MAX + 1];
    na_host_t host;
    struct json_object *environment_obj;
    struct json_object *param_obj;

    environment_obj = json_object_array_get_idx(environments_obj, idx);

    for (int i=0;i<NA_PARAM_MAX;++i) {

        param_obj = json_object_object_get(environment_obj, na_param_name(i));

        if (param_obj == NULL) {
            continue;
        }

        switch (i) {
        case NA_PARAM_NAME:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
            strncpy(na_env->name, json_object_get_string(param_obj), NA_NAME_MAX);
            break;
        case NA_PARAM_SOCKPATH:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
            strncpy(na_env->fssockpath, json_object_get_string(param_obj), NA_SOCKPATH_MAX);
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
            strncpy(na_env->stsockpath, json_object_get_string(param_obj), NA_SOCKPATH_MAX);
            break;
        case NA_PARAM_ACCESS_MASK:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_string);
            na_env->access_mask = (mode_t)strtol(json_object_get_string(param_obj), &e, 8);
            break;
        case NA_PARAM_IS_CONNPOOL_ONLY:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_boolean);
            na_env->is_connpool_only = json_object_get_boolean(param_obj) == 1 ? true : false;
            break;
        case NA_PARAM_REQUEST_BUFSIZE:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
            na_env->request_bufsize = json_object_get_int(param_obj);
            break;
        case NA_PARAM_REQUEST_BUFSIZE_MAX:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
            na_env->request_bufsize_max = json_object_get_int(param_obj);
            break;
        case NA_PARAM_RESPONSE_BUFSIZE:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
            na_env->response_bufsize = json_object_get_int(param_obj);
            break;
        case NA_PARAM_RESPONSE_BUFSIZE_MAX:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
            na_env->response_bufsize_max = json_object_get_int(param_obj);
            break;
        case NA_PARAM_WORKER_MAX:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
            na_env->worker_max = json_object_get_int(param_obj);
            break;
        case NA_PARAM_CONN_MAX:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
            na_env->conn_max = json_object_get_int(param_obj);
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
        case NA_PARAM_ERROR_COUNT_MAX:
            NA_PARAM_TYPE_CHECK(param_obj, json_type_int);
            na_env->error_count_max = json_object_get_int(param_obj);
            break;
        default:
            // no through
            assert(false);
            break;
        }
    }

    na_env->is_extensible_request_buf  = na_env->request_bufsize  < na_env->request_bufsize_max  ? true : false;
    na_env->is_extensible_response_buf = na_env->response_bufsize < na_env->response_bufsize_max ? true : false;

}
