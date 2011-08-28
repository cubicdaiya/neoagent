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

static const int NEOAGENT_JSON_BUF_MAX = 65536;

const char *neoagent_params[NEOAGENT_PARAM_MAX] = {
    [NEOAGENT_PARAM_NAME]              = "name",
    [NEOAGENT_PARAM_PORT]              = "port",
    [NEOAGENT_PARAM_SOCKPATH]          = "sockpath",
    [NEOAGENT_PARAM_ACCESS_MASK]       = "access_mask",
    [NEOAGENT_PARAM_TARGET_SERVER]     = "target_server",
    [NEOAGENT_PARAM_BACKUP_SERVER]     = "backup_server",
    [NEOAGENT_PARAM_STPORT]            = "stport",
    [NEOAGENT_PARAM_CONN_MAX]          = "conn_max",
    [NEOAGENT_PARAM_CONNPOOL_MAX]      = "connpool_max",
    [NEOAGENT_PARAM_IS_CONNPOOL_ONLY]  = "is_connpool_only",
    [NEOAGENT_PARAM_ERROR_COUNT_MAX]   = "error_count_max",
};

static const char *neoagent_param_name (neoagent_param_t param);

static const char *neoagent_param_name (neoagent_param_t param)
{
    return neoagent_params[param];
}

struct json_object *neoagent_cnf_get_environments(const char *conf_file_json, int *env_cnt)
{
    struct json_object *overall_obj;
    struct json_object *environments_obj;
    char json_buf[NEOAGENT_JSON_BUF_MAX + 1];
    int json_fd, size;

    if ((json_fd = open(conf_file_json, O_RDONLY)) < 0) {
        neoagent_die_with_error(NEOAGENT_ERROR_INVALID_CONFPATH);
    }

    if ((size = read(json_fd, json_buf, NEOAGENT_JSON_BUF_MAX)) < 0) {
        neoagent_die_with_error(NEOAGENT_ERROR_INVALID_CONFPATH);
    }

    overall_obj      = json_tokener_parse(json_buf);
    if (is_error(overall_obj)) {
        neoagent_die_with_error(NEOAGENT_ERROR_PARSE_JSON_CONFIG);
    }
    environments_obj = json_object_object_get(overall_obj, "environments");
    *env_cnt         = json_object_array_length(environments_obj);
    
    close(json_fd);
    free(overall_obj);

    return environments_obj;
    
}


void neoagent_conf_env_init(struct json_object *environments_obj, neoagent_env_t *neoagent_env, int idx)
{
    char *e;
    char host_buf[NEOAGENT_HOSTNAME_MAX + 1];
    neoagent_host_t host;
    struct json_object *environment_obj = json_object_array_get_idx(environments_obj, idx);
    struct json_object *param_obj;
    for (int i=0;i<NEOAGENT_PARAM_MAX;++i) {

        param_obj = json_object_object_get(environment_obj, neoagent_param_name(i));

        if (param_obj == NULL) {
            continue;
        }

        switch (i) {
        case NEOAGENT_PARAM_NAME:
            if (!json_object_is_type(param_obj, json_type_string)) {
                neoagent_die_with_error(NEOAGENT_ERROR_INVALID_JSON_CONFIG);
            }
            strncpy(neoagent_env->name, json_object_get_string(param_obj), NEOAGENT_NAME_MAX);
            break;
        case NEOAGENT_PARAM_SOCKPATH:
            if (!json_object_is_type(param_obj, json_type_string)) {
                neoagent_die_with_error(NEOAGENT_ERROR_INVALID_JSON_CONFIG);
            }
            strncpy(neoagent_env->fssockpath, json_object_get_string(param_obj), NEOAGENT_SOCKPATH_MAX);
            break;
        case NEOAGENT_PARAM_TARGET_SERVER:
            if (!json_object_is_type(param_obj, json_type_string)) {
                neoagent_die_with_error(NEOAGENT_ERROR_INVALID_JSON_CONFIG);
            }
            strncpy(host_buf, json_object_get_string(param_obj), NEOAGENT_HOSTNAME_MAX);
            host = neoagent_create_host(host_buf);
            memcpy(&neoagent_env->target_server.host, &host, sizeof(host));
            neoagent_set_sockaddr(&host, &neoagent_env->target_server.addr);
            break;
        case NEOAGENT_PARAM_BACKUP_SERVER:
            strncpy(host_buf, json_object_get_string(param_obj), NEOAGENT_HOSTNAME_MAX);
            host = neoagent_create_host(host_buf);
            memcpy(&neoagent_env->backup_server.host, &host, sizeof(host));
            neoagent_set_sockaddr(&host, &neoagent_env->backup_server.addr);
            break;
        case NEOAGENT_PARAM_PORT:
            if (!json_object_is_type(param_obj, json_type_int)) {
                neoagent_die_with_error(NEOAGENT_ERROR_INVALID_JSON_CONFIG);
            }
            neoagent_env->fsport          = json_object_get_int(param_obj);
            break;
        case NEOAGENT_PARAM_STPORT:
            if (!json_object_is_type(param_obj, json_type_int)) {
                neoagent_die_with_error(NEOAGENT_ERROR_INVALID_JSON_CONFIG);
            }
            neoagent_env->stport          = json_object_get_int(param_obj);
            break;
        case NEOAGENT_PARAM_ACCESS_MASK:
            if (!json_object_is_type(param_obj, json_type_string)) {
                neoagent_die_with_error(NEOAGENT_ERROR_INVALID_JSON_CONFIG);
            }
            neoagent_env->access_mask     = (mode_t)strtol(json_object_get_string(param_obj), &e, 8);
            break;
        case NEOAGENT_PARAM_CONN_MAX:
            if (!json_object_is_type(param_obj, json_type_int)) {
                neoagent_die_with_error(NEOAGENT_ERROR_INVALID_JSON_CONFIG);
            }
            neoagent_env->conn_max        = json_object_get_int(param_obj);
            break;
        case NEOAGENT_PARAM_CONNPOOL_MAX:
            if (!json_object_is_type(param_obj, json_type_int)) {
                neoagent_die_with_error(NEOAGENT_ERROR_INVALID_JSON_CONFIG);
            }
            neoagent_env->connpool_max    = json_object_get_int(param_obj);
            break;
        case NEOAGENT_PARAM_ERROR_COUNT_MAX:
            if (!json_object_is_type(param_obj, json_type_int)) {
                neoagent_die_with_error(NEOAGENT_ERROR_INVALID_JSON_CONFIG);
            }
            neoagent_env->error_count_max = json_object_get_int(param_obj);
            break;
        default:
            // no through
            assert(false);
            break;
        }
    }
}



