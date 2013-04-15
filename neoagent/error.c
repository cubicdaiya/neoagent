/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "defines.h"

const char *na_error_messages[NA_ERROR_MAX] = {
    [NA_ERROR_INVALID_FD]            = "invalid file descriptor",
    [NA_ERROR_INVALID_CMD]           = "invalid memcached command",
    [NA_ERROR_INVALID_HOSTNAME]      = "invalid hostname",
    [NA_ERROR_INVALID_PORT]          = "invalid port number",
    [NA_ERROR_INVALID_SOCKPATH]      = "invalid socket file path",
    [NA_ERROR_INVALID_CONFPATH]      = "invalid configuration file path",
    [NA_ERROR_INVALID_JSON_CONFIG]   = "invalid json configuration",
    [NA_ERROR_INVALID_CONNPOOL]      = "invalid connection pool",
    [NA_ERROR_CONNECTION_FAILED]     = "connection failed",
    [NA_ERROR_OUTOF_MEMORY]          = "out of memory",
    [NA_ERROR_OUTOF_BUFFER]          = "out of buffer",
    [NA_ERROR_OUTOF_LOOP]            = "out of loop",
    [NA_ERROR_PARSE_JSON_CONFIG]     = "json configuration parse error",
    [NA_ERROR_FAILED_SETUP_SIGNAL]   = "failed to set signal handler",
    [NA_ERROR_FAILED_IGNORE_SIGNAL]  = "failed to ignore signal",
    [NA_ERROR_FAILED_DAEMONIZE]      = "failed to daemonize",
    [NA_ERROR_FAILED_READ]           = "failed to read",
    [NA_ERROR_FAILED_WRITE]          = "failed to write",
    [NA_ERROR_BROKEN_PIPE]           = "broken pipe",
    [NA_ERROR_TOO_MANY_ENVIRONMENTS] = "too many environments",
    [NA_ERROR_CANT_OPEN_SLOWLOG]     = "can't open slow query log file",
    [NA_ERROR_FAILED_CREATE_PROCESS] = "failed to create process",
    [NA_ERROR_INVALID_CTL_CMD]       = "invalid ctl command",
    [NA_ERROR_FAILED_EXECUTE_CTM_CMD]= "failed to execute ctl command",
    [NA_ERROR_UNKNOWN]               = "unknown error"
};

const char *na_error_message (na_error_t error)
{
    if (error >= NA_ERROR_MAX) {
        return na_error_messages[NA_ERROR_UNKNOWN];
    }
    return na_error_messages[error];
}
