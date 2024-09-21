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

#include <stdbool.h>
#include <time.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <stdlib.h>
#include <string.h> /* memset() */
#include <setjmp.h>
#include "system-sdl.h"
#include "thread-sdl.h"
#include "../kernel-internal.h"
#include "core_alloc.h"

/* Define this as 1 to show informational messages that are not errors. */
#define THREAD_SDL_DEBUGF_ENABLED 1

#if THREAD_SDL_DEBUGF_ENABLED
#define THREAD_SDL_DEBUGF(...) DEBUGF(__VA_ARGS__)
static char __name[sizeof (((struct thread_debug_info *)0)->name)];
#define THREAD_SDL_GET_NAME(thread) \
    ({ format_thread_name(__name, sizeof (__name), thread); __name; })
#else
#define THREAD_SDL_DEBUGF(...)
#define THREAD_SDL_GET_NAME(thread)
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
static SDL_mutex *m;
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
    SDL_LockMutex(m);

    /* Signal all threads on delay or block */
    for (i = 0; i < MAXTHREADS; i++)
    {
        struct thread_entry *thread = __thread_slot_entry(i);
        if (thread->context.s == NULL)
            continue;
        SDL_SemPost(thread->context.s);
    }

    /* Wait for all threads to finish and cleanup old ones. */
    for (i = 0; i < MAXTHREADS; i++)
    {
        struct thread_entry *thread = __thread_slot_entry(i);
        SDL_Thread *t = thread->context.t;

        if (t != NULL)
        {
            SDL_UnlockMutex(m);
            /* Wait for it to finish */
            SDL_WaitThread(t, NULL);
            /* Relock for next thread signal */
            SDL_LockMutex(m);
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
            SDL_WaitThread(thread->context.told, NULL);
        }
    }

    SDL_UnlockMutex(m);

    /* Signal completion of operation */
    threads_status = THREADS_EXIT_COMMAND_DONE;
}

void sim_thread_exception_wait(void)
{
    while (1)
    {
        SDL_Delay(HZ/10);
        if (threads_status != THREADS_RUN)
            thread_exit();
    }
}

/* A way to yield and leave the threading system for extended periods */
void sim_thread_lock(void *me)
{
    SDL_LockMutex(m);
    __running_self_entry() = (struct thread_entry *)me;

    if (threads_status != THREADS_RUN)
        thread_exit();
}

