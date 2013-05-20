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
static const int NA_ENV_MAX = 20;
static const int NA_PID_MAX = 32768;

// external globals
extern na_graceful_phase_t GracefulPhase; // for worker prcess
extern time_t StartTimestamp;
extern pid_t MasterPid;

// globals
pthread_rwlock_t LockReconf;

static void na_version(void);
static void na_usage(void);
static void na_setup_signals (void);

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

static void na_setup_signals (void)
{
    if (sigignore(SIGPIPE) == -1) {
        NA_DIE_WITH_ERROR(NULL, NA_ERROR_FAILED_IGNORE_SIGNAL);
    }
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
    sigset_t ss;
    int sig;


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

    if (na_is_master_process()) {
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
        na_ctl_env_setup_default(&env_ctl);
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

    sigemptyset(&ss);
    sigaddset(&ss, SIGTERM);
    sigaddset(&ss, SIGINT);
    sigaddset(&ss, SIGALRM);
    if (na_is_master_process()) {
        sigaddset(&ss, SIGCONT);
        sigaddset(&ss, SIGWINCH);
        sigaddset(&ss, SIGUSR1);
    } else {
        sigaddset(&ss, SIGHUP);
        sigaddset(&ss, SIGUSR2);
    }
    sigprocmask(SIG_BLOCK, &ss, NULL);

    // monitoring signal
    while (true) {

        if (!na_is_master_process()) {
            if (sigwait(&ss, &sig) != 0) {
                continue;
            }
            switch (sig) {
            case SIGTERM:
            case SIGINT:
            case SIGALRM:
                goto exit;
            case SIGHUP:
                if (GracefulPhase == NA_GRACEFUL_PHASE_DISABLED) {
                    sleep(5);
                    GracefulPhase = NA_GRACEFUL_PHASE_ENABLED;
                }
                // wait until available connection becomes zero
                while (true) {
                    pthread_mutex_lock(&env.lock_current_conn);
                    if (env.current_conn == 0) {
                        pthread_mutex_unlock(&env.lock_current_conn);
                        goto exit;
                    }
                    pthread_mutex_unlock(&env.lock_current_conn);
                    sleep(1);
                }
                break;
            case SIGUSR2:
                conf_obj         = na_get_conf(conf_file);
                environments_obj = na_get_environments(conf_obj, &env_cnt);

                pthread_rwlock_wrlock(&LockReconf);
                na_conf_env_init(environments_obj, &env, pidx - 1, true);
                pthread_rwlock_unlock(&LockReconf);

                json_object_put(conf_obj);
                break;
            default:
                break;
            }
            continue;
        }

        // for master process
        if (sigwait(&ss, &sig) != 0) {
            continue;
        }

        switch (sig) {
        case SIGTERM:
        case SIGINT:
        case SIGALRM:
            na_process_shutdown(pids, env_cnt);
            goto exit;
        case SIGCONT:
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
            break;
        case SIGWINCH:
            if (env_cnt >= NA_ENV_MAX) {
                continue;
            }
            pid_t pid = fork();
            int env_cnt_prev;
            int status;
            env_cnt_prev = env_cnt;
            conf_obj         = na_get_conf(conf_file);
            environments_obj = na_get_environments(conf_obj, &env_cnt);
            if (env_cnt_prev != env_cnt - 1) {
                if (pid == 0) {
                    NA_CTL_DIE_WITH_ERROR(&env_ctl, NA_ERROR_UNKNOWN);
                } else {
                    waitpid(pid, &status, 0);
                }
            } else {
                if (pid == -1) {
                    NA_CTL_DIE_WITH_ERROR(&env_ctl, NA_ERROR_FAILED_CREATE_PROCESS);
                } else if (pid == 0) { // child
                    na_memproto_bm_skip_init();
                    memset(&env, 0, sizeof(env));
                    na_env_setup_default(&env, env_cnt - 1);
                    na_conf_env_init(environments_obj, &env, env_cnt - 1, false);
                    argv[0][strlen(argv[0]) - (sizeof(": master") - 1)] = '\0';
                    na_set_env_proc_name(argv[0], env.name);
                    na_env_init(&env);
                    pthread_create(&event_th, NULL, na_event_loop, &env);
                    json_object_put(conf_obj);
                } else { // master
                    pids[env_cnt - 1] = pid;
                }
            }
            break;
        case SIGUSR1:
            na_on_the_fly_update(&env_ctl, conf_file, pids, env_cnt);
            goto exit;
        default:
            assert(false);
            break;
        }

    }

 exit:

    return 0;
}
