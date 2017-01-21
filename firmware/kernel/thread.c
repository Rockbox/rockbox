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

/* Define THREAD_EXTRA_CHECKS as 1 to enable additional state checks */
#ifdef DEBUG
#define THREAD_EXTRA_CHECKS 1 /* Always 1 for DEBUG */
#else
#define THREAD_EXTRA_CHECKS 0
#endif

/****************************************************************************
 *                              ATTENTION!!                                 *
 *    See notes below on implementing processor-specific portions!          *
 ****************************************************************************
 *
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
 * to block_thread  and wakeup_thread both to themselves and to each other.
 * Objects' queues are also protected here.
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
 *
 *
 *---------------------------------------------------------------------------
 * Priority distribution structure (one category for each possible priority):
 *
 *       +----+----+----+ ... +------+
 * hist: | F0 | F1 | F2 |     | Fn-1 |
 *       +----+----+----+ ... +------+
 * mask: | b0 | b1 | b2 |     | bn-1 |
 *       +----+----+----+ ... +------+
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
static FORCE_INLINE void core_sleep(IF_COP_VOID(unsigned int core));
static FORCE_INLINE void store_context(void* addr);
static FORCE_INLINE void load_context(const void* addr);

/****************************************************************************
 * Processor/OS-specific section - include necessary core support
 */

#include "asm/thread.c"

#if defined (CPU_PP)
#include "thread-pp.c"
#endif /* CPU_PP */

/*
 * End Processor-specific section
 ***************************************************************************/

static NO_INLINE NORETURN_ATTR
  void thread_panicf(const char *msg, struct thread_entry *thread)
{
    IF_COP( const unsigned int core = thread->core; )
    static char name[sizeof (((struct thread_debug_info *)0)->name)];
    format_thread_name(name, sizeof (name), thread);
    panicf ("%s %s" IF_COP(" (%d)"), msg, name IF_COP(, core));
}

static NO_INLINE void thread_stkov(struct thread_entry *thread)
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
    do {} while (1)
#define THREAD_ASSERT(exp, msg, thread) \
    do {} while (0)
#endif /* THREAD_EXTRA_CHECKS */

/* Thread locking */
#if NUM_CORES > 1
#define LOCK_THREAD(thread) \
    ({ corelock_lock(&(thread)->slot_cl); })
#define TRY_LOCK_THREAD(thread) \
    ({ corelock_try_lock(&(thread)->slot_cl); })
#define UNLOCK_THREAD(thread) \
    ({ corelock_unlock(&(thread)->slot_cl); })
#else /* NUM_CORES == 1*/
#define LOCK_THREAD(thread) \
    ({ (void)(thread); })
#define TRY_LOCK_THREAD(thread) \
    ({ (void)(thread); })
#define UNLOCK_THREAD(thread) \
    ({ (void)(thread); })
#endif /* NUM_CORES */

/* RTR list */
#define RTR_LOCK(corep) \
    corelock_lock(&(corep)->rtr_cl)
#define RTR_UNLOCK(corep) \
    corelock_unlock(&(corep)->rtr_cl)

#ifdef HAVE_PRIORITY_SCHEDULING
#define rtr_add_entry(corep, priority) \
    prio_add_entry(&(corep)->rtr_dist, (priority))
#define rtr_subtract_entry(corep, priority) \
    prio_subtract_entry(&(corep)->rtr_dist, (priority))
#define rtr_move_entry(corep, from, to) \
    prio_move_entry(&(corep)->rtr_dist, (from), (to))
#else /* !HAVE_PRIORITY_SCHEDULING */
#define rtr_add_entry(corep, priority) \
    do {} while (0)
#define rtr_subtract_entry(corep, priority) \
    do {} while (0)
#define rtr_move_entry(corep, from, to) \
    do {} while (0)
#endif /* HAVE_PRIORITY_SCHEDULING */

static FORCE_INLINE void thread_store_context(struct thread_entry *thread)
{
    store_context(&thread->context);
#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
    thread->__errno = errno;
#endif
}

static FORCE_INLINE void thread_load_context(struct thread_entry *thread)
{
#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
    errno = thread->__errno;
#endif
    load_context(&thread->context);
}

