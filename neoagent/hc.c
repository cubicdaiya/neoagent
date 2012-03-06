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

#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <ev.h>

#include "hc.h"
#include "error.h"
#include "connpool.h"
#include "env.h"

// external globals
volatile sig_atomic_t SigExit;

void na_health_check_callback (EV_P_ ev_timer *w, int revents)
{
    int tsfd;
    int th_ret;
    na_env_t *env;

    th_ret = 0;
    env    = (na_env_t *)w->data;

    if (SigExit == 1) {
        pthread_exit(&th_ret);
        return;
    }
    
    if (env->error_count_max > 0 && (env->error_count > env->error_count_max)) {
        env->error_count = 0;
        return;
    }

    tsfd = na_target_server_tcpsock_init();

    if (tsfd <= 0) {
        na_error_count_up(env);
        NA_STDERR_MESSAGE(NA_ERROR_INVALID_FD);
        return;
    }

    na_target_server_hcsock_setup(tsfd);

    // health check
    if (!na_server_connect(tsfd, &env->target_server.addr)) {
        if (!env->is_refused_active) {
            if (errno != EINPROGRESS && errno != EALREADY) {
                env->is_refused_active = true;
                pthread_mutex_lock(&env->lock_connpool);
                na_connpool_switch(env);
                pthread_mutex_unlock(&env->lock_connpool);
                env->current_conn = 0;
                NA_STDERR("switch backup server");
            }
        }
    } else {
        if (env->is_refused_active) {
            env->is_refused_active = false;
            pthread_mutex_lock(&env->lock_connpool);
            na_connpool_switch(env);
            pthread_mutex_unlock(&env->lock_connpool);
            env->current_conn = 0;
            NA_STDERR("switch target server");
        }
    }

    close(tsfd);

    ev_timer_stop(EV_A_ w);
    ev_timer_set(w, 5., 0.);
    ev_timer_start(EV_A_ w);
}
