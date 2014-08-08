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
#include "config.h"

#ifdef HAVE_SIGALTSTACK_THREADS
/*
 * The sp check in glibc __longjmp_chk() will cause
 * a fatal error when switching threads via longjmp().
 */
#undef _FORTIFY_SOURCE
#endif

#include "thread-internal.h"
#include "kernel.h"
#include "cpu.h"
#include "string.h"
#ifdef RB_PROFILE
#include <profile.h>
#endif
#include "core_alloc.h"

/****************************************************************************
 *                              ATTENTION!!                                 *
 *    See notes below on implementing processor-specific portions!          *
 ***************************************************************************/

/* Define THREAD_EXTRA_CHECKS as 1 to enable additional state checks */
#ifdef DEBUG
#define THREAD_EXTRA_CHECKS 1 /* Always 1 for DEBUG */
#else
#define THREAD_EXTRA_CHECKS 0
#endif

/**
 * General locking order to guarantee progress. Order must be observed but
 * all stages are not nescessarily obligatory. Going from 1) to 3) is
 * perfectly legal.
 *
 * 1) IRQ
 * This is first because of the likelyhood of having an interrupt occur that
 * also accesses one of the objects farther down the list. Any non-blocking
 * synchronization done may already have a lock on something during normal
 * execution and if an interrupt handler running on the same processor as
 * the one that has the resource locked were to attempt to access the
 * resource, the interrupt handler would wait forever waiting for an unlock
 * that will never happen. There is no danger if the interrupt occurs on
 * a different processor because the one that has the lock will eventually
 * unlock and the other processor's handler may proceed at that time. Not
 * nescessary when the resource in question is definitely not available to
 * interrupt handlers.
 *  
 * 2) Kernel Object
 * 1) May be needed beforehand if the kernel object allows dual-use such as
 * event queues. The kernel object must have a scheme to protect itself from
 * access by another processor and is responsible for serializing the calls
 * to block_thread(_w_tmo) and wakeup_thread both to themselves and to each
 * other. Objects' queues are also protected here.
 * 
 * 3) Thread Slot
 * This locks access to the thread's slot such that its state cannot be
 * altered by another processor when a state change is in progress such as
 * when it is in the process of going on a blocked list. An attempt to wake
 * a thread while it is still blocking will likely desync its state with
 * the other resources used for that state.
 *
 * 4) Core Lists
 * These lists are specific to a particular processor core and are accessible
 * by all processor cores and interrupt handlers. The running (rtr) list is
 * the prime example where a thread may be added by any means.
 */

/*---------------------------------------------------------------------------
 * Processor specific: core_sleep/core_wake/misc. notes
 *
 * ARM notes:
 * FIQ is not dealt with by the scheduler code and is simply restored if it
 * must by masked for some reason - because threading modifies a register
 * that FIQ may also modify and there's no way to accomplish it atomically.
 * s3c2440 is such a case.
 *
 * Audio interrupts are generally treated at a higher priority than others
 * usage of scheduler code with interrupts higher than HIGHEST_IRQ_LEVEL
 * are not in general safe. Special cases may be constructed on a per-
 * source basis and blocking operations are not available.
 *
 * core_sleep procedure to implement for any CPU to ensure an asychronous
 * wakup never results in requiring a wait until the next tick (up to
 * 10000uS!). May require assembly and careful instruction ordering.
 *
 * 1) On multicore, stay awake if directed to do so by another. If so, goto
 *    step 4.
 * 2) If processor requires, atomically reenable interrupts and perform step
 *    3.
 * 3) Sleep the CPU core. If wakeup itself enables interrupts (stop #0x2000
 *    on Coldfire) goto step 5.
 * 4) Enable interrupts.
 * 5) Exit procedure.
 *
 * core_wake and multprocessor notes for sleep/wake coordination:
 * If possible, to wake up another processor, the forcing of an interrupt on
 * the woken core by the waker core is the easiest way to ensure a non-
 * delayed wake and immediate execution of any woken threads. If that isn't
 * available then some careful non-blocking synchonization is needed (as on
 * PP targets at the moment).
 *---------------------------------------------------------------------------
 */

/* Cast to the the machine pointer size, whose size could be < 4 or > 32
 * (someday :). */
static struct core_entry cores[NUM_CORES] IBSS_ATTR;
struct thread_entry threads[MAXTHREADS] IBSS_ATTR;

static const char main_thread_name[] = "main";
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
extern uintptr_t stackbegin[];
extern uintptr_t stackend[];
#else
extern uintptr_t *stackbegin;
extern uintptr_t *stackend;
#endif

static inline void core_sleep(IF_COP_VOID(unsigned int core))
        __attribute__((always_inline));

void check_tmo_threads(void)
        __attribute__((noinline));

static inline void block_thread_on_l(struct thread_entry *thread, unsigned state)
        __attribute__((always_inline));

static void add_to_list_tmo(struct thread_entry *thread)
        __attribute__((noinline));

static void core_schedule_wakeup(struct thread_entry *thread)
        __attribute__((noinline));

#if NUM_CORES > 1
static inline void run_blocking_ops(
    unsigned int core, struct thread_entry *thread)
        __attribute__((always_inline));
#endif

static void thread_stkov(struct thread_entry *thread)
        __attribute__((noinline));

static inline void store_context(void* addr)
        __attribute__((always_inline));

static inline void load_context(const void* addr)
        __attribute__((always_inline));

#if NUM_CORES > 1
static void thread_final_exit_do(struct thread_entry *current)
    __attribute__((noinline)) NORETURN_ATTR USED_ATTR;
#else
static inline void thread_final_exit(struct thread_entry *current)
    __attribute__((always_inline)) NORETURN_ATTR;
#endif

void switch_thread(void)
        __attribute__((noinline));

/****************************************************************************
 * Processor/OS-specific section - include necessary core support
 */


#include "asm/thread.c"

#if defined (CPU_PP)
#include "thread-pp.c"
#endif /* CPU_PP */

#ifndef IF_NO_SKIP_YIELD
#define IF_NO_SKIP_YIELD(...)
#endif

/*
 * End Processor-specific section
 ***************************************************************************/

static NO_INLINE
  void thread_panicf(const char *msg, struct thread_entry *thread)
{
    IF_COP( const unsigned int core = thread->core; )
    static char namebuf[sizeof (((struct thread_debug_info *)0)->name)];
    const char *name = thread->name;
    if (!name)
        name = "";
    snprintf(namebuf, sizeof (namebuf), *name ? "%s" : "%s%08lX",
             name, (unsigned long)thread->id);
    panicf ("%s %s" IF_COP(" (%d)"), msg, name IF_COP(, core));
}

static void thread_stkov(struct thread_entry *thread)
{
    thread_panicf("Stkov", thread);
}

#if THREAD_EXTRA_CHECKS
#define THREAD_PANICF(msg, thread) \
    thread_panicf(msg, thread)
#define THREAD_ASSERT(exp, msg, thread) \
    ({ if (!({ exp; })) thread_panicf((msg), (thread)); })
#else
#define THREAD_PANICF(msg, thread) \
    do {} while (0)
#define THREAD_ASSERT(exp, msg, thread) \
    do {} while (0)
#endif /* THREAD_EXTRA_CHECKS */

/* RTR list */
#define RTR_LOCK(core) \
    ({ corelock_lock(&cores[core].rtr_cl); })
#define RTR_UNLOCK(core) \
    ({ corelock_unlock(&cores[core].rtr_cl); })

#ifdef HAVE_PRIORITY_SCHEDULING
#define rtr_add_entry(core, priority) \
    prio_add_entry(&cores[core].rtr, (priority))

#define rtr_subtract_entry(core, priority) \
    prio_subtract_entry(&cores[core].rtr, (priority))

#define rtr_move_entry(core, from, to) \
    prio_move_entry(&cores[core].rtr, (from), (to))