static FORCE_INLINE unsigned int
should_switch_tasks(struct thread_entry *thread)
{
#ifdef HAVE_PRIORITY_SCHEDULING
    const unsigned int core = CURRENT_CORE;
#if NUM_CORES > 1
    /* Forget about it if different CPU */
    if (thread->core != core)
        return THREAD_OK;
#endif
    /* Just woke something therefore a thread is on the run queue */
    struct thread_entry *current =
        RTR_THREAD_FIRST(&__core_id_entry(core)->rtr);
    if (LIKELY(thread->priority >= current->priority))
        return THREAD_OK;

    /* There is a thread ready to run of higher priority on the same
     * core as the current one; recommend a task switch. */
    return THREAD_OK | THREAD_SWITCH;
#else
    return THREAD_OK;
    (void)thread;
#endif /* HAVE_PRIORITY_SCHEDULING */
}

#ifdef HAVE_PRIORITY_SCHEDULING

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
 * Common init for new thread basic info
 *---------------------------------------------------------------------------
 */
static void new_thread_base_init(struct thread_entry *thread,
                                 void **stackp, size_t *stack_sizep,
                                 const char *name IF_PRIO(, int priority)
                                 IF_COP(, unsigned int core))
{
    ALIGN_BUFFER(*stackp, *stack_sizep, MIN_STACK_ALIGN);
    thread->stack = *stackp;
    thread->stack_size = *stack_sizep;

    thread->name = name;
    wait_queue_init(&thread->queue);
    thread->wqp = NULL;
    tmo_set_dequeued(thread);
#ifdef HAVE_PRIORITY_SCHEDULING
    thread->skip_count    = 0;
    thread->blocker       = NULL;
    thread->base_priority = priority;
    thread->priority      = priority;
    memset(&thread->pdist, 0, sizeof(thread->pdist));
    prio_add_entry(&thread->pdist, priority);
#endif
#if NUM_CORES > 1
    thread->core = core;
#endif
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    thread->cpu_boost = 0;
#endif
}

/*---------------------------------------------------------------------------
 * Move a thread onto the core's run queue and promote it
 *---------------------------------------------------------------------------
 */
static inline void core_rtr_add(struct core_entry *corep,
                                struct thread_entry *thread)
{
    RTR_LOCK(corep);
    rtr_queue_add(&corep->rtr, thread);
    rtr_add_entry(corep, thread->priority);
#ifdef HAVE_PRIORITY_SCHEDULING
    thread->skip_count = thread->base_priority;
#endif
    thread->state = STATE_RUNNING;
    RTR_UNLOCK(corep);
}

/*---------------------------------------------------------------------------
 * Remove a thread from the core's run queue
 *---------------------------------------------------------------------------
 */
static inline void core_rtr_remove(struct core_entry *corep,
                                   struct thread_entry *thread)
{
    RTR_LOCK(corep);
    rtr_queue_remove(&corep->rtr, thread);
    rtr_subtract_entry(corep, thread->priority);
    /* Does not demote state */
    RTR_UNLOCK(corep);
}

/*---------------------------------------------------------------------------
 * Move a thread back to a running state on its core
 *---------------------------------------------------------------------------
 */
