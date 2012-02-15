/**
   In short, neoagent is distributed under so called "BSD license",
   
   Copyright (c) 2011-2012 Tatsuhiko Kubo <cubicdaiya@gmail.com>
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
    NA_ERROR_UNKNOWN,
    NA_ERROR_MAX // Always add new codes to the end before this one
} na_error_t;

#define NA_STDERR(s) do {                                               \
        char buf_dt[NA_DATETIME_BUF_MAX];                               \
        time_t cts = time(NULL);                                        \
        na_ts2dt(&cts, "%Y-%m-%d %H:%M:%S", buf_dt, NA_DATETIME_BUF_MAX);  \
        fprintf(stderr, "%s, %s: %s %d\n", buf_dt, s, __FILE__, __LINE__); \
    } while (false)

#define NA_STDERR_MESSAGE(na_error) do {                                \
        char buf_dt[NA_DATETIME_BUF_MAX];                               \
        time_t cts = time(NULL);                                        \
        na_ts2dt(&cts, "%Y-%m-%d %H:%M:%S", buf_dt, NA_DATETIME_BUF_MAX); \
        fprintf(stderr, "%s %s: %s %d\n", buf_dt, na_error_message(na_error), __FILE__, __LINE__); \
    } while (false)

#define NA_DIE_WITH_ERROR(na_error)                                     \
    do {                                                                \
        char buf_dt[NA_DATETIME_BUF_MAX];                               \
        time_t cts = time(NULL);                                        \
        na_ts2dt(&cts, "%Y-%m-%d %H:%M:%S", buf_dt, NA_DATETIME_BUF_MAX); \
        fprintf(stderr, "%s %s: %s, %s %d\n", buf_dt, na_error_message(na_error), __FILE__, __FUNCTION__, __LINE__); \
        exit(1);                                                        \
    } while (false)

const char *na_error_message (na_error_t error);

#endif // NA_ERROR_H