#else
#define rtr_add_entry(core, priority)
#define rtr_add_entry_inl(core, priority)
#define rtr_subtract_entry(core, priority)
#define rtr_subtract_entry_inl(core, priotity)
#define rtr_move_entry(core, from, to)
#define rtr_move_entry_inl(core, from, to)
#endif

static inline void thread_store_context(struct thread_entry *thread)
{
#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
    thread->__errno = errno;
#endif
    store_context(&thread->context);
}

static inline void thread_load_context(struct thread_entry *thread)
{
    load_context(&thread->context);
#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
    errno = thread->__errno;
#endif
}

static inline unsigned int should_switch_tasks(void)
{
    unsigned int result = THREAD_OK;

#ifdef HAVE_PRIORITY_SCHEDULING
    struct thread_entry *current = cores[CURRENT_CORE].running;
    if (current &&
        priobit_ffs(&cores[IF_COP_CORE(current->core)].rtr.mask)
            < current->priority)
    {
        /* There is a thread ready to run of higher priority on the same
         * core as the current one; recommend a task switch. */
        result |= THREAD_SWITCH;
    }
#endif /* HAVE_PRIORITY_SCHEDULING */

    return result;
}

#ifdef HAVE_PRIORITY_SCHEDULING
/*---------------------------------------------------------------------------
 * Locks the thread registered as the owner of the block and makes sure it
 * didn't change in the meantime
 *---------------------------------------------------------------------------
 */
#if NUM_CORES == 1
static inline struct thread_entry * lock_blocker_thread(struct blocker *bl)
{
    return bl->thread;
}
#else /* NUM_CORES > 1 */
static struct thread_entry * lock_blocker_thread(struct blocker *bl)
{
    /* The blocker thread may change during the process of trying to
       capture it */
    while (1)
    {
        struct thread_entry *t = bl->thread;

        /* TRY, or else deadlocks are possible */
        if (!t)
        {
            struct blocker_splay *blsplay = (struct blocker_splay *)bl;
            if (corelock_try_lock(&blsplay->cl))
            {
                if (!bl->thread)
                    return NULL;    /* Still multi */

                corelock_unlock(&blsplay->cl);
            }
        }
        else
        {
            if (TRY_LOCK_THREAD(t))
            {
                if (bl->thread == t)
                    return t;

                UNLOCK_THREAD(t);
            }
        }
    }
}
#endif /* NUM_CORES */

static inline void unlock_blocker_thread(struct blocker *bl)
{
#if NUM_CORES > 1
    struct thread_entry *blt = bl->thread;
    if (blt)
        UNLOCK_THREAD(blt);
    else
        corelock_unlock(&((struct blocker_splay *)bl)->cl);
#endif /* NUM_CORES > 1*/
    (void)bl;
}
#endif /* HAVE_PRIORITY_SCHEDULING */

/*---------------------------------------------------------------------------
 * Thread list structure - circular:
 *    +------------------------------+
 *    |                              |
 *    +--+---+<-+---+<-+---+<-+---+<-+
 * Head->| T |  | T |  | T |  | T |
 *    +->+---+->+---+->+---+->+---+--+
 *    |                              |
 *    +------------------------------+
 *---------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------
 * Adds a thread to a list of threads using "insert last". Uses the "l"
 * links.
 *---------------------------------------------------------------------------
 */
static void add_to_list_l(struct thread_entry **list,
                          struct thread_entry *thread)
{
    struct thread_entry *l = *list;

    if (l == NULL)
    {
        /* Insert into unoccupied list */
        thread->l.prev = thread;
        thread->l.next = thread;
        *list = thread;
        return;
    }

    /* Insert last */
    thread->l.prev = l->l.prev;
    thread->l.next = l;
    l->l.prev->l.next = thread;
    l->l.prev = thread;
}

/*---------------------------------------------------------------------------
 * Removes a thread from a list of threads. Uses the "l" links.
 *---------------------------------------------------------------------------
 */
static void remove_from_list_l(struct thread_entry **list,
                               struct thread_entry *thread)
{
    struct thread_entry *prev, *next;

    next = thread->l.next;

    if (thread == next)
    {
        /* The only item */
        *list = NULL;
        return;
    }

    if (thread == *list)
    {
        /* List becomes next item */
        *list = next;
    }

    prev = thread->l.prev;
    
    /* Fix links to jump over the removed entry. */
    next->l.prev = prev;
    prev->l.next = next;
}

/*---------------------------------------------------------------------------
 * Timeout list structure - circular reverse (to make "remove item" O(1)),
 * NULL-terminated forward (to ease the far more common forward traversal):
 *    +------------------------------+
 *    |                              |
 *    +--+---+<-+---+<-+---+<-+---+<-+
 * Head->| T |  | T |  | T |  | T |
 *       +---+->+---+->+---+->+---+-X
 *---------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------
 * Add a thread from the core's timout list by linking the pointers in its
 * tmo structure.
 *---------------------------------------------------------------------------
 */
static void add_to_list_tmo(struct thread_entry *thread)
{
    struct thread_entry *tmo = cores[IF_COP_CORE(thread->core)].timeout;
    THREAD_ASSERT(thread->tmo.prev == NULL,
                  "add_to_list_tmo->already listed", thread);

    thread->tmo.next = NULL;

    if (tmo == NULL)
    {
        /* Insert into unoccupied list */
        thread->tmo.prev = thread;
        cores[IF_COP_CORE(thread->core)].timeout = thread;
        return;
    }

    /* Insert Last */
    thread->tmo.prev = tmo->tmo.prev;
    tmo->tmo.prev->tmo.next = thread;
    tmo->tmo.prev = thread;
}

/*---------------------------------------------------------------------------
 * Remove a thread from the core's timout list by unlinking the pointers in
 * its tmo structure. Sets thread->tmo.prev to NULL to indicate the timeout
 * is cancelled.
 *---------------------------------------------------------------------------
 */
static void remove_from_list_tmo(struct thread_entry *thread)
{
    struct thread_entry **list = &cores[IF_COP_CORE(thread->core)].timeout;
    struct thread_entry *prev = thread->tmo.prev;
    struct thread_entry *next = thread->tmo.next;

    THREAD_ASSERT(prev != NULL, "remove_from_list_tmo->not listed", thread);

    if (next != NULL)
        next->tmo.prev = prev;

    if (thread == *list)
    {
        /* List becomes next item and empty if next == NULL */
        *list = next;
        /* Mark as unlisted */
        thread->tmo.prev = NULL;
    }
    else
    {
        if (next == NULL)
            (*list)->tmo.prev = prev;
        prev->tmo.next = next;
        /* Mark as unlisted */
        thread->tmo.prev = NULL;
    }
}

#ifdef HAVE_PRIORITY_SCHEDULING
/*---------------------------------------------------------------------------
 * Priority distribution structure (one category for each possible priority):
 *
 *       +----+----+----+ ... +-----+
 * hist: | F0 | F1 | F2 |     | F31 |
 *       +----+----+----+ ... +-----+
 * mask: | b0 | b1 | b2 |     | b31 |
 *       +----+----+----+ ... +-----+
 *
 * F = count of threads at priority category n (frequency)
 * b = bitmask of non-zero priority categories (occupancy)
 *
 *        / if H[n] != 0 : 1
 * b[n] = |
 *        \ else         : 0 
 *
 *---------------------------------------------------------------------------
 * Basic priority inheritance priotocol (PIP):
 *
 * Mn = mutex n, Tn = thread n
 *
 * A lower priority thread inherits the priority of the highest priority
 * thread blocked waiting for it to complete an action (such as release a
 * mutex or respond to a message via queue_send):
 *
 * 1) T2->M1->T1
 *
 * T1 owns M1, T2 is waiting for M1 to realease M1. If T2 has a higher
 * priority than T1 then T1 inherits the priority of T2.
 *
 * 2) T3
 *    \/
 *    T2->M1->T1
 *
 * Situation is like 1) but T2 and T3 are both queued waiting for M1 and so
 * T1 inherits the higher of T2 and T3.
 *
 * 3) T3->M2->T2->M1->T1
 *
 * T1 owns M1, T2 owns M2. If T3 has a higher priority than both T1 and T2,
 * then T1 inherits the priority of T3 through T2.
 *
 * Blocking chains can grow arbitrarily complex (though it's best that they
 * not form at all very often :) and build-up from these units.
 *---------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------
 * Increment frequency at category "priority"
 *---------------------------------------------------------------------------
 */
