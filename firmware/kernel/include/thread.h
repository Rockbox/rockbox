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
#ifndef THREAD_H
#define THREAD_H

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include "config.h"
#include "gcc_extensions.h"
#include "linked_list.h"
#include "bitarray.h"
#include "corelock.h"

/* Priority scheduling (when enabled with HAVE_PRIORITY_SCHEDULING) works
 * by giving high priority threads more CPU time than lower priority threads
 * when they need it. Priority is differential such that the priority
 * difference between a lower priority runnable thread and the highest priority
 * runnable thread determines the amount of aging necessary for the lower
 * priority thread to be scheduled in order to prevent starvation.
 *
 * If software playback codec pcm buffer is going down to critical, codec
 * can gradually raise its own priority to override user interface and
 * prevent playback skipping.
 */
#define PRIORITY_RESERVED_HIGH   0   /* Reserved */
#define PRIORITY_RESERVED_LOW    32  /* Reserved */
#define HIGHEST_PRIORITY         1   /* The highest possible thread priority */
#define LOWEST_PRIORITY          31  /* The lowest possible thread priority */
/* Realtime range reserved for threads that will not allow threads of lower
 * priority to age and run (future expansion) */
#define PRIORITY_REALTIME_1      1
#define PRIORITY_REALTIME_2      2
#define PRIORITY_REALTIME_3      3
#define PRIORITY_REALTIME_4      4
#define PRIORITY_REALTIME        4   /* Lowest realtime range */
#define PRIORITY_BUFFERING       15  /* Codec buffering thread */
#define PRIORITY_USER_INTERFACE  16  /* For most UI thrads */
#define PRIORITY_RECORDING       16  /* Recording thread */
#define PRIORITY_PLAYBACK        16  /* Variable between this and MAX */
#define PRIORITY_PLAYBACK_MAX    5   /* Maximum allowable playback priority */
#define PRIORITY_SYSTEM          18  /* All other firmware threads */
#define PRIORITY_BACKGROUND      20  /* Normal application threads */
#define NUM_PRIORITIES           32
#define PRIORITY_IDLE            32  /* Priority representative of no tasks */

#define PRIORITY_MAIN_THREAD     PRIORITY_USER_INTERFACE
#define IO_PRIORITY_IMMEDIATE    0
#define IO_PRIORITY_BACKGROUND   32

# ifdef HAVE_HARDWARE_CLICK
#  define BASETHREADS  17
# else
#  define BASETHREADS  16
# endif

#ifndef TARGET_EXTRA_THREADS
#define TARGET_EXTRA_THREADS 0
#endif

#define MAXTHREADS (BASETHREADS+TARGET_EXTRA_THREADS)

BITARRAY_TYPE_DECLARE(threadbit_t, threadbit, MAXTHREADS)
BITARRAY_TYPE_DECLARE(priobit_t, priobit, NUM_PRIORITIES)

struct thread_entry;

/*
 * We need more stack when we run under a host
 * maybe more expensive C lib functions?
 *
 * simulator (possibly) doesn't simulate stack usage anyway but well ... */

#if defined(HAVE_SDL_THREADS) || defined(__PCTOOL__)
#define DEFAULT_STACK_SIZE 0x100 /* tiny, ignored anyway */
#else
#include "asm/thread.h"
#endif /* HAVE_SDL_THREADS */

extern void yield(void);
extern unsigned sleep(unsigned ticks);

#ifdef HAVE_PRIORITY_SCHEDULING
#define IF_PRIO(...)    __VA_ARGS__
#define IFN_PRIO(...)
#else
#define IF_PRIO(...)
#define IFN_PRIO(...)   __VA_ARGS__
#endif

#define __wait_queue      lld_head
#define __wait_queue_node lld_node

/* Basic structure describing the owner of an object */
struct blocker
{
    struct thread_entry * volatile thread; /* thread blocking other threads
                                              (aka. object owner) */
#ifdef HAVE_PRIORITY_SCHEDULING
    int priority;                          /* highest priority waiter */
#endif
};

/* If a thread has a blocker but the blocker's registered thread is NULL,
   then it references this and the struct blocker pointer may be
   reinterpreted as such. */
struct blocker_splay
{
    struct blocker  blocker;             /* blocker info (first!) */
#ifdef HAVE_PRIORITY_SCHEDULING
    threadbit_t     mask;                /* mask of nonzero tcounts */
#if NUM_CORES > 1
    struct corelock cl;                  /* mutual exclusion */
#endif
#endif /* HAVE_PRIORITY_SCHEDULING */
};

void core_idle(void);
void core_wake(IF_COP_VOID(unsigned int core));

/* Allocate a thread in the scheduler */
#define CREATE_THREAD_FROZEN   0x00000001 /* Thread is frozen at create time */
unsigned int create_thread(void (*function)(void),
                           void* stack, size_t stack_size,
                           unsigned flags, const char *name
                           IF_PRIO(, int priority)
                           IF_COP(, unsigned int core));

/* Set and clear the CPU frequency boost flag for the calling thread */
#ifdef HAVE_SCHEDULER_BOOSTCTRL
void trigger_cpu_boost(void);
void cancel_cpu_boost(void);
#else
#define trigger_cpu_boost() do { } while(0)
#define cancel_cpu_boost() do { } while(0)
#endif
/* Make a frozen thread runnable (when started with CREATE_THREAD_FROZEN).
 * Has no effect on a thread not frozen. */
void thread_thaw(unsigned int thread_id);
/* Wait for a thread to exit */
void thread_wait(unsigned int thread_id);
/* Exit the current thread */
void thread_exit(void) NORETURN_ATTR;

#ifdef HAVE_PRIORITY_SCHEDULING
int thread_set_priority(unsigned int thread_id, int priority);
int thread_get_priority(unsigned int thread_id);
#endif /* HAVE_PRIORITY_SCHEDULING */

#if NUM_CORES > 1
unsigned int switch_core(unsigned int new_core);
#endif

/* Return the id of the calling thread. */
unsigned int thread_self(void);

/* Debugging info - only! */
#if NUM_CORES > 1
struct core_debug_info
{
    unsigned int idle_stack_usage;
};

int core_get_debug_info(unsigned int core, struct core_debug_info *infop);

#endif /* NUM_CORES */

#ifdef HAVE_SDL_THREADS
#define IF_SDL(x...) x
#define IFN_SDL(x...)
#else
#define IF_SDL(x...)
#define IFN_SDL(x...) x
#endif

struct thread_debug_info
{
    char         statusstr[4];
    char         name[32];
#ifndef HAVE_SDL_THREADS
    unsigned int stack_usage;
#endif
#if NUM_CORES > 1
    unsigned int core;
#endif
#ifdef HAVE_PRIORITY_SCHEDULING
    int          base_priority;
    int          current_priority;
#endif
};
int thread_get_debug_info(unsigned int thread_id,
                          struct thread_debug_info *infop);

#endif /* THREAD_H */
