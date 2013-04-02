/**
   In short, fnv.h is distributed under so called "BSD license",
   
   Copyright (c) 2010 Tatsuhiko Kubo <cubicdaiya@gmail.com>
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

/* written by C99 style */


#ifndef FNV_H
#define FNV_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define FNV_MALLOC(p, n)                        \
    do {                                        \
        if (((p) = malloc(n)) == NULL) {        \
            printf("malloc failed");            \
            exit(-1);                           \
        }                                       \
    } while(false)

#define FNV_FREE(p)                                 \
    do {                                            \
        if (p != NULL) {                            \
            free(p);                                \
            (p) = NULL;                             \
        }                                           \
    } while(false)

#define FNV_CHKOVERSIZ(siz, max, ret)             \
    do {                                          \
        if (siz > max) {                          \
            return ret;                           \
        }                                         \
    } while(false)
/**
 * config
 */
#define FNV_TBL_CNT_DEFAULT 1023
#define FNV_PRIME           0x01000193
#define FNV_OFFSET_BASIS    0x811c9dc5

#define FNV_KEY_MAX_LENGTH 1024
#define FNV_VAL_MAX_LENGTH 8388608

typedef int fnv_type_t;
#define FNV_TYPE_KEY 1
#define FNV_TYPE_VAL 2

#define FNV_PUT_SUCCESS   1
#define FNV_PUT_DUPLICATE 2

#define FNV_PUT_OVER_KEYSIZ -1
#define FNV_PUT_OVER_VALSIZ -2

#define FNV_OUT_SUCCESS     1
#define FNV_OUT_NOTFOUND    -1
#define FNV_OUT_OVER_KEYSIZ -2

typedef unsigned int uint_t;
typedef unsigned char uchar_t;

typedef struct fnv_ent_s {
    char *k;                // key
    void *v;                // value
    struct fnv_ent_s *next; // next element of entry list
} fnv_ent_t;                // entry list

typedef struct fnv_tbl_s {
    fnv_ent_t *ents; // entires
    size_t c;        // table size
} fnv_tbl_t;         // hash table

fnv_tbl_t *fnv_tbl_create(fnv_ent_t *ents, size_t c);
char *fnv_get(fnv_tbl_t *tbl, const char *k, size_t ksiz);
int fnv_put(fnv_tbl_t *tbl, const char *k, const void *v, size_t ksiz, size_t vsiz);
int fnv_out(fnv_tbl_t *tbl, const char *k, size_t ksiz);
void fnv_tbl_destroy(fnv_tbl_t *tbl);
void fnv_tbl_print(fnv_tbl_t *tbl, size_t c);

#endif // FNV_H
