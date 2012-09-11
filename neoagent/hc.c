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

#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <ev.h>

#include "hc.h"
#include "error.h"
#include "connpool.h"
#include "env.h"

// constants
static const char *na_hc_test_key = "neoagent_test_key";
static const char *na_hc_test_val = "neoagent_test_val";

// external globals
volatile sig_atomic_t SigExit;

static void na_hc_event_set(EV_P_ ev_timer * w, int revents);
static bool na_hc_test_request(int tsfd);

static void na_hc_event_set(EV_P_ ev_timer * w, int revents)
{
    ev_timer_stop(EV_A_ w);
    ev_timer_set(w, 5., 0.);
    ev_timer_start(EV_A_ w);
}

static bool is_success_command(int tsfd, char *command, const char *expected)
{
    int len;
    char rbuf[BUFSIZ];
    if (write(tsfd, command, strlen(command)) < 0) {
        return false;
    }

    if ((len = read(tsfd, rbuf, BUFSIZ)) < 0) {
        return false;
    }
    rbuf[len] = '\0';

    if (strcmp(rbuf, expected) != 0) {
        return false;
    }


    return true;
}

static bool na_hc_test_request(int tsfd)
{
    int  cnt_fail, try_max;
    char hostname[BUFSIZ];

    char scmd[BUFSIZ]; // set
    char gcmd[BUFSIZ]; // get
    char dcmd[BUFSIZ]; // delete
    char gres[BUFSIZ]; // get response

    cnt_fail = 0;
    try_max  = 5;
    gethostname(hostname, BUFSIZ);
    snprintf(scmd, BUFSIZ, "set %s_%s 0 0 %ld\r\n%s_%s\r\n", na_hc_test_key, hostname, strlen(na_hc_test_val) + 1 + strlen(hostname), na_hc_test_val, hostname);
    snprintf(gcmd, BUFSIZ, "get %s_%s\r\n", na_hc_test_key, hostname);
    snprintf(dcmd, BUFSIZ, "delete %s_%s\r\n", na_hc_test_key, hostname);
    snprintf(gres, BUFSIZ, "VALUE %s_%s 0 %ld\r\n%s_%s\r\nEND\r\n", na_hc_test_key, hostname, strlen(na_hc_test_val) + 1 + strlen(hostname), na_hc_test_val, hostname);

    for (int i=0;i<try_max;++i) {

        if (!is_success_command(tsfd, scmd, "STORED\r\n")) {
            ++cnt_fail;
        }

        if (!is_success_command(tsfd, gcmd, gres)){
            ++cnt_fail;
        }

        if (!is_success_command(tsfd, dcmd, "DELETED\r\n")) {
            ++cnt_fail;
        }
    }

    if (cnt_fail == (try_max * 3)) {
        return false;
    }

    return true;
}

void na_hc_callback (EV_P_ ev_timer *w, int revents)
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
    
    pthread_mutex_lock(&env->lock_error_count);
    if (env->error_count_max > 0 && (env->error_count > env->error_count_max)) {
        na_hc_event_set(EV_A_ w, revents);
        env->error_count = 0;
        pthread_mutex_unlock(&env->lock_error_count);
        return;
    }
    pthread_mutex_unlock(&env->lock_error_count);

    tsfd = na_target_server_tcpsock_init();

    if (tsfd <= 0) {
        na_hc_event_set(EV_A_ w, revents);
        na_error_count_up(env);
        NA_STDERR_MESSAGE(NA_ERROR_INVALID_FD);
        return;
    }

    na_target_server_hcsock_setup(tsfd);

    // health check
    if (!na_server_connect(tsfd, &env->target_server.addr)) {
        if (!env->is_refused_active) {
            if (errno != EINPROGRESS && errno != EALREADY) {
                pthread_rwlock_wrlock(&env->lock_refused);
                env->is_refused_accept = true;
                env->is_refused_active = true;
                pthread_mutex_lock(&env->lock_connpool);
                na_connpool_switch(env);
                pthread_mutex_unlock(&env->lock_connpool);
                pthread_mutex_lock(&env->lock_current_conn);
                env->current_conn = 0;
                pthread_mutex_unlock(&env->lock_current_conn);
                env->is_refused_accept = false;
                pthread_rwlock_unlock(&env->lock_refused);
                NA_STDERR("switch backup server");
            }
        }
    } else {
        if (env->is_refused_active && na_hc_test_request(tsfd)) {
            pthread_rwlock_wrlock(&env->lock_refused);
            env->is_refused_accept = true;
            env->is_refused_active = false;
            pthread_mutex_lock(&env->lock_connpool);
            na_connpool_switch(env);
            pthread_mutex_unlock(&env->lock_connpool);
            pthread_mutex_lock(&env->lock_current_conn);
            env->current_conn = 0;
            pthread_mutex_unlock(&env->lock_current_conn);
            env->is_refused_accept = false;
            pthread_rwlock_unlock(&env->lock_refused);
            NA_STDERR("switch target server");
        } else {
            if (!env->is_refused_active && !na_hc_test_request(tsfd)) {
                pthread_rwlock_wrlock(&env->lock_refused);
                env->is_refused_accept = true;
                env->is_refused_active = true;
                pthread_mutex_lock(&env->lock_connpool);
                na_connpool_switch(env);
                pthread_mutex_unlock(&env->lock_connpool);
                pthread_mutex_lock(&env->lock_current_conn);
                env->current_conn = 0;
                pthread_mutex_unlock(&env->lock_current_conn);
                env->is_refused_accept = false;
                pthread_rwlock_unlock(&env->lock_refused);
                NA_STDERR("switch backup server");
            }
        }
    }

    close(tsfd);

    na_hc_event_set(EV_A_ w, revents);
}