static inline unsigned int prio_add_entry(
    struct priority_distribution *pd, int priority)
{
    unsigned int count = ++pd->hist[priority];
    if (count == 1)
        priobit_set_bit(&pd->mask, priority);
    return count;
}

/*---------------------------------------------------------------------------
 * Decrement frequency at category "priority"
 *---------------------------------------------------------------------------
 */
static inline unsigned int prio_subtract_entry(
    struct priority_distribution *pd, int priority)
{
    unsigned int count = --pd->hist[priority];
    if (count == 0)
        priobit_clear_bit(&pd->mask, priority);
    return count;
}

/*---------------------------------------------------------------------------
 * Remove from one category and add to another
 *---------------------------------------------------------------------------
 */
static inline void prio_move_entry(
    struct priority_distribution *pd, int from, int to)
{
    if (--pd->hist[from] == 0)
        priobit_clear_bit(&pd->mask, from);

    if (++pd->hist[to] == 1)
        priobit_set_bit(&pd->mask, to);
}
#endif /* HAVE_PRIORITY_SCHEDULING */

/*---------------------------------------------------------------------------
 * Move a thread back to a running state on its core.
 *---------------------------------------------------------------------------
 */
static void core_schedule_wakeup(struct thread_entry *thread)
{
    const unsigned int core = IF_COP_CORE(thread->core);

    RTR_LOCK(core);

    thread->state = STATE_RUNNING;

    add_to_list_l(&cores[core].running, thread);
    rtr_add_entry(core, thread->priority);

    RTR_UNLOCK(core);

#if NUM_CORES > 1
    if (core != CURRENT_CORE)
        core_wake(core);
#endif
}

#ifdef HAVE_PRIORITY_SCHEDULING
/*---------------------------------------------------------------------------
 * Change the priority and rtr entry for a running thread
 *---------------------------------------------------------------------------
 */
static inline void set_running_thread_priority(
    struct thread_entry *thread, int priority)
{
    const unsigned int core = IF_COP_CORE(thread->core);
    RTR_LOCK(core);
    rtr_move_entry(core, thread->priority, priority);
    thread->priority = priority;
    RTR_UNLOCK(core);
}

/*---------------------------------------------------------------------------
 * Finds the highest priority thread in a list of threads. If the list is
 * empty, the PRIORITY_IDLE is returned.
 *
 * It is possible to use the struct priority_distribution within an object
 * instead of scanning the remaining threads in the list but as a compromise,
 * the resulting per-object memory overhead is saved at a slight speed
 * penalty under high contention.
 *---------------------------------------------------------------------------
 */
static int find_highest_priority_in_list_l(
    struct thread_entry * const thread)
{
    if (LIKELY(thread != NULL))
    {
        /* Go though list until the ending up at the initial thread */
        int highest_priority = thread->priority;
        struct thread_entry *curr = thread;

        do
        {
            int priority = curr->priority;

            if (priority < highest_priority)
                highest_priority = priority;

            curr = curr->l.next;
        }
        while (curr != thread);

        return highest_priority;
    }

    return PRIORITY_IDLE;
}

/*---------------------------------------------------------------------------
 * Register priority with blocking system and bubble it down the chain if
 * any until we reach the end or something is already equal or higher.
 *
 * NOTE: A simultaneous circular wait could spin deadlock on multiprocessor
 * targets but that same action also guarantees a circular block anyway and
 * those are prevented, right? :-)
 *---------------------------------------------------------------------------
 */
static void inherit_priority(
    struct blocker * const blocker0, struct blocker *bl,
    struct thread_entry *blt, int newblpr)
{
    int oldblpr = bl->priority;

    while (1)
    {
        if (blt == NULL)
        {
            /* Multiple owners */
            struct blocker_splay *blsplay = (struct blocker_splay *)bl;
            
            /* Recurse down the all the branches of this; it's the only way.
               We might meet the same queue several times if more than one of
               these threads is waiting the same queue. That isn't a problem
               for us since we early-terminate, just notable. */
            FOR_EACH_BITARRAY_SET_BIT(&blsplay->mask, slotnum)
            {
                bl->priority = oldblpr; /* To see the change each time */
                blt = &threads[slotnum];
                LOCK_THREAD(blt);
                inherit_priority(blocker0, bl, blt, newblpr);
            }

            corelock_unlock(&blsplay->cl);
            return;
        }

        bl->priority = newblpr;

        /* Update blocker thread inheritance record */
        if (newblpr < PRIORITY_IDLE)
            prio_add_entry(&blt->pdist, newblpr);

        if (oldblpr < PRIORITY_IDLE)
            prio_subtract_entry(&blt->pdist, oldblpr);

        int oldpr = blt->priority;
        int newpr = priobit_ffs(&blt->pdist.mask);
        if (newpr == oldpr)
            break; /* No blocker thread priority change */

        if (blt->state == STATE_RUNNING)
        {
            set_running_thread_priority(blt, newpr);
            break; /* Running: last in chain */
        }

        /* Blocker is blocked */
        blt->priority = newpr;

        bl = blt->blocker;
        if (LIKELY(bl == NULL))
            break; /* Block doesn't support PIP */

        if (UNLIKELY(bl == blocker0))
            break; /* Full circle - deadlock! */

        /* Blocker becomes current thread and the process repeats */
        struct thread_entry **bqp = blt->bqp;
        struct thread_entry *t = blt;
        blt = lock_blocker_thread(bl);

        UNLOCK_THREAD(t);

        /* Adjust this wait queue */
        oldblpr = bl->priority;
        if (newpr <= oldblpr)
            newblpr = newpr;
        else if (oldpr <= oldblpr)
            newblpr = find_highest_priority_in_list_l(*bqp);

        if (newblpr == oldblpr)
            break; /* Queue priority not changing */
    }

    UNLOCK_THREAD(blt);
}

/*---------------------------------------------------------------------------
 * Quick-disinherit of priority elevation. 'thread' must be a running thread.
 *---------------------------------------------------------------------------
 */
static void priority_disinherit_internal(struct thread_entry *thread,
                                         int blpr)
{
    if (blpr < PRIORITY_IDLE &&
        prio_subtract_entry(&thread->pdist, blpr) == 0 &&
        blpr <= thread->priority)
    {
        int priority = priobit_ffs(&thread->pdist.mask);
        if (priority != thread->priority)
            set_running_thread_priority(thread, priority);
    }
}

void priority_disinherit(struct thread_entry *thread, struct blocker *bl)
{
    LOCK_THREAD(thread);
    priority_disinherit_internal(thread, bl->priority);
    UNLOCK_THREAD(thread);
}

/*---------------------------------------------------------------------------
 * Transfer ownership from a single owner to a multi-owner splay from a wait
 * queue
 *---------------------------------------------------------------------------
 */
