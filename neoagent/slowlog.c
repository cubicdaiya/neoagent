/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <string.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "defines.h"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 0
static int clock_gettime(int dummy, struct timespec *ts); 
static int clock_gettime(int dummy, struct timespec *ts)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts->tv_sec  = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
    return 0;
} 
#endif

void na_slow_query_gettime(na_env_t *env, struct timespec *time)
{
    if ((env->slow_query_sec.tv_sec != 0) ||
        (env->slow_query_sec.tv_nsec != 0))
    {
        clock_gettime(CLOCK_MONOTONIC, time);
    }
}

void na_slow_query_check(na_client_t *client)
{
    na_env_t *env = client->env;
    struct timespec na_to_ts_time, na_from_ts_time, na_to_client_time, total_query_time;

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
        memset(&caddr, 0, sizeof(caddr));
        socklen_t clen = sizeof(caddr);

        if (getpeername(client->cfd, &caddr, &clen) == 0) {
            char host[HOST_NAME_MAX];
            if (gethostname(host, HOST_NAME_MAX) < 0) {
                host[HOST_NAME_MAX - 1] = '\0';
            }
            time_t now = time(NULL);
            char *clientaddr = inet_ntoa(caddr.sin_addr);
            uint16_t clientport = ntohs(caddr.sin_port);
            double na_to_ts     = (double)((double)na_to_ts_time.tv_sec     + (double)na_to_ts_time.tv_nsec / 1000000000L),
                   na_from_ts   = (double)((double)na_from_ts_time.tv_sec   + (double)na_from_ts_time.tv_nsec / 1000000000L),
                   na_to_client = (double)((double)na_to_client_time.tv_sec + (double)na_to_client_time.tv_nsec / 1000000000L);

            client->crbuf[client->crbufsize - 2] = '\0'; // don't want newline
            if (env->slow_query_log_format == NA_LOG_FORMAT_JSON) {
                struct json_object *json;
                const size_t bufsz = 128;
                char querybuf[bufsz];

                snprintf(querybuf, bufsz, "%s", client->crbuf);
                json = json_object_new_object();
                json_object_object_add(json, "time",             json_object_new_int(now));
                json_object_object_add(json, "type",             json_object_new_string(env->name));
                json_object_object_add(json, "host",             json_object_new_string(host));
                json_object_object_add(json, "clientaddr",       json_object_new_string(clientaddr));
                json_object_object_add(json, "clientport",       json_object_new_int(clientport));
                json_object_object_add(json, "na_to_ts",         json_object_new_double(na_to_ts));
                json_object_object_add(json, "na_from_ts",       json_object_new_double(na_from_ts));
                json_object_object_add(json, "na_to_client",     json_object_new_double(na_to_client));
                json_object_object_add(json, "querytxt",         json_object_new_string(querybuf));
                json_object_object_add(json, "request_bufsize",  json_object_new_int(client->crbufsize));
                json_object_object_add(json, "response_bufsize", json_object_new_int(client->cwbufsize));

                fprintf(env->slow_query_fp, "%s\n", json_object_to_json_string(json));
                json_object_put(json);
            } else { // plain text format
                fprintf(env->slow_query_fp,
                        "SLOWQUERY: time %lu type %s host %s client %s:%hu "
                        "na->ts %g na<-ts %g na->c %g querytxt \"%.128s\" "
                        "request_bufsize %d, response_bufsize %d\n",
                        now, env->name, host, clientaddr, clientport, 
                        na_to_ts, na_from_ts, na_to_client, client->crbuf, 
                        client->crbufsize, client->cwbufsize);
            }
        }
    }

    memset(&client->na_from_ts_time_begin,   0, sizeof(struct timespec));
    memset(&client->na_from_ts_time_end,     0, sizeof(struct timespec));
    memset(&client->na_to_ts_time_begin,     0, sizeof(struct timespec));
    memset(&client->na_to_ts_time_end,       0, sizeof(struct timespec));
    memset(&client->na_to_client_time_begin, 0, sizeof(struct timespec));
    memset(&client->na_to_client_time_end,   0, sizeof(struct timespec));
}

void na_slow_query_open(na_env_t *env)
{
    // close and reopen log file, to permit rotation
    if (env->slow_query_fp) {
        fclose(env->slow_query_fp);
    }
    env->slow_query_fp = fopen(env->slow_query_log_path, "a");

    if (!env->slow_query_fp) {
        NA_ERROR_OUTPUT_MESSAGE(env, NA_ERROR_CANT_OPEN_SLOWLOG);
        // disable slow query log, since we couldn't open the file
        memset(&env->slow_query_sec, 0, sizeof(struct timespec));
    } else {
        chmod(env->slow_query_log_path, env->slow_query_log_access_mask);
    }
}
