/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include "defines.h"

// constants
static const int NA_PROC_NAME_MAX = 64;

// refs to external globals
pid_t MasterPid;

void na_kill_childs(pid_t *pids, size_t c, int sig)
{
    for (int i=0;i<c;++i) {
        kill(pids[i], sig);
    }
}

void na_set_env_proc_name (char *orig_proc_name, const char *env_proc_name)
{
    strncpy(orig_proc_name + strlen(orig_proc_name), ": ",          NA_PROC_NAME_MAX + 1);
    strncpy(orig_proc_name + strlen(orig_proc_name), env_proc_name, NA_PROC_NAME_MAX + 1);
}

bool na_is_master_process(void)
{
    if (getpid() == MasterPid) {
        return true;
    }
    return false;
}

void na_process_shutdown(pid_t *pids, int env_cnt)
{
    na_kill_childs(pids, env_cnt, SIGINT);
}

void na_on_the_fly_update(na_ctl_env_t *env_ctl, char *conf_file, pid_t *pids, int env_cnt)
{
    pid_t pid = fork();
    if (pid == -1) {
        NA_CTL_DIE_WITH_ERROR(env_ctl, NA_ERROR_FAILED_CREATE_PROCESS);
    } else if (pid == 0) { // child
        execl(env_ctl->binpath, env_ctl->binpath, "-f", conf_file, NULL);
    }
    na_kill_childs(pids, env_cnt, SIGUSR2);
}
