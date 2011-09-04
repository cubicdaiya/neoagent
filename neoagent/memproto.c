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

#include <string.h>

#include "memproto.h"
#include "bm.h"

typedef enum neoagent_memproto_bm_skip_t {
    NEOAGENT_MEMPROTO_BM_SKIP_CRLF,
    NEOAGENT_MEMPROTO_BM_SKIP_ENDCRLF,
    NEOAGENT_MEMPROTO_BM_SKIP_MAX // Always add new codes to the end before this one
} neoagent_memproto_bm_skip_t;

static int neoagent_bm_skip[NEOAGENT_MEMPROTO_BM_SKIP_MAX][NEOAGENT_BM_SKIP_SIZE] = {
    [NEOAGENT_MEMPROTO_BM_SKIP_CRLF]    = {},
    [NEOAGENT_MEMPROTO_BM_SKIP_ENDCRLF] = {},
};

void neoagent_memproto_bm_skip_init (void)
{
    neoagent_bm_create_table("\r\n",    neoagent_bm_skip[NEOAGENT_MEMPROTO_BM_SKIP_CRLF]);
    neoagent_bm_create_table("END\r\n", neoagent_bm_skip[NEOAGENT_MEMPROTO_BM_SKIP_ENDCRLF]);
}

neoagent_memproto_cmd_t neoagent_memproto_detect_command (char *buf)
{
    if (strncmp(buf, "get", 3) == 0) {
        return NEOAGENT_MEMPROTO_CMD_GET;
    } else if (strncmp(buf, "set", 3) == 0) {
        return NEOAGENT_MEMPROTO_CMD_SET;
    } else if (strncmp(buf, "incr", 4) == 0) {
        return NEOAGENT_MEMPROTO_CMD_INCR;
    } else if (strncmp(buf, "add", 3) == 0) {
        return NEOAGENT_MEMPROTO_CMD_ADD;
    } else if (strncmp(buf, "delete", 6) == 0) {
        return NEOAGENT_MEMPROTO_CMD_DELETE;
    } else if (strncmp(buf, "quit", 4) == 0) {
        return NEOAGENT_MEMPROTO_CMD_QUIT;
    }
    return NEOAGENT_MEMPROTO_CMD_UNKNOWN;
}

int neoagent_memproto_count_request (char *buf, int bufsize)
{
    return neoagent_bm_search(buf, "\r\n", neoagent_bm_skip[NEOAGENT_MEMPROTO_BM_SKIP_CRLF], bufsize, 2);
}

int neoagent_memproto_count_response(char *buf, int bufsize)
{
    return neoagent_bm_search(buf, "END\r\n", neoagent_bm_skip[NEOAGENT_MEMPROTO_BM_SKIP_ENDCRLF], bufsize, 5);
}
