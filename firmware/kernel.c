/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "kernel.h"
#include "thread.h"

long current_tick = 0;

void sleep(int ticks)
{
    int timeout = current_tick + ticks;

    /* always yield at least once */
    do {
        yield();
    } while (TIME_BEFORE( current_tick, timeout ));
}

void yield(void)
{
    switch_thread();
}

/****************************************************************************
 * Queue handling stuff
 ****************************************************************************/
void queue_init(struct event_queue *q)
{
    q->read = 0;
    q->write = 0;
}

struct event *queue_wait(struct event_queue *q)
{
    while(q->read == q->write)
    {
        switch_thread();
    }

    return &q->events[(q->read++) & QUEUE_LENGTH_MASK];
}

void queue_post(struct event_queue *q, int id, void *data)
{
    int wr = (q->write++) & QUEUE_LENGTH_MASK;

    q->events[wr].id = id;
    q->events[wr].data = data;
}
