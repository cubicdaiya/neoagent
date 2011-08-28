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

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "error.h"

const char *neoagent_error_messages[NEOAGENT_ERROR_MAX] = {
    [NEOAGENT_ERROR_INVALID_ENV]           = "invalid environment",
    [NEOAGENT_ERROR_INVALID_FD]            = "invalid file descriptor",
    [NEOAGENT_ERROR_INVALID_HOSTNAME]      = "invalid hostname",
    [NEOAGENT_ERROR_INVALID_PORT]          = "invalid port number",
    [NEOAGENT_ERROR_INVALID_SOCKPATH]      = "invalid socket file path",
    [NEOAGENT_ERROR_INVALID_CONFPATH]      = "invalid configuration file path",
    [NEOAGENT_ERROR_INVALID_JSON_CONFIG]   = "invalid json configuration",
    [NEOAGENT_ERROR_CONNECTION_FAILED]     = "connection failed",
    [NEOAGENT_ERROR_OUTOF_MEMORY]          = "out of memory",
    [NEOAGENT_ERROR_PARSE_JSON_CONFIG]     = "json configuration parse error",
    [NEOAGENT_ERROR_FAILED_SETUP_SIGNAL]   = "failed to set signal handler",
    [NEOAGENT_ERROR_FAILED_IGNORE_SIGNAL]  = "failed to ignore signal",
    [NEOAGENT_ERROR_REMAIN_DATA]           = "still data remains",
    [NEOAGENT_ERROR_TOO_MANY_ENVIRONMENTS] = "too many environments",
    [NEOAGENT_ERROR_UNKNOWN]               = "unknown error",
};

const char *neoagent_error_message (neoagent_error_t error)
{
    if (error >= NEOAGENT_ERROR_MAX) {
        return neoagent_error_messages[NEOAGENT_ERROR_UNKNOWN];
    }
    return neoagent_error_messages[error];
}

void neoagent_die (const char *s)
{
    fprintf(stderr, "%s\n", s);
    exit(1);
}

void neoagent_die_with_error (neoagent_error_t error)
{
    fprintf(stderr, "%s\n", neoagent_error_message(error));
    exit(1);
}