static NO_INLINE void core_schedule_wakeup(struct thread_entry *thread)
{
    const unsigned int core = IF_COP_CORE(thread->core);
    struct core_entry *corep = __core_id_entry(core);
    core_rtr_add(corep, thread);
#if NUM_CORES > 1
    if (core != CURRENT_CORE)
        core_wake(core);
#endif
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

/*---------------------------------------------------------------------------
 * Change the priority and rtr entry for a running thread
 *---------------------------------------------------------------------------
 */
static inline void set_rtr_thread_priority(
    struct thread_entry *thread, int priority)
{
    const unsigned int core = IF_COP_CORE(thread->core);
    struct core_entry *corep = __core_id_entry(core);
    RTR_LOCK(corep);
    rtr_move_entry(corep, thread->priority, priority);
    thread->priority = priority;
    RTR_UNLOCK(corep);
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
static int wait_queue_find_priority(struct __wait_queue *wqp)
{
    int highest_priority = PRIORITY_IDLE;
    struct thread_entry *thread = WQ_THREAD_FIRST(wqp);

    while (thread != NULL)
    {
        int priority = thread->priority;
        if (priority < highest_priority)
            highest_priority = priority;

        thread = WQ_THREAD_NEXT(thread);
    }

    return highest_priority;
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
                blt = __thread_slot_entry(slotnum);
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
            set_rtr_thread_priority(blt, newpr);
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
        struct __wait_queue *wqp = wait_queue_ptr(blt);
        struct thread_entry *t = blt;
        blt = lock_blocker_thread(bl);

        UNLOCK_THREAD(t);

        /* Adjust this wait queue */
        oldblpr = bl->priority;
        if (newpr <= oldblpr)
            newblpr = newpr;
        else if (oldpr <= oldblpr)
            newblpr = wait_queue_find_priority(wqp);

        if (newblpr == oldblpr)
            break; /* Queue priority not changing */
    }

    UNLOCK_THREAD(blt);
}

/*---------------------------------------------------------------------------
 * Quick-inherit of priority elevation. 'thread' must be not runnable
 *---------------------------------------------------------------------------
 */
static void priority_inherit_internal_inner(struct thread_entry *thread,
                                            int blpr)
{
    if (prio_add_entry(&thread->pdist, blpr) == 1 && blpr < thread->priority)
        thread->priority = blpr;
}

static inline void priority_inherit_internal(struct thread_entry *thread,
                                             int blpr)
{
    if (blpr < PRIORITY_IDLE)
        priority_inherit_internal_inner(thread, blpr);
}

/*---------------------------------------------------------------------------
 * Quick-disinherit of priority elevation. 'thread' must current
 *---------------------------------------------------------------------------
 */
static void priority_disinherit_internal_inner(struct thread_entry *thread,
                                               int blpr)
{
    if (prio_subtract_entry(&thread->pdist, blpr) == 0 &&
        blpr <= thread->priority)
    {
        int priority = priobit_ffs(&thread->pdist.mask);
        if (priority != thread->priority)
            set_rtr_thread_priority(thread, priority);
    }
}

static inline void priority_disinherit_internal(struct thread_entry *thread,
                                                int blpr)
{
    if (blpr < PRIORITY_IDLE)
        priority_disinherit_internal_inner(thread, blpr);
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
    struct __wait_queue *wqp = wait_queue_ptr(thread);
    struct blocker *bl = thread->blocker;
    struct blocker_splay *blsplay = (struct blocker_splay *)bl;
    struct thread_entry *blt = bl->thread;

    /* The first thread is already locked and is assumed tagged "multi" */
    int count = 1;

    /* Multiple versions of the wait queue may be seen if doing more than
       one thread; queue removal isn't destructive to the pointers of the node
       being removed; this may lead to the blocker priority being wrong for a
       time but it gets fixed up below after getting exclusive access to the
       queue */
    while (1)
    {
        thread->blocker = NULL;
        wait_queue_remove(thread);

        unsigned int slotnum = THREAD_ID_SLOT(thread->id);
        threadbit_set_bit(&blsplay->mask, slotnum);

        struct thread_entry *tnext = WQ_THREAD_NEXT(thread);
        if (tnext == NULL || tnext->retval == 0)
            break;

        UNLOCK_THREAD(thread);

        count++;
        thread = tnext;

        LOCK_THREAD(thread);
    }

    /* Locking order reverses here since the threads are no longer on the
       queued side */
    if (count > 1)
        corelock_lock(&blsplay->cl);

    LOCK_THREAD(blt);

    int blpr = bl->priority;
    priority_disinherit_internal(blt, blpr);

    if (count > 1)
    {
        blsplay->blocker.thread = NULL;

        blpr = wait_queue_find_priority(wqp);

        FOR_EACH_BITARRAY_SET_BIT(&blsplay->mask, slotnum)
        {
            UNLOCK_THREAD(thread);
            thread = __thread_slot_entry(slotnum);
            LOCK_THREAD(thread);
            priority_inherit_internal(thread, blpr);
            core_schedule_wakeup(thread);
        }
    }
    else
    {
        /* Becomes a simple, direct transfer */
        blsplay->blocker.thread = thread;

        if (thread->priority <= blpr)
            blpr = wait_queue_find_priority(wqp);

        priority_inherit_internal(thread, blpr);
        core_schedule_wakeup(thread);
    }

    UNLOCK_THREAD(thread);

    bl->priority = blpr;

    UNLOCK_THREAD(blt);

    if (count > 1)
        corelock_unlock(&blsplay->cl);

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

    THREAD_ASSERT(__running_self_entry() == blt,
                  "UPPT->wrong thread", __running_self_entry());

    LOCK_THREAD(blt);

    thread->blocker = NULL;
    struct __wait_queue *wqp = wait_queue_remove(thread);

    int blpr = bl->priority;

    /* Remove the object's boost from the owning thread */
    priority_disinherit_internal_inner(blt, blpr);

    struct thread_entry *tnext = WQ_THREAD_FIRST(wqp);
    if (LIKELY(tnext == NULL))
    {
        /* Expected shortcut - no more waiters */
        blpr = PRIORITY_IDLE;
    }
    else
    {
        /* If thread is at the blocker priority, its removal may drop it */
        if (thread->priority <= blpr)
            blpr = wait_queue_find_priority(wqp);

        priority_inherit_internal_inner(thread, blpr);
    }

    bl->thread = thread; /* This thread pwns */

    core_schedule_wakeup(thread);
    UNLOCK_THREAD(thread);

    bl->priority = blpr; /* Save highest blocked priority */

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

    thread->blocker = NULL;
    struct __wait_queue *wqp = wait_queue_remove(thread);

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

    int newblpr = wait_queue_find_priority(wqp);
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
 * INTERNAL: Intended for use by kernel and not programs.
 *---------------------------------------------------------------------------
 */
unsigned int wakeup_thread_(struct thread_entry *thread
                            IF_PRIO(, enum wakeup_thread_protocol proto))
{
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
            wait_queue_remove(thread);
            core_schedule_wakeup(thread);
            UNLOCK_THREAD(thread);
        }

        return should_switch_tasks(thread);

    case STATE_RUNNING:
        if (wait_queue_try_remove(thread))
        {
            UNLOCK_THREAD(thread);
            return THREAD_OK; /* timed out */
        }

    default:
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
static NO_INLINE void check_tmo_expired_inner(struct core_entry *corep)
{
    const long tick = current_tick; /* snapshot the current tick */
    long next_tmo_check = tick + 60*HZ; /* minimum duration: once/minute */
    struct thread_entry *prev = NULL;
    struct thread_entry *thread = TMO_THREAD_FIRST(&corep->tmo);

    /* If there are no processes waiting for a timeout, just keep the check
       tick from falling into the past. */

    /* Break the loop once we have walked through the list of all
     * sleeping processes or have removed them all. */
    while (thread != NULL)
    {
        /* Check sleeping threads. Allow interrupts between checks. */
        enable_irq();

        struct thread_entry *next = TMO_THREAD_NEXT(thread);

        /* Lock thread slot against explicit wakeup */
        disable_irq();
        LOCK_THREAD(thread);

        unsigned int state = thread->state;

        if (LIKELY(state >= TIMEOUT_STATE_FIRST &&
                   TIME_BEFORE(tick, thread->tmo_tick)))
        {
            /* Timeout still pending - this will be the usual case */
            if (TIME_BEFORE(thread->tmo_tick, next_tmo_check))
            {
                /* Move the next check up to its time */
                next_tmo_check = thread->tmo_tick;
            }

            prev = thread;
        }
        else
        {
            /* TODO: there are no priority-inheriting timeout blocks
               right now but the procedure should be established */

            /* Sleep timeout has been reached / garbage collect stale list
               items */
            tmo_queue_expire(&corep->tmo, prev, thread);

            if (state >= TIMEOUT_STATE_FIRST)
                core_rtr_add(corep, thread);

            /* removed this one - prev doesn't change */
        }

        UNLOCK_THREAD(thread);

        thread = next;
    }

    corep->next_tmo_check = next_tmo_check;
}

static FORCE_INLINE void check_tmo_expired(struct core_entry *corep)
{
    if (!TIME_BEFORE(current_tick, corep->next_tmo_check))
        check_tmo_expired_inner(corep);
}

/*---------------------------------------------------------------------------
 * Prepares a the current thread to sleep forever or for the given duration.
 *---------------------------------------------------------------------------
 */
static FORCE_INLINE void prepare_block(struct thread_entry *current,
                                       unsigned int state, int timeout)
{
    const unsigned int core = IF_COP_CORE(current->core);

    /* Remove the thread from the list of running threads. */
    struct core_entry *corep = __core_id_entry(core);
    core_rtr_remove(corep, current);

    if (timeout >= 0)
    {
        /* Sleep may expire. */
        long tmo_tick = current_tick + timeout;
        current->tmo_tick = tmo_tick;

        if (TIME_BEFORE(tmo_tick, corep->next_tmo_check))
            corep->next_tmo_check = tmo_tick;

        tmo_queue_register(&corep->tmo, current);

        if (state == STATE_BLOCKED)
            state = STATE_BLOCKED_W_TMO;
    }

    /* Report new state. */
    current->state = state;
}

/*---------------------------------------------------------------------------
 * Switch thread in round robin fashion for any given priority. Any thread
 * that removed itself from the running list first must specify itself in
 * the paramter.
 *
 * INTERNAL: Intended for use by kernel and not programs.
 *---------------------------------------------------------------------------
 */
void switch_thread(void)
{
    const unsigned int core = CURRENT_CORE;
    struct core_entry *corep = __core_id_entry(core);
    struct thread_entry *thread = corep->running;

    if (thread)
    {
#ifdef RB_PROFILE
        profile_thread_stopped(THREAD_ID_SLOT(thread->id));
#endif
#ifdef DEBUG
        /* Check core_ctx buflib integrity */
        core_check_valid();
#endif
        thread_store_context(thread);

        /* Check if the current thread stack is overflown */
        if (UNLIKELY(thread->stack[0] != DEADBEEF) && thread->stack_size > 0)
            thread_stkov(thread);
    }

    /* TODO: make a real idle task */
    for (;;)
    {
        disable_irq();

        /* Check for expired timeouts */
        check_tmo_expired(corep);

        RTR_LOCK(corep);

        if (!RTR_EMPTY(&corep->rtr))
            break;

        thread = NULL;
        
        /* Enter sleep mode to reduce power usage */
        RTR_UNLOCK(corep);
        core_sleep(IF_COP(core));

        /* Awakened by interrupt or other CPU */
    }

    thread = (thread && thread->state == STATE_RUNNING) ?
        RTR_THREAD_NEXT(thread) : RTR_THREAD_FIRST(&corep->rtr);

#ifdef HAVE_PRIORITY_SCHEDULING
    /* Select the new task based on priorities and the last time a
     * process got CPU time relative to the highest priority runnable
     * task. If priority is not a feature, then FCFS is used (above). */
    int max = priobit_ffs(&corep->rtr_dist.mask);

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
            (priority > PRIORITY_REALTIME &&
             (diff = priority - max, ++thread->skip_count > diff*diff)))
        {
            break;
        }

        thread = RTR_THREAD_NEXT(thread);
    }

    thread->skip_count = 0; /* Reset aging counter */
#endif /* HAVE_PRIORITY_SCHEDULING */

    rtr_queue_make_first(&corep->rtr, thread);
    corep->running = thread;

    RTR_UNLOCK(corep);
    enable_irq();

#ifdef RB_PROFILE
    profile_thread_started(THREAD_ID_SLOT(thread->id));
#endif

    /* And finally, give control to the next thread. */
    thread_load_context(thread);
}

