/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Ulf Ralberg
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
#include "thread-internal.h"
#include "system.h"

/*---------------------------------------------------------------------------
 * Wakeup an entire queue of threads - returns bitwise-or of return bitmask
 * from each operation or THREAD_NONE of nothing was awakened. Object owning
 * the queue must be locked first.
 *
 * INTERNAL: Intended for use by kernel objects and not for programs.
 *---------------------------------------------------------------------------
 */
unsigned int thread_queue_wake(struct thread_entry **list)
{
    unsigned result = THREAD_NONE;

    for (;;)
    {
        unsigned int rc = wakeup_thread(list, WAKEUP_DEFAULT);

        if (rc == THREAD_NONE)
            break; /* No more threads */

        result |= rc;
    }

    return result;
}


/** Debug screen stuff **/

/*---------------------------------------------------------------------------
 * returns the stack space used in bytes
 *---------------------------------------------------------------------------
 */
static unsigned int stack_usage(uintptr_t *stackptr, size_t stack_size)
{
    unsigned int usage = 0;
    unsigned int stack_words = stack_size / sizeof (uintptr_t);

    for (unsigned int i = 0; i < stack_words; i++)
    {
        if (stackptr[i] != DEADBEEF)
        {
            usage = (stack_words - i) * 100 / stack_words;
            break;
        }
    }

    return usage;
}

#if NUM_CORES > 1
/*---------------------------------------------------------------------------
 * Returns the maximum percentage of the core's idle stack ever used during
 * runtime.
 *---------------------------------------------------------------------------
 */
int core_get_debug_info(unsigned int core, struct core_debug_info *infop)
{
    extern uintptr_t * const idle_stacks[NUM_CORES];

    if (core >= NUM_CORES || !infop)
        return -1;

    infop->idle_stack_usage = stack_usage(idle_stacks[core], IDLE_STACK_SIZE);
    return 1;
}
#endif /* NUM_CORES > 1 */

int thread_get_debug_info(unsigned int thread_id,
                          struct thread_debug_info *infop)
{
    static const char status_chars[THREAD_NUM_STATES+1] =
    {
        [0 ... THREAD_NUM_STATES] = '?',
        [STATE_RUNNING]           = 'R',
        [STATE_BLOCKED]           = 'B',
        [STATE_SLEEPING]          = 'S',
        [STATE_BLOCKED_W_TMO]     = 'T',
        [STATE_FROZEN]            = 'F',
        [STATE_KILLED]            = 'K',
    };

    if (!infop)
        return -1;

    unsigned int slot = THREAD_ID_SLOT(thread_id);
    if (slot >= MAXTHREADS)
        return -1;

    extern struct thread_entry threads[MAXTHREADS];
    struct thread_entry *thread = &threads[slot];

    int oldlevel = disable_irq_save();
    LOCK_THREAD(thread);

    unsigned int state = thread->state;

    if (state != STATE_KILLED)
    {
        const char *name = thread->name;
        if (!name)
            name = "";

        bool cpu_boost = false;
#ifdef HAVE_SCHEDULER_BOOSTCTRL
        cpu_boost = thread->cpu_boost;
#endif
        infop->stack_usage = stack_usage(thread->stack, thread->stack_size);
#if NUM_CORES > 1
        infop->core = thread->core;
#endif
#ifdef HAVE_PRIORITY_SCHEDULING
        infop->base_priority = thread->base_priority;
        infop->current_priority = thread->priority;
#endif

        snprintf(infop->statusstr, sizeof (infop->statusstr), "%c%c",
                 cpu_boost ? '+' : (state == STATE_RUNNING ? '*' : ' '),
                 status_chars[state]);

        const char *fmt = *name ? "%s" : "%s%08lX";
        snprintf(infop->name, sizeof (infop->name), fmt, name,
                 thread->id);
    }

    UNLOCK_THREAD(thread);
    restore_irq(oldlevel);

    return state == STATE_KILLED ? 0 : 1;
}
