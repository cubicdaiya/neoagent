/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

// In Mac OS X 10.5 and later, 'daemon' is deprecated.
// This hack is for eliminating the warning message about it.
#if __APPLE__
#define daemon na_fake_daemon
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#include "defines.h"
#include "version.h"

#if __APPLE__
#undef daemon
extern int daemon(int, int);
#endif

// constants
static const int NA_ENV_MAX       = 20;
static const int NA_PID_MAX       = 32768;
static const int NA_PROC_NAME_MAX = 64;

// external globals
extern volatile sig_atomic_t SigExit;
extern volatile sig_atomic_t SigGraceful; // for worker prcess
extern time_t StartTimestamp;

// globals
volatile sig_atomic_t SigReconf;
pthread_rwlock_t LockReconf;
pid_t MasterPid;

static void na_version(void);
static void na_usage(void);
static void na_signal_exit_handler (int sig);
static void na_signal_reconf_handler (int sig);
static void na_signal_graceful_handler (int sig);
static void na_setup_signals (void);
static void na_set_env_proc_name (char *orig_proc_name, const char *env_proc_name);
static void na_kill_childs(pid_t *pids, size_t c, int sig);
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

static void na_signal_reconf_handler (int sig)
{
    SigReconf = 1;
}

static void na_signal_graceful_handler (int sig)
{
    sleep(5);
    SigGraceful = NA_GRACEFUL_PHASE_ENABLED;
}

static void na_setup_signals (void)
{
    struct sigaction sig_exit_handler;
    struct sigaction sig_reconf_handler;
    struct sigaction sig_graceful_handler;

    sigemptyset(&sig_exit_handler.sa_mask);
    sigemptyset(&sig_reconf_handler.sa_mask);
    sigemptyset(&sig_graceful_handler.sa_mask);

    sig_exit_handler.sa_handler     = na_signal_exit_handler;
    sig_reconf_handler.sa_handler   = na_signal_reconf_handler;
    sig_graceful_handler.sa_handler = na_signal_graceful_handler;

    sig_exit_handler.sa_flags     = 0;
    sig_reconf_handler.sa_flags   = 0;
    sig_graceful_handler.sa_flags = 0;

    if (sigaction(SIGTERM, &sig_exit_handler,     NULL) == -1  ||
        sigaction(SIGINT,  &sig_exit_handler,     NULL) == -1  ||
        sigaction(SIGALRM, &sig_exit_handler,     NULL) == -1  ||
        sigaction(SIGHUP,  &sig_graceful_handler, NULL) == -1 ||
        sigaction(SIGUSR2, &sig_reconf_handler,   NULL) == -1)
        {
            NA_DIE_WITH_ERROR(NULL, NA_ERROR_FAILED_SETUP_SIGNAL);
        }

    if (sigignore(SIGPIPE) == -1) {
        NA_DIE_WITH_ERROR(NULL, NA_ERROR_FAILED_IGNORE_SIGNAL);
    }

    SigExit     = 0;
    SigReconf   = 0;
    SigGraceful = NA_GRACEFUL_PHASE_DISABLED;
}

static void na_set_env_proc_name (char *orig_proc_name, const char *env_proc_name)
{
    strncpy(orig_proc_name + strlen(orig_proc_name), ": ",          NA_PROC_NAME_MAX + 1);
    strncpy(orig_proc_name + strlen(orig_proc_name), env_proc_name, NA_PROC_NAME_MAX + 1);
}

