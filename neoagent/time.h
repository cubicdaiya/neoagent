/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#ifndef NA_TIME_H
#define NA_TIME_H

#include <time.h>

#define NA_DATETIME_BUF_MAX 32

void na_ts2dt(time_t time, char *format, char *buf, size_t bufsize);
void na_elapsed_time(time_t time, char *buf, size_t bufsize);
void na_difftime(struct timespec *ret, struct timespec *start, struct timespec *end);
void na_addtime(struct timespec *ret, struct timespec *t1, struct timespec *t2);

#endif // NA_TIME_H
