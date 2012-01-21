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

#include "stat.h"
#include "version.h"

static const int NA_STAT_MBUF_MAX = 4096;

void na_env_set_jbuf(char *buf, int bufsize, na_env_t *env)
{
    na_connpool_t *connpool;
    struct json_object *stat_obj;
    struct json_object *connpoolmap_obj;

    connpool        = env->is_refused_active ? &env->connpool_backup : &env->connpool_active;
    stat_obj        = json_object_new_object();
    connpoolmap_obj = na_connpoolmap_array_json(connpool);

    json_object_object_add(stat_obj, "name",                json_object_new_string(NA_NAME));
    json_object_object_add(stat_obj, "version",             json_object_new_string(NA_VERSION));
    json_object_object_add(stat_obj, "environment name",    json_object_new_string(env->name));
    json_object_object_add(stat_obj, "fsfd",                json_object_new_int(env->fsfd));
    json_object_object_add(stat_obj, "fsport",              json_object_new_int(env->fsport));
    json_object_object_add(stat_obj, "fssockpath",          json_object_new_string(env->fssockpath));
    json_object_object_add(stat_obj, "target_host",         json_object_new_string(env->target_server.host.ipaddr));
    json_object_object_add(stat_obj, "target_port",         json_object_new_int(env->target_server.host.port));
    json_object_object_add(stat_obj, "backup_host",         json_object_new_string(env->backup_server.host.ipaddr));
    json_object_object_add(stat_obj, "backup_port",         json_object_new_int(env->backup_server.host.port));
    json_object_object_add(stat_obj, "current_target_host", json_object_new_string(env->is_refused_active ? 
                                                                                   env->backup_server.host.ipaddr : env->target_server.host.ipaddr));
    json_object_object_add(stat_obj, "current_target_port", json_object_new_int(env->is_refused_active ?
                                                                                env->backup_server.host.port : env->target_server.host.port));
    json_object_object_add(stat_obj, "error_count",         json_object_new_int(env->error_count));
    json_object_object_add(stat_obj, "error_count_max",     json_object_new_int(env->error_count_max));
    json_object_object_add(stat_obj, "conn_max",            json_object_new_int(env->conn_max));
    json_object_object_add(stat_obj, "connpool_max",        json_object_new_int(env->connpool_max));
    json_object_object_add(stat_obj, "is_connpool_only",    json_object_new_string(env->is_connpool_only ?  "true" : "false"));
    json_object_object_add(stat_obj, "is_refused_active",   json_object_new_string(env->is_refused_active ? "true" : "false"));
    json_object_object_add(stat_obj, "bufsize",             json_object_new_int(env->bufsize));
    json_object_object_add(stat_obj, "current_conn",        json_object_new_int(env->current_conn));
    json_object_object_add(stat_obj, "available_conn",      json_object_new_int(na_available_conn(connpool)));
    json_object_object_add(stat_obj, "connpool_map",        connpoolmap_obj);

    snprintf(buf, bufsize, "%s", json_object_to_json_string(stat_obj));

    json_object_put(stat_obj);
}    

int na_available_conn (na_connpool_t *connpool)
{
    int available_conn;

    available_conn = 0;

    for (int i=0;i<connpool->max;++i) {
        if (connpool->mark[i] == 0) {
            ++available_conn;
        }
    }

    return available_conn;
}

void na_env_set_buf(char *buf, int bufsize, na_env_t *env)
{
    na_connpool_t *connpool;
    char mbuf[NA_STAT_MBUF_MAX + 1];

    connpool = env->is_refused_active ? &env->connpool_backup : &env->connpool_active;

    na_connpoolmap_set_buf(mbuf, NA_STAT_MBUF_MAX, connpool);
    snprintf(buf, 
             bufsize,
             "environment stats\n\n"
             "name               :%s\n"
             "version            :%s\n"
             "environment name   :%s\n"
             "fsfd               :%d\n"
             "fsport             :%d\n"
             "fssockpath         :%s\n"
             "target host        :%s\n"
             "target port        :%d\n"
             "backup host        :%s\n"
             "backup port        :%d\n"
             "current target host:%s\n"
             "current target port:%d\n"
             "error count        :%d\n"
             "error count max    :%d\n"
             "conn max           :%d\n"
             "connpool max       :%d\n"
             "is_connpool_only   :%s\n"
             "is_refused_active  :%s\n"
             "bufsize            :%d\n"
             "current conn       :%d\n"
             "available conn     :%d\n"
             "connpool_map       :%s\n"
             ,
             NA_NAME, NA_VERSION,
             env->name, env->fsfd, env->fsport, env->fssockpath, 
             env->target_server.host.ipaddr, env->target_server.host.port,
             env->backup_server.host.ipaddr, env->backup_server.host.port,
             env->is_refused_active ? env->backup_server.host.ipaddr : env->target_server.host.ipaddr,
             env->is_refused_active ? env->backup_server.host.port   : env->target_server.host.port,
             env->error_count, env->error_count_max, env->conn_max, env->connpool_max, 
             env->is_connpool_only  ? "true" : "false",
             env->is_refused_active ? "true" : "false",
             env->bufsize, env->current_conn, na_available_conn(connpool), mbuf
             );
}

json_object *na_connpoolmap_array_json(na_connpool_t *connpool)
{
    struct json_object *connpoolmap_obj;
    connpoolmap_obj = json_object_new_array();
    for (int i=0;i<connpool->max;++i) {
        json_object_array_add(connpoolmap_obj, json_object_new_int(connpool->mark[i]));
    }
    return connpoolmap_obj;
}

void na_connpoolmap_set_buf(char *buf, int bufsize, na_connpool_t *connpool)
{
    for (int i=0;i<connpool->max;++i) {
        snprintf(buf + i * 2, bufsize - i * 2, "%d ", connpool->mark[i]);
    }
    buf[connpool->max * 2] = '\0';
}
