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

#ifdef HAVE_CORELOCK_OBJECT
/* Operations to be performed just before stopping a thread and starting
   a new one if specified before calling switch_thread */
enum
{
    TBOP_CLEAR = 0,       /* No operation to do */
    TBOP_UNLOCK_CORELOCK, /* Unlock a corelock variable */
    TBOP_SWITCH_CORE,     /* Call the core switch preparation routine */
};

struct thread_blk_ops
{
    struct corelock *cl_p;    /* pointer to corelock */
    unsigned char    flags;   /* TBOP_* flags */
};
#endif /* NUM_CORES > 1 */

/* Link information for lists thread is in */
struct thread_entry; /* forward */
struct thread_list
{
    struct thread_entry *prev; /* Previous thread in a list */
    struct thread_entry *next; /* Next thread in a list */
};

/* Information kept in each thread slot
 * members are arranged according to size - largest first - in order
 * to ensure both alignment and packing at the same time.
 */
struct thread_entry
{
    struct regs context;       /* Register context at switch -
                                  _must_ be first member */
    uintptr_t *stack;          /* Pointer to top of stack */
    const char *name;          /* Thread name */
    long tmo_tick;             /* Tick when thread should be woken from
                                  timeout -
                                  states: STATE_SLEEPING/STATE_BLOCKED_W_TMO */
    struct thread_list l;      /* Links for blocked/waking/running -
                                  circular linkage in both directions */
    struct thread_list tmo;    /* Links for timeout list -
                                  Circular in reverse direction, NULL-terminated in
                                  forward direction -
                                  states: STATE_SLEEPING/STATE_BLOCKED_W_TMO */
    struct thread_entry **bqp; /* Pointer to list variable in kernel
                                  object where thread is blocked - used
                                  for implicit unblock and explicit wake
                                  states: STATE_BLOCKED/STATE_BLOCKED_W_TMO  */
#ifdef HAVE_CORELOCK_OBJECT
    struct corelock *obj_cl;   /* Object corelock where thead is blocked -
                                  states: STATE_BLOCKED/STATE_BLOCKED_W_TMO */
    struct corelock waiter_cl; /* Corelock for thread_wait */
    struct corelock slot_cl;   /* Corelock to lock thread slot */
    unsigned char core;        /* The core to which thread belongs */
#endif
    struct thread_entry *queue; /* List of threads waiting for thread to be
                                  removed */
#ifdef HAVE_WAKEUP_EXT_CB
    void (*wakeup_ext_cb)(struct thread_entry *thread); /* Callback that
                                  performs special steps needed when being
                                  forced off of an object's wait queue that
                                  go beyond the standard wait queue removal
                                  and priority disinheritance */
    /* Only enabled when using queue_send for now */
#endif
#if defined(HAVE_SEMAPHORE_OBJECTS) || \
    defined(HAVE_EXTENDED_MESSAGING_AND_NAME) || \
    NUM_CORES > 1
    volatile intptr_t retval;  /* Return value from a blocked operation/
                                  misc. use */
#endif
    uint32_t id;               /* Current slot id */
    int __errno;               /* Thread error number (errno tls) */
#ifdef HAVE_PRIORITY_SCHEDULING
    /* Priority summary of owned objects that support inheritance */
    struct blocker *blocker;   /* Pointer to blocker when this thread is blocked
                                  on an object that supports PIP -
                                  states: STATE_BLOCKED/STATE_BLOCKED_W_TMO  */
    struct priority_distribution pdist; /* Priority summary of owned objects
                                  that have blocked threads and thread's own
                                  base priority */
    int skip_count;            /* Number of times skipped if higher priority
                                  thread was running */
    unsigned char base_priority; /* Base priority (set explicitly during
                                  creation or thread_set_priority) */
    unsigned char priority;    /* Scheduled priority (higher of base or
                                  all threads blocked by this one) */
#endif
    unsigned short stack_size; /* Size of stack in bytes */
    unsigned char state;       /* Thread slot state (STATE_*) */
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    unsigned char cpu_boost;   /* CPU frequency boost flag */
#endif
#ifdef HAVE_IO_PRIORITY
    unsigned char io_priority;
#endif
};

/* Information kept for each core
 * Members are arranged for the same reason as in thread_entry
 */
