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

#include "env.h"
#include "mpool.h"
#include "util.h"

static const int  NA_PORT_DEFAULT             = 30001;
static const int  NA_STPORT_DEFAULT           = 30011;
static const int  NA_CONN_MAX_DEFAULT         = 1000;
static const int  NA_CONNPOOL_MAX_DEFAULT     = 20;
static const int  NA_ERROR_COUNT_MAX_DEFAULT  = 1000;
static const int  NA_ACCESS_MASK_DEFAULT      = 0664;
static const int  NA_BUFSIZE_DEFAULT          = 65536;
static const bool NA_IS_CONNPOOL_ONLY_DEFAULT = false;

mpool_t *na_pool_create (int size)
{
    return mpool_create(size);
}

void na_pool_destroy (mpool_t *pool)
{
    mpool_destroy(pool);
}

na_env_t *na_env_add (mpool_t **env_pool)
{
    na_env_t *env;
    env = mpool_alloc(sizeof(*env), *env_pool);
    memset(env, 0, sizeof(*env));
    return env;
}

void na_env_setup_default(na_env_t *env, int idx)
{
    char *target_server_s = "127.0.0.1:11211";
    char *backup_server_s = "127.0.0.1:11212";
    na_host_t host;
    sprintf(env->name, "env%d", idx);
    env->fsport           = NA_PORT_DEFAULT + idx;
    env->access_mask      = NA_ACCESS_MASK_DEFAULT;
    host                  = na_create_host(target_server_s);
    memcpy(&env->target_server.host, &host, sizeof(host));
    na_set_sockaddr(&host, &env->target_server.addr);
    host                  = na_create_host(backup_server_s);
    memcpy(&env->backup_server.host, &host, sizeof(host));
    na_set_sockaddr(&host, &env->backup_server.addr);
    env->stport           = NA_STPORT_DEFAULT + idx;
    env->conn_max         = NA_CONN_MAX_DEFAULT;
    env->connpool_max     = NA_CONNPOOL_MAX_DEFAULT;
    env->error_count_max  = NA_ERROR_COUNT_MAX_DEFAULT;
    env->is_connpool_only = NA_IS_CONNPOOL_ONLY_DEFAULT;
    env->request_bufsize  = NA_BUFSIZE_DEFAULT;
    env->response_bufsize = NA_BUFSIZE_DEFAULT;
}

void na_env_clear (na_env_t *env)
{
    env->error_count      = 0;
    env->current_conn_max = 0;
}

void na_connpool_create (na_connpool_t *connpool, int c)
{
    connpool->fd_pool = calloc(sizeof(int), c);
    connpool->mark    = calloc(sizeof(int), c);
    connpool->max     = c;
}

void na_connpool_destroy (na_connpool_t *connpool)
{
    NA_FREE(connpool->fd_pool);
    NA_FREE(connpool->mark);
}
