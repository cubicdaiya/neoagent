/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <ev.h>

#include "defines.h"
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
        pthread_mutex_lock(&env_ctl->lock_restart);
        env_ctl->restart_envname = envname;
        pthread_mutex_unlock(&env_ctl->lock_restart);
        kill(pid, SIGHUP);
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
        NA_CTL_ERROR_OUTPUT_MESSAGE(env, NA_ERROR_INVALID_FD);
        return;
    }

    if ((crbufsize = read(cfd, crbuf, BUFSIZ)) == -1) {
        NA_CTL_ERROR_OUTPUT_MESSAGE(env, NA_ERROR_FAILED_READ);
        goto finally;
    }

    crbuf[crbufsize] = '\0';
    if (!na_ctl_parse(env, crbuf, cmd, envname, BUFSIZ)) {
        NA_CTL_ERROR_OUTPUT_MESSAGE(env, NA_ERROR_INVALID_CTL_CMD);
        goto finally;
    }

    if (!na_is_valid_ctl_cmd(cmd)) {
        NA_CTL_ERROR_OUTPUT_MESSAGE(env, NA_ERROR_INVALID_CTL_CMD);
        goto finally;
    }

    if (!na_ctl_cmd_execute(env, cmd, envname)) {
        NA_CTL_ERROR_OUTPUT_MESSAGE(env, NA_ERROR_FAILED_EXECUTE_CTM_CMD);
        goto finally;
    }

    snprintf(cwbuf, BUFSIZ + 1, "kicked:%s %s", cmd, envname);

 finally:
    if (write(cfd, cwbuf, strlen(cwbuf)) == -1) {
        NA_CTL_ERROR_OUTPUT_MESSAGE(env, NA_ERROR_FAILED_WRITE);
    }

    close(cfd);
}

void *na_ctl_loop (void *args)
{
    struct ev_loop *loop;
    na_ctl_env_t   *env;

    env = (na_ctl_env_t *)args;
    
    if (strlen(env->sockpath) > 0) {
        env->fd = na_stat_server_unixsock_init(env->sockpath, env->access_mask);
    } else {
        NA_CTL_DIE_WITH_ERROR(env, NA_ERROR_INVALID_FD);
    }

    if (env->fd < 0) {
        NA_CTL_DIE_WITH_ERROR(env, NA_ERROR_INVALID_FD);
    }

    env->watcher.data = env;

    loop = ev_loop_new(EVFLAG_AUTO);
    ev_io_init(&env->watcher, na_ctl_callback, env->fd, EV_READ);
    ev_io_start(EV_A_ &env->watcher);
    ev_loop(EV_A_ 0);

    return NULL;
}
