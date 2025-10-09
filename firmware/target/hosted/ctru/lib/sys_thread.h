/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Mauricio G.
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

#ifndef __SYSTHREAD_H__
#define __SYSTHREAD_H__

#include "thread.h"

#include <3ds/synchronization.h>
#include <3ds/thread.h>
#include <3ds/services/apt.h>

/* Complementary atomic operations */
bool _AtomicCAS(u32 *ptr, int oldval, int newval);
#define AtomicGet(ptr) __atomic_load_n((u32*)(ptr), __ATOMIC_SEQ_CST)
#define AtomicSet(ptr, value) AtomicSwap(ptr, value)
#define AtomicCAS(ptr, oldvalue, newvalue) _AtomicCAS((u32 *)(ptr), oldvalue, newvalue)
void AtomicLock(int *lock);
void AtomicUnlock(int *lock);

/* This code was taken from SDL2 thread implementation */

enum thread_state_t
{
    THREAD_STATE_ALIVE,
    THREAD_STATE_DETACHED,
    THREAD_STATE_ZOMBIE,
    THREAD_STATE_CLEANED,
};

typedef struct _thread
{
    int threadid;
    Thread handle;
    int status;
    int state;
    size_t stacksize;
    int(* userfunc)(void *);
    void *userdata;
    void *data;
} sysThread;

typedef struct _cond
{
    RecursiveLock lock;
    int waiting;
    int signals;
    LightSemaphore wait_sem;
    LightSemaphore wait_done;
} sysCond;

int sys_sem_wait(LightSemaphore *sem);
int sys_sem_wait_timeout(LightSemaphore *sem, u32 timeout);
int sys_sem_try_wait(LightSemaphore *sem);
u32 sys_sem_value(LightSemaphore *sem);

sysThread *sys_create_thread(int(*fn)(void *), const char *name, const size_t stacksize,
                           void *data IF_PRIO(, int priority) IF_COP(, unsigned int core));
void sys_run_thread(sysThread *thread);
void sys_wait_thread(sysThread *thread, int *status);
int sys_thread_id(void);
int sys_set_thread_priority(sysThread *thread, int priority);

sysCond *sys_cond_create(void);
void sys_cond_destroy(sysCond *cond);
int sys_cond_signal(sysCond *cond);
int sys_cond_broadcast(sysCond *cond);
int sys_cond_wait(sysCond *cond, RecursiveLock *mutex);
#endif /* #ifndef __SYSTHREAD_H__ */

