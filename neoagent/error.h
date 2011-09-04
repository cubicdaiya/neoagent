/**
   In short, neoagent is distributed under so called "BSD license",
   
   Copyright (c) 2011 Tatsuhiko Kubo <cubicdaiya@gmail.com>
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

#ifndef NEOAGENT_ERROR_H
#define NEOAGENT_ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum neoagent_error_t {
    NEOAGENT_ERROR_INVALID_ENV,
    NEOAGENT_ERROR_INVALID_FD,
    NEOAGENT_ERROR_INVALID_CMD,
    NEOAGENT_ERROR_INVALID_HOSTNAME,
    NEOAGENT_ERROR_INVALID_PORT,
    NEOAGENT_ERROR_INVALID_SOCKPATH,
    NEOAGENT_ERROR_INVALID_CONFPATH,
    NEOAGENT_ERROR_INVALID_JSON_CONFIG,
    NEOAGENT_ERROR_CONNECTION_FAILED,
    NEOAGENT_ERROR_OUTOF_MEMORY,
    NEOAGENT_ERROR_PARSE_JSON_CONFIG,
    NEOAGENT_ERROR_FAILED_SETUP_SIGNAL,
    NEOAGENT_ERROR_FAILED_IGNORE_SIGNAL,
    NEOAGENT_ERROR_FAILED_DAEMONIZE,
    NEOAGENT_ERROR_FAILED_WRITE,
    NEOAGENT_ERROR_REMAIN_DATA,
    NEOAGENT_ERROR_TOO_MANY_ENVIRONMENTS,
    NEOAGENT_ERROR_UNKNOWN,
    NEOAGENT_ERROR_MAX // Always add new codes to the end before this one
} neoagent_error_t;

#define NEOAGENT_STDERR(s) fprintf(stderr, "%s: %s %d\n", s, __FILE__, __LINE__)
#define NEOAGENT_STDERR_MESSAGE(neoagent_error) fprintf(stderr, "%s: %s %d\n", neoagent_error_message(neoagent_error), __FILE__, __LINE__)
#define NEOAGENT_DIE_WITH_ERROR(neoagent_error)                         \
    do {                                                                \
        fprintf(stderr, "%s: %s %d\n", neoagent_error_message(neoagent_error), __FILE__, __LINE__); \
        exit(1);                                                        \
    } while (false)

const char *neoagent_error_message (neoagent_error_t error);

#endif // NEOAGENT_ERROR_H