/*---------------------------------------------------------------------------
 * Sleeps a thread for at least a specified number of ticks with zero being
 * a wait until the next tick.
 *
 * INTERNAL: Intended for use by kernel and not programs.
 *---------------------------------------------------------------------------
 */
void sleep_thread(int ticks)
{
    struct thread_entry *current = __running_self_entry();
    LOCK_THREAD(current);
    prepare_block(current, STATE_SLEEPING, MAX(ticks, 0) + 1);
    UNLOCK_THREAD(current);
}

/*---------------------------------------------------------------------------
 * Block a thread on a blocking queue for explicit wakeup. If timeout is
 * negative, the block is infinite.
 *
 * INTERNAL: Intended for use by kernel and not programs.
 *---------------------------------------------------------------------------
 */
void block_thread_(struct thread_entry *current, int timeout)
{
    LOCK_THREAD(current);

#ifdef HAVE_PRIORITY_SCHEDULING
    struct blocker *bl = current->blocker;
    struct thread_entry *blt = NULL;
    if (bl != NULL)
    {
        current->blocker = bl;
        blt = lock_blocker_thread(bl);
    }
#endif /* HAVE_PRIORITY_SCHEDULING */

    wait_queue_register(current);
    prepare_block(current, STATE_BLOCKED, timeout);

#ifdef HAVE_PRIORITY_SCHEDULING
    if (bl != NULL)
    {
        int newblpr = current->priority;
        UNLOCK_THREAD(current);

        if (newblpr < bl->priority)
            inherit_priority(bl, bl, blt, newblpr);
        else
            unlock_blocker_thread(bl); /* Queue priority won't change */
    }
    else
#endif /* HAVE_PRIORITY_SCHEDULING */
    {
        UNLOCK_THREAD(current);
    }
}