static void wakeup_thread_queue_multi_transfer(struct thread_entry *thread)
{
    /* All threads will have the same blocker and queue; only we are changing
       it now */
    struct thread_entry **bqp = thread->bqp;
    struct blocker_splay *blsplay = (struct blocker_splay *)thread->blocker;
    struct thread_entry *blt = blsplay->blocker.thread;

    /* The first thread is already locked and is assumed tagged "multi" */
    int count = 1;
    struct thread_entry *temp_queue = NULL;

    /* 'thread' is locked on entry */
    while (1)
    {
        LOCK_THREAD(blt);

        remove_from_list_l(bqp, thread);
        thread->blocker = NULL;

        struct thread_entry *tnext = *bqp;
        if (tnext == NULL || tnext->retval == 0)
            break;

        add_to_list_l(&temp_queue, thread);

        UNLOCK_THREAD(thread);
        UNLOCK_THREAD(blt);

        count++;
        thread = tnext;

        LOCK_THREAD(thread);
    }

    int blpr = blsplay->blocker.priority;
    priority_disinherit_internal(blt, blpr);

    /* Locking order reverses here since the threads are no longer on the
       queue side */
    if (count > 1)
    {
        add_to_list_l(&temp_queue, thread);
        UNLOCK_THREAD(thread);
        corelock_lock(&blsplay->cl);

        blpr = find_highest_priority_in_list_l(*bqp);
        blsplay->blocker.thread = NULL;

        thread = temp_queue;
        LOCK_THREAD(thread);
    }
    else
    {
        /* Becomes a simple, direct transfer */
        if (thread->priority <= blpr)
            blpr = find_highest_priority_in_list_l(*bqp);
        blsplay->blocker.thread = thread;
    }

    blsplay->blocker.priority = blpr;

    while (1)
    {
        unsigned int slotnum = THREAD_ID_SLOT(thread->id);
        threadbit_set_bit(&blsplay->mask, slotnum);

        if (blpr < PRIORITY_IDLE)
        {
            prio_add_entry(&thread->pdist, blpr);
            if (blpr < thread->priority)
                thread->priority = blpr;
        }

        if (count > 1)
            remove_from_list_l(&temp_queue, thread);

        core_schedule_wakeup(thread);

        UNLOCK_THREAD(thread);

        thread = temp_queue;
        if (thread == NULL)
            break;

        LOCK_THREAD(thread);
    }

    UNLOCK_THREAD(blt);

    if (count > 1)
    {
        corelock_unlock(&blsplay->cl);
    }

    blt->retval = count;
}

/*---------------------------------------------------------------------------
 * Transfer ownership to a thread waiting for an objects and transfer
 * inherited priority boost from other waiters. This algorithm knows that
 * blocking chains may only unblock from the very end.
 *
 * Only the owning thread itself may call this and so the assumption that
 * it is the running thread is made.
 *---------------------------------------------------------------------------
 */
static void wakeup_thread_transfer(struct thread_entry *thread)
{
    /* Waking thread inherits priority boost from object owner (blt) */
    struct blocker *bl = thread->blocker;
    struct thread_entry *blt = bl->thread;

    THREAD_ASSERT(cores[CURRENT_CORE].running == blt,
                  "UPPT->wrong thread", cores[CURRENT_CORE].running);

    LOCK_THREAD(blt);

    struct thread_entry **bqp = thread->bqp;
    remove_from_list_l(bqp, thread);
    thread->blocker = NULL;

    int blpr = bl->priority;

    /* Remove the object's boost from the owning thread */
    if (prio_subtract_entry(&blt->pdist, blpr) == 0 && blpr <= blt->priority)
    {
        /* No more threads at this priority are waiting and the old level is
         * at least the thread level */
        int priority = priobit_ffs(&blt->pdist.mask);
        if (priority != blt->priority)
            set_running_thread_priority(blt, priority);
    }

    struct thread_entry *tnext = *bqp;

    if (LIKELY(tnext == NULL))
    {
        /* Expected shortcut - no more waiters */
        blpr = PRIORITY_IDLE;
    }
    else
    {
        /* If lowering, we need to scan threads remaining in queue */
        int priority = thread->priority;
        if (priority <= blpr)
            blpr = find_highest_priority_in_list_l(tnext);

        if (prio_add_entry(&thread->pdist, blpr) == 1 && blpr < priority)
            thread->priority = blpr; /* Raise new owner */
    }

    core_schedule_wakeup(thread);
    UNLOCK_THREAD(thread);

    bl->thread   = thread;  /* This thread pwns */
    bl->priority = blpr;    /* Save highest blocked priority */
    UNLOCK_THREAD(blt);
}

/*---------------------------------------------------------------------------
 * Readjust priorities when waking a thread blocked waiting for another
 * in essence "releasing" the thread's effect on the object owner. Can be
 * performed from any context.
 *---------------------------------------------------------------------------
 */
static void wakeup_thread_release(struct thread_entry *thread)
{
    struct blocker *bl = thread->blocker;
    struct thread_entry *blt = lock_blocker_thread(bl);
    struct thread_entry **bqp = thread->bqp;
    remove_from_list_l(bqp, thread);
    thread->blocker = NULL;

    /* Off to see the wizard... */
    core_schedule_wakeup(thread);

    if (thread->priority > bl->priority)
    {
        /* Queue priority won't change */
        UNLOCK_THREAD(thread);
        unlock_blocker_thread(bl);
        return;
    }

    UNLOCK_THREAD(thread);

    int newblpr = find_highest_priority_in_list_l(*bqp);
    if (newblpr == bl->priority)
    {
        /* Blocker priority won't change */
        unlock_blocker_thread(bl);
        return;
    }

    inherit_priority(bl, bl, blt, newblpr);
}

#endif /* HAVE_PRIORITY_SCHEDULING */

/*---------------------------------------------------------------------------
 * Explicitly wakeup a thread on a blocking queue. Only effects threads of
 * STATE_BLOCKED and STATE_BLOCKED_W_TMO.
 *
 * This code should be considered a critical section by the caller meaning
 * that the object's corelock should be held.
 *
 * INTERNAL: Intended for use by kernel objects and not for programs.
 *---------------------------------------------------------------------------
 */
unsigned int wakeup_thread_(struct thread_entry **list
                            IF_PRIO(, enum wakeup_thread_protocol proto))
{
    struct thread_entry *thread = *list;

    /* Check if there is a blocked thread at all. */
    if (*list == NULL)
        return THREAD_NONE;

    LOCK_THREAD(thread);

    /* Determine thread's current state. */
    switch (thread->state)
    {
    case STATE_BLOCKED:
    case STATE_BLOCKED_W_TMO:
#ifdef HAVE_PRIORITY_SCHEDULING
        /* Threads with PIP blockers cannot specify "WAKEUP_DEFAULT" */
        if (thread->blocker != NULL)
        {
            static void (* const funcs[])(struct thread_entry *thread)
                ICONST_ATTR =
            {
                [WAKEUP_DEFAULT]        = NULL,
                [WAKEUP_TRANSFER]       = wakeup_thread_transfer,
                [WAKEUP_RELEASE]        = wakeup_thread_release,
                [WAKEUP_TRANSFER_MULTI] = wakeup_thread_queue_multi_transfer,
            };

            /* Call the specified unblocking PIP (does the rest) */
            funcs[proto](thread);
        }
        else
#endif /* HAVE_PRIORITY_SCHEDULING */
        {
            /* No PIP - just boost the thread by aging */
#ifdef HAVE_PRIORITY_SCHEDULING
            IF_NO_SKIP_YIELD( if (thread->skip_count != -1) )
                thread->skip_count = thread->priority;
#endif /* HAVE_PRIORITY_SCHEDULING */
            remove_from_list_l(list, thread);
            core_schedule_wakeup(thread);
            UNLOCK_THREAD(thread);
        }

        return should_switch_tasks();

    /* Nothing to do. State is not blocked. */
    default:
#if THREAD_EXTRA_CHECKS
        THREAD_PANICF("wakeup_thread->block invalid", thread);
    case STATE_RUNNING:
    case STATE_KILLED:
#endif
        UNLOCK_THREAD(thread);
        return THREAD_NONE;
    }
}

/*---------------------------------------------------------------------------
 * Check the core's timeout list when at least one thread is due to wake.
 * Filtering for the condition is done before making the call. Resets the
 * tick when the next check will occur.
 *---------------------------------------------------------------------------
 */
