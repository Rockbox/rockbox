/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
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

#include "autoconf.h"

#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <string.h> /* memset() */
#include <setjmp.h>
#include "system-ctru.h"
#include "thread-ctru.h"
#include "sys_thread.h"
#include "../kernel-internal.h"
#include "core_alloc.h"

/* Define this as 1 to show informational messages that are not errors. */
#define THREAD_DEBUGF_ENABLED 0

#if THREAD_DEBUGF_ENABLED
#define THREAD_DEBUGF(...) DEBUGF(__VA_ARGS__)
static char __name[sizeof (((struct thread_debug_info *)0)->name)];
#define THREAD_GET_NAME(thread) \
    ({ format_thread_name(__name, sizeof (__name), thread); __name; })
#else
#define THREAD_DEBUGF(...)
#define THREAD_GET_NAME(thread)
#endif

#define THREAD_PANICF(str...) \
    ({ fprintf(stderr, str); exit(-1); })

/* Jump buffers for graceful exit - kernel threads don't stay neatly
 * in their start routines responding to messages so this is the only
 * way to get them back in there so they may exit */
static jmp_buf thread_jmpbufs[MAXTHREADS];
/* this mutex locks out other Rockbox threads while one runs,
 * that enables us to simulate a cooperative environment even if
 * the host is preemptive */
static RecursiveLock m;
#define THREADS_RUN                 0
#define THREADS_EXIT                1
#define THREADS_EXIT_COMMAND_DONE   2
static volatile int threads_status = THREADS_RUN;

extern long start_tick;

void sim_thread_shutdown(void)
{
    int i;

    /* This *has* to be a push operation from a thread not in the pool
       so that they may be dislodged from their blocking calls. */

    /* Tell all threads jump back to their start routines, unlock and exit
       gracefully - we'll check each one in turn for it's status. Threads
       _could_ terminate via thread_exit or multiple threads could exit
       on each unlock but that is safe. */

    /* Do this before trying to acquire lock */
    threads_status = THREADS_EXIT;

    /* Take control */
    RecursiveLock_Lock(&m);

    /* Signal all threads on delay or block */
    for (i = 0; i < MAXTHREADS; i++)
    {
        struct thread_entry *thread = __thread_slot_entry(i);
        if (thread->context.s == NULL)
            continue;
        LightSemaphore_Release(thread->context.s, 1);
    }

    /* Wait for all threads to finish and cleanup old ones. */
    for (i = 0; i < MAXTHREADS; i++)
    {
        struct thread_entry *thread = __thread_slot_entry(i);
        sysThread *t = thread->context.t;

        if (t != NULL)
        {
            RecursiveLock_Unlock(&m);
            /* Wait for it to finish */
            sys_wait_thread(t, NULL);
            /* Relock for next thread signal */
            RecursiveLock_Lock(&m);
            /* Already waited and exiting thread would have waited .told,
             * replacing it with t. */
            thread->context.told = NULL;
        }
        else
        {
            /* Wait on any previous thread in this location-- could be one not
             * quite finished exiting but has just unlocked the mutex. If it's
             * NULL, the call returns immediately.
             *
             * See thread_exit below for more information. */
            sys_wait_thread(thread->context.told, NULL);
        }
    }

    RecursiveLock_Unlock(&m);

    /* Signal completion of operation */
    threads_status = THREADS_EXIT_COMMAND_DONE;
}

void sim_thread_exception_wait(void)
{
    while (1)
    {
        sys_delay(HZ/10);
        if (threads_status != THREADS_RUN)
            thread_exit();
    }
}

/* A way to yield and leave the threading system for extended periods */
void sim_thread_lock(void *me)
{
    RecursiveLock_Lock(&m);
    __running_self_entry() = (struct thread_entry *)me;

    if (threads_status != THREADS_RUN)
        thread_exit();
}

void * sim_thread_unlock(void)
{
    struct thread_entry *current = __running_self_entry();
    RecursiveLock_Unlock(&m);
    return current;
}

