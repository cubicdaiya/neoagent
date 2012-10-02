/**
   In short, neoagent is distributed under so called "BSD license",

   Copyright (c) 2012 Tatsuhiko Kubo <cubicdaiya@gmail.com>
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

#include "bm.h"

#include <string.h>

/**
 * The algorithm implemented here is based Boyer-Moore algorithm
 */

void na_bm_create_table (char *pattern, int *skip)
{
    int skip_size, len;

    skip_size = sizeof(skip) / sizeof(int);

    for (int i=0;i<skip_size;++i) {
        skip[i] = 0;
    }

    len = strlen(pattern);
    for (int i=0;i<len-1;++i) {
        skip[(int)((unsigned char)*(pattern + i))] = len - i - 1;
    }
}

int na_bm_search (char *haystack, char *pattern, int *skip, int hlen, int plen)
{
    int i, cnt;

    i   = 0;
    cnt = 0;

 loop:
    while (i + plen <= hlen) {

        for (int j=plen-1;j>=0;--j) {
            if (pattern[j] != haystack[i + j]) {
                if (skip[(int)((unsigned char)haystack[i + j])] == 0) {
                    if (j == plen - 1) {
                        i += plen;
                    } else {
                        i += plen - j - 1;
                    }
                } else {
                    int s = skip[(int)((unsigned char)haystack[i + j])] - (plen - 1 - j);
                    if (s <= 0) {
                        ++i;
                    } else {
                        i += s;
                    }
                }
                goto loop;
            }
        }

        if (skip[(int)((unsigned char)haystack[i + plen - 1])] != 0) {
            i += skip[(int)((unsigned char)haystack[i + plen - 1])];
        } else {
            i += plen;
        }

        ++cnt;
    }

    return cnt;
}
