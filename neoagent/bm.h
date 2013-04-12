/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#ifndef NA_BM_H
#define NA_BM_H

#define NA_BM_SKIP_SIZE 256

void na_bm_create_table (char *pattern, int *skip);
int na_bm_search (char *haystack, char *pattern, int *skip, int hlen, int plen);

#endif // NA_BM_H