static void na_kill_childs(pid_t *pids, size_t c, int sig)
{
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
    int                 c;
    na_env_t            env;              // for child
    na_env_t            envs[NA_ENV_MAX]; // for master
    na_ctl_env_t        env_ctl;
    pthread_t           event_th, ctl_th;
    int                 env_cnt          = 0;
    bool                is_daemon        = false;
    struct json_object *conf_obj         = NULL;
    struct json_object *environments_obj = NULL;
    struct json_object *ctl_obj          = NULL;
    pid_t pids[NA_PID_MAX];
    int pidx;
    fnv_ent_t ent_env[NA_PID_MAX];
    fnv_tbl_t *tbl_env;
    char conf_file[NA_PATH_MAX + 1];

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
            strncpy(conf_file, optarg, NA_PATH_MAX + 1);
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
        NA_DIE_WITH_ERROR(NULL, NA_ERROR_FAILED_DAEMONIZE);
    }

    if (env_cnt > NA_ENV_MAX) {
        NA_DIE_WITH_ERROR(NULL, NA_ERROR_TOO_MANY_ENVIRONMENTS);
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
            NA_DIE_WITH_ERROR(NULL, NA_ERROR_FAILED_CREATE_PROCESS);
        } else if (pid == 0) { // child
            break;
        } else {
            pids[i] = pid;
        }
    }

    if (is_master_process()) {
        if (conf_obj == NULL) {
            NA_DIE_WITH_ERROR(NULL, NA_ERROR_INVALID_CONFPATH);
        }
        na_set_env_proc_name(argv[0], "master");
        for (int i=0;i<env_cnt;++i) {
            na_env_setup_default(&envs[i], i);
            na_conf_env_init(environments_obj, &envs[i], i, false);
            fnv_put(tbl_env, envs[i].name, &pids[i], strlen(envs[i].name), sizeof(pid_t));
        }
        memset(&env_ctl, 0, sizeof(na_ctl_env_t));
        ctl_obj = na_get_ctl(conf_obj);
        na_conf_ctl_init(ctl_obj, &env_ctl);
        env_ctl.tbl_env = tbl_env;
        env_ctl.restart_envname = NULL;
        pthread_mutex_init(&env_ctl.lock_restart, NULL);
        pthread_create(&ctl_th, NULL, na_ctl_loop, &env_ctl);
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
    pthread_create(&event_th, NULL, na_event_loop, &env);

 MASTER_CYCLE:

    json_object_put(conf_obj);

    // monitoring signal
    while (true) {

        if (SigExit == 1) {
            if (is_master_process()) {
                na_kill_childs(pids, env_cnt, SIGINT);
            }
            break;
        }

        if (is_master_process()) {

            pthread_mutex_lock(&env_ctl.lock_restart);
            if (env_ctl.restart_envname != NULL) {
                pid_t pid = fork();
                void *th_ret;
                int ridx;
                conf_obj         = na_get_conf(conf_file);
                environments_obj = na_get_environments(conf_obj, &env_cnt);
                ridx             = na_conf_get_environment_idx(environments_obj, env_ctl.restart_envname);
                if (pid == -1) {
                    pthread_mutex_unlock(&env_ctl.lock_restart);
                    NA_CTL_DIE_WITH_ERROR(&env_ctl, NA_ERROR_FAILED_CREATE_PROCESS);
                } else if (pid == 0) { // child
                    pthread_cancel(ctl_th);
                    pthread_join(ctl_th, &th_ret);

                    na_memproto_bm_skip_init();

                    memset(&env, 0, sizeof(env));
                    na_env_setup_default(&env, ridx);
                    na_conf_env_init(environments_obj, &env, ridx, false);
                    argv[0][strlen(argv[0]) - (sizeof(": master") - 1)] = '\0';
                    na_set_env_proc_name(argv[0], env.name);
                    na_env_init(&env);
                    pthread_create(&event_th, NULL, na_event_loop, &env);
                    json_object_put(conf_obj);
                } else { // master
                    int status;
                    waitpid(pids[ridx], &status, 0);
                    pids[ridx] = pid;
                }
                env_ctl.restart_envname = NULL;
            }
            pthread_mutex_unlock(&env_ctl.lock_restart);
        }

        if (!is_master_process() && SigReconf == 1) {
            conf_obj         = na_get_conf(conf_file);
            environments_obj = na_get_environments(conf_obj, &env_cnt);

            pthread_rwlock_wrlock(&LockReconf);
            na_conf_env_init(environments_obj, &env, pidx - 1, true);
            pthread_rwlock_unlock(&LockReconf);

            json_object_put(conf_obj);
            SigReconf = 0;
        }

        if (!is_master_process() && SigGraceful >= NA_GRACEFUL_PHASE_ENABLED) {
            pthread_mutex_lock(&env.lock_current_conn);
            if (env.current_conn == 0) {
                pthread_mutex_unlock(&env.lock_current_conn);
                return 0;
            }
            pthread_mutex_unlock(&env.lock_current_conn);
        }

        // XXX we should sleep forever and only wake upon a signal
        sleep(1);
    }

    return 0;
}
