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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "kernel-internal.h"
#include "semaphore.h"

/****************************************************************************
 * Simple semaphore functions ;)
 ****************************************************************************/
/* Initialize the semaphore object.
 * max = maximum up count the semaphore may assume (max >= 1)
 * start = initial count of semaphore (0 <= count <= max) */
void semaphore_init(struct semaphore *s, int max, int start)
{
    KERNEL_ASSERT(max > 0 && start >= 0 && start <= max,
                  "semaphore_init->inv arg\n");
    s->queue = NULL;
    s->max = max;
    s->count = start;
    corelock_init(&s->cl);
}

/* Down the semaphore's count or wait for 'timeout' ticks for it to go up if
 * it is already 0. 'timeout' as TIMEOUT_NOBLOCK (0) will not block and may
 * safely be used in an ISR. */
int semaphore_wait(struct semaphore *s, int timeout)
{
    int ret;
    int oldlevel;
    int count;

    oldlevel = disable_irq_save();
    corelock_lock(&s->cl);

    count = s->count;

    if(LIKELY(count > 0))
    {
        /* count is not zero; down it */
        s->count = count - 1;
        ret = OBJ_WAIT_SUCCEEDED;
    }
    else if(timeout == 0)
    {
        /* just polling it */
        ret = OBJ_WAIT_TIMEDOUT;
    }
    else
    {
        /* too many waits - block until count is upped... */
        struct thread_entry * current = thread_self_entry();
        IF_COP( current->obj_cl = &s->cl; )
        current->bqp = &s->queue;
        /* return value will be OBJ_WAIT_SUCCEEDED after wait if wake was
         * explicit in semaphore_release */
        current->retval = OBJ_WAIT_TIMEDOUT;

        block_thread(current, timeout);
        corelock_unlock(&s->cl);

        /* ...and turn control over to next thread */
        switch_thread();

        return current->retval;
    }

    corelock_unlock(&s->cl);
    restore_irq(oldlevel);

    return ret;
}

/* Up the semaphore's count and release any thread waiting at the head of the
 * queue. The count is saturated to the value of the 'max' parameter specified
 * in 'semaphore_init'. */
void semaphore_release(struct semaphore *s)
{
    unsigned int result = THREAD_NONE;
    int oldlevel;

    oldlevel = disable_irq_save();
    corelock_lock(&s->cl);

    if(LIKELY(s->queue != NULL))
    {
        /* a thread was queued - wake it up and keep count at 0 */
        KERNEL_ASSERT(s->count == 0,
            "semaphore_release->threads queued but count=%d!\n", s->count);
        s->queue->retval = OBJ_WAIT_SUCCEEDED; /* indicate explicit wake */
        result = wakeup_thread(&s->queue, WAKEUP_DEFAULT);
    }
    else
    {
        int count = s->count;
        if(count < s->max)
        {
            /* nothing waiting - up it */
            s->count = count + 1;
        }
    }

    corelock_unlock(&s->cl);
    restore_irq(oldlevel);

#if defined(HAVE_PRIORITY_SCHEDULING) && defined(is_thread_context)
    /* No thread switch if not thread context */
    if((result & THREAD_SWITCH) && is_thread_context())
        switch_thread();
#endif
    (void)result;
}
