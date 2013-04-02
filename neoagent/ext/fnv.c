/**
   In short, fnv.c is distributed under so called "BSD license",
   
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

#include "fnv.h"

/**
 * private function
 */
static uint_t fnv_hash(fnv_tbl_t *tbl, const char *k);
static fnv_ent_t *fnv_ent_create();
static fnv_ent_t *fnv_get_tail(fnv_ent_t *ent, const char *k, size_t ksiz);
static void fnv_ent_init(fnv_ent_t *ent, const char *k, const void *v);

/**
 * create hash table
 */
fnv_tbl_t *fnv_tbl_create(fnv_ent_t *ents, size_t c) {
    fnv_tbl_t *tbl;
    FNV_MALLOC(tbl, sizeof(fnv_tbl_t));
    tbl->ents = ents;
    tbl->c    = c;
    for (int i=0;i<c;++i) {
        fnv_ent_init(&ents[i], NULL, NULL);
    }
    return tbl;
}

/**
 * get entry from key
 */
char *fnv_get(fnv_tbl_t *tbl, const char *k, size_t ksiz) {
    FNV_CHKOVERSIZ(ksiz, FNV_KEY_MAX_LENGTH, NULL);
    fnv_ent_t *ents = tbl->ents;
    uint_t h = fnv_hash(tbl, k);
    if (ents[h].k == NULL) {
        return NULL;
    }
    if (strcmp(ents[h].k, k) == 0) {
        return ents[h].v;
    } else {
        fnv_ent_t *tail = &ents[h];
        while (tail->next) {
            tail = tail->next;
            if (strcmp(tail->k, k) == 0) {
                return tail->v;
            }
        }
    }
    return NULL;
}

/**
 * insert entry to hash table
 */
int fnv_put(fnv_tbl_t *tbl, const char *k, const void *v, size_t ksiz, size_t vsiz) {
    FNV_CHKOVERSIZ(ksiz, FNV_KEY_MAX_LENGTH, FNV_PUT_OVER_KEYSIZ);
    FNV_CHKOVERSIZ(vsiz, FNV_VAL_MAX_LENGTH, FNV_PUT_OVER_VALSIZ);
    uint_t h = fnv_hash(tbl, k);
    fnv_ent_t *ents = tbl->ents;
    if (ents[h].k != NULL) {
        if (strcmp(ents[h].k, k) == 0) {
            return FNV_PUT_DUPLICATE;
        }
        fnv_ent_t *tail = fnv_get_tail(&ents[h], k, ksiz);
        if (!tail) {
            return FNV_PUT_DUPLICATE;
        }
        tail->next = fnv_ent_create();
        fnv_ent_init(tail->next, k, v);
    } else {
        fnv_ent_init(&ents[h], k, v);
    }
    return FNV_PUT_SUCCESS;
}

/**
 * delete entry from hash table
 */
int fnv_out(fnv_tbl_t *tbl, const char *k, size_t ksiz) {
    FNV_CHKOVERSIZ(ksiz, FNV_KEY_MAX_LENGTH, FNV_OUT_OVER_KEYSIZ);
    uint_t h = fnv_hash(tbl, k);
    fnv_ent_t *tail = &tbl->ents[h];
    if (!tail || tail->k == NULL) {
        return FNV_OUT_NOTFOUND;
    }
    if (strcmp(tail->k, k) == 0) {
        tail->k = NULL;
        tail->v = NULL;
        if (tail->next) {
            fnv_ent_t *new_next = tail->next->next;
            memcpy(tail, tail->next, sizeof(tail));
            FNV_FREE(tail->next);
            tail->next = new_next;
        }
    } else {
        while (tail->next) {
            fnv_ent_t *current = tail;
            tail = tail->next;
            if (strcmp(tail->k, k) == 0) {
                fnv_ent_t *next;
                next = tail->next;
                FNV_FREE(tail);
                if (next) {
                    current->next = next;
                } else {
                    current->next = NULL;
                }
                return FNV_OUT_SUCCESS;
            }
        }
        return FNV_OUT_NOTFOUND;
    }
    return FNV_OUT_SUCCESS;
}

/**
 * release hash table
 */
void fnv_tbl_destroy(fnv_tbl_t *tbl) {
    fnv_ent_t *ents = tbl->ents;
    size_t c = tbl->c;
    for (int i=0;i<c;++i) {
        fnv_ent_t *tail = ents[i].next;
        while (tail) {
            fnv_ent_t *current = tail;
            tail = tail->next;
            FNV_FREE(current);
        }
    }
    FNV_FREE(tbl);
}

/**
 * print entries of hash table
 */
void fnv_tbl_print(fnv_tbl_t *tbl, size_t c) {
    fnv_ent_t *ents = tbl->ents;
    for (int i=0;i<c;++i) {
        fnv_ent_t *ent = &ents[i];
        if (ent->k == NULL) continue;
        printf("i = %d, key = %s, val = %s", i, ent->k, (char *)ent->v);
        if (ent->next != NULL) {
            fnv_ent_t *tail = ent->next;
            while (tail) {
                printf(" -> key = %s, val = %s", tail->k, (char *)tail->v);
                tail = tail->next;
            }
        }
        printf("\n");
    }
}

/* following is private function */ 

/**
 * The algorithm implemented here is based on FNV hash algorithm
 * described by Glenn Fowler, Phong Vo, Landon Curt Noll
 */
static uint_t fnv_hash(fnv_tbl_t *tbl, const char *k) {
    uint_t h = FNV_OFFSET_BASIS;
    for (uchar_t *p=(uchar_t *)k;*p!='\0';++p) {
        h *= FNV_PRIME;
        h ^= *p;
    }
    return h % tbl->c;
}

/**
 * initialize entry
 */
static void fnv_ent_init(fnv_ent_t *ent, const char *k, const void *v) {
    ent->k  = (char *)k;
    ent->v  = (char *)v;
    ent->next = NULL;
}

/**
 * get trailing element of entry list
 */
static fnv_ent_t *fnv_get_tail(fnv_ent_t *ent, const char *k, size_t ksiz) {
    fnv_ent_t *tail = ent;
    while (tail->next) {
        tail = tail->next;
        if (strcmp(tail->k, k) == 0) {
            return NULL;
        }
    }
    return tail;
}

/**
 * create entry of hash table
 */
static fnv_ent_t *fnv_ent_create() {
    fnv_ent_t *ent;
    FNV_MALLOC(ent, sizeof(fnv_ent_t));
    return ent;
}

