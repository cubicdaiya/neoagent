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

// version number
#define NEOAGENT_VERSION "0.1.0"

// constants
static const int NEOAGENT_ENV_MAX = 10;

// external globals
extern volatile sig_atomic_t SigCatched;

static void neoagent_version(void);
static void neoagent_usage(void);
static void neoagent_signal_handler (int sig);
static void neoagent_setup_signals (void);

static void neoagent_version(void)
{
    const char *s = "neoagent " NEOAGENT_VERSION "\n";
    printf("%s", s);
}

static void neoagent_usage(void)
{
    const char *s = "Usage:\n\n"
        "-d go to background\n"
        "-f configuration file with JSON\n"
        "-t check configuration file with JSON\n"
        "-v show version and information";
    printf("%s\n", s);
}

static void neoagent_signal_handler (int sig)
{
    SigCatched = 1;
}

static void neoagent_setup_signals (void)
{
    struct sigaction sig_handler;

    sig_handler.sa_handler = neoagent_signal_handler;
    sig_handler.sa_flags   = 0;

    if (sigaction(SIGTERM, &sig_handler, NULL) == -1 ||
        sigaction(SIGINT,  &sig_handler, NULL) == -1 ||
        sigaction(SIGALRM, &sig_handler, NULL) == -1 ||
        sigaction(SIGHUP,  &sig_handler, NULL) == -1)
    {
        neoagent_die_with_error(NEOAGENT_ERROR_FAILED_SETUP_SIGNAL);
    }

    if (sigignore(SIGPIPE) == -1) {
        neoagent_die_with_error(NEOAGENT_ERROR_FAILED_IGNORE_SIGNAL);
    }

    SigCatched = 0;

}

int main (int argc, char *argv[])
{
    pthread_t           th[NEOAGENT_ENV_MAX];
    neoagent_env_t     *env[NEOAGENT_ENV_MAX];
    mpool_t            *env_pool;
    int                 c;
    int                 env_cnt          = 0;
    bool                is_daemon        = false;
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
            environments_obj = neoagent_cnf_get_environments(optarg, &env_cnt);
            break;
        case 't':
            environments_obj = neoagent_cnf_get_environments(optarg, &env_cnt);
            printf("JSON configuration is OK\n");
            return 0;
            break;
        case 'v':
            neoagent_version();
            return 0;
            break;
        case 'h':
            neoagent_usage();
            return 0;
            break;
        default:
            break;
        }
    }

    if (is_daemon && daemon(0, 0) == -1) {
        neoagent_die("daemonize failed\n");
    }

    if (env_cnt > NEOAGENT_ENV_MAX) {
        neoagent_die_with_error(NEOAGENT_ERROR_TOO_MANY_ENVIRONMENTS);
    }

    neoagent_setup_signals();
    neoagent_memproto_bm_skip_init();

    env_pool = mpool_create(0);
    if (env_cnt == 0) {
        env_cnt = 1;
        env[0]  = neoagent_env_add(&env_pool);
        neoagent_env_setup_default(env[0], 0);
    } else {
        for (int i=0;i<env_cnt;++i) {
            env[i] = neoagent_env_add(&env_pool);
            neoagent_env_setup_default(env[i], i);
            neoagent_conf_env_init(environments_obj, env[i], i);
        }
    }

    for (int i=0;i<env_cnt;++i) {
        env[i]->current_conn      = 0;
        env[i]->is_refused_active = false;
        env[i]->error_count       = 0;
        neoagent_connpool_create(&env[i]->connpool_active, env[i]->connpool_max);
    }

    for (int i=0;i<env_cnt;++i) {
        pthread_create(&th[i], NULL, neoagent_event_loop, env[i]);
    }

    // monitoring signal
    while (true) {

        if (SigCatched == 1) {
            break;
        }

        sleep(1);
    }

    mpool_destroy(env_pool);

    return 0;
}
