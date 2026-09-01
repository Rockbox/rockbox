/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

/* sysMutex */
sysMutex *sys_mutex_create(void)
{
    sysMutex *m = (sysMutex *) malloc(sizeof(sysMutex));
    if (m == NULL) {
        DEBUGF("sys_create_mutex: no memory\n");
        return NULL;
    }

    sys_mutex_init(m);
    return m;
}

void sys_mutex_init(sysMutex *mutex)
{
    memset(mutex, 0, sizeof(sysMutex));
    RecursiveLock_Init(&mutex->lock);
}

int sys_mutex_lock(sysMutex *mutex)
{
    RecursiveLock_Lock(&mutex->lock);
    return 0;
}

int sys_mutex_try_lock(sysMutex *mutex)
{
    int res = RecursiveLock_TryLock(&mutex->lock);
    if (res < 0) {
        logf("RecursiveLock_TryLock: %d", res);
        return -1;
    }

    return 0;
}

int sys_mutex_unlock(sysMutex *mutex)
{
    RecursiveLock_Unlock(&mutex->lock);
    return 0;
}

/* sysSem */
void sys_sem_init(sysSem *sem, u32 initial_value)
{
    LightSemaphore_Init(&sem->semaphore, initial_value, ((s16)0x7FFF));
}

sysSem *sys_sem_create(u32 initial_value)
{
    sysSem *s = (sysSem *) malloc(sizeof(sysSem));
    if (s == NULL)
    {
        DEBUGF("sys_create_sem: no memory\n");
        return NULL;
    }

    sys_sem_init(s, initial_value);
    return s;
}

int wait_on_semaphore_for(sysSem *sem, u32 timeout)
{
    u64 stop_time = sys_get_ticks64() + timeout;
    u64 current_time = sys_get_ticks64();
    while (current_time < stop_time) {
        if (LightSemaphore_TryAcquire(&sem->semaphore, 1) == 0) {
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

int sys_sem_try_wait(sysSem *sem)
{
    if (LightSemaphore_TryAcquire(&sem->semaphore, 1) != 0) {
        /* If we failed, yield to avoid starvation on busy waits */
        svcSleepThread(1);
        return 1;
    }

    return 0;
}

int sys_sem_wait_timeout(sysSem *sem, u32 timeout)
{
    if (timeout == (~(u32)0)) {
        LightSemaphore_Acquire(&sem->semaphore, 1);
        return 0;
    }

    if (LightSemaphore_TryAcquire(&sem->semaphore, 1) != 0) {
        return wait_on_semaphore_for(sem, timeout);
    }

    return 0;
}

int sys_sem_wait(sysSem *sem)
{
    return sys_sem_wait_timeout(sem, (~(u32)0));
}

u32 sys_sem_value(sysSem *sem)
{
    return sem->semaphore.current_count;
}

int sys_sem_post(sysSem *sem)
{
    LightSemaphore_Release(&sem->semaphore, 1);
    return 0;
}

/* sysThread */
/* Convert rockbox priority value to libctru value */
int get_3ds_thread_priority(int priority)
{
    if ((priority == PRIORITY_REALTIME_1) || (priority == PRIORITY_REALTIME_2) ||
        (priority == PRIORITY_REALTIME_3) || (priority == PRIORITY_REALTIME_4) ||
        (priority == PRIORITY_REALTIME))
        return 0x18;
    else if (priority == PRIORITY_BUFFERING)
        return 0x2F;
    else if ((priority == PRIORITY_USER_INTERFACE) || (priority == PRIORITY_RECORDING) ||
             (priority == PRIORITY_PLAYBACK))
        return 0x2F;
    else if (priority == PRIORITY_PLAYBACK_MAX)
        return 0x20;
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

sysThread *sys_thread_create(int(*fn)(void *), const char *name, const size_t stacksize,
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
    
    int cpu = 0;
    if (name && (strncmp(name, "buffering", 9) == 0) && R_SUCCEEDED(APT_SetAppCpuTimeLimit(30))) {
        cpu = 1;
        printf("thread: %s, running in cpu 1\n", name);
    }

    thread->handle = threadCreate(thread_entry,
                                  thread,
                                  get_thread_stack_size(stacksize),
                                  get_3ds_thread_priority(priority),
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
    Result res = svcSetThreadPriority(h, get_3ds_thread_priority(priority));
    if (R_SUCCEEDED(res)) {
        return priority;
    }

    return old_priority;
}