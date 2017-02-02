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
#ifndef THREAD_INTERNAL_H
#define THREAD_INTERNAL_H

#include "thread.h"
#include <stdio.h>
#include "panic.h"
#include "debug.h"

/*
 * We need more stack when we run under a host
 * maybe more expensive C lib functions?
 *
 * simulator (possibly) doesn't simulate stack usage anyway but well ... */

#if defined(HAVE_SDL_THREADS) || defined(__PCTOOL__)
struct regs
{
    void *t;             /* OS thread */
    void *told;          /* Last thread in slot (explained in thead-sdl.c) */
    void *s;             /* Semaphore for blocking and wakeup */
    void (*start)(void); /* Start function */
};

#define DEFAULT_STACK_SIZE 0x100 /* tiny, ignored anyway */
#else
#include "asm/thread.h"
#endif /* HAVE_SDL_THREADS */

/* NOTE: The use of the word "queue" may also refer to a linked list of
   threads being maintained that are normally dealt with in FIFO order
   and not necessarily kernel event_queue */
enum
{
    /* States without a timeout must be first */
    STATE_KILLED = 0,    /* Thread is killed (default) */
    STATE_RUNNING,       /* Thread is currently running */
    STATE_BLOCKED,       /* Thread is indefinitely blocked on a queue */
    /* These states involve adding the thread to the tmo list */
    STATE_SLEEPING,      /* Thread is sleeping with a timeout */
    STATE_BLOCKED_W_TMO, /* Thread is blocked on a queue with a timeout */
    /* Miscellaneous states */
    STATE_FROZEN,        /* Thread is suspended and will not run until
                            thread_thaw is called with its ID */
    THREAD_NUM_STATES,
    TIMEOUT_STATE_FIRST = STATE_SLEEPING,
};

#ifdef HAVE_PRIORITY_SCHEDULING

/* Quick-disinherit of priority elevation. Must be a running thread. */
void priority_disinherit(struct thread_entry *thread, struct blocker *bl);

struct priority_distribution
{
    uint8_t   hist[NUM_PRIORITIES]; /* Histogram: Frequency for each priority */
    priobit_t mask;                 /* Bitmask of hist entries that are not zero */
};

#endif /* HAVE_PRIORITY_SCHEDULING */

#define __rtr_queue         lldc_head
#define __rtr_queue_node    lldc_node

#define __tmo_queue         ll_head
#define __tmo_queue_node    ll_node

/* Information kept in each thread slot
 * members are arranged according to size - largest first - in order
 * to ensure both alignment and packing at the same time.
 */
struct thread_entry
{
    struct regs context;         /* Register context at switch -
                                    _must_ be first member */
#ifndef HAVE_SDL_THREADS
    uintptr_t *stack;            /* Pointer to top of stack */
#endif
    const char *name;            /* Thread name */
    long tmo_tick;               /* Tick when thread should be woken */
    struct __rtr_queue_node rtr; /* Node for run queue */
    struct __tmo_queue_node tmo; /* Links for timeout list */
    struct __wait_queue_node wq; /* Node for wait queue */
    struct __wait_queue *volatile wqp; /* Pointer to registered wait queue */
#if NUM_CORES > 1
    struct corelock waiter_cl;   /* Corelock for thread_wait */
    struct corelock slot_cl;     /* Corelock to lock thread slot */
    unsigned char core;          /* The core to which thread belongs */
#endif
    struct __wait_queue queue;   /* List of threads waiting for thread to be
                                    removed */
    volatile intptr_t retval;    /* Return value from a blocked operation/
                                    misc. use */
    uint32_t id;                 /* Current slot id */
    int __errno;                 /* Thread error number (errno tls) */
#ifdef HAVE_PRIORITY_SCHEDULING
    /* Priority summary of owned objects that support inheritance */
    struct blocker *blocker;     /* Pointer to blocker when this thread is blocked
                                    on an object that supports PIP -
                                    states: STATE_BLOCKED/STATE_BLOCKED_W_TMO  */
    struct priority_distribution pdist; /* Priority summary of owned objects
                                    that have blocked threads and thread's own
                                    base priority */
    int skip_count;              /* Number of times skipped if higher priority
                                    thread was running */
    unsigned char base_priority; /* Base priority (set explicitly during
                                  creation or thread_set_priority) */
    unsigned char priority;      /* Scheduled priority (higher of base or
                                    all threads blocked by this one) */
#endif
#ifndef HAVE_SDL_THREADS
    unsigned short stack_size;   /* Size of stack in bytes */
#endif
    unsigned char state;         /* Thread slot state (STATE_*) */
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    unsigned char cpu_boost;     /* CPU frequency boost flag */
#endif
};