void * sim_thread_unlock(void)
{
    struct thread_entry *current = __running_self_entry();
    SDL_UnlockMutex(m);
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
        SDL_UnlockMutex(m);
        /* Any other thread waiting already will get it first */
        SDL_LockMutex(m);
        break;
        } /* STATE_RUNNING: */

    case STATE_BLOCKED:
    {
        int oldlevel;

        SDL_UnlockMutex(m);
        SDL_SemWait(current->context.s);
        SDL_LockMutex(m);

        oldlevel = disable_irq_save();
        current->state = STATE_RUNNING;
        restore_irq(oldlevel);
        break;
        } /* STATE_BLOCKED: */

    case STATE_BLOCKED_W_TMO:
    {
        int result, oldlevel;

        SDL_UnlockMutex(m);
        result = SDL_SemWaitTimeout(current->context.s, current->tmo_tick);
        SDL_LockMutex(m);

        oldlevel = disable_irq_save();

        current->state = STATE_RUNNING;

        if (result == SDL_MUTEX_TIMEDOUT)
        {
            /* Other signals from an explicit wake could have been made before
             * arriving here if we timed out waiting for the semaphore. Make
             * sure the count is reset. */
            while (SDL_SemValue(current->context.s) > 0)
                SDL_SemTryWait(current->context.s);
        }

        restore_irq(oldlevel);
        break;
        } /* STATE_BLOCKED_W_TMO: */

    case STATE_SLEEPING:
    {
        SDL_UnlockMutex(m);
        SDL_SemWaitTimeout(current->context.s, current->tmo_tick);
        SDL_LockMutex(m);
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

    rem = (SDL_GetTicks() - start_tick) % (1000/HZ);
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

unsigned int wakeup_thread_(struct thread_entry *thread)
{
    switch (thread->state)
    {
    case STATE_BLOCKED:
    case STATE_BLOCKED_W_TMO:
        wait_queue_remove(thread);
        thread->state = STATE_RUNNING;
        SDL_SemPost(thread->context.s);
        return THREAD_OK;
    }

    return THREAD_NONE;
}

void thread_thaw(unsigned int thread_id)
{
    struct thread_entry *thread = __thread_id_entry(thread_id);

    if (thread->id == thread_id && thread->state == STATE_FROZEN)
    {
        thread->state = STATE_RUNNING;
        SDL_SemPost(thread->context.s);
    }
}

int runthread(void *data)
{
    /* Cannot access thread variables before locking the mutex as the
       data structures may not be filled-in yet. */
    SDL_LockMutex(m);

    struct thread_entry *current = (struct thread_entry *)data;
    __running_self_entry() = current;

    jmp_buf *current_jmpbuf = &thread_jmpbufs[THREAD_ID_SLOT(current->id)];

    /* Setup jump for exit */
    if (setjmp(*current_jmpbuf) == 0)
    {
        /* Run the thread routine */
        if (current->state == STATE_FROZEN)
        {
            SDL_UnlockMutex(m);
            SDL_SemWait(current->context.s);
            SDL_LockMutex(m);
            __running_self_entry() = current;
        }

        if (threads_status == THREADS_RUN)
        {
            current->context.start();
            THREAD_SDL_DEBUGF("Thread Done: %d (%s)\n",
                              THREAD_ID_SLOT(current->id),
                              THREAD_SDL_GET_NAME(current));
            /* Thread routine returned - suicide */
        }

        thread_exit();
    }
    else
    {
        /* Unlock and exit */
        SDL_UnlockMutex(m);
    }

    return 0;
}

unsigned int create_thread(void (*function)(void),
                           void* stack, size_t stack_size,
                           unsigned flags, const char *name)
{
    THREAD_SDL_DEBUGF("Creating thread: (%s)\n", name ? name : "");

    struct thread_entry *thread = thread_alloc();
    if (thread == NULL)
    {
        DEBUGF("Failed to find thread slot\n");
        return 0;
    }

    SDL_sem *s = SDL_CreateSemaphore(0);
    if (s == NULL)
    {
        DEBUGF("Failed to create semaphore\n");
        return 0;
    }

    SDL_Thread *t = SDL_CreateThread(runthread, NULL, thread);
    if (t == NULL)
    {
        DEBUGF("Failed to create SDL thread\n");
        SDL_DestroySemaphore(s);
        return 0;
    }

    thread->name = name;
    thread->state = (flags & CREATE_THREAD_FROZEN) ?
        STATE_FROZEN : STATE_RUNNING;
    thread->context.start = function;
    thread->context.t = t;
    thread->context.s = s;

    THREAD_SDL_DEBUGF("New Thread: %lu (%s)\n",
                      (unsigned long)thread->id,
                      THREAD_SDL_GET_NAME(thread));

    return thread->id;
    (void)stack; (void)stack_size;
}

void thread_exit(void)
{
    struct thread_entry *current = __running_self_entry();

    int oldlevel = disable_irq_save();

    SDL_Thread *t = current->context.t;
    SDL_sem *s = current->context.s;

    /* Wait the last thread here and keep this one or SDL will leak it since
     * it doesn't free its own library allocations unless a wait is performed.
     * Such behavior guards against the memory being invalid by the time
     * SDL_WaitThread is reached and also against two different threads having
     * the same pointer. It also makes SDL_WaitThread a non-concurrent function.
     *
     * However, see more below about SDL_KillThread.
     */
    SDL_WaitThread(current->context.told, NULL);

    current->context.t = NULL;
    current->context.s = NULL;
    current->context.told = t;

    unsigned int id = current->id;
    new_thread_id(current);
    current->state = STATE_KILLED;
    wait_queue_wake(&current->queue);

    SDL_DestroySemaphore(s);

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
        block_thread(current, TIMEOUT_BLOCK, &thread->queue);
        switch_thread();
    }
}

/* Initialize SDL threading */
void init_threads(void)
{
    m = SDL_CreateMutex();

    if (SDL_LockMutex(m) == -1)
    {
        fprintf(stderr, "Couldn't lock mutex\n");
        return;
    }

    thread_alloc_init();

    struct thread_entry *thread = thread_alloc();
    if (thread == NULL)
    {
        fprintf(stderr, "Main thread alloc failed\n");
        return;
    }

    /* Slot 0 is reserved for the main thread - initialize it here and
       then create the SDL thread - it is possible to have a quick, early
       shutdown try to access the structure. */
    thread->name = __main_thread_name;
    thread->state = STATE_RUNNING;
    thread->context.s = SDL_CreateSemaphore(0);
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
        THREAD_SDL_DEBUGF("Main Thread: %lu (%s)\n",
                          (unsigned long)thread->id,
                          THREAD_SDL_GET_NAME(thread));
        return;
    }

    SDL_UnlockMutex(m);

    /* Set to 'COMMAND_DONE' when other rockbox threads have exited. */
    while (threads_status < THREADS_EXIT_COMMAND_DONE)
        SDL_Delay(10);

    SDL_DestroyMutex(m);

    /* We're the main thead - perform exit - doesn't return. */
    sim_do_exit();
}
