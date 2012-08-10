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

#include "queue.h"
#include "util.h"

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
    return q;
}

void na_event_queue_destroy(na_event_queue_t *q)
{
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