/* Thread ID, 32 bits = |VVVVVVVV|VVVVVVVV|VVVVVVVV|SSSSSSSS| */
#define THREAD_ID_VERSION_SHIFT 8
#define THREAD_ID_VERSION_MASK  0xffffff00
#define THREAD_ID_SLOT_MASK     0x000000ff
#define THREAD_ID_INIT(n)       ((1u << THREAD_ID_VERSION_SHIFT) | (n))
#define THREAD_ID_SLOT(id)      ((id) & THREAD_ID_SLOT_MASK)

#define DEADBEEF ((uintptr_t)0xdeadbeefdeadbeefull)

/* Information kept for each core
 * Members are arranged for the same reason as in thread_entry
 */
struct core_entry
{
    /* "Active" lists - core is constantly active on these and are never
       locked and interrupts do not access them */
    struct __rtr_queue rtr;          /* Threads that are runnable */
    struct __tmo_queue tmo;          /* Threads on a bounded wait */
    struct thread_entry *running;    /* Currently running thread */
#ifdef HAVE_PRIORITY_SCHEDULING
    struct priority_distribution rtr_dist; /* Summary of runnables */
#endif
    long next_tmo_check;             /* Next due timeout check */
#if NUM_CORES > 1
    struct corelock rtr_cl;          /* Lock for rtr list */
#endif /* NUM_CORES */
};

/* Hide a few scheduler details from itself to make allocation more flexible */
#define __main_thread_name \
    ({ extern const char __main_thread_name_str[]; \
       __main_thread_name_str; })

static FORCE_INLINE
    void * __get_main_stack(size_t *stacksize)
{
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    extern uintptr_t stackbegin[];
    extern uintptr_t stackend[];
#else
    extern uintptr_t *stackbegin;
    extern uintptr_t *stackend;
#endif
    *stacksize = (uintptr_t)stackend - (uintptr_t)stackbegin;
    return stackbegin;
}

void format_thread_name(char *buf, size_t bufsize,
                        const struct thread_entry *thread);

static FORCE_INLINE
    struct core_entry * __core_id_entry(unsigned int core)
{
#if NUM_CORES > 1
    extern struct core_entry * __cores[NUM_CORES];
    return __cores[core];
#else
    extern struct core_entry __cores[NUM_CORES];
    return &__cores[core];
#endif
}

#define __running_self_entry() \
    __core_id_entry(CURRENT_CORE)->running

static FORCE_INLINE
    struct thread_entry * __thread_slot_entry(unsigned int slotnum)
{
    extern struct thread_entry * __threads[MAXTHREADS];
    return __threads[slotnum];
}

#define __thread_id_entry(id) \
    __thread_slot_entry(THREAD_ID_SLOT(id))

#define THREAD_FROM(p, member) \
    container_of(p, struct thread_entry, member)

#define RTR_EMPTY(rtrp) \
    ({ (rtrp)->head == NULL; })

#define RTR_THREAD_FIRST(rtrp) \
    ({ THREAD_FROM((rtrp)->head, rtr); })

#define RTR_THREAD_NEXT(thread) \
    ({ THREAD_FROM((thread)->rtr.next, rtr); })

#define TMO_THREAD_FIRST(tmop) \
    ({ struct __tmo_queue *__tmop = (tmop); \
       __tmop->head ? THREAD_FROM(__tmop->head, tmo) : NULL; })

#define TMO_THREAD_NEXT(thread) \
    ({ struct __tmo_queue_node *__next = (thread)->tmo.next; \
       __next ? THREAD_FROM(__next, tmo) : NULL; })

#define WQ_THREAD_FIRST(wqp) \
    ({ struct __wait_queue *__wqp = (wqp); \
       __wqp->head ? THREAD_FROM(__wqp->head, wq) : NULL; })

#define WQ_THREAD_NEXT(thread) \
    ({ struct __wait_queue_node *__next = (thread)->wq.next; \
       __next ? THREAD_FROM(__next, wq) : NULL; })

void thread_alloc_init(void) INIT_ATTR;
struct thread_entry * thread_alloc(void);
void thread_free(struct thread_entry *thread);
void new_thread_id(struct thread_entry *thread);

/* Switch to next runnable thread */
void switch_thread(void);
/* Blocks a thread for at least the specified number of ticks (0 = wait until
 * next tick) */
void sleep_thread(int ticks);
/* Blocks the current thread on a thread queue (< 0 == infinite) */
void block_thread_(struct thread_entry *current, int timeout);

#ifdef HAVE_PRIORITY_SCHEDULING
#define block_thread(thread, timeout, __wqp, bl) \
    ({ struct thread_entry *__t = (thread);   \
       __t->wqp = (__wqp);                    \
       if (!__builtin_constant_p(bl) || (bl)) \
           __t->blocker = (bl);               \
       block_thread_(__t, (timeout)); })
