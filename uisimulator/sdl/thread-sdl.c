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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <time.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <stdlib.h>
#include <memory.h>
#include "thread-sdl.h"
#include "kernel.h"
#include "thread.h"
#include "debug.h"

/* Define this as 1 to show informational messages that are not errors. */
#define THREAD_SDL_DEBUGF_ENABLED 0

#if THREAD_SDL_DEBUGF_ENABLED
#define THREAD_SDL_DEBUGF(...) DEBUGF(__VA_ARGS__)
static char __name[32];
#define THREAD_SDL_GET_NAME(thread) \
    ({ thread_get_name(__name, sizeof(__name)/sizeof(__name[0]), thread); __name; })
#else
#define THREAD_SDL_DEBUGF(...)
#define THREAD_SDL_GET_NAME(thread)
#endif

#define THREAD_PANICF(str...) \
    ({ fprintf(stderr, str); exit(-1); })

struct thread_entry threads[MAXTHREADS];            /* Thread entries as in core */
static SDL_mutex *m;
static SDL_sem *s;
static struct thread_entry *running;

extern long start_tick;

void kill_sim_threads(void)
{
    int i;
    SDL_LockMutex(m);
    for (i = 0; i < MAXTHREADS; i++)
    {
        struct thread_entry *thread = &threads[i];
        if (thread->context.t != NULL)
        {
            SDL_LockMutex(m);
            if (thread->statearg == STATE_RUNNING)
                SDL_SemPost(s);
            else
                SDL_CondSignal(thread->context.c);
            SDL_Delay(10);
            SDL_KillThread(thread->context.t);
            SDL_DestroyCond(thread->context.c);
        }
    }
    SDL_DestroyMutex(m);
    SDL_DestroySemaphore(s);
}

static int find_empty_thread_slot(void)
{
    int n;

    for (n = 0; n < MAXTHREADS; n++)
    {
        if (threads[n].name == NULL)
            break;
    }

    return n;
}

static void add_to_list(struct thread_entry **list,
                        struct thread_entry *thread)
{
    if (*list == NULL)
    {
        /* Insert into unoccupied list */
        thread->next = thread;
        thread->prev = thread;
        *list = thread;
    }
    else
    {
        /* Insert last */
        thread->next = *list;
        thread->prev = (*list)->prev;
        thread->prev->next = thread;
        (*list)->prev = thread;
    }
}

static void remove_from_list(struct thread_entry **list,
                             struct thread_entry *thread)
{
    if (thread == thread->next)
    {
        /* The only item */
        *list = NULL;
        return;
    }

    if (thread == *list)
    {
        /* List becomes next item */
        *list = thread->next;
    }

    /* Fix links to jump over the removed entry. */
    thread->prev->next = thread->next;
    thread->next->prev = thread->prev;
}

struct thread_entry *thread_get_current(void)
{
    return running;
}

void thread_sdl_lock(void)
{
    SDL_LockMutex(m);
}

void thread_sdl_unlock(void)
{
    SDL_UnlockMutex(m);
}

void switch_thread(bool save_context, struct thread_entry **blocked_list)
{
    struct thread_entry *current = running;

    SDL_UnlockMutex(m);

    SDL_SemWait(s);

    SDL_Delay(0);

    SDL_LockMutex(m);
    running = current;

    SDL_SemPost(s);

    (void)save_context; (void)blocked_list;
}

void sleep_thread(int ticks)
{
    struct thread_entry *current;
    int rem;

    current = running;
    current->statearg = STATE_SLEEPING;

    rem = (SDL_GetTicks() - start_tick) % (1000/HZ);
    if (rem < 0)
        rem = 0;

    SDL_UnlockMutex(m);
    SDL_Delay((1000/HZ) * ticks + ((1000/HZ)-1) - rem);
    SDL_LockMutex(m);

    running = current;

    current->statearg = STATE_RUNNING;
}

int runthread(void *data)
{
    struct thread_entry *current;

    /* Cannot access thread variables before locking the mutex as the
       data structures may not be filled-in yet. */
    SDL_LockMutex(m);
    running = (struct thread_entry *)data;
    current = running;
    current->context.start();

    THREAD_SDL_DEBUGF("Thread Done: %d (%s)\n",
                      current - threads, THREAD_SDL_GET_NAME(current));
    remove_thread(NULL);
    return 0;
}

