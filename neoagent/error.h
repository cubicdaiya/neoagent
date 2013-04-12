/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#ifndef NA_ERROR_H
#define NA_ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "time.h"

typedef enum na_error_t {
    NA_ERROR_INVALID_FD,
    NA_ERROR_INVALID_CMD,
    NA_ERROR_INVALID_HOSTNAME,
    NA_ERROR_INVALID_PORT,
    NA_ERROR_INVALID_SOCKPATH,
    NA_ERROR_INVALID_CONFPATH,
    NA_ERROR_INVALID_JSON_CONFIG,
    NA_ERROR_INVALID_CONNPOOL,
    NA_ERROR_CONNECTION_FAILED,
    NA_ERROR_OUTOF_MEMORY,
    NA_ERROR_OUTOF_BUFFER,
    NA_ERROR_OUTOF_LOOP,
    NA_ERROR_PARSE_JSON_CONFIG,
    NA_ERROR_FAILED_SETUP_SIGNAL,
    NA_ERROR_FAILED_IGNORE_SIGNAL,
    NA_ERROR_FAILED_DAEMONIZE,
    NA_ERROR_FAILED_READ,
    NA_ERROR_FAILED_WRITE,
    NA_ERROR_BROKEN_PIPE,
    NA_ERROR_TOO_MANY_ENVIRONMENTS,
    NA_ERROR_CANT_OPEN_SLOWLOG,
    NA_ERROR_FAILED_CREATE_PROCESS,
    NA_ERROR_INVALID_CTL_CMD,
    NA_ERROR_FAILED_EXECUTE_CTM_CMD,
    NA_ERROR_UNKNOWN,
    NA_ERROR_MAX // Always add new codes to the end before this one
} na_error_t;

#define NA_STDERR(s) do {                                               \
        char buf_dt[NA_DATETIME_BUF_MAX];                               \
        time_t cts = time(NULL);                                        \
        na_ts2dt(cts, "%Y-%m-%d %H:%M:%S", buf_dt, NA_DATETIME_BUF_MAX);  \
        fprintf(stderr, "%s, %s: %s %d\n", buf_dt, s, __FILE__, __LINE__); \
    } while (false)

#define NA_STDERR_MESSAGE(na_error) do {                                \
        char buf_dt[NA_DATETIME_BUF_MAX];                               \
        time_t cts = time(NULL);                                        \
        na_ts2dt(cts, "%Y-%m-%d %H:%M:%S", buf_dt, NA_DATETIME_BUF_MAX); \
        fprintf(stderr, "%s %s: %s %d\n", buf_dt, na_error_message(na_error), __FILE__, __LINE__); \
    } while (false)

#define NA_DIE_WITH_ERROR(na_error)                                     \
    do {                                                                \
        char buf_dt[NA_DATETIME_BUF_MAX];                               \
        time_t cts = time(NULL);                                        \
        na_ts2dt(cts, "%Y-%m-%d %H:%M:%S", buf_dt, NA_DATETIME_BUF_MAX); \
        fprintf(stderr, "%s %s: %s, %s %d\n", buf_dt, na_error_message(na_error), __FILE__, __FUNCTION__, __LINE__); \
        exit(1);                                                        \
    } while (false)

const char *na_error_message (na_error_t error);

#endif // NA_ERROR_H