#else
#define block_thread(thread, timeout, __wqp, bl...) \
    ({ struct thread_entry *__t = (thread); \
       __t->wqp = (__wqp);                  \
       block_thread_(__t, (timeout)); })
#endif

/* Return bit flags for thread wakeup */
#define THREAD_NONE     0x0 /* No thread woken up (exclusive) */
#define THREAD_OK       0x1 /* A thread was woken up */
#define THREAD_SWITCH   0x2 /* Task switch recommended (one or more of
                               higher priority than current were woken) */

/* A convenience function for waking an entire queue of threads. */
unsigned int wait_queue_wake(struct __wait_queue *wqp);

/* Wakeup a thread at the head of a list */
enum wakeup_thread_protocol
{
    WAKEUP_DEFAULT,
    WAKEUP_TRANSFER,
    WAKEUP_RELEASE,
    WAKEUP_TRANSFER_MULTI,
};

unsigned int wakeup_thread_(struct thread_entry *thread
                            IF_PRIO(, enum wakeup_thread_protocol proto));

#ifdef HAVE_PRIORITY_SCHEDULING
#define wakeup_thread(thread, proto) \
    wakeup_thread_((thread), (proto))
#else
#define wakeup_thread(thread, proto...) \
    wakeup_thread_((thread));
#endif

#ifdef RB_PROFILE
void profile_thread(void);
#endif

static inline void rtr_queue_init(struct __rtr_queue *rtrp)
{
    lldc_init(rtrp);
}

static inline void rtr_queue_make_first(struct __rtr_queue *rtrp,
                                        struct thread_entry *thread)
{
    rtrp->head = &thread->rtr;
}

static inline void rtr_queue_add(struct __rtr_queue *rtrp,
                                 struct thread_entry *thread)
{
    lldc_insert_last(rtrp, &thread->rtr);
}

static inline void rtr_queue_remove(struct __rtr_queue *rtrp,
                                    struct thread_entry *thread)
{
    lldc_remove(rtrp, &thread->rtr);
}

#define TMO_NOT_QUEUED (NULL + 1)

static inline bool tmo_is_queued(struct thread_entry *thread)
{
    return thread->tmo.next != TMO_NOT_QUEUED;
}

static inline void tmo_set_dequeued(struct thread_entry *thread)
{
    thread->tmo.next = TMO_NOT_QUEUED;
}

static inline void tmo_queue_init(struct __tmo_queue *tmop)
{
    ll_init(tmop);
}

static inline void tmo_queue_expire(struct __tmo_queue *tmop,
                                    struct thread_entry *prev,
                                    struct thread_entry *thread)
{
    ll_remove_next(tmop, prev ? &prev->tmo : NULL);
    tmo_set_dequeued(thread);
}

static inline void tmo_queue_remove(struct __tmo_queue *tmop,
                                    struct thread_entry *thread)
{
    if (tmo_is_queued(thread))
    {
        ll_remove(tmop, &thread->tmo);
        tmo_set_dequeued(thread);
    }
}

static inline void tmo_queue_register(struct __tmo_queue *tmop,
                                      struct thread_entry *thread)
{
    if (!tmo_is_queued(thread))
        ll_insert_last(tmop, &thread->tmo);
}

static inline void wait_queue_init(struct __wait_queue *wqp)
{
    lld_init(wqp);
}

static inline void wait_queue_register(struct thread_entry *thread)
{
    lld_insert_last(thread->wqp, &thread->wq);
}

static inline struct __wait_queue *
    wait_queue_ptr(struct thread_entry *thread)
{
    return thread->wqp;
}

static inline struct __wait_queue *
    wait_queue_remove(struct thread_entry *thread)
{
    struct __wait_queue *wqp = thread->wqp;
    thread->wqp = NULL;
    lld_remove(wqp, &thread->wq);
    return wqp;
}

static inline struct __wait_queue *
    wait_queue_try_remove(struct thread_entry *thread)
{
    struct __wait_queue *wqp = thread->wqp;
    if (wqp)
    {
        thread->wqp = NULL;
        lld_remove(wqp, &thread->wq);
    }

    return wqp;
}

static inline void blocker_init(struct blocker *bl)
{
    bl->thread = NULL;
#ifdef HAVE_PRIORITY_SCHEDULING
    bl->priority = PRIORITY_IDLE;
#endif
}

static inline void blocker_splay_init(struct blocker_splay *blsplay)
{
    blocker_init(&blsplay->blocker);
#ifdef HAVE_PRIORITY_SCHEDULING
    threadbit_clear(&blsplay->mask);
#endif
    corelock_init(&blsplay->cl);
}

static inline long get_tmo_tick(struct thread_entry *thread)
{
    return thread->tmo_tick;
}

#endif /* THREAD_INTERNAL_H */