/*---------------------------------------------------------------------------
 * Place the current core in idle mode - woken up on interrupt or wake
 * request from another core.
 *---------------------------------------------------------------------------
 */
void core_idle(void)
{
    disable_irq();
    core_sleep(IF_COP(CURRENT_CORE));
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
    struct thread_entry *thread = thread_alloc();
    if (thread == NULL)
        return 0;

    new_thread_base_init(thread, &stack, &stack_size, name
                         IF_PRIO(, priority) IF_COP(, core));

    unsigned int stack_words = stack_size / sizeof (uintptr_t);
    if (stack_words == 0)
        return 0;

    /* Munge the stack to make it easy to spot stack overflows */
    for (unsigned int i = 0; i < stack_words; i++)
        ((uintptr_t *)stack)[i] = DEADBEEF;

#if NUM_CORES > 1
    /* Writeback stack munging or anything else before starting */
    if (core != CURRENT_CORE)
        commit_dcache();
#endif

    thread->context.sp = (typeof (thread->context.sp))(stack + stack_size);
    THREAD_STARTUP_INIT(core, thread, function);

    int oldlevel = disable_irq_save();
    LOCK_THREAD(thread);

    thread->state = STATE_FROZEN;

    if (!(flags & CREATE_THREAD_FROZEN))
        core_schedule_wakeup(thread);

    unsigned int id = thread->id; /* Snapshot while locked */

    UNLOCK_THREAD(thread);
    restore_irq(oldlevel);

    return id;
}

