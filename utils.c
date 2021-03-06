/*
    Waveform seekbar plugin for the DeaDBeeF audio player

    Copyright (C) 2014 Christian Boxdörfer <christian.boxdoerfer@posteo.de>

    Based on sndfile-tools waveform by Erik de Castro Lopo.
        waveform.c - v1.04
        Copyright (C) 2007-2012 Erik de Castro Lopo <erikd@mega-nerd.com>
        Copyright (C) 2012 Robin Gareus <robin@gareus.org>
        Copyright (C) 2013 driedfruit <driedfruit@mindloop.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <fcntl.h>

#include <deadbeef/deadbeef.h>

#include "waveform.h"
#include "utils.h"

typedef struct cache_query_s
{
    char *fname;
    struct cache_query_s *next;
} cache_query_t;

static uintptr_t mutex = 0;
static cache_query_t *queue;
static cache_query_t *queue_tail;

int
queue_add (const char *fname)
{
    if (!mutex) {
        mutex = deadbeef->mutex_create ();
    }
    deadbeef->mutex_lock (mutex);
    for (cache_query_t *q = queue; q; q = q->next) {
        if (!strcmp (fname, q->fname)) {
            // already queued
            trace ("waveform: already queued. (%s)\n",fname);
            deadbeef->mutex_unlock (mutex);
            return 0;
        }
    }
    cache_query_t *q = malloc (sizeof (cache_query_t));
    memset (q, 0, sizeof (cache_query_t));
    q->fname = strdup (fname);
    if (queue_tail) {
        queue_tail->next = q;
        queue_tail = q;
    }
    else {
        queue = queue_tail = q;
    }
    trace ("waveform: queued. (%s)\n",fname);
    deadbeef->mutex_unlock (mutex);
    return 1;
}

void
queue_pop (const char *fname)
{
    deadbeef->mutex_lock (mutex);
    cache_query_t *next = NULL;
    for (cache_query_t *q = queue; q; q = q->next) {
        if (!strcmp (fname, q->fname)) {
            next = q->next;
            if (q->fname) {
                trace ("waveform: removed from queue. (%s)\n",q->fname);
                free (q->fname);
            }
            free (q);
            break;
        }
    }
    queue = next;
    if (!queue) {
        queue_tail = NULL;
    }
    deadbeef->mutex_unlock (mutex);
}