void check_tmo_threads(void)
{
    const unsigned int core = CURRENT_CORE;
    const long tick = current_tick; /* snapshot the current tick */
    long next_tmo_check = tick + 60*HZ; /* minimum duration: once/minute */
    struct thread_entry *next = cores[core].timeout;

    /* If there are no processes waiting for a timeout, just keep the check
       tick from falling into the past. */

    /* Break the loop once we have walked through the list of all
     * sleeping processes or have removed them all. */
    while (next != NULL)
    {
        /* Check sleeping threads. Allow interrupts between checks. */
        enable_irq();

        struct thread_entry *curr = next;

        next = curr->tmo.next;

        /* Lock thread slot against explicit wakeup */
        disable_irq();
        LOCK_THREAD(curr);

        unsigned state = curr->state;

        if (state < TIMEOUT_STATE_FIRST)
        {
            /* Cleanup threads no longer on a timeout but still on the
             * list. */
            remove_from_list_tmo(curr);
        }
        else if (LIKELY(TIME_BEFORE(tick, curr->tmo_tick)))
        {
            /* Timeout still pending - this will be the usual case */
            if (TIME_BEFORE(curr->tmo_tick, next_tmo_check))
            {
                /* Earliest timeout found so far - move the next check up
                   to its time */
                next_tmo_check = curr->tmo_tick;
            }
        }
        else
        {
            /* Sleep timeout has been reached so bring the thread back to
             * life again. */
            if (state == STATE_BLOCKED_W_TMO)
            {
#ifdef HAVE_CORELOCK_OBJECT
                /* Lock the waiting thread's kernel object */
                struct corelock *ocl = curr->obj_cl;

                if (UNLIKELY(corelock_try_lock(ocl) == 0))
                {
                    /* Need to retry in the correct order though the need is
                     * unlikely */
                    UNLOCK_THREAD(curr);
                    corelock_lock(ocl);
                    LOCK_THREAD(curr);

                    if (UNLIKELY(curr->state != STATE_BLOCKED_W_TMO))
                    {
                        /* Thread was woken or removed explicitely while slot
                         * was unlocked */
                        corelock_unlock(ocl);
                        remove_from_list_tmo(curr);
                        UNLOCK_THREAD(curr);
                        continue;
                    }
                }
#endif /* NUM_CORES */

#ifdef HAVE_WAKEUP_EXT_CB
                if (curr->wakeup_ext_cb != NULL)
                    curr->wakeup_ext_cb(curr);
#endif

#ifdef HAVE_PRIORITY_SCHEDULING
                if (curr->blocker != NULL)
                    wakeup_thread_release(curr);
                else
#endif
                    remove_from_list_l(curr->bqp, curr);

                corelock_unlock(ocl);
            }
            /* else state == STATE_SLEEPING */

            remove_from_list_tmo(curr);

            RTR_LOCK(core);

            curr->state = STATE_RUNNING;

            add_to_list_l(&cores[core].running, curr);
            rtr_add_entry(core, curr->priority);

            RTR_UNLOCK(core);
        }

        UNLOCK_THREAD(curr);
    }

    cores[core].next_tmo_check = next_tmo_check;
}

/*---------------------------------------------------------------------------
 * Performs operations that must be done before blocking a thread but after
 * the state is saved.
 *---------------------------------------------------------------------------
 */
#if NUM_CORES > 1
static inline void run_blocking_ops(
    unsigned int core, struct thread_entry *thread)
{
    struct thread_blk_ops *ops = &cores[core].blk_ops;
    const unsigned flags = ops->flags;

    if (LIKELY(flags == TBOP_CLEAR))
        return;

    switch (flags)
    {
    case TBOP_SWITCH_CORE:
        core_switch_blk_op(core, thread);
        /* Fall-through */
    case TBOP_UNLOCK_CORELOCK:
        corelock_unlock(ops->cl_p);
        break;
    }

    ops->flags = TBOP_CLEAR;
}
#endif /* NUM_CORES > 1 */

#ifdef RB_PROFILE
void profile_thread(void)
{
    profstart(cores[CURRENT_CORE].running - threads);
}
#endif

/*---------------------------------------------------------------------------
 * Prepares a thread to block on an object's list and/or for a specified
 * duration - expects object and slot to be appropriately locked if needed
 * and interrupts to be masked.
 *---------------------------------------------------------------------------
 */
static inline void block_thread_on_l(struct thread_entry *thread,
                                     unsigned state)
{
    /* If inlined, unreachable branches will be pruned with no size penalty
       because state is passed as a constant parameter. */
    const unsigned int core = IF_COP_CORE(thread->core);

    /* Remove the thread from the list of running threads. */
    RTR_LOCK(core);
    remove_from_list_l(&cores[core].running, thread);
    rtr_subtract_entry(core, thread->priority);
    RTR_UNLOCK(core);

    /* Add a timeout to the block if not infinite */
    switch (state)
    {
    case STATE_BLOCKED:
    case STATE_BLOCKED_W_TMO:
        /* Put the thread into a new list of inactive threads. */
        add_to_list_l(thread->bqp, thread);

        if (state == STATE_BLOCKED)
            break;

        /* Fall-through */
    case STATE_SLEEPING:
        /* If this thread times out sooner than any other thread, update
           next_tmo_check to its timeout */
        if (TIME_BEFORE(thread->tmo_tick, cores[core].next_tmo_check))
        {
            cores[core].next_tmo_check = thread->tmo_tick;
        }

        if (thread->tmo.prev == NULL)
        {
            add_to_list_tmo(thread);
        }
        /* else thread was never removed from list - just keep it there */
        break;
    }

    /* Remember the the next thread about to block. */
    cores[core].block_task = thread;

    /* Report new state. */
    thread->state = state;
}

/*---------------------------------------------------------------------------
 * Switch thread in round robin fashion for any given priority. Any thread
 * that removed itself from the running list first must specify itself in
 * the paramter.
 *
 * INTERNAL: Intended for use by kernel and not for programs.
 *---------------------------------------------------------------------------
 */
