/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <stdio.h>

#include "defines.h"

void na_ts2dt(time_t time, char *format, char *buf, size_t bufsize)
{
    size_t len;
    struct tm tmp;
    len = strftime(buf, bufsize, format, localtime_r(&time, &tmp));
    if (len == 0) {
        buf[0] = '\0';
    }
}

void na_elapsed_time(time_t time, char *buf, size_t bufsize)
{
    int hour, min, sec;
    hour = time / 3600;
    min  = (time - hour * 3600) / 60;
    sec  = time % 60;
    snprintf(buf, bufsize, "%02d:%02d:%02d", hour, min, sec);
}

void na_difftime(struct timespec *ret, struct timespec *start, struct timespec *end)
{
    if ((end->tv_nsec - start->tv_nsec) < 0) {
        ret->tv_sec = end->tv_sec - start->tv_sec - 1;
        ret->tv_nsec = 1000000000L + end->tv_nsec - start->tv_nsec;
    } else {
        ret->tv_sec = end->tv_sec - start->tv_sec;
        ret->tv_nsec = end->tv_nsec - start->tv_nsec;
    }
}

void na_addtime(struct timespec *ret, struct timespec *t1, struct timespec *t2)
{
    ret->tv_sec = t1->tv_sec + t2->tv_sec;
    ret->tv_nsec = t1->tv_nsec + t2->tv_nsec;

    if (ret->tv_nsec >= 1000000000L) {
        ret->tv_sec++;
        ret->tv_nsec = ret->tv_nsec - 1000000000L;
    }
}
