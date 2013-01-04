/**
   In short, neoagent is distributed under so called "BSD license",

   Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
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

#include <string.h>

#include "memproto.h"
#include "bm.h"

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