void switch_thread(void)
{

    const unsigned int core = CURRENT_CORE;
    struct thread_entry *block = cores[core].block_task;
    struct thread_entry *thread = cores[core].running;

    /* Get context to save - next thread to run is unknown until all wakeups
     * are evaluated */
    if (block != NULL)
    {
        cores[core].block_task = NULL;

#if NUM_CORES > 1
        if (UNLIKELY(thread == block))
        {
            /* This was the last thread running and another core woke us before
             * reaching here. Force next thread selection to give tmo threads or
             * other threads woken before this block a first chance. */
            block = NULL;
        }
        else
#endif
        {
            /* Blocking task is the old one */
            thread = block;
        }
    }

#ifdef RB_PROFILE
#ifdef CPU_COLDFIRE
    _profile_thread_stopped(thread->id & THREAD_ID_SLOT_MASK);
#else
    profile_thread_stopped(thread->id & THREAD_ID_SLOT_MASK);
#endif
#endif

    /* Begin task switching by saving our current context so that we can
     * restore the state of the current thread later to the point prior
     * to this call. */
    thread_store_context(thread);
#ifdef DEBUG
    /* Check core_ctx buflib integrity */
    core_check_valid();
#endif

    /* Check if the current thread stack is overflown */
    if (UNLIKELY(thread->stack[0] != DEADBEEF) && thread->stack_size > 0)
        thread_stkov(thread);

#if NUM_CORES > 1
    /* Run any blocking operations requested before switching/sleeping */
    run_blocking_ops(core, thread);
#endif

#ifdef HAVE_PRIORITY_SCHEDULING
    IF_NO_SKIP_YIELD( if (thread->skip_count != -1) )
    /* Reset the value of thread's skip count */
        thread->skip_count = 0;
#endif

    for (;;)
    {
        /* If there are threads on a timeout and the earliest wakeup is due,
         * check the list and wake any threads that need to start running
         * again. */
        if (!TIME_BEFORE(current_tick, cores[core].next_tmo_check))
        {
            check_tmo_threads();
        }

        disable_irq();
        RTR_LOCK(core);

        thread = cores[core].running;

        if (UNLIKELY(thread == NULL))
        {
            /* Enter sleep mode to reduce power usage - woken up on interrupt
             * or wakeup request from another core - expected to enable
             * interrupts. */
            RTR_UNLOCK(core);
            core_sleep(IF_COP(core));
        }
        else
        {
#ifdef HAVE_PRIORITY_SCHEDULING
            /* Select the new task based on priorities and the last time a
             * process got CPU time relative to the highest priority runnable
             * task. */
            int max = priobit_ffs(&cores[core].rtr.mask);

            if (block == NULL)
            {
                /* Not switching on a block, tentatively select next thread */
                thread = thread->l.next;
            }

            for (;;)
            {
                int priority = thread->priority;
                int diff;

                /* This ridiculously simple method of aging seems to work
                 * suspiciously well. It does tend to reward CPU hogs (under
                 * yielding) but that's generally not desirable at all. On
                 * the plus side, it, relatively to other threads, penalizes
                 * excess yielding which is good if some high priority thread
                 * is performing no useful work such as polling for a device
                 * to be ready. Of course, aging is only employed when higher
                 * and lower priority threads are runnable. The highest
                 * priority runnable thread(s) are never skipped unless a
                 * lower-priority process has aged sufficiently. Priorities
                 * of REALTIME class are run strictly according to priority
                 * thus are not subject to switchout due to lower-priority
                 * processes aging; they must give up the processor by going
                 * off the run list. */
                if (LIKELY(priority <= max) ||
                    IF_NO_SKIP_YIELD( thread->skip_count == -1 || )
                    (priority > PRIORITY_REALTIME &&
                     (diff = priority - max,
                         ++thread->skip_count > diff*diff)))
                {
                    cores[core].running = thread;
                    break;
                }

                thread = thread->l.next;
            }
#else
            /* Without priority use a simple FCFS algorithm */
            if (block == NULL)
            {
                /* Not switching on a block, select next thread */
                thread = thread->l.next;
                cores[core].running = thread;
            }
#endif /* HAVE_PRIORITY_SCHEDULING */

            RTR_UNLOCK(core);
            enable_irq();
            break;
        }
    }

    /* And finally give control to the next thread. */
    thread_load_context(thread);

#ifdef RB_PROFILE
    profile_thread_started(thread->id & THREAD_ID_SLOT_MASK);
#endif

}

/*---------------------------------------------------------------------------
 * Sleeps a thread for at least a specified number of ticks with zero being
 * a wait until the next tick.
 *
 * INTERNAL: Intended for use by kernel and not for programs.
 *---------------------------------------------------------------------------
 */
void sleep_thread(int ticks)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;

    LOCK_THREAD(current);

    /* Set our timeout, remove from run list and join timeout list. */
    current->tmo_tick = current_tick + MAX(ticks, 0) + 1;
    block_thread_on_l(current, STATE_SLEEPING);

    UNLOCK_THREAD(current);
}

/*---------------------------------------------------------------------------
 * Block a thread on a blocking queue for explicit wakeup. If timeout is
 * negative, the block is infinite.
 *
 * INTERNAL: Intended for use by kernel objects and not for programs.
 *---------------------------------------------------------------------------
 */
void block_thread(struct thread_entry *current, int timeout)
{
    LOCK_THREAD(current);

    struct blocker *bl = NULL;
#ifdef HAVE_PRIORITY_SCHEDULING
    bl = current->blocker;
    struct thread_entry *blt = bl ? lock_blocker_thread(bl) : NULL;
#endif /* HAVE_PRIORITY_SCHEDULING */

    if (LIKELY(timeout < 0))
    {
        /* Block until explicitly woken */
        block_thread_on_l(current, STATE_BLOCKED);
    }
    else
    {
        /* Set the state to blocked with the specified timeout */
        current->tmo_tick = current_tick + timeout;
        block_thread_on_l(current, STATE_BLOCKED_W_TMO);
    }

    if (bl == NULL)
    {
        UNLOCK_THREAD(current);
        return;
    }

#ifdef HAVE_PRIORITY_SCHEDULING
    int newblpr = current->priority;
    UNLOCK_THREAD(current);

    if (newblpr >= bl->priority)
    {
        unlock_blocker_thread(bl);
        return; /* Queue priority won't change */
    }

    inherit_priority(bl, bl, blt, newblpr);
#endif /* HAVE_PRIORITY_SCHEDULING */
}

/*---------------------------------------------------------------------------
 * Assign the thread slot a new ID. Version is 0x00000100..0xffffff00.
 *---------------------------------------------------------------------------
 */
static void new_thread_id(unsigned int slot_num,
                          struct thread_entry *thread)
{
    unsigned int version =
        (thread->id + (1u << THREAD_ID_VERSION_SHIFT))
                & THREAD_ID_VERSION_MASK;

    /* If wrapped to 0, make it 1 */
    if (version == 0)
        version = 1u << THREAD_ID_VERSION_SHIFT;

    thread->id = version | (slot_num & THREAD_ID_SLOT_MASK);
}

/*---------------------------------------------------------------------------
 * Find an empty thread slot or MAXTHREADS if none found. The slot returned
 * will be locked on multicore.
 *---------------------------------------------------------------------------
 */
static struct thread_entry * find_empty_thread_slot(void)
{
    /* Any slot could be on an interrupt-accessible list */
    IF_COP( int oldlevel = disable_irq_save(); )
    struct thread_entry *thread = NULL;
    int n;

    for (n = 0; n < MAXTHREADS; n++)
    {
        /* Obtain current slot state - lock it on multicore */
        struct thread_entry *t = &threads[n];
        LOCK_THREAD(t);

        if (t->state == STATE_KILLED)
        {
            /* Slot is empty - leave it locked and caller will unlock */
            thread = t;
            break;
        }

        /* Finished examining slot - no longer busy - unlock on multicore */
        UNLOCK_THREAD(t);
    }

    IF_COP( restore_irq(oldlevel); ) /* Reenable interrups - this slot is
                                          not accesible to them yet */
    return thread;
}

/*---------------------------------------------------------------------------
 * Return the thread_entry pointer for a thread_id. Return the current
 * thread if the ID is (unsigned int)-1 (alias for current).
 *---------------------------------------------------------------------------
 */
struct thread_entry * thread_id_entry(unsigned int thread_id)
{
    return &threads[thread_id & THREAD_ID_SLOT_MASK];
}

/*---------------------------------------------------------------------------
 * Return the thread id of the calling thread
 * --------------------------------------------------------------------------
 */
unsigned int thread_self(void)
{
    return cores[CURRENT_CORE].running->id;
}

/*---------------------------------------------------------------------------
 * Return the thread entry of the calling thread.
 *
 * INTERNAL: Intended for use by kernel and not for programs.
 *---------------------------------------------------------------------------
 */
struct thread_entry* thread_self_entry(void)
{
    return cores[CURRENT_CORE].running;
}

/*---------------------------------------------------------------------------
 * Place the current core in idle mode - woken up on interrupt or wake
 * request from another core.
 *---------------------------------------------------------------------------
 */
void core_idle(void)
{
    IF_COP( const unsigned int core = CURRENT_CORE; )
    disable_irq();
    core_sleep(IF_COP(core));
}

/*---------------------------------------------------------------------------
 * Create a thread. If using a dual core architecture, specify which core to
 * start the thread on.
 *
 * Return ID if context area could be allocated, else NULL.
 *---------------------------------------------------------------------------
 */
