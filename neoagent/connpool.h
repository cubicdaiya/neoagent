/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#ifndef NA_CONNPOOL_H
#define NA_CONNPOOL_H

#include "env.h"

void na_connpool_create (na_connpool_t *connpool, int c);
void na_connpool_destroy (na_connpool_t *connpool);
bool na_connpool_assign (na_env_t *env, na_connpool_t *connpool, int *cur, int *fd);
void na_connpool_init (na_env_t *env);
na_connpool_t *na_connpool_select(na_env_t *env);
void na_connpool_switch (na_env_t *env);

#endif // NA_CONNPOOL_H
