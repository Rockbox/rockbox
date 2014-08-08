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
#include "kernel-internal.h"
#include "system.h"

/* Unless otherwise defined, do nothing */
#ifndef YIELD_KERNEL_HOOK
#define YIELD_KERNEL_HOOK() false
#endif
#ifndef SLEEP_KERNEL_HOOK
#define SLEEP_KERNEL_HOOK(ticks) false
#endif

const char __main_thread_name_str[] = "main";

/* Array indexing is more efficient in inlines if the elements are a native
   word size (100s of bytes fewer instructions) */

#if NUM_CORES > 1
static struct core_entry __core_entries[NUM_CORES] IBSS_ATTR;
struct core_entry *__cores[NUM_CORES] IBSS_ATTR;
#else
struct core_entry __cores[NUM_CORES] IBSS_ATTR;
#endif

static struct thread_entry __thread_entries[MAXTHREADS] IBSS_ATTR;
struct thread_entry *__threads[MAXTHREADS] IBSS_ATTR;


/** Internal functions **/

/*---------------------------------------------------------------------------
 * Find an empty thread slot or NULL if none found. The slot returned will
 * be locked on multicore.
 *---------------------------------------------------------------------------
 */
static struct threadalloc
{
    threadbit_t avail;
#if NUM_CORES > 1
    struct corelock cl;
#endif
} threadalloc SHAREDBSS_ATTR;

/*---------------------------------------------------------------------------
 * Initialize the thread allocator
 *---------------------------------------------------------------------------
 */
void thread_alloc_init(void)
{
    corelock_init(&threadalloc.cl);

    for (unsigned int core = 0; core < NUM_CORES; core++)
    {
    #if NUM_CORES > 1
        struct core_entry *c = &__core_entries[core];
        __cores[core] = c;
    #else
        struct core_entry *c = &__cores[core];
    #endif
        rtr_queue_init(&c->rtr);
        corelock_init(&c->rtr_cl);
        tmo_queue_init(&c->tmo);
        c->next_tmo_check = current_tick; /* Something not in the past */
    }

    for (unsigned int slotnum = 0; slotnum < MAXTHREADS; slotnum++)
    {
        struct thread_entry *t = &__thread_entries[slotnum];
        __threads[slotnum] = t;
        corelock_init(&t->waiter_cl);
        corelock_init(&t->slot_cl);
        t->id = THREAD_ID_INIT(slotnum);
        threadbit_set_bit(&threadalloc.avail, slotnum);
    }
}

/*---------------------------------------------------------------------------
 * Allocate a thread alot 
 *---------------------------------------------------------------------------
 */
struct thread_entry * thread_alloc(void)
{
    struct thread_entry *thread = NULL;

    corelock_lock(&threadalloc.cl);

    unsigned int slotnum = threadbit_ffs(&threadalloc.avail);
    if (slotnum < MAXTHREADS)
    {
        threadbit_clear_bit(&threadalloc.avail, slotnum);
        thread = __threads[slotnum];
    }

    corelock_unlock(&threadalloc.cl);

    return thread;
}

/*---------------------------------------------------------------------------
 * Free the thread slot of 'thread'
 *---------------------------------------------------------------------------
 */
void thread_free(struct thread_entry *thread)
{
    corelock_lock(&threadalloc.cl);
    threadbit_set_bit(&threadalloc.avail, THREAD_ID_SLOT(thread->id));
    corelock_unlock(&threadalloc.cl);
}

/*---------------------------------------------------------------------------
 * Assign the thread slot a new ID. Version is 0x00000100..0xffffff00.
 *---------------------------------------------------------------------------
 */
void new_thread_id(struct thread_entry *thread)
{
    uint32_t id = thread->id + (1u << THREAD_ID_VERSION_SHIFT);

    /* If wrapped to 0, make it 1 */
    if ((id & THREAD_ID_VERSION_MASK) == 0)
        id |= (1u << THREAD_ID_VERSION_SHIFT);

    thread->id = id;
}