struct core_entry
{
    /* "Active" lists - core is constantly active on these and are never
       locked and interrupts do not access them */
    struct thread_entry *running;  /* threads that are running (RTR) */
    struct thread_entry *timeout;  /* threads that are on a timeout before
                                      running again */
    struct thread_entry *block_task; /* Task going off running list */
#ifdef HAVE_PRIORITY_SCHEDULING
    struct priority_distribution rtr; /* Summary of running and ready-to-run
                                         threads */
#endif
    long next_tmo_check;           /* soonest time to check tmo threads */
#ifdef HAVE_CORELOCK_OBJECT
    struct thread_blk_ops blk_ops; /* operations to perform when
                                      blocking a thread */
    struct corelock rtr_cl;        /* Lock for rtr list */
#endif /* NUM_CORES */
};

/* Thread ID, 32 bits = |VVVVVVVV|VVVVVVVV|VVVVVVVV|SSSSSSSS| */
#define THREAD_ID_VERSION_SHIFT 8
#define THREAD_ID_VERSION_MASK  0xffffff00
#define THREAD_ID_SLOT_MASK     0x000000ff
#define THREAD_ID_INIT(n)       ((1u << THREAD_ID_VERSION_SHIFT) | (n))
#define THREAD_ID_SLOT(id)      ((id) & THREAD_ID_SLOT_MASK)

/* Thread locking */
#if NUM_CORES > 1
#define LOCK_THREAD(thread) \
    ({ corelock_lock(&(thread)->slot_cl); })
#define TRY_LOCK_THREAD(thread) \
    ({ corelock_try_lock(&(thread)->slot_cl); })
#define UNLOCK_THREAD(thread) \
    ({ corelock_unlock(&(thread)->slot_cl); })
#define UNLOCK_THREAD_AT_TASK_SWITCH(thread) \
    ({ unsigned int _core = (thread)->core; \
       cores[_core].blk_ops.flags |= TBOP_UNLOCK_CORELOCK; \
       cores[_core].blk_ops.cl_p = &(thread)->slot_cl; })
#else /* NUM_CORES == 1*/
#define LOCK_THREAD(thread) \
    ({ (void)(thread); })
#define TRY_LOCK_THREAD(thread) \
    ({ (void)(thread); })
#define UNLOCK_THREAD(thread) \
    ({ (void)(thread); })
#define UNLOCK_THREAD_AT_TASK_SWITCH(thread) \
    ({ (void)(thread); })
#endif /* NUM_CORES */

#define DEADBEEF ((uintptr_t)0xdeadbeefdeadbeefull)

/* Switch to next runnable thread */
void switch_thread(void);
/* Blocks a thread for at least the specified number of ticks (0 = wait until
 * next tick) */
void sleep_thread(int ticks);
/* Blocks the current thread on a thread queue (< 0 == infinite) */
void block_thread(struct thread_entry *current, int timeout);

/* Return bit flags for thread wakeup */
#define THREAD_NONE     0x0 /* No thread woken up (exclusive) */
#define THREAD_OK       0x1 /* A thread was woken up */
#define THREAD_SWITCH   0x2 /* Task switch recommended (one or more of
                               higher priority than current were woken) */

/* A convenience function for waking an entire queue of threads. */
unsigned int thread_queue_wake(struct thread_entry **list);

/* Wakeup a thread at the head of a list */
enum wakeup_thread_protocol
{
    WAKEUP_DEFAULT,
    WAKEUP_TRANSFER,
    WAKEUP_RELEASE,
    WAKEUP_TRANSFER_MULTI,
};

unsigned int wakeup_thread_(struct thread_entry **list
                            IF_PRIO(, enum wakeup_thread_protocol proto));

#ifdef HAVE_PRIORITY_SCHEDULING
#define wakeup_thread(list, proto) \
    wakeup_thread_((list), (proto))
#else /* !HAVE_PRIORITY_SCHEDULING */
#define wakeup_thread(list, proto...) \
    wakeup_thread_((list));
#endif /* HAVE_PRIORITY_SCHEDULING */

#ifdef HAVE_IO_PRIORITY
void thread_set_io_priority(unsigned int thread_id, int io_priority);
int thread_get_io_priority(unsigned int thread_id);
#endif /* HAVE_IO_PRIORITY */
#if NUM_CORES > 1
unsigned int switch_core(unsigned int new_core);
#endif

/* Return the id of the calling thread. */
unsigned int thread_self(void);

/* Return the thread_entry for the calling thread */
struct thread_entry* thread_self_entry(void);

/* Return thread entry from id */
struct thread_entry *thread_id_entry(unsigned int thread_id);

#ifdef RB_PROFILE
void profile_thread(void);
#endif

#endif /* THREAD_INTERNAL_H */