unsigned int create_thread(void (*function)(void),
                           void* stack, size_t stack_size,
                           unsigned flags, const char *name
                           IF_PRIO(, int priority)
                           IF_COP(, unsigned int core))
{
    unsigned int i;
    unsigned int stack_words;
    uintptr_t stackptr, stackend;
    struct thread_entry *thread;
    unsigned state;
    int oldlevel;

    thread = find_empty_thread_slot();
    if (thread == NULL)
    {
        return 0;
    }

    oldlevel = disable_irq_save();

    /* Munge the stack to make it easy to spot stack overflows */
    stackptr = ALIGN_UP((uintptr_t)stack, sizeof (uintptr_t));
    stackend = ALIGN_DOWN((uintptr_t)stack + stack_size, sizeof (uintptr_t));
    stack_size = stackend - stackptr;
    stack_words = stack_size / sizeof (uintptr_t);

    for (i = 0; i < stack_words; i++)
    {
        ((uintptr_t *)stackptr)[i] = DEADBEEF;
    }

    /* Store interesting information */
    thread->name = name;
    thread->stack = (uintptr_t *)stackptr;
    thread->stack_size = stack_size;
    thread->queue = NULL;
#ifdef HAVE_WAKEUP_EXT_CB
    thread->wakeup_ext_cb = NULL;
#endif
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    thread->cpu_boost = 0;
#endif
#ifdef HAVE_PRIORITY_SCHEDULING
    memset(&thread->pdist, 0, sizeof(thread->pdist));
    thread->blocker = NULL;
    thread->base_priority = priority;
    thread->priority = priority;
    thread->skip_count = priority;
    prio_add_entry(&thread->pdist, priority);
#endif

#ifdef HAVE_IO_PRIORITY
    /* Default to high (foreground) priority */
    thread->io_priority = IO_PRIORITY_IMMEDIATE;
#endif

#if NUM_CORES > 1
    thread->core = core;

    /* Writeback stack munging or anything else before starting */
    if (core != CURRENT_CORE)
    {
        commit_dcache();
    }
#endif

    /* Thread is not on any timeout list but be a bit paranoid */
    thread->tmo.prev = NULL;

    state = (flags & CREATE_THREAD_FROZEN) ?
        STATE_FROZEN : STATE_RUNNING;
    
    thread->context.sp = (typeof (thread->context.sp))stackend;

    /* Load the thread's context structure with needed startup information */
    THREAD_STARTUP_INIT(core, thread, function);

    thread->state = state;
    i = thread->id; /* Snapshot while locked */

    if (state == STATE_RUNNING)
        core_schedule_wakeup(thread);

    UNLOCK_THREAD(thread);
    restore_irq(oldlevel);

    return i;
}

#ifdef HAVE_SCHEDULER_BOOSTCTRL
/*---------------------------------------------------------------------------
 * Change the boost state of a thread boosting or unboosting the CPU
 * as required.
 *---------------------------------------------------------------------------
 */
static inline void boost_thread(struct thread_entry *thread, bool boost)
{
    if ((thread->cpu_boost != 0) != boost)
    {
        thread->cpu_boost = boost;
        cpu_boost(boost);
    }
}

void trigger_cpu_boost(void)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    boost_thread(current, true);
}

void cancel_cpu_boost(void)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    boost_thread(current, false);
}
#endif /* HAVE_SCHEDULER_BOOSTCTRL */

/*---------------------------------------------------------------------------
 * Block the current thread until another thread terminates. A thread may
 * wait on itself to terminate which prevents it from running again and it
 * will need to be killed externally.
 * Parameter is the ID as returned from create_thread().
 *---------------------------------------------------------------------------
 */
void thread_wait(unsigned int thread_id)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    struct thread_entry *thread = thread_id_entry(thread_id);

    /* Lock thread-as-waitable-object lock */
    corelock_lock(&thread->waiter_cl);

    /* Be sure it hasn't been killed yet */
    if (thread->id == thread_id && thread->state != STATE_KILLED)
    {
        IF_COP( current->obj_cl = &thread->waiter_cl; )
        current->bqp = &thread->queue;

        disable_irq();
        block_thread(current, TIMEOUT_BLOCK);

        corelock_unlock(&thread->waiter_cl);

        switch_thread();
        return;
    }

    corelock_unlock(&thread->waiter_cl);
}

/*---------------------------------------------------------------------------
 * Exit the current thread. The Right Way to Do Things (TM).
 *---------------------------------------------------------------------------
 */
/* This is done to foil optimizations that may require the current stack,
 * such as optimizing subexpressions that put variables on the stack that
 * get used after switching stacks. */
#if NUM_CORES > 1
/* Called by ASM stub */
static void thread_final_exit_do(struct thread_entry *current)
#else
/* No special procedure is required before calling */
static inline void thread_final_exit(struct thread_entry *current)
#endif
{
    /* At this point, this thread isn't using resources allocated for
     * execution except the slot itself. */

    /* Signal this thread */
    thread_queue_wake(&current->queue);
    corelock_unlock(&current->waiter_cl);
    switch_thread();
    /* This should never and must never be reached - if it is, the
     * state is corrupted */
    THREAD_PANICF("thread_exit->K:*R", current);
    while (1);
}

void thread_exit(void)
{
    register struct thread_entry * current = cores[CURRENT_CORE].running;

    /* Cancel CPU boost if any */
    cancel_cpu_boost();

    disable_irq();

    corelock_lock(&current->waiter_cl);
    LOCK_THREAD(current);

#ifdef HAVE_PRIORITY_SCHEDULING
    /* Only one bit in the mask should be set with a frequency on 1 which
     * represents the thread's own base priority otherwise threads are waiting
     * on an abandoned object */
    if (priobit_popcount(&current->pdist.mask) != 1 ||
        current->pdist.hist[priobit_ffs(&current->pdist.mask)] > 1)
        thread_panicf("abandon ship!", current);
#endif /* HAVE_PRIORITY_SCHEDULING */

    if (current->tmo.prev != NULL)
    {
        /* Cancel pending timeout list removal */
        remove_from_list_tmo(current);
    }

    /* Switch tasks and never return */
    block_thread_on_l(current, STATE_KILLED);

    /* Slot must be unusable until thread is really gone */
    UNLOCK_THREAD_AT_TASK_SWITCH(current);

    /* Update ID for this slot */
    new_thread_id(current->id, current);
    current->name = NULL;

    /* Do final cleanup and remove the thread */
    thread_final_exit(current);
}

#ifdef HAVE_PRIORITY_SCHEDULING
/*---------------------------------------------------------------------------
 * Sets the thread's relative base priority for the core it runs on. Any
 * needed inheritance changes also may happen.
 *---------------------------------------------------------------------------
 */
int thread_set_priority(unsigned int thread_id, int priority)
{
    if (priority < HIGHEST_PRIORITY || priority > LOWEST_PRIORITY)
        return -1; /* Invalid priority argument */

    int old_base_priority = -1;
    struct thread_entry *thread = thread_id_entry(thread_id);

    /* Thread could be on any list and therefore on an interrupt accessible
       one - disable interrupts */
    const int oldlevel = disable_irq_save();
    LOCK_THREAD(thread);

    if (thread->id != thread_id || thread->state == STATE_KILLED)
        goto done; /* Invalid thread */

    old_base_priority = thread->base_priority;
    if (priority == old_base_priority)
        goto done; /* No base priority change */

    thread->base_priority = priority;

    /* Adjust the thread's priority influence on itself */
    prio_move_entry(&thread->pdist, old_base_priority, priority);

    int old_priority = thread->priority;
    int new_priority = priobit_ffs(&thread->pdist.mask);

    if (old_priority == new_priority)
        goto done; /* No running priority change */

    if (thread->state == STATE_RUNNING)
    {
        /* This thread is running - just change location on the run queue.
           Also sets thread->priority. */
        set_running_thread_priority(thread, new_priority);
        goto done;
    }

    /* Thread is blocked */
    struct blocker *bl = thread->blocker;
    if (bl == NULL)
    {
        thread->priority = new_priority;
        goto done; /* End of transitive blocks */
    }

    struct thread_entry *blt = lock_blocker_thread(bl);
    struct thread_entry **bqp = thread->bqp;

    thread->priority = new_priority;

    UNLOCK_THREAD(thread);
    thread = NULL;

    int oldblpr = bl->priority;
    int newblpr = oldblpr;
    if (new_priority < oldblpr)
        newblpr = new_priority;
    else if (old_priority <= oldblpr)
        newblpr = find_highest_priority_in_list_l(*bqp);

    if (newblpr == oldblpr)
    {
        unlock_blocker_thread(bl);
        goto done;
    }

    inherit_priority(bl, bl, blt, newblpr);
done:
    if (thread)
        UNLOCK_THREAD(thread);
    restore_irq(oldlevel);
    return old_base_priority;
}

