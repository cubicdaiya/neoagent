/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#ifndef NA_SLOWLOG_H
#define NA_SLOWLOG_H

#include "env.h"
#include "error.h"

void na_slow_query_gettime(na_env_t *env, struct timespec *time);
void na_slow_query_check(na_client_t *client);
void na_slow_query_open(na_env_t *env);

#endif // NA_SLOWLOG_H
