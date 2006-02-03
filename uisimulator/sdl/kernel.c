/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdlib.h>
#include "uisdl.h"
#include "kernel.h"
#include "thread-sdl.h"
#include "thread.h"
#include "debug.h"

/* (Daniel 2002-10-31) Mingw32 requires this errno variable to be present.
   I'm not quite sure why and I don't know if this breaks the MSVC compile.
   If it does, we should put this within #ifdef __MINGW32__ */
int errno;

static void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

int set_irq_level (int level)
{
    static int _lv = 0;
    return (_lv = level);
}

void queue_init(struct event_queue *q)
{
    q->read = 0;
    q->write = 0;
}

void queue_delete(struct event_queue *q)
{
    (void)q;
}

void queue_wait(struct event_queue *q, struct event *ev)
{
    while(q->read == q->write)
    {
        switch_thread();
    }

    *ev = q->events[(q->read++) & QUEUE_LENGTH_MASK];
}

void queue_wait_w_tmo(struct event_queue *q, struct event *ev, int ticks)
{
    unsigned int timeout = current_tick + ticks;

    while(q->read == q->write && TIME_BEFORE( current_tick, timeout ))
    {
        sleep(1);
    }

    if(q->read != q->write)
    {
        *ev = q->events[(q->read++) & QUEUE_LENGTH_MASK];
    }
    else
    {
        ev->id = SYS_TIMEOUT;
    }
}

void queue_post(struct event_queue *q, long id, void *data)
{
    int wr;
    int oldlevel;

    oldlevel = set_irq_level(15<<4);
    wr = (q->write++) & QUEUE_LENGTH_MASK;

    q->events[wr].id = id;
    q->events[wr].data = data;
    set_irq_level(oldlevel);
}

bool queue_empty(const struct event_queue* q)
{
    return ( q->read == q->write );
}

void queue_clear(struct event_queue* q)
{
    /* fixme: This is potentially unsafe in case we do interrupt-like processing */
    q->read = 0;
    q->write = 0;
}

void switch_thread (void)
{
    yield ();
}

void sim_tick_tasks(void)
{
    int i;

    /* Run through the list of tick tasks */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i])
        {
            tick_funcs[i]();
        }
    }
}

int tick_add_task(void (*f)(void))
{
    int i;

    /* Add a task if there is room */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i] == NULL)
        {
            tick_funcs[i] = f;
            return 0;
        }
    }
    DEBUGF("Error! tick_add_task(): out of tasks");
    return -1;
}

int tick_remove_task(void (*f)(void))
{
    int i;

    /* Remove a task if it is there */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i] == f)
        {
            tick_funcs[i] = NULL;
            return 0;
        }
    }
    
    return -1;
}

/* TODO: Implement mutexes for win32 */
void mutex_init(struct mutex *m)
{
    (void)m;
}

void mutex_lock(struct mutex *m)
{
    (void)m;
}

void mutex_unlock(struct mutex *m)
{
    (void)m;
}