/*---------------------------------------------------------------------------
 * Returns the current base priority for a thread.
 *---------------------------------------------------------------------------
 */
int thread_get_priority(unsigned int thread_id)
{
    struct thread_entry *thread = thread_id_entry(thread_id);
    int base_priority = thread->base_priority;

    /* Simply check without locking slot. It may or may not be valid by the
     * time the function returns anyway. If all tests pass, it is the
     * correct value for when it was valid. */
    if (thread->id != thread_id || thread->state == STATE_KILLED)
        base_priority = -1;

    return base_priority;
}
#endif /* HAVE_PRIORITY_SCHEDULING */

#ifdef HAVE_IO_PRIORITY
int thread_get_io_priority(unsigned int thread_id)
{
    struct thread_entry *thread = thread_id_entry(thread_id);
    return thread->io_priority;
}

void thread_set_io_priority(unsigned int thread_id,int io_priority)
{
    struct thread_entry *thread = thread_id_entry(thread_id);
    thread->io_priority = io_priority;
}
#endif

/*---------------------------------------------------------------------------
 * Starts a frozen thread - similar semantics to wakeup_thread except that
 * the thread is on no scheduler or wakeup queue at all. It exists simply by
 * virtue of the slot having a state of STATE_FROZEN.
 *---------------------------------------------------------------------------
 */
void thread_thaw(unsigned int thread_id)
{
    struct thread_entry *thread = thread_id_entry(thread_id);
    int oldlevel = disable_irq_save();

    LOCK_THREAD(thread);

    /* If thread is the current one, it cannot be frozen, therefore
     * there is no need to check that. */
    if (thread->id == thread_id && thread->state == STATE_FROZEN)
        core_schedule_wakeup(thread);

    UNLOCK_THREAD(thread);
    restore_irq(oldlevel);
}

#if NUM_CORES > 1
/*---------------------------------------------------------------------------
 * Switch the processor that the currently executing thread runs on.
 *---------------------------------------------------------------------------
 */
unsigned int switch_core(unsigned int new_core)
{
    const unsigned int core = CURRENT_CORE;
    struct thread_entry *current = cores[core].running;

    if (core == new_core)
    {
        /* No change - just return same core */
        return core;
    }

    disable_irq();
    LOCK_THREAD(current);

    /* Get us off the running list for the current core */
    RTR_LOCK(core);
    remove_from_list_l(&cores[core].running, current);
    rtr_subtract_entry(core, current->priority);
    RTR_UNLOCK(core);

    /* Stash return value (old core) in a safe place */
    current->retval = core;

    /* If a timeout hadn't yet been cleaned-up it must be removed now or
     * the other core will likely attempt a removal from the wrong list! */
    if (current->tmo.prev != NULL)
    {
        remove_from_list_tmo(current);
    }

    /* Change the core number for this thread slot */
    current->core = new_core;

    /* Do not use core_schedule_wakeup here since this will result in
     * the thread starting to run on the other core before being finished on
     * this one. Delay the  list unlock to keep the other core stuck
     * until this thread is ready. */
    RTR_LOCK(new_core);

    rtr_add_entry(new_core, current->priority);
    add_to_list_l(&cores[new_core].running, current);

    /* Make a callback into device-specific code, unlock the wakeup list so
     * that execution may resume on the new core, unlock our slot and finally
     * restore the interrupt level */
    cores[core].blk_ops.flags = TBOP_SWITCH_CORE;
    cores[core].blk_ops.cl_p  = &cores[new_core].rtr_cl;
    cores[core].block_task    = current;

    UNLOCK_THREAD(current);

    /* Alert other core to activity */
    core_wake(new_core);

    /* Do the stack switching, cache_maintenence and switch_thread call -
       requires native code */
    switch_thread_core(core, current);

    /* Finally return the old core to caller */
    return current->retval;
}
#endif /* NUM_CORES > 1 */

/*---------------------------------------------------------------------------
 * Initialize threading API. This assumes interrupts are not yet enabled. On
 * multicore setups, no core is allowed to proceed until create_thread calls
 * are safe to perform.
 *---------------------------------------------------------------------------
 */
void INIT_ATTR init_threads(void)
{
    const unsigned int core = CURRENT_CORE;
    struct thread_entry *thread;

    if (core == CPU)
    {
        /* Initialize core locks and IDs in all slots */
        int n;
        for (n = 0; n < MAXTHREADS; n++)
        {
            thread = &threads[n];
            corelock_init(&thread->waiter_cl);
            corelock_init(&thread->slot_cl);
            thread->id = THREAD_ID_INIT(n);
        }
    }

    /* CPU will initialize first and then sleep */
    thread = find_empty_thread_slot();

    if (thread == NULL)
    {
        /* WTF? There really must be a slot available at this stage.
         * This can fail if, for example, .bss isn't zero'ed out by the loader
         * or threads is in the wrong section. */
        THREAD_PANICF("init_threads->no slot", NULL);
    }

    /* Initialize initially non-zero members of core */
    cores[core].next_tmo_check = current_tick; /* Something not in the past */

    /* Initialize initially non-zero members of slot */
    UNLOCK_THREAD(thread); /* No sync worries yet */
    thread->name = main_thread_name;
    thread->state = STATE_RUNNING;
    IF_COP( thread->core = core; )
#ifdef HAVE_PRIORITY_SCHEDULING
    corelock_init(&cores[core].rtr_cl);
    thread->base_priority = PRIORITY_USER_INTERFACE;
    prio_add_entry(&thread->pdist, PRIORITY_USER_INTERFACE);
    thread->priority = PRIORITY_USER_INTERFACE;
    rtr_add_entry(core, PRIORITY_USER_INTERFACE);
#endif

    add_to_list_l(&cores[core].running, thread);

    if (core == CPU)
    {
        thread->stack = stackbegin;
        thread->stack_size = (uintptr_t)stackend - (uintptr_t)stackbegin;
#if NUM_CORES > 1  /* This code path will not be run on single core targets */
        /* Wait for other processors to finish their inits since create_thread
         * isn't safe to call until the kernel inits are done. The first
         * threads created in the system must of course be created by CPU.
         * Another possible approach is to initialize all cores and slots
         * for each core by CPU, let the remainder proceed in parallel and
         * signal CPU when all are finished. */
        core_thread_init(CPU);
    } 
    else
    {
        /* Initial stack is the idle stack */
        thread->stack = idle_stacks[core];
        thread->stack_size = IDLE_STACK_SIZE;
        /* After last processor completes, it should signal all others to
         * proceed or may signal the next and call thread_exit(). The last one
         * to finish will signal CPU. */
        core_thread_init(core);
        /* Other cores do not have a main thread - go idle inside switch_thread
         * until a thread can run on the core. */
        thread_exit();
#endif /* NUM_CORES */
    }
#ifdef INIT_MAIN_THREAD
    init_main_thread(&thread->context);
#endif
}

/* Unless otherwise defined, do nothing */
#ifndef YIELD_KERNEL_HOOK
#define YIELD_KERNEL_HOOK() false
#endif
#ifndef SLEEP_KERNEL_HOOK
#define SLEEP_KERNEL_HOOK(ticks) false
#endif

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
        return; /* handled */

    switch_thread();
}
