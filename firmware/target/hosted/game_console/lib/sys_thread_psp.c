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

/* pspsdk uses SceUID handle type for semaphores */
#define id semaphore

/* sysSem */
void sys_sem_init(sysSem *sem, u32 initial_value)
{
    sem->id = sceKernelCreateSema("semaphore", 0, initial_value, 255, NULL);
    if (sem->id < 0) {
        logf("sceKernelCreateSema: error 0x%x\n", sem->id);
    }
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

int sys_sem_wait_timeout(sysSem *sem, u32 timeout)
{
    u32 *pTimeout;
    int res;

    if (timeout == 0) {
        res = sceKernelPollSema(sem->id, 1);
        if (res < 0) {
            return 1;
        }
        return 0;
    }

    if (timeout == (~(u32)0)) {
        pTimeout = NULL;
    } else {
        timeout *= 1000; /* Convert to microseconds. */
        pTimeout = &timeout;
    }

    res = sceKernelWaitSema(sem->id, 1, (SceUInt *)pTimeout);
    switch (res) {
    case SCE_KERNEL_ERROR_OK:
        return 0;
    case SCE_KERNEL_ERROR_WAIT_TIMEOUT:
        return 1;
    default:
        logf("sceKernelWaitSema: error 0x%x\n", res);
        return -1;
    }
}

int sys_sem_try_wait(sysSem *sem)
{
    return sys_sem_wait_timeout(sem, 0);
}

int sys_sem_wait(sysSem *sem)
{
    return sys_sem_wait_timeout(sem, (~(u32)0));
}

u32 sys_sem_value(sysSem *sem)
{
    SceKernelSemaInfo info;
    memset(&info, 0, sizeof(SceKernelSemaInfo));
    if (sceKernelReferSemaStatus(sem->id, &info) >= 0) {
        return info.currentCount;
    }

    return 0;
}

int sys_sem_post(sysSem *sem)
{
    int res = sceKernelSignalSema(sem->id, 1);
    if (res < 0) {
        logf("sceKernelSignalSema: error 0x%x\n", res);
        return -1;
    }

    return 0;
}

/* sysThread */

/* PSPSDK thread priority value range from 0 to 255 */
/* System thread runs at a 0x11 (17), so apparently we cannot go below that value */
/* Convert rockbox priority value to pspsdk value */
static int get_psp_thread_priority(int priority)
{
    /* no task */
    if (priority == PRIORITY_IDLE)
        return 96;

    /* float prio = (((float) priority - 1.0f) * 15.0f / 19.0f) + 17.0f;
    return (int) floorf(prio); */

    if ((priority == PRIORITY_REALTIME_1) || (priority == PRIORITY_REALTIME_2) ||
        (priority == PRIORITY_REALTIME_3) || (priority == PRIORITY_REALTIME_4) ||
        (priority == PRIORITY_REALTIME))
        return 16;
    else if (priority == PRIORITY_BUFFERING)
        return 18;
    else if ((priority == PRIORITY_USER_INTERFACE) || (priority == PRIORITY_RECORDING) ||
             (priority == PRIORITY_PLAYBACK))
        return 24;
    else if (priority == PRIORITY_PLAYBACK_MAX)
        return 17;
    else if (priority == PRIORITY_SYSTEM)
        return 30;
    else if (priority == PRIORITY_BACKGROUND)
        return 32; /* Lowest */
    else
        return 32;
}

static size_t get_thread_stack_size(size_t requested_size)
{
    if (requested_size == 0) {
        return (80 * 1024); /* 80 kB */
    }

    return requested_size;
}

static int thread_entry(SceSize args, void *argp)
{
    sys_run_thread((sysThread *) argp);
    sceKernelExitThread(0);
    return 0;
    (void) args;
}

int sys_thread_id(void)
{
    return (int) sceKernelGetThreadId();
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
    /* sys_ticks_init(); */

    /* Allocate memory for the thread info structure */
    sysThread *thread = (sysThread *) calloc(1, sizeof(sysThread));
    if (thread == NULL) {
        DEBUGF("sys_thread_create: could not allocate memory\n");
        return NULL;
    }
    thread->status = -1;
    AtomicSet(&thread->state, THREAD_STATE_ALIVE);

    /* Set up the arguments for the thread */
    thread->userfunc = fn;
    thread->userdata = data;
    thread->stacksize = stacksize;

    /* DEBUGF("stacksize: 0x%x (0x%x)\n", stacksize, get_thread_stack_size(stacksize)); */
    /* DEBUGF("priority: %d\n", get_psp_thread_priority(priority)); */

    thread->handle = sceKernelCreateThread(name,
                                           thread_entry,
                                           get_psp_thread_priority(priority),
                                           get_thread_stack_size(stacksize),
                                           PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU,
                                           NULL);
    if (thread->handle < 0) {
        DEBUGF("sys_thread_create: sceKernelCreateThread failed (0x%x)\n", thread->handle);
        free(thread);
        thread = NULL;
    }

    sceKernelStartThread(thread->handle, sizeof(sysThread), thread);
    return thread;
}

void sys_wait_thread(sysThread *thread, int *status)
{
    if (thread) {
        sceKernelWaitThreadEnd(thread->handle, NULL);
        sceKernelDeleteThread(thread->handle);
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
        sceKernelDeleteThread(thread->handle);
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
    return sceKernelChangeThreadPriority(thread->handle,
                                         get_psp_thread_priority(priority));
}

/* sysMutex */
void sys_mutex_init(sysMutex *mutex)
{
    memset(mutex, 0, sizeof(sysMutex));
    int res = sceKernelCreateLwMutex(&mutex->lock,
                                     "mutex",
                                     PSP_LW_MUTEX_ATTR_RECURSIVE,
                                     0,
                                     NULL);
    if (res < 0) {
        logf("sceKernelCreateLwMutex: error 0x%x\n", res);
    }
}

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

int sys_mutex_lock(sysMutex *mutex)
{
    SceInt32 res = sceKernelLockLwMutex(&mutex->lock, 1, NULL);
    if (res != SCE_KERNEL_ERROR_OK) {
        logf("sceKernelLockLwMutex: error 0x%lx", res);
        return -1;
    }

    return 0;
}

int sys_mutex_try_lock(sysMutex *mutex)
{
    SceInt32 res = sceKernelTryLockLwMutex(&mutex->lock, 1);
    switch (res) {
    case SCE_KERNEL_ERROR_OK:
        return 0;
        break;
    case SCE_KERNEL_ERROR_WAIT_TIMEOUT:
        return 1;
        break;
    default:
        logf("sceKernelTryLockLwMutex: 0x%lx", res);
        break;
    }

    return -1;
}

int sys_mutex_unlock(sysMutex *mutex)
{
    SceInt32 res = sceKernelUnlockLwMutex(&mutex->lock, 1);
    if (res != 0) {
        logf("sceKernelUnlockLwMutex: 0x%lx", res);
        return -1;
    }

    return 0;
}