/*---------------------------------------------------------------------------
 * Block the current thread until another thread terminates. A thread may
 * wait on itself to terminate but that will deadlock
 *.
 * Parameter is the ID as returned from create_thread().
 *---------------------------------------------------------------------------
 */
void thread_wait(unsigned int thread_id)
{
    ASSERT_CPU_MODE(CPU_MODE_THREAD_CONTEXT);

    struct thread_entry *current = __running_self_entry();
    struct thread_entry *thread = __thread_id_entry(thread_id);

    corelock_lock(&thread->waiter_cl);

    if (thread->id == thread_id && thread->state != STATE_KILLED)
    {
        disable_irq();
        block_thread(current, TIMEOUT_BLOCK, &thread->queue, NULL);

        corelock_unlock(&thread->waiter_cl);

        switch_thread();
        return;
    }

    corelock_unlock(&thread->waiter_cl);
}

/*---------------------------------------------------------------------------
 * Exit the current thread
 *---------------------------------------------------------------------------
 */
static USED_ATTR NORETURN_ATTR
void thread_exit_final(struct thread_entry *current)
{
    /* Slot is no longer this thread */
    new_thread_id(current);
    current->name = NULL;

    /* No longer using resources from creator */
    wait_queue_wake(&current->queue);

    UNLOCK_THREAD(current);
    corelock_unlock(&current->waiter_cl);

    thread_free(current);

    switch_thread();

    /* This should never and must never be reached - if it is, the
     * state is corrupted */
    THREAD_PANICF("thread_exit->K:*R", current);
}

void thread_exit(void)
{
    struct core_entry *corep = __core_id_entry(CURRENT_CORE);
    register struct thread_entry *current = corep->running;

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

    /* Remove from scheduler lists */
    tmo_queue_remove(&corep->tmo, current);
    prepare_block(current, STATE_KILLED, -1);
    corep->running = NULL; /* No switch_thread context save */

#ifdef RB_PROFILE
    profile_thread_stopped(THREAD_ID_SLOT(current->id));
#endif

    /* Do final release of resources and remove the thread */
#if NUM_CORES > 1
    thread_exit_finalize(current->core, current);
#else
    thread_exit_final(current);
#endif
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
    struct thread_entry *thread = __thread_id_entry(thread_id);

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
        set_rtr_thread_priority(thread, new_priority);
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
    struct __wait_queue *wqp = wait_queue_ptr(thread);

    thread->priority = new_priority;

    UNLOCK_THREAD(thread);
    thread = NULL;

    int oldblpr = bl->priority;
    int newblpr = oldblpr;
    if (new_priority < oldblpr)
        newblpr = new_priority;
    else if (old_priority <= oldblpr)
        newblpr = wait_queue_find_priority(wqp);

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
    struct thread_entry *thread = __thread_id_entry(thread_id);
    int base_priority = thread->base_priority;

    /* Simply check without locking slot. It may or may not be valid by the
     * time the function returns anyway. If all tests pass, it is the
     * correct value for when it was valid. */
    if (thread->id != thread_id || thread->state == STATE_KILLED)
        base_priority = -1;

    return base_priority;
}
#endif /* HAVE_PRIORITY_SCHEDULING */

