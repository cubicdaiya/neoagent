/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#ifndef NA_UTIL_H
#define NA_UTIL_H

#define NA_FREE(p)                                      \
    do {                                                \
        if (p != NULL) {                                \
            free(p);                                    \
            (p) = NULL;                                 \
        }                                               \
    } while(false)

#endif // NA_UTIL_H
