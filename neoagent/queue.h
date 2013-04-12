/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#ifndef NA_QUEUE_H
#define NA_QUEUE_H

#include <pthread.h>
#include <stdbool.h>

#include "env.h"

typedef struct na_event_queue_t {
    na_client_t **queue;
    size_t top;
    size_t bot;
    size_t cnt;
    size_t max;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
} na_event_queue_t;

na_event_queue_t *na_event_queue_create(int c);
void na_event_queue_destroy(na_event_queue_t *q);
bool na_event_queue_push(na_event_queue_t *q, na_client_t *e);
na_client_t *na_event_queue_pop(na_event_queue_t *q);

#endif // NA_QUEUE_H
