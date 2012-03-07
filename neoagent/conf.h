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

#ifndef NA_CONF_H
#define NA_CONF_H

#include "env.h"

typedef enum na_param_t {
    NA_PARAM_NAME,
    NA_PARAM_PORT,
    NA_PARAM_SOCKPATH,
    NA_PARAM_ACCESS_MASK,
    NA_PARAM_TARGET_SERVER,
    NA_PARAM_BACKUP_SERVER,
    NA_PARAM_STPORT,
    NA_PARAM_STSOCKPATH,
    NA_PARAM_CONN_MAX,
    NA_PARAM_CONNPOOL_MAX,
    NA_PARAM_LOOP_MAX,
    NA_PARAM_ERROR_COUNT_MAX,
    NA_PARAM_IS_CONNPOOL_ONLY,
    NA_PARAM_REQUEST_BUFSIZE,
    NA_PARAM_REQUEST_BUFSIZE_MAX,
    NA_PARAM_RESPONSE_BUFSIZE,
    NA_PARAM_RESPONSE_BUFSIZE_MAX,
    NA_PARAM_MAX // Always add new codes to the end before this one
} na_param_t;

struct json_object *na_get_conf(const char *conf_file_json);
struct json_object *na_get_environments(struct json_object *conf_obj, int *env_cnt);
void na_conf_env_init(struct json_object *environments_obj, na_env_t *na_env, int idx);

#endif // NA_CONF_H

