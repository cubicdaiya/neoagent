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
