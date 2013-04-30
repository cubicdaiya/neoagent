/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <stdio.h>

#include "defines.h"

void na_log_open(na_env_t *env)
{
    // close and reopen log file, to permit rotation
    if (env->log_fp) {
        fclose(env->log_fp);
    }
    env->log_fp = fopen(env->logpath, "a");

    if (!env->log_fp) {
        NA_ERROR_OUTPUT_MESSAGE(env, NA_ERROR_CANT_OPEN_LOG);
    } else {
        chmod(env->logpath, env->log_access_mask);
    }
}

void na_ctl_log_open(na_ctl_env_t *env)
{
    // close and reopen log file, to permit rotation
    if (env->log_fp) {
        fclose(env->log_fp);
    }
    env->log_fp = fopen(env->logpath, "a");

    if (!env->log_fp) {
        NA_ERROR_OUTPUT_MESSAGE(NULL, NA_ERROR_CANT_OPEN_LOG);
    } else {
        chmod(env->logpath, env->log_access_mask);
    }
}
