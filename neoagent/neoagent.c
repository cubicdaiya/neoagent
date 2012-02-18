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

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>

#include <json/json.h>

#include "env.h"
#include "event.h"
#include "conf.h"
#include "error.h"
#include "memproto.h"
#include "version.h"

// constants
static const int NA_ENV_MAX = 1;

// external globals
extern volatile sig_atomic_t SigExit;
extern volatile sig_atomic_t SigClear;
extern time_t StartTimestamp;

static void na_version(void);
static void na_usage(void);
static void na_signal_exit_handler (int sig);
static void na_signal_clear_handler (int sig);
static void na_setup_signals (void);

static void na_version(void)
{
    const char *v = NA_NAME " " NA_VERSION;
    const char *c = "Copyright 2011-2012 Tatsuhiko Kubo.";
    printf("%s\n"
           "%s\n", 
           v, c);
}

static void na_usage(void)
{
    const char *s = "Usage:\n\n"
        "-d go to background\n"
        "-f configuration file with JSON\n"
        "-t check configuration file with JSON\n"
        "-v show version and information";
    printf("%s\n", s);
}

static void na_signal_exit_handler (int sig)
{
    SigExit = 1;
}

static void na_signal_clear_handler (int sig)
{
    SigClear = 1;
}

static void na_setup_signals (void)
{
    struct sigaction sig_exit_handler;
    struct sigaction sig_clear_handler;

    sigemptyset(&sig_exit_handler.sa_mask);
    sigemptyset(&sig_clear_handler.sa_mask);
    sig_exit_handler.sa_handler  = na_signal_exit_handler;
    sig_clear_handler.sa_handler = na_signal_clear_handler;
    sig_exit_handler.sa_flags    = 0;
    sig_clear_handler.sa_flags   = 0;

    if (sigaction(SIGTERM, &sig_exit_handler,  NULL) == -1 ||
        sigaction(SIGINT,  &sig_exit_handler,  NULL) == -1 ||
        sigaction(SIGALRM, &sig_exit_handler,  NULL) == -1 ||
        sigaction(SIGHUP,  &sig_exit_handler,  NULL) == -1 ||
        sigaction(SIGUSR1, &sig_clear_handler, NULL) == -1)
    {
        NA_DIE_WITH_ERROR(NA_ERROR_FAILED_SETUP_SIGNAL);
    }

    if (sigignore(SIGPIPE) == -1) {
        NA_DIE_WITH_ERROR(NA_ERROR_FAILED_IGNORE_SIGNAL);
    }

    SigExit  = 0;
    SigClear = 0;

}

int main (int argc, char *argv[])
{
    na_env_t           *env[NA_ENV_MAX];
    mpool_t            *env_pool;
    int                 c;
    int                 env_cnt          = 0;
    bool                is_daemon        = false;
    struct json_object *conf_obj         = NULL;
    struct json_object *environments_obj = NULL;

    while (-1 != (c = getopt(argc, argv,
           "f:" /* configuration file with JSON */
           "t:" /* check configuration file */
           "d"  /* go to background */
           "v"  /* show version and information */
           "h"  /* show help */
    )))
    {
        switch (c) {
        case 'd':
            is_daemon = true;
            break;
        case 'f':
            conf_obj         = na_get_conf(optarg);
            environments_obj = na_get_environments(conf_obj, &env_cnt);
            break;
        case 't':
            conf_obj         = na_get_conf(optarg);
            environments_obj = na_get_environments(conf_obj, &env_cnt);
            printf("JSON configuration is OK\n");
            return 0;
            break;
        case 'v':
            na_version();
            return 0;
            break;
        case 'h':
            na_usage();
            return 0;
            break;
        default:
            break;
        }
    }

    if (is_daemon && daemon(0, 0) == -1) {
        NA_DIE_WITH_ERROR(NA_ERROR_FAILED_DAEMONIZE);
    }

    if (env_cnt > NA_ENV_MAX) {
        NA_DIE_WITH_ERROR(NA_ERROR_TOO_MANY_ENVIRONMENTS);
    }

    StartTimestamp = time(NULL);
    na_setup_signals();
    na_memproto_bm_skip_init();

    env_pool = mpool_create(0);
    if (env_cnt == 0) {
        env_cnt = 1;
        env[0]  = na_env_add(&env_pool);
        na_env_setup_default(env[0], 0);
    } else {
        for (int i=0;i<env_cnt;++i) {
            env[i] = na_env_add(&env_pool);
            na_env_setup_default(env[i], i);
            na_conf_env_init(environments_obj, env[i], i);
        }
    }

    json_object_put(conf_obj);

    for (int i=0;i<env_cnt;++i) {
        env[i]->current_conn      = 0;
        env[i]->is_refused_active = false;
        env[i]->error_count       = 0;
        env[i]->current_conn_max  = 0;
        pthread_mutex_init(&env[i]->lock_connpool, NULL);
        na_connpool_create(&env[i]->connpool_active, env[i]->connpool_max);
        na_connpool_create(&env[i]->connpool_backup, env[i]->connpool_max);
    }

    na_event_loop(env[0]);

    for (int i=0;i<env_cnt;++i) {
        na_connpool_destroy(&env[i]->connpool_active);
        na_connpool_destroy(&env[i]->connpool_backup);
        pthread_mutex_destroy(&env[i]->lock_connpool);
    }

    mpool_destroy(env_pool);

    return 0;
}