void switch_thread(void)
{
    struct thread_entry *current = __running_self_entry();

    enable_irq();

    switch (current->state)
    {
    case STATE_RUNNING:
    {
        RecursiveLock_Unlock(&m);
        /* Any other thread waiting already will get it first */
        RecursiveLock_Lock(&m);
        break;
        } /* STATE_RUNNING: */

    case STATE_BLOCKED:
    {
        int oldlevel;

        RecursiveLock_Unlock(&m);
        sys_sem_wait(current->context.s);
        RecursiveLock_Lock(&m);

        oldlevel = disable_irq_save();
        current->state = STATE_RUNNING;
        restore_irq(oldlevel);
        break;
        } /* STATE_BLOCKED: */

    case STATE_BLOCKED_W_TMO:
    {
        int result, oldlevel;

        RecursiveLock_Unlock(&m);
        result = sys_sem_wait_timeout(current->context.s, current->tmo_tick);
        RecursiveLock_Lock(&m);

        oldlevel = disable_irq_save();

        current->state = STATE_RUNNING;

        if (result == 1)
        {
            /* Other signals from an explicit wake could have been made before
             * arriving here if we timed out waiting for the semaphore. Make
             * sure the count is reset. */
            while (sys_sem_value(current->context.s) > 0)
                sys_sem_try_wait(current->context.s);
        }

        restore_irq(oldlevel);
        break;
        } /* STATE_BLOCKED_W_TMO: */

    case STATE_SLEEPING:
    {
        RecursiveLock_Unlock(&m);
        sys_sem_wait_timeout(current->context.s, current->tmo_tick);
        RecursiveLock_Lock(&m);
        current->state = STATE_RUNNING;
        break;
        } /* STATE_SLEEPING: */
    }

#ifdef BUFLIB_DEBUG_CHECK_VALID
    core_check_valid();
#endif
    __running_self_entry() = current;

    if (threads_status != THREADS_RUN)
        thread_exit();
}

void sleep_thread(int ticks)
{
    struct thread_entry *current = __running_self_entry();
    int rem;

    current->state = STATE_SLEEPING;

    rem = (sys_get_ticks() - start_tick) % (1000/HZ);
    if (rem < 0)
        rem = 0;

    current->tmo_tick = (1000/HZ) * ticks + ((1000/HZ)-1) - rem;
}

void block_thread_(struct thread_entry *current, int ticks)
{
    if (ticks < 0)
        current->state = STATE_BLOCKED;
    else
    {
        current->state = STATE_BLOCKED_W_TMO;
        current->tmo_tick = (1000/HZ)*ticks;
    }

    wait_queue_register(current);
}

unsigned int wakeup_thread_(struct thread_entry *thread
                            IF_PRIO(, enum wakeup_thread_protocol proto))
{
    switch (thread->state)
    {
    case STATE_BLOCKED:
    case STATE_BLOCKED_W_TMO:
        wait_queue_remove(thread);
        thread->state = STATE_RUNNING;
        LightSemaphore_Release(thread->context.s, 1);
        return THREAD_OK;
    }

    return THREAD_NONE;
    (void) proto;
}

void thread_thaw(unsigned int thread_id)
{
    struct thread_entry *thread = __thread_id_entry(thread_id);

    if (thread->id == thread_id && thread->state == STATE_FROZEN)
    {
        thread->state = STATE_RUNNING;
        LightSemaphore_Release(thread->context.s, 1);
    }
}

int runthread(void *data)
{
    /* Cannot access thread variables before locking the mutex as the
       data structures may not be filled-in yet. */
    RecursiveLock_Lock(&m);

    struct thread_entry *current = (struct thread_entry *)data;
    __running_self_entry() = current;

    jmp_buf *current_jmpbuf = &thread_jmpbufs[THREAD_ID_SLOT(current->id)];

    /* Setup jump for exit */
    if (setjmp(*current_jmpbuf) == 0)
    {
        /* Run the thread routine */
        if (current->state == STATE_FROZEN)
        {
            RecursiveLock_Unlock(&m);
            sys_sem_wait(current->context.s);
            RecursiveLock_Lock(&m);
            __running_self_entry() = current;
        }

        if (threads_status == THREADS_RUN)
        {
            current->context.start();
            THREAD_DEBUGF("Thread Done: %d (%s)\n",
                              THREAD_ID_SLOT(current->id),
                              THREAD_GET_NAME(current));
            /* Thread routine returned - suicide */
        }

        thread_exit();
    }
    else
    {
        /* Unlock and exit */
        RecursiveLock_Unlock(&m);
    }

    return 0;
}