/*---------------------------------------------------------------------------
 * Starts a frozen thread - similar semantics to wakeup_thread except that
 * the thread is on no scheduler or wakeup queue at all. It exists simply by
 * virtue of the slot having a state of STATE_FROZEN.
 *---------------------------------------------------------------------------
 */
void thread_thaw(unsigned int thread_id)
{
    struct thread_entry *thread = __thread_id_entry(thread_id);
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
static USED_ATTR NORETURN_ATTR
void switch_core_final(unsigned int old_core, struct thread_entry *current)
{
    /* Old core won't be using slot resources at this point */
    core_schedule_wakeup(current);
    UNLOCK_THREAD(current);
#ifdef RB_PROFILE
    profile_thread_stopped(THREAD_ID_SLOT(current->id));
#endif
    switch_thread();
    /* not reached */
    THREAD_PANICF("switch_core_final->same core!", current);
    (void)old_core;
}

unsigned int switch_core(unsigned int new_core)
{
    const unsigned int old_core = CURRENT_CORE;
    if (old_core == new_core)
        return old_core; /* No change */

    struct core_entry *corep = __core_id_entry(old_core);
    struct thread_entry *current = corep->running;

    disable_irq();
    LOCK_THREAD(current);

    /* Remove us from old core lists */
    tmo_queue_remove(&corep->tmo, current);
    core_rtr_remove(corep, current);
    corep->running = NULL; /* No switch_thread context save */

    /* Do the actual migration */
    current->core = new_core;
    switch_thread_core(old_core, current);

    /* Executing on new core */
    return old_core;
}
#endif /* NUM_CORES > 1 */

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
    boost_thread(__running_self_entry(), true);
}

void cancel_cpu_boost(void)
{
    boost_thread(__running_self_entry(), false);
}
#endif /* HAVE_SCHEDULER_BOOSTCTRL */

/*---------------------------------------------------------------------------
 * Initialize threading API. This assumes interrupts are not yet enabled. On
 * multicore setups, no core is allowed to proceed until create_thread calls
 * are safe to perform.
 *---------------------------------------------------------------------------
 */
void INIT_ATTR init_threads(void)
{
    const unsigned int core = CURRENT_CORE;

    if (core == CPU)
    {
        thread_alloc_init(); /* before using cores! */

        /* Create main thread */
        struct thread_entry *thread = thread_alloc();
        if (thread == NULL)
        {
            /* WTF? There really must be a slot available at this stage.
             * This can fail if, for example, .bss isn't zero'ed out by the
             * loader or threads is in the wrong section. */
            THREAD_PANICF("init_threads->no slot", NULL);
        }

        size_t stack_size;
        void *stack = __get_main_stack(&stack_size);
        new_thread_base_init(thread, &stack, &stack_size, __main_thread_name
                             IF_PRIO(, PRIORITY_MAIN_THREAD) IF_COP(, core));

        struct core_entry *corep = __core_id_entry(core);
        core_rtr_add(corep, thread);
        corep->running = thread;

#ifdef INIT_MAIN_THREAD
        init_main_thread(&thread->context);
#endif
    }

#if NUM_CORES > 1
    /* Boot CPU:
     * Wait for other processors to finish their inits since create_thread
     * isn't safe to call until the kernel inits are done. The first
     * threads created in the system must of course be created by CPU.
     * Another possible approach is to initialize all cores and slots
     * for each core by CPU, let the remainder proceed in parallel and
     * signal CPU when all are finished.
     *
     * Other:
     * After last processor completes, it should signal all others to
     * proceed or may signal the next and call thread_exit(). The last one
     * to finish will signal CPU.
     */
    core_thread_init(core);

    if (core != CPU)
    {
        /* No main thread on coprocessors - go idle and wait */
        switch_thread();
        THREAD_PANICF("init_threads() - coprocessor returned", NULL);
    }
#endif /* NUM_CORES */
}
