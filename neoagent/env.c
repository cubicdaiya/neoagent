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

#include "env.h"
#include "mpool.h"

static const int NEOAGENT_PORT_DEFAULT            = 30001;
static const int NEOAGENT_STPORT_DEFAULT          = 30011;
static const int NEOAGENT_CONN_MAX_DEFAULT        = 1000;
static const int NEOAGENT_CONNPOOL_MAX_DEFAULT    = 20;
static const int NEOAGENT_ERROR_COUNT_MAX_DEFAULT = 1000;
static const int NEOAGENT_ACCESS_MASK_DEFAULT     = 0664;

// private functions
static void neoagent_connpool_destory (neoagent_connpool_t *connpool);

static void neoagent_connpool_destory (neoagent_connpool_t *connpool)
{
    for (int i=0;i<connpool->max;++i) {
        if (connpool->fd_pool[i] > 0) {
            close(connpool->fd_pool[i]);
            connpool->fd_pool[i] = 0;
            connpool->mark[i]    = 0;
        }
    }
    free(connpool->fd_pool);
    free(connpool->mark);
    connpool->fd_pool = NULL;
    connpool->mark    = NULL;
    connpool->cur     = 0;
    connpool->is_full = false;
}


mpool_t *neoagent_pool_create (int size)
{
    return mpool_create(size);
}

void neoagent_pool_destroy (mpool_t *pool)
{
    mpool_destroy(pool);
}

neoagent_env_t *neoagent_env_add (mpool_t **env_pool)
{
    neoagent_env_t *env;
    env = mpool_alloc(sizeof(*env), *env_pool);
    memset(env, 0, sizeof(*env));
    return env;
}

void neoagent_env_setup_default(neoagent_env_t *env, int idx)
{
    char *target_server_s = "127.0.0.1:11211";
    char *backup_server_s = "127.0.0.1:11212";
    neoagent_host_t host;
    sprintf(env->name, "env%d", idx);
    env->fsport          = NEOAGENT_PORT_DEFAULT + idx;
    env->access_mask     = NEOAGENT_ACCESS_MASK_DEFAULT;
    host                 = neoagent_create_host(target_server_s);
    memcpy(&env->target_server.host, &host, sizeof(host));
    neoagent_set_sockaddr(&host, &env->target_server.addr);
    host                 = neoagent_create_host(backup_server_s);
    memcpy(&env->backup_server.host, &host, sizeof(host));
    neoagent_set_sockaddr(&host, &env->backup_server.addr);
    env->stport          = NEOAGENT_STPORT_DEFAULT + idx;
    env->conn_max        = NEOAGENT_CONN_MAX_DEFAULT;
    env->connpool_max    = NEOAGENT_CONNPOOL_MAX_DEFAULT;
    env->error_count_max = NEOAGENT_ERROR_COUNT_MAX_DEFAULT;
}

void neoagent_connpool_create (neoagent_connpool_t *connpool, int c)
{
    connpool->fd_pool = calloc(sizeof(int), c);
    connpool->mark    = calloc(sizeof(int), c);
    connpool->cur     = 0;
    connpool->max     = c;
    connpool->is_full = false;
}

void neoagent_connpool_switch (neoagent_env_t *env, int c)
{
    neoagent_connpool_destory(&env->connpool_active);
    neoagent_connpool_create(&env->connpool_active, c);
}
