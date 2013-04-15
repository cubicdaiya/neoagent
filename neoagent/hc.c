/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "defines.h"

// constants
static const char *na_hc_test_key = "neoagent_test_key";
static const char *na_hc_test_val = "neoagent_test_val";

// refs to external globals
extern pthread_rwlock_t LockReconf;

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

    pthread_rwlock_rdlock(&LockReconf);

    tsfd = na_target_server_tcpsock_init();

    if (tsfd <= 0) {
        na_hc_event_set(EV_A_ w, revents);
        NA_STDERR_MESSAGE(NA_ERROR_INVALID_FD);
        goto unlock_reconf;
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
unlock_reconf:
    pthread_rwlock_unlock(&LockReconf);
}
