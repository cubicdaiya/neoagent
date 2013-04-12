/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
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
    int i, c;

    i = 0;
    c = 0;

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

        ++c;
    }

    return c;
}
