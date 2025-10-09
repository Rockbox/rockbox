/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Mauricio Ga.
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

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "sys_thread.h"
#include "sys_timer.h"
#include "debug.h"
#include "logf.h"

bool _AtomicCAS(u32 *ptr, int oldval, int newval)
{
    int expected = oldval;
    int desired = newval;
    return __atomic_compare_exchange(ptr, &expected, &desired, false,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

bool _AtomicTryLock(int *lock)
{
    int result;
    asm volatile(
        "ldrex    %0, [%2]          \n"
        "teq      %0, #0            \n"
        "strexeq  %0, %1, [%2]      \n"
        : "=&r"(result)
        : "r"(1), "r"(lock)
        : "cc", "memory"
    );
    return result == 0;
}

#define CPUPauseInstruction() asm volatile("yield" ::: "memory")
void AtomicLock(int *lock)
{
    int iterations = 0;
    while (!_AtomicTryLock(lock)) {
        if (iterations < 32) {
            iterations++;
            CPUPauseInstruction();
        } else {
            sys_delay(0);
        }
    }
}

void AtomicUnlock(int *lock)
{
    *lock = 0;
}

/* Convert rockbox priority value to libctru value */
int get_ctru_thread_priority(int priority)
{
    if ((priority == PRIORITY_REALTIME_1) || (priority == PRIORITY_REALTIME_2) ||
        (priority == PRIORITY_REALTIME_3) || (priority == PRIORITY_REALTIME_4) ||
        (priority == PRIORITY_REALTIME))
        return 0x18;
    else if (priority == PRIORITY_BUFFERING)
        return 0x18; /* Highest */
    else if ((priority == PRIORITY_USER_INTERFACE) || (priority == PRIORITY_RECORDING) ||
             (priority == PRIORITY_PLAYBACK))
        return 0x30;
    else if (priority == PRIORITY_PLAYBACK_MAX)
        return 0x2F;
    else if (priority == PRIORITY_SYSTEM)
        return 0x30;
    else if (priority == PRIORITY_BACKGROUND)
        return 0x3F; /* Lowest */
    else
        return 0x30;
}

static size_t get_thread_stack_size(size_t requested_size)
{
    if (requested_size == 0) {
        return (80 * 1024); /* 80 kB */
    }

    return requested_size;
}

static void thread_entry(void *arg)
{
    sys_run_thread((sysThread *)arg);
    threadExit(0);
}

int wait_on_semaphore_for(LightSemaphore *sem, u32 timeout)
{
    u64 stop_time = sys_get_ticks64() + timeout;
    u64 current_time = sys_get_ticks64();
    while (current_time < stop_time) {
        if (LightSemaphore_TryAcquire(sem, 1) == 0) {
            return 0;
        }
        /* 100 microseconds seems to be the sweet spot */
        svcSleepThread(100000LL);
        current_time = sys_get_ticks64();
    }

    /* If we failed, yield to avoid starvation on busy waits */
    svcSleepThread(1);
    return 1;
}

int sys_sem_try_wait(LightSemaphore *sem)
{
    if (LightSemaphore_TryAcquire(sem, 1) != 0) {
        /* If we failed, yield to avoid starvation on busy waits */
        svcSleepThread(1);
        return 1;
    }

    return 0;
}

int sys_sem_wait_timeout(LightSemaphore *sem, u32 timeout)
{
    if (timeout == (~(u32)0)) {
        LightSemaphore_Acquire(sem, 1);
        return 0;
    }

    if (LightSemaphore_TryAcquire(sem, 1) != 0) {
        return wait_on_semaphore_for(sem, timeout);
    }

    return 0;
}

int sys_sem_wait(LightSemaphore *sem)
{
    return sys_sem_wait_timeout(sem, (~(u32)0));
}

u32 sys_sem_value(LightSemaphore *sem)
{
    return sem->current_count;
}

int sys_thread_id(void)
{
    u32 thread_ID = 0;
    svcGetThreadId(&thread_ID, CUR_THREAD_HANDLE);
    return (int)thread_ID;
}

void sys_run_thread(sysThread *thread)
{
    void *userdata = thread->userdata;
    int(* userfunc)(void *) = thread->userfunc;

    int *statusloc = &thread->status;

    /* Get the thread id */
    thread->threadid = sys_thread_id();

    /* Run the function */
    *statusloc = userfunc(userdata);

    /* Mark us as ready to be joined (or detached) */
    if (!AtomicCAS(&thread->state, THREAD_STATE_ALIVE, THREAD_STATE_ZOMBIE)) {
        /* Clean up if something already detached us. */
        if (AtomicCAS(&thread->state, THREAD_STATE_DETACHED, THREAD_STATE_CLEANED)) {
            free(thread);
        }
    }
}

sysThread *sys_create_thread(int(*fn)(void *), const char *name, const size_t stacksize,
                           void *data IF_PRIO(, int priority) IF_COP(, unsigned int core))
{
    sys_ticks_init();

    /* Allocate memory for the thread info structure */
    sysThread *thread = (sysThread *) calloc(1, sizeof(sysThread));
    if (thread == NULL) {
        DEBUGF("sys_create_thread: could not allocate memory\n");
        return NULL;
    }
    thread->status = -1;
    AtomicSet(&thread->state, THREAD_STATE_ALIVE);

    /* Set up the arguments for the thread */
    thread->userfunc = fn;
    thread->userdata = data;
    thread->stacksize = stacksize;
    
    int cpu = -1;
    if (name && (strncmp(name, "tagcache", 8) == 0) && R_SUCCEEDED(APT_SetAppCpuTimeLimit(30))) {
        cpu = 1;
        printf("thread: %s, running in cpu 1\n", name);
    }

    thread->handle = threadCreate(thread_entry,
                                  thread,
                                  get_thread_stack_size(stacksize),
                                  get_ctru_thread_priority(priority),
                                  cpu,
                                  false);

    if (!thread->handle) {
        DEBUGF("sys_create_thread: threadCreate failed\n");
        free(thread);
        thread = NULL;
    }

    /* Everything is running now */
    return thread;
}

void sys_wait_thread(sysThread *thread, int *status)
{
    if (thread) {
        Result res = threadJoin(thread->handle, U64_MAX);

        /*
          Detached threads can be waited on, but should NOT be cleaned manually
          as it would result in a fatal error.
        */
        if (R_SUCCEEDED(res) && AtomicGet(&thread->state) != THREAD_STATE_DETACHED) {
            threadFree(thread->handle);
        }
        if (status) {
            *status = thread->status;
        }
        free(thread);
    }
}

void sys_detach_thread(sysThread *thread)
{
    if (!thread) {
        return;
    }

    /* Grab dibs if the state is alive+joinable. */
    if (AtomicCAS(&thread->state, THREAD_STATE_ALIVE, THREAD_STATE_DETACHED)) {
        threadDetach(thread->handle);
    } else {
        /* all other states are pretty final, see where we landed. */
        const int thread_state = AtomicGet(&thread->state);
        if ((thread_state == THREAD_STATE_DETACHED) || (thread_state == THREAD_STATE_CLEANED)) {
            return; /* already detached (you shouldn't call this twice!) */
        } else if (thread_state == THREAD_STATE_ZOMBIE) {
            sys_wait_thread(thread, NULL); /* already done, clean it up. */
        } else {
            assert(0 && "Unexpected thread state");
        }
    }
}

int sys_set_thread_priority(sysThread *thread, int priority)
{   
    Handle h = threadGetHandle(thread->handle);
    int old_priority = priority;
    Result res = svcSetThreadPriority(h, get_ctru_thread_priority(priority));
    if (R_SUCCEEDED(res)) {
        return priority;
    }

    return old_priority;
}

/* sysCond */
sysCond *sys_cond_create(void)
{
    sysCond *cond;

    cond = (sysCond *)malloc(sizeof(sysCond));
    if (cond) {
        RecursiveLock_Init(&cond->lock);
        LightSemaphore_Init(&cond->wait_sem, 0, ((s16)0x7FFF));
        LightSemaphore_Init(&cond->wait_done, 0, ((s16)0x7FFF));
        cond->waiting = cond->signals = 0;
    } else {
        DEBUGF("sys_cond_create: out of memory.\n");;
    }
    return cond;
}

/* Destroy a condition variable */
void sys_cond_destroy(sysCond *cond)
{
    if (cond) {
        free(cond);
    }
}

/* Restart one of the threads that are waiting on the condition variable */
int sys_cond_signal(sysCond *cond)
{
    if (!cond) {
        DEBUGF("sys_cond_signal: Invalid param 'cond'\n");
        return -1;
    }

    /* If there are waiting threads not already signalled, then
       signal the condition and wait for the thread to respond.
     */
    RecursiveLock_Lock(&cond->lock);
    if (cond->waiting > cond->signals) {
        ++cond->signals;
        LightSemaphore_Release(&cond->wait_sem, 1);
        RecursiveLock_Unlock(&cond->lock);
        LightSemaphore_Acquire(&cond->wait_done, 1);
    } else {
        RecursiveLock_Unlock(&cond->lock);
    }

    return 0;
}

/* Restart all threads that are waiting on the condition variable */
int sys_cond_broadcast(sysCond *cond)
{
    if (!cond) {
        DEBUGF("sys_cond_signal: Invalid param 'cond'\n");
        return -1;
    }

    /* If there are waiting threads not already signalled, then
       signal the condition and wait for the thread to respond.
     */
    RecursiveLock_Lock(&cond->lock);
    if (cond->waiting > cond->signals) {
        int i, num_waiting;

        num_waiting = (cond->waiting - cond->signals);
        cond->signals = cond->waiting;
        for (i = 0; i < num_waiting; ++i) {
            LightSemaphore_Release(&cond->wait_sem, 1);
        }
        /* Now all released threads are blocked here, waiting for us.
           Collect them all (and win fabulous prizes!) :-)
         */
        RecursiveLock_Unlock(&cond->lock);
        for (i = 0; i < num_waiting; ++i) {
            LightSemaphore_Acquire(&cond->wait_done, 1);
        }
    } else {
        RecursiveLock_Unlock(&cond->lock);
    }

    return 0;
}

int sys_cond_wait(sysCond *cond, RecursiveLock *mutex)
{
    if (!cond) {
        DEBUGF("sys_cond_signal: Invalid param 'cond'\n");
        return -1;
    }

    RecursiveLock_Lock(&cond->lock);
    ++cond->waiting;
    RecursiveLock_Unlock(&cond->lock);

    /* Unlock the mutex, as is required by condition variable semantics */
    RecursiveLock_Unlock(mutex);

    /* Wait for a signal */
    LightSemaphore_Acquire(&cond->wait_sem, 1);

    RecursiveLock_Lock(&cond->lock);
    if (cond->signals > 0) {
        /* We always notify the signal thread that we are done */
        LightSemaphore_Release(&cond->wait_done, 1);

        /* Signal handshake complete */
        --cond->signals;
    }
    --cond->waiting;
     RecursiveLock_Unlock(&cond->lock);

    /* Lock the mutex, as is required by condition variable semantics */
    RecursiveLock_Lock(mutex);

    return 0;
}