/*---------------------------------------------------------------------------
 * Wakeup an entire queue of threads - returns bitwise-or of return bitmask
 * from each operation or THREAD_NONE of nothing was awakened.
 *---------------------------------------------------------------------------
 */
unsigned int wait_queue_wake(struct __wait_queue *wqp)
{
    unsigned result = THREAD_NONE;
    struct thread_entry *thread;

    while ((thread = WQ_THREAD_FIRST(wqp)))
        result |= wakeup_thread(thread, WAKEUP_DEFAULT);

    return result;
}


/** Public functions **/

#ifdef RB_PROFILE
void profile_thread(void)
{
    profstart(THREAD_ID_SLOT(__running_self_entry()->id));
}
#endif

/*---------------------------------------------------------------------------
 * Return the thread id of the calling thread
 * --------------------------------------------------------------------------
 */
unsigned int thread_self(void)
{
    return __running_self_entry()->id;
}

/*---------------------------------------------------------------------------
 * Suspends a thread's execution for at least the specified number of ticks.
 *
 * May result in CPU core entering wait-for-interrupt mode if no other thread
 * may be scheduled.
 *
 * NOTE: sleep(0) sleeps until the end of the current tick
 *       sleep(n) that doesn't result in rescheduling:
 *                      n <= ticks suspended < n + 1
 *       n to n+1 is a lower bound. Other factors may affect the actual time
 *       a thread is suspended before it runs again.
 *---------------------------------------------------------------------------
 */
unsigned sleep(unsigned ticks)
{
    /* In certain situations, certain bootloaders in particular, a normal
     * threading call is inappropriate. */
    if (SLEEP_KERNEL_HOOK(ticks))
        return 0; /* Handled */

    disable_irq();
    sleep_thread(ticks);
    switch_thread();
    return 0;
}

/*---------------------------------------------------------------------------
 * Elects another thread to run or, if no other thread may be made ready to
 * run, immediately returns control back to the calling thread.
 *---------------------------------------------------------------------------
 */
void yield(void)
{
    /* In certain situations, certain bootloaders in particular, a normal
     * threading call is inappropriate. */
    if (YIELD_KERNEL_HOOK())
        return; /* Handled */

    switch_thread();
}


/** Debug screen stuff **/

void format_thread_name(char *buf, size_t bufsize,
                        const struct thread_entry *thread)
{
    const char *name = thread->name;
    if (!name)
        name = "";

    const char *fmt = *name ? "%s" : "%s%08lX";
    snprintf(buf, bufsize, fmt, name, thread->id);
}

#ifndef HAVE_SDL_THREADS
/*---------------------------------------------------------------------------
 * Returns the maximum percentage of the stack ever used during runtime.
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
#endif /* HAVE_SDL_THREADS */

#if NUM_CORES > 1
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

    unsigned int slotnum = THREAD_ID_SLOT(thread_id);
    if (slotnum >= MAXTHREADS)
        return -1;

    struct thread_entry *thread = __thread_slot_entry(slotnum);

    int oldlevel = disable_irq_save();
    corelock_lock(&threadalloc.cl);
    corelock_lock(&thread->slot_cl);

    unsigned int state = thread->state;

    int ret = 0;

    if (threadbit_test_bit(&threadalloc.avail, slotnum) == 0)
    {
        bool cpu_boost = false;
#ifdef HAVE_SCHEDULER_BOOSTCTRL
        cpu_boost = thread->cpu_boost;
#endif
#ifndef HAVE_SDL_THREADS
        infop->stack_usage = stack_usage(thread->stack, thread->stack_size);
#endif
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

        format_thread_name(infop->name, sizeof (infop->name), thread);
        ret = 1;
    }

    corelock_unlock(&thread->slot_cl);
    corelock_unlock(&threadalloc.cl);
    restore_irq(oldlevel);

    return ret;
}
