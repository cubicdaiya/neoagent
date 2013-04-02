/**
   In short, neoagent is distributed under so called "BSD license",

   Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
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
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>

#include <json/json.h>

#include "ext/fnv.h"
#include "env.h"
#include "event.h"
#include "conf.h"
#include "error.h"
#include "memproto.h"
#include "util.h"
#include "version.h"

typedef struct na_slave_process_t {
    pid_t     pid;
    na_env_t *env;
} na_slave_process_t;

// constants
static const int NA_ENV_MAX       = 20;
static const int NA_PID_MAX       = 32768;
static const int NA_PROC_NAME_MAX = 512;

// external globals
extern volatile sig_atomic_t SigExit;
extern volatile sig_atomic_t SigClear;
extern time_t StartTimestamp;

// globals
volatile sig_atomic_t SigReconf;
pthread_rwlock_t LockReconf;
const char *ConfFile;
pid_t MasterPid;

static void na_version(void);
static void na_usage(void);
static void na_signal_exit_handler (int sig);
static void na_signal_clear_handler (int sig);
static void na_signal_reconf_handler (int sig);
static void na_setup_signals (void);
static void na_set_env_proc_name (char *orig_proc_name, const char *env_proc_name);
static void na_kill_childs(pid_t *pids, int sig);
static bool is_master_process(void);

static void na_version(void)
{
    const char *v = NA_NAME " " NA_VERSION;
    const char *c = "Copyright 2013 Tatsuhiko Kubo.";
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

static void na_signal_reconf_handler (int sig)
{
    SigReconf = 1;
}

static void na_setup_signals (void)
{
    struct sigaction sig_exit_handler;
    struct sigaction sig_clear_handler;
    struct sigaction sig_reconf_handler;

    sigemptyset(&sig_exit_handler.sa_mask);
    sigemptyset(&sig_clear_handler.sa_mask);
    sigemptyset(&sig_reconf_handler.sa_mask);
    sig_exit_handler.sa_handler   = na_signal_exit_handler;
    sig_clear_handler.sa_handler  = na_signal_clear_handler;
    sig_reconf_handler.sa_handler = na_signal_reconf_handler;
    sig_exit_handler.sa_flags     = 0;
    sig_clear_handler.sa_flags    = 0;
    sig_reconf_handler.sa_flags   = 0;

    if (sigaction(SIGTERM, &sig_exit_handler,   NULL) == -1 ||
        sigaction(SIGINT,  &sig_exit_handler,   NULL) == -1 ||
        sigaction(SIGALRM, &sig_exit_handler,   NULL) == -1 ||
        sigaction(SIGHUP,  &sig_exit_handler,   NULL) == -1 ||
        sigaction(SIGUSR1, &sig_clear_handler,  NULL) == -1 ||
        sigaction(SIGUSR2, &sig_reconf_handler, NULL) == -1)
        {
            NA_DIE_WITH_ERROR(NA_ERROR_FAILED_SETUP_SIGNAL);
        }

    if (sigignore(SIGPIPE) == -1) {
        NA_DIE_WITH_ERROR(NA_ERROR_FAILED_IGNORE_SIGNAL);
    }

    SigExit   = 0;
    SigClear  = 0;
    SigReconf = 0;

}

static void na_set_env_proc_name (char *orig_proc_name, const char *env_proc_name)
{
    strncpy(orig_proc_name + strlen(orig_proc_name), ": ",          NA_PROC_NAME_MAX);
    strncpy(orig_proc_name + strlen(orig_proc_name), env_proc_name, NA_PROC_NAME_MAX);
}

static void na_kill_childs(pid_t *pids, int sig)
{
    size_t c = sizeof(pids) / sizeof(pid_t);
    for (int i=0;i<c;++i) {
        kill(pids[i], sig);
    }
}

static bool is_master_process(void)
{
    if (getpid() == MasterPid) {
        return true;
    }
    return false;
}

int main (int argc, char *argv[])
{
    na_env_t            env;              // for child
    na_env_t            envs[NA_ENV_MAX]; // for master
    int                 c;
    pthread_t           th;
    int                 env_cnt          = 0;
    bool                is_daemon        = false;
    struct json_object *conf_obj         = NULL;
    struct json_object *environments_obj = NULL;
    pid_t pids[NA_PID_MAX];
    int pidx;
    fnv_ent_t ent_env[NA_PID_MAX];
    fnv_tbl_t *tbl_env;

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
            ConfFile         = optarg;
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

    tbl_env   = fnv_tbl_create(ent_env, NA_PID_MAX);
    MasterPid = getpid();
    pidx      = 0;
    for (int i=0;i<env_cnt;++i) {
        pidx++;
        pid_t pid = fork();
        if (pid == -1) {
            NA_DIE_WITH_ERROR(NA_ERROR_FAILED_CREATE_PROCESS);
        } else if (pid == 0) { // child
            break;
        } else {
            pids[i] = pid;
        }
    }

    if (is_master_process()) {
        na_set_env_proc_name(argv[0], "master");
        for (int i=0;i<env_cnt;++i) {
            na_env_setup_default(&envs[i], i);
            na_conf_env_init(environments_obj, &envs[i], i, false);
            fnv_put(tbl_env, envs[i].name, &pids[i], strlen(envs[i].name), sizeof(pid_t));
        }
        goto MASTER_CYCLE;
    }

    na_memproto_bm_skip_init();

    memset(&env, 0, sizeof(env));
    if (env_cnt == 0) {
        na_env_setup_default(&env, 0);
    } else {
        na_env_setup_default(&env, pidx);
        na_conf_env_init(environments_obj, &env, pidx - 1, false);
    }

    na_set_env_proc_name(argv[0], env.name);
    na_env_init(&env);
    pthread_create(&th, NULL, na_event_loop, &env);

 MASTER_CYCLE:

    json_object_put(conf_obj);

    // monitoring signal
    while (true) {
        if (SigExit == 1) {
            if (is_master_process()) {
                na_kill_childs(pids, SIGINT);
            }
            break;
        }

        if (SigReconf == 1) {
            conf_obj         = na_get_conf(ConfFile);
            environments_obj = na_get_environments(conf_obj, &env_cnt);

            pthread_rwlock_wrlock(&LockReconf);
            na_conf_env_init(environments_obj, &env, pidx - 1, true);
            pthread_rwlock_unlock(&LockReconf);

            json_object_put(conf_obj);
            SigReconf = 0;
        }

        // XXX we should sleep forever and only wake upon a signal
        sleep(1);
    }

    return 0;
}
