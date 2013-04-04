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

#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <ev.h>

#include "ctl.h"
#include "error.h"
#include "env.h"
#include "ext/fnv.h"

typedef enum na_ctl_cmd_t {
    NA_CTL_CMD_RELOAD = 0,
    NA_CTL_CMD_RESTART,
    NA_CTL_CMD_MAX
} na_ctl_cmd_t; 

const char *na_ctl_cmds[NA_CTL_CMD_MAX] = {
    [NA_CTL_CMD_RELOAD]  = "reload",
    [NA_CTL_CMD_RESTART] = "restart"
};

static void na_ctl_callback (EV_P_ struct ev_io *w, int revents);
static bool na_ctl_parse (na_ctl_env_t *env_ctl, char *buf, char *cmd, char *envname, size_t bufsize);
static bool na_is_valid_ctl_cmd(char *cmd);
static pid_t na_envname2pid(fnv_tbl_t *tbl, char *envname);
static bool na_ctl_cmd_execute(na_ctl_env_t *env_ctl, char *cmd, char *envname);

static bool na_ctl_parse (na_ctl_env_t *env_ctl, char *buf, char *cmd, char *envname, size_t bufsize)
{
    char *tk;
    tk = strtok(buf, " ");
    if (tk == NULL) {
        return false;
    }
    strncpy(cmd, tk, bufsize + 1);

    tk = strtok(NULL, " ");
    if (tk == NULL) {
        return false;
    }
    strncpy(envname, tk, bufsize + 1);

    // assume that the tail of envname is CRLF
    if (strlen(envname) < 2) {
        return false;
    }
    envname[strlen(tk) - 2] = '\0';
    return true;
}

static na_ctl_cmd_t na_ctl_cmd(char *cmd)
{
    for (int i=0;i<NA_CTL_CMD_MAX;++i) {
        if (strcmp(cmd, na_ctl_cmds[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static bool na_is_valid_ctl_cmd(char *cmd)
{
    if (na_ctl_cmd(cmd) == -1) {
        return false;
    }
    return true;
}

static pid_t na_envname2pid(fnv_tbl_t *tbl, char *envname)
{
    pid_t *pid;
    pid = (pid_t *)fnv_get(tbl, envname, strlen(envname));
    if (pid == NULL) {
        return -1;
    }
    return *pid;
}

static bool na_ctl_cmd_execute(na_ctl_env_t *env_ctl, char *cmd, char *envname)
{
    pid_t pid;
    int status;
    pid = na_envname2pid(env_ctl->tbl_env, envname);
    if (pid == -1) {
        return false;
    }

    switch (na_ctl_cmd(cmd)) {
    case NA_CTL_CMD_RELOAD:
        kill(pid, SIGUSR2);
        break;
    case NA_CTL_CMD_RESTART:
        kill(pid, SIGINT);
        waitpid(pid, &status, 0);
#if 0
        lock(env_ctl->lock);
        env_ctl->restart_flg = true;
        unlock(env_ctl->lock);
#endif
        break;
    default:
        return false;
        break;
    }

    return true;
}

static void na_ctl_callback (EV_P_ struct ev_io *w, int revents)
{
    na_ctl_env_t *env;
    int cfd, sfd;
    char crbuf  [BUFSIZ + 1];
    char cwbuf  [BUFSIZ + 1];
    char cmd    [BUFSIZ + 1];
    char envname[BUFSIZ + 1];
    size_t crbufsize;

    if (!(revents & EV_READ)) {
        return;
    }

    env = (na_ctl_env_t *)w->data;
    sfd = w->fd;

    if ((cfd = na_server_accept(sfd)) < 0) {
        NA_STDERR_MESSAGE(NA_ERROR_INVALID_FD);
        return;
    }

    if ((crbufsize = read(cfd, crbuf, BUFSIZ)) == -1) {
        NA_STDERR_MESSAGE(NA_ERROR_FAILED_READ);
        goto finally;
    }

    crbuf[crbufsize] = '\0';
    if (!na_ctl_parse(env, crbuf, cmd, envname, BUFSIZ)) {
        NA_STDERR_MESSAGE(NA_ERROR_INVALID_CTL_CMD);
        goto finally;
    }

    if (!na_is_valid_ctl_cmd(cmd)) {
        NA_STDERR_MESSAGE(NA_ERROR_INVALID_CTL_CMD);
        goto finally;
    }

    if (!na_ctl_cmd_execute(env, cmd, envname)) {
        NA_STDERR_MESSAGE(NA_ERROR_FAILED_EXECUTE_CTM_CMD);
        goto finally;
    }

    strncpy(cwbuf, "cmd executed\r\n", BUFSIZ + 1);

 finally:
    if (write(cfd, cwbuf, strlen(cwbuf)) == -1) {
        NA_STDERR_MESSAGE(NA_ERROR_FAILED_WRITE);
    }

    close(cfd);
}

void *na_ctl_loop (void *args)
{
    struct ev_loop *loop;
    na_ctl_env_t   *env;

    env = (na_ctl_env_t *)args;
    
    if (strlen(env->sockpath) > 0) {
        //env->fd = na_stat_server_unixsock_init(env->sockpath, env->access_mask);
        env->fd = na_front_server_tcpsock_init(50000, 32);
    } else {
        NA_DIE_WITH_ERROR(NA_ERROR_INVALID_FD);
    }

    if (env->fd < 0) {
        NA_DIE_WITH_ERROR(NA_ERROR_INVALID_FD);
    }

    env->watcher.data = env;

    loop = ev_loop_new(EVFLAG_AUTO);
    ev_io_init(&env->watcher, na_ctl_callback, env->fd, EV_READ);
    ev_io_start(EV_A_ &env->watcher);
    ev_loop(EV_A_ 0);

    return NULL;
}
