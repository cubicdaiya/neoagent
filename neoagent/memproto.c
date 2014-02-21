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
    na_bm_create_table("\r\n",    na_bm_skip[NA_MEMPROTO_BM_SKIP_CRLF],    NA_BM_SKIP_SIZE);
    na_bm_create_table("END\r\n", na_bm_skip[NA_MEMPROTO_BM_SKIP_ENDCRLF], NA_BM_SKIP_SIZE);
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
    //return na_bm_search(buf, "END\r\n", na_bm_skip[NA_MEMPROTO_BM_SKIP_ENDCRLF], bufsize, 5);
    //VALUE key flag len
    char *p;
    char *tk, *sp;
    int i;
    char sizbuf[10];
    int c;
    int count = 0;
    int j;

    if (!(buf[bufsize - 5] == 'E'  &&
          buf[bufsize - 4] == 'N'  &&
          buf[bufsize - 3] == 'D'  &&
          buf[bufsize - 2] == '\r' &&
          buf[bufsize - 1] == '\n'))
    {
        return 0;
    } else {
        if (bufsize == sizeof("END\r\n") - 1) {
            return 1;
        }
    }

    p = buf;
    i = 0;
 count:
    c = 0;
    while (c != 3) {
        if (*p == ' ') {
            c++;
        }
        i++;
        if (i >= bufsize) {
            return count;
        }
        p++;
    }

    j = 0;
    while (*p != '\n') {
        sizbuf[j++] = *p++;
        i++;
    }
    if (i >= bufsize) {
        return count;
    }
    sizbuf[j] = '\0';
    int size = atoi(sizbuf);
    p += size + 3;
    i += size + 3;
    if (i >= bufsize) {
        return count;
    }

    if (strncmp("END\r\n", p, sizeof("END\r\n") - 1) == 0) {
        count++;
        i += sizeof("END\r\n") - 1;
        p += sizeof("END\r\n") - 1;
        if (i >= bufsize) {
            return count;
        }
        goto count;
    } else {
        goto count;
    }
    //return na_bm_search(buf, "END\r\n", na_bm_skip[NA_MEMPROTO_BM_SKIP_ENDCRLF], bufsize, 5);
}
