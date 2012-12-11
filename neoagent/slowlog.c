/**
   In short, neoagent is distributed under so called "BSD license",

   Copyright (c) 2012 Tatsuhiko Kubo <cubicdaiya@gmail.com>
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

#include "slowlog.h"

void na_slow_query_gettime(na_env_t *env, struct timespec *time)
{
    if ((env->slow_query_sec.tv_sec != 0) ||
        (env->slow_query_sec.tv_nsec != 0))
        clock_gettime(CLOCK_MONOTONIC, time);
}

void na_slow_query_check(na_client_t *client)
{
    na_env_t *env = client->env;
    struct timespec na_to_ts_time, na_from_ts_time, na_to_client_time,
      total_query_time;

    na_difftime(&na_to_ts_time, &client->na_to_ts_time_begin,
                &client->na_to_ts_time_end);
    na_difftime(&na_from_ts_time, &client->na_from_ts_time_begin,
                &client->na_from_ts_time_end);
    na_difftime(&na_to_client_time, &client->na_to_client_time_begin,
                &client->na_to_client_time_end);
    na_addtime(&total_query_time, &na_to_ts_time, &na_from_ts_time);
    na_addtime(&total_query_time, &total_query_time, &na_to_client_time);

    if ((env->slow_query_sec.tv_sec < total_query_time.tv_sec) ||
        ((env->slow_query_sec.tv_sec == total_query_time.tv_sec) &&
         (env->slow_query_sec.tv_nsec < total_query_time.tv_nsec))) {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        const size_t bufsz = 256;
        char buf[bufsz];

        if (getpeername(client->cfd, &caddr, &clen) == 0) {
            client->crbuf[client->crbufsize - 2] = '\0'; // don't want newline
            snprintf(buf, bufsz,
                     "SLOWQUERY: client %s:%hu na->ts %g na<-ts %g na->c %g querytxt \"%s\"",
                     inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port),
                     (double)((double)na_to_ts_time.tv_sec     + (double)na_to_ts_time.tv_nsec / 1000000000L),
                     (double)((double)na_from_ts_time.tv_sec   + (double)na_from_ts_time.tv_nsec / 1000000000L),
                     (double)((double)na_to_client_time.tv_sec + (double)na_to_client_time.tv_nsec / 1000000000L),
                     client->crbuf);
            NA_STDERR(buf);
        }
    }

    memset(&client->na_from_ts_time_begin, 0, sizeof(struct timespec));
    memset(&client->na_from_ts_time_end, 0, sizeof(struct timespec));
    memset(&client->na_to_ts_time_begin, 0, sizeof(struct timespec));
    memset(&client->na_to_ts_time_end, 0, sizeof(struct timespec));
    memset(&client->na_to_client_time_begin, 0, sizeof(struct timespec));
    memset(&client->na_to_client_time_end, 0, sizeof(struct timespec));
}
