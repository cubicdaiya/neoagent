/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <stdlib.h>

#include "defines.h"

na_event_queue_t *na_event_queue_create(int c)
{
    na_event_queue_t *q;
    q        = malloc(sizeof(na_event_queue_t));
    q->top   = 0;
    q->bot   = 0;
    q->cnt   = 0;
    q->max   = c;
    q->queue = calloc(sizeof(na_client_t *), c);
    for (int i=0;i<c;++i) {
        q->queue[i] = (na_client_t *)NULL;
    }
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init (&q->cond, NULL);
    return q;
}

void na_event_queue_destroy(na_event_queue_t *q)
{
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond);
    NA_FREE(q->queue);
    NA_FREE(q);
}

bool na_event_queue_push(na_event_queue_t *q, na_client_t *e)
{
    pthread_mutex_lock(&q->lock);
    if (q->cnt >= q->max) {
        pthread_mutex_unlock(&q->lock);
        return false;
    }

    q->queue[q->bot++] = e;

    if (q->bot >= q->max) {
        q->bot = 0;
    }

    ++q->cnt;

    if (q->cnt == 1) {
        pthread_cond_broadcast(&q->cond);
    }

    pthread_mutex_unlock(&q->lock);
    return true;
}

na_client_t *na_event_queue_pop(na_event_queue_t *q)
{
    na_client_t *c;
    pthread_mutex_lock(&q->lock);
    if (q->cnt <= 0) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    c = q->queue[q->top];
    q->queue[q->top++] = 0;

    if (q->top >= q->max) {
        q->top = 0;
    }

    --q->cnt;

    pthread_mutex_unlock(&q->lock);
    return c;
}