struct thread_entry* 
    create_thread(void (*function)(void), void* stack, int stack_size,
                  const char *name)
{
    /** Avoid compiler warnings */
    SDL_Thread* t;
    SDL_cond *cond;
    int slot;

    THREAD_SDL_DEBUGF("Creating thread: (%s)\n", name ? name : "");

    slot = find_empty_thread_slot();
    if (slot >= MAXTHREADS)
    {
        DEBUGF("Failed to find thread slot\n");
        return NULL;
    }

    cond = SDL_CreateCond();
    if (cond == NULL)
    {
        DEBUGF("Failed to create condition variable\n");
        return NULL;
    }

    t = SDL_CreateThread(runthread, &threads[slot]);
    if (t == NULL)
    {
        DEBUGF("Failed to create SDL thread\n");
        SDL_DestroyCond(cond);
        return NULL;
    }

    threads[slot].stack = stack;
    threads[slot].stack_size = stack_size;
    threads[slot].name = name;
    threads[slot].statearg = STATE_RUNNING;
    threads[slot].context.start = function;
    threads[slot].context.t = t;
    threads[slot].context.c = cond;

    THREAD_SDL_DEBUGF("New Thread: %d (%s)\n",
                      slot, THREAD_SDL_GET_NAME(&threads[slot]));

    return &threads[slot];
}

void block_thread(struct thread_entry **list)
{
    struct thread_entry *thread = running;

    thread->statearg = STATE_BLOCKED;
    add_to_list(list, thread);

    SDL_CondWait(thread->context.c, m);
    running = thread;
}

void block_thread_w_tmo(struct thread_entry **list, int ticks)
{
    struct thread_entry *thread = running;

    thread->statearg = STATE_BLOCKED_W_TMO;
    add_to_list(list, thread);

    SDL_CondWaitTimeout(thread->context.c, m, (1000/HZ) * ticks);
    running = thread;

    if (thread->statearg == STATE_BLOCKED_W_TMO)
    {
        /* Timed out */
        remove_from_list(list, thread);
        thread->statearg = STATE_RUNNING;
    }
}

void wakeup_thread(struct thread_entry **list)
{
    struct thread_entry *thread = *list;

    if (thread == NULL)
    {
        return;
    }

    switch (thread->statearg)
    {
    case STATE_BLOCKED:
    case STATE_BLOCKED_W_TMO:
        remove_from_list(list, thread);
        thread->statearg = STATE_RUNNING;
        SDL_CondSignal(thread->context.c);
        break;
    }
}

void init_threads(void)
{
    int slot;

    m = SDL_CreateMutex();
    s = SDL_CreateSemaphore(0);

    memset(threads, 0, sizeof(threads));

    slot = find_empty_thread_slot();
    if (slot >= MAXTHREADS)
    {
        THREAD_PANICF("Couldn't find slot for main thread.\n");
    }

    threads[slot].stack = "       ";
    threads[slot].stack_size = 8;
    threads[slot].name = "main";
    threads[slot].statearg = STATE_RUNNING;
    threads[slot].context.t = gui_thread;
    threads[slot].context.c = SDL_CreateCond();

    running = &threads[slot];

    THREAD_SDL_DEBUGF("First Thread: %d (%s)\n",
            slot, THREAD_SDL_GET_NAME(&threads[slot]));

    if (SDL_LockMutex(m) == -1) {
        THREAD_PANICF("Couldn't lock mutex\n");
    }

    if (SDL_SemPost(s) == -1) {
        THREAD_PANICF("Couldn't post to semaphore\n");
    }
}

void remove_thread(struct thread_entry *thread)
{
    struct thread_entry *current = running;
    SDL_Thread *t;
    SDL_cond *c;

    if (thread == NULL)
    {
        thread = current;
    }

    t = thread->context.t;
    c = thread->context.c;
    thread->context.t = NULL;

    if (thread != current)
    {
        if (thread->statearg == STATE_RUNNING)
            SDL_SemPost(s);
        else
            SDL_CondSignal(c);
    }

    THREAD_SDL_DEBUGF("Removing thread: %d (%s)\n",
        thread - threads, THREAD_SDL_GET_NAME(thread));

    thread->name = NULL;

    SDL_DestroyCond(c);

    if (thread == current)
    {
        SDL_UnlockMutex(m);
    }

    SDL_KillThread(t);
}

int thread_stack_usage(const struct thread_entry *thread)
{
    return 50;
    (void)thread;
}

int thread_get_status(const struct thread_entry *thread)
{
    return thread->statearg;
}

/* Return name if one or ID if none */
void thread_get_name(char *buffer, int size,
                     struct thread_entry *thread)
{
    if (size <= 0)
        return;

    *buffer = '\0';

    if (thread)
    {
        /* Display thread name if one or ID if none */
        bool named = thread->name && *thread->name;
        const char *fmt = named ? "%s" : "%08lX";
        intptr_t name = named ?
            (intptr_t)thread->name : (intptr_t)thread;
        snprintf(buffer, size, fmt, name);
    }
}
