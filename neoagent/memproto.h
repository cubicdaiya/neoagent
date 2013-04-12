/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#ifndef NA_MEMPROTO_H
#define NA_MEMPROTO_H

#include <stdbool.h>

typedef enum na_memproto_cmd_t {
    NA_MEMPROTO_CMD_GET,
    NA_MEMPROTO_CMD_SET,
    NA_MEMPROTO_CMD_INCR,
    NA_MEMPROTO_CMD_DECR,
    NA_MEMPROTO_CMD_ADD,
    NA_MEMPROTO_CMD_DELETE,
    NA_MEMPROTO_CMD_QUIT,
    NA_MEMPROTO_CMD_UNKNOWN,
    NA_MEMPROTO_CMD_NOT_DETECTED,
    NA_MEMPROTO_CMD_MAX // Always add new codes to the end before this one
} na_memproto_cmd_t;

void na_memproto_bm_skip_init (void);
na_memproto_cmd_t na_memproto_detect_command (char *buf);
int na_memproto_count_request_get(char *buf, int bufsize);
int na_memproto_count_response_get(char *buf, int bufsize);

#endif // NA_MEMPROTO_H
