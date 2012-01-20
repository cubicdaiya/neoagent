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

static const int NA_JSON_BUF_MAX = 65536;

const char *na_params[NA_PARAM_MAX] = {
    [NA_PARAM_NAME]              = "name",
    [NA_PARAM_PORT]              = "port",
    [NA_PARAM_SOCKPATH]          = "sockpath",
    [NA_PARAM_ACCESS_MASK]       = "access_mask",
    [NA_PARAM_TARGET_SERVER]     = "target_server",
    [NA_PARAM_BACKUP_SERVER]     = "backup_server",
    [NA_PARAM_STPORT]            = "stport",
    [NA_PARAM_CONN_MAX]          = "conn_max",
    [NA_PARAM_CONNPOOL_MAX]      = "connpool_max",
    [NA_PARAM_LOOP_MAX]          = "loop_max",
    [NA_PARAM_ERROR_COUNT_MAX]   = "error_count_max",
    [NA_PARAM_IS_CONNPOOL_ONLY]  = "is_connpool_only",
    [NA_PARAM_BUFSIZE]           = "bufsize",
};

static const char *na_param_name (na_param_t param);

static const char *na_param_name (na_param_t param)
{
    return na_params[param];
}

struct json_object *na_cnf_get_environments(const char *conf_file_json, int *env_cnt)
{
    struct json_object *overall_obj;
    struct json_object *environments_obj;
    char json_buf[NA_JSON_BUF_MAX + 1];
    int json_fd, size;

    if ((json_fd = open(conf_file_json, O_RDONLY)) < 0) {
        NA_DIE_WITH_ERROR(NA_ERROR_INVALID_CONFPATH);
    }

    memset(json_buf, 0, NA_JSON_BUF_MAX + 1);
    if ((size = read(json_fd, json_buf, NA_JSON_BUF_MAX)) < 0) {
        NA_DIE_WITH_ERROR(NA_ERROR_INVALID_CONFPATH);
    }

    overall_obj      = json_tokener_parse(json_buf);
    if (is_error(overall_obj)) {
        NA_DIE_WITH_ERROR(NA_ERROR_PARSE_JSON_CONFIG);
    }
    environments_obj = json_object_object_get(overall_obj, "environments");
    *env_cnt         = json_object_array_length(environments_obj);
    
    close(json_fd);
    free(overall_obj);

    return environments_obj;
    
}


void na_conf_env_init(struct json_object *environments_obj, na_env_t *na_env, int idx)
{
    char *e;
    char host_buf[NA_HOSTNAME_MAX + 1];
    na_host_t host;
    struct json_object *environment_obj = json_object_array_get_idx(environments_obj, idx);
    struct json_object *param_obj;
    for (int i=0;i<NA_PARAM_MAX;++i) {

        param_obj = json_object_object_get(environment_obj, na_param_name(i));

        if (param_obj == NULL) {
            continue;
        }

        switch (i) {
        case NA_PARAM_NAME:
            if (!json_object_is_type(param_obj, json_type_string)) {
                NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
            }
            strncpy(na_env->name, json_object_get_string(param_obj), NA_NAME_MAX);
            break;
        case NA_PARAM_SOCKPATH:
            if (!json_object_is_type(param_obj, json_type_string)) {
                NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
            }
            strncpy(na_env->fssockpath, json_object_get_string(param_obj), NA_SOCKPATH_MAX);
            break;
        case NA_PARAM_TARGET_SERVER:
            if (!json_object_is_type(param_obj, json_type_string)) {
                NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
            }
            strncpy(host_buf, json_object_get_string(param_obj), NA_HOSTNAME_MAX);
            host = na_create_host(host_buf);
            memcpy(&na_env->target_server.host, &host, sizeof(host));
            na_set_sockaddr(&host, &na_env->target_server.addr);
            break;
        case NA_PARAM_BACKUP_SERVER:
            strncpy(host_buf, json_object_get_string(param_obj), NA_HOSTNAME_MAX);
            host = na_create_host(host_buf);
            memcpy(&na_env->backup_server.host, &host, sizeof(host));
            na_set_sockaddr(&host, &na_env->backup_server.addr);
            break;
        case NA_PARAM_PORT:
            if (!json_object_is_type(param_obj, json_type_int)) {
                NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
            }
            na_env->fsport = json_object_get_int(param_obj);
            break;
        case NA_PARAM_STPORT:
            if (!json_object_is_type(param_obj, json_type_int)) {
                NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
            }
            na_env->stport = json_object_get_int(param_obj);
            break;
        case NA_PARAM_ACCESS_MASK:
            if (!json_object_is_type(param_obj, json_type_string)) {
                NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
            }
            na_env->access_mask = (mode_t)strtol(json_object_get_string(param_obj), &e, 8);
            break;
        case NA_PARAM_IS_CONNPOOL_ONLY:
            if (!json_object_is_type(param_obj, json_type_boolean)) {
                NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
            }
            na_env->is_connpool_only = json_object_get_boolean(param_obj) == 1 ? true : false;
            break;
        case NA_PARAM_BUFSIZE:
            if (!json_object_is_type(param_obj, json_type_int)) {
                NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
            }
            na_env->bufsize = json_object_get_int(param_obj);
            break;
        case NA_PARAM_CONN_MAX:
            if (!json_object_is_type(param_obj, json_type_int)) {
                NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
            }
            na_env->conn_max = json_object_get_int(param_obj);
            break;
        case NA_PARAM_CONNPOOL_MAX:
            if (!json_object_is_type(param_obj, json_type_int)) {
                NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
            }
            na_env->connpool_max = json_object_get_int(param_obj);
            break;
        case NA_PARAM_LOOP_MAX:
            if (!json_object_is_type(param_obj, json_type_int)) {
                NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
            }
            na_env->loop_max = json_object_get_int(param_obj);
            break;
        case NA_PARAM_ERROR_COUNT_MAX:
            if (!json_object_is_type(param_obj, json_type_int)) {
                NA_DIE_WITH_ERROR(NA_ERROR_INVALID_JSON_CONFIG);
            }
            na_env->error_count_max = json_object_get_int(param_obj);
            break;
        default:
            // no through
            assert(false);
            break;
        }
    }
}