unsigned int create_thread(void (*function)(void),
                           void* stack, size_t stack_size,
                           unsigned flags, const char *name
                           IF_PRIO(, int priority)
                           IF_COP(, unsigned int core))
{
    THREAD_DEBUGF("Creating thread: (%s)\n", name ? name : "");

    struct thread_entry *thread = thread_alloc();
    if (thread == NULL)
    {
        DEBUGF("Failed to find thread slot\n");
        return 0;
    }

    LightSemaphore *s = (LightSemaphore *) malloc(sizeof(LightSemaphore));
    if (s == NULL)
    {
        DEBUGF("Failed to create semaphore\n");
        return 0;
    }

    LightSemaphore_Init(s, 0, 255);

    sysThread *t = sys_create_thread(runthread,
                                     name,
                                     stack_size,
                                     thread
                                     IF_PRIO(, priority)
                                     IF_COP(, core));
    if (t == NULL)
    {
        DEBUGF("Failed to create thread\n");
        free(s);
        return 0;
    }

    thread->name = name;
    thread->state = (flags & CREATE_THREAD_FROZEN) ?
        STATE_FROZEN : STATE_RUNNING;
    thread->context.start = function;
    thread->context.t = t;
    thread->context.s = s;
    thread->priority = priority;

    THREAD_DEBUGF("New Thread: %lu (%s)\n",
                      (unsigned long)thread->id,
                      THREAD_GET_NAME(thread));

    return thread->id;
    (void)stack;
}

void thread_exit(void)
{
    struct thread_entry *current = __running_self_entry();

    int oldlevel = disable_irq_save();

    sysThread *t = current->context.t;
    LightSemaphore *s = current->context.s;

    /* Wait the last thread here and keep this one or ctru will leak it since
     * it doesn't free its own library allocations unless a wait is performed.
     * Such behavior guards against the memory being invalid by the time
     * sys_wait_thread is reached and also against two different threads having
     * the same pointer. It also makes sys_wait_thread a non-concurrent function.
     *
     * However, see more below about sys_kill_thread.
     */
    sys_wait_thread(current->context.told, NULL);

    current->context.t = NULL;
    current->context.s = NULL;
    current->context.told = t;

    unsigned int id = current->id;
    new_thread_id(current);
    current->state = STATE_KILLED;
    wait_queue_wake(&current->queue);

    free(s);

    /* Do a graceful exit - perform the longjmp back into the thread
       function to return */
    restore_irq(oldlevel);

    thread_free(current);

    longjmp(thread_jmpbufs[THREAD_ID_SLOT(id)], 1);

    /* This should never and must never be reached - if it is, the
     * state is corrupted */
    THREAD_PANICF("thread_exit->K:*R (ID: %d)", id);
    while (1);
}

void thread_wait(unsigned int thread_id)
{
    struct thread_entry *current = __running_self_entry();
    struct thread_entry *thread = __thread_id_entry(thread_id);

    if (thread->id == thread_id && thread->state != STATE_KILLED)
    {
        block_thread(current, TIMEOUT_BLOCK, &thread->queue, NULL);
        switch_thread();
    }
}

int thread_set_priority(unsigned int thread_id, int priority)
{
    struct thread_entry *thread = __thread_id_entry(thread_id);
    sysThread *t = thread->context.t;
    thread->priority = sys_set_thread_priority(t, priority);
    return thread->priority;
}

int thread_get_priority(unsigned int thread_id)
{
    struct thread_entry *thread = __thread_id_entry(thread_id);
    return thread->priority;
}

/* Initialize threading */
void init_threads(void)
{
    RecursiveLock_Init(&m);
    RecursiveLock_Lock(&m);

    thread_alloc_init();

    struct thread_entry *thread = thread_alloc();
    if (thread == NULL)
    {
        fprintf(stderr, "Main thread alloc failed\n");
        return;
    }

    /* Slot 0 is reserved for the main thread - initialize it here and
       then create the thread - it is possible to have a quick, early
       shutdown try to access the structure. */
    thread->name = __main_thread_name;
    thread->state = STATE_RUNNING;
    thread->context.s = (LightSemaphore *) malloc(sizeof(LightSemaphore));
    LightSemaphore_Init(thread->context.s, 0, 255);
    thread->context.t = NULL; /* NULL for the implicit main thread */
    __running_self_entry() = thread;

    if (thread->context.s == NULL)
    {
        fprintf(stderr, "Failed to create main semaphore\n");
        return;
    }

    /* Tell all threads jump back to their start routines, unlock and exit
       gracefully - we'll check each one in turn for it's status. Threads
       _could_ terminate via thread_exit or multiple threads could exit
       on each unlock but that is safe. */

    /* Setup jump for exit */
    if (setjmp(thread_jmpbufs[THREAD_ID_SLOT(thread->id)]) == 0)
    {
        THREAD_DEBUGF("Main Thread: %lu (%s)\n",
                          (unsigned long)thread->id,
                          THREAD_GET_NAME(thread));
        return;
    }

    RecursiveLock_Unlock(&m);

    /* Set to 'COMMAND_DONE' when other rockbox threads have exited. */
    while (threads_status < THREADS_EXIT_COMMAND_DONE)
        sys_delay(10);

    /* We're the main thead - perform exit - doesn't return. */
    sim_do_exit();
}

