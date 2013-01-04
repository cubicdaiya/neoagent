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

#include "time.h"

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
