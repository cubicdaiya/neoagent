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

#ifndef NEOAGENT_MEMPROTO_H
#define NEOAGENT_MEMPROTO_H

#include <stdbool.h>

typedef enum neoagent_memproto_cmd_t {
    NEOAGENT_MEMPROTO_CMD_GET,
    NEOAGENT_MEMPROTO_CMD_SET,
    NEOAGENT_MEMPROTO_CMD_INCR,
    NEOAGENT_MEMPROTO_CMD_ADD,
    NEOAGENT_MEMPROTO_CMD_DELETE,
    NEOAGENT_MEMPROTO_CMD_UNKNOWN,
    NEOAGENT_MEMPROTO_CMD_MAX,
} neoagent_memproto_cmd_t;

void neoagent_memproto_bm_skip_init (void);
neoagent_memproto_cmd_t neoagent_memproto_detect_command (char *buf);
int neoagent_memproto_count_request (char *buf, int bufsize);
int neoagent_memproto_count_response(char *buf, int bufsize);

inline bool neoagent_memproto_is_request_divided (int req_cnt)
{
    return req_cnt > 1;
}

#endif // NEOAGENT_MEMPROTO_H
