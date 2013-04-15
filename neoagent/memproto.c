/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <string.h>

#include "defines.h"

typedef enum na_memproto_bm_skip_t {
    NA_MEMPROTO_BM_SKIP_CRLF,
    NA_MEMPROTO_BM_SKIP_ENDCRLF,
    NA_MEMPROTO_BM_SKIP_MAX // Always add new codes to the end before this one
} na_memproto_bm_skip_t;

static int na_bm_skip[NA_MEMPROTO_BM_SKIP_MAX][NA_BM_SKIP_SIZE] = {
    [NA_MEMPROTO_BM_SKIP_CRLF]    = {},
    [NA_MEMPROTO_BM_SKIP_ENDCRLF] = {},
};

void na_memproto_bm_skip_init (void)
{
    na_bm_create_table("\r\n",    na_bm_skip[NA_MEMPROTO_BM_SKIP_CRLF]);
    na_bm_create_table("END\r\n", na_bm_skip[NA_MEMPROTO_BM_SKIP_ENDCRLF]);
}

na_memproto_cmd_t na_memproto_detect_command (char *buf)
{
    if (strncmp(buf, "get", 3) == 0) {
        return NA_MEMPROTO_CMD_GET;
    } else if (strncmp(buf, "set", 3) == 0) {
        return NA_MEMPROTO_CMD_SET;
    } else if (strncmp(buf, "incr", 4) == 0) {
        return NA_MEMPROTO_CMD_INCR;
    } else if (strncmp(buf, "decr", 4) == 0) {
        return NA_MEMPROTO_CMD_DECR;
    } else if (strncmp(buf, "add", 3) == 0) {
        return NA_MEMPROTO_CMD_ADD;
    } else if (strncmp(buf, "delete", 6) == 0) {
        return NA_MEMPROTO_CMD_DELETE;
    } else if (strncmp(buf, "quit", 4) == 0) {
        return NA_MEMPROTO_CMD_QUIT;
    }
    return NA_MEMPROTO_CMD_UNKNOWN;
}

int na_memproto_count_request_get (char *buf, int bufsize)
{
    return na_bm_search(buf, "\r\n", na_bm_skip[NA_MEMPROTO_BM_SKIP_CRLF], bufsize, 2);
}

int na_memproto_count_response_get(char *buf, int bufsize)
{
    return na_bm_search(buf, "END\r\n", na_bm_skip[NA_MEMPROTO_BM_SKIP_ENDCRLF], bufsize, 5);
}
