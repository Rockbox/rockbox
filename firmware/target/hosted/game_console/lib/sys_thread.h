/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

#include "sys_types.h"
#include "sys_atomic.h"

#include "thread.h"

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
    sys_thread_type handle;
    int status;
    int state;
    size_t stacksize;
    int(* userfunc)(void *);
    void *userdata;
    void *data;
} sysThread;

typedef struct _mutex
{
    sys_mutex_type lock;
} sysMutex;

typedef struct _sem
{
    sys_sem_type semaphore;
} sysSem;

typedef struct _cond
{
    sysMutex lock;
    int waiting;
    int signals;
    sysSem wait_sem;
    sysSem wait_done;
} sysCond;

sysMutex *sys_mutex_create(void);
void sys_mutex_init(sysMutex *mutex);
int sys_mutex_lock(sysMutex *mutex);
int sys_mutex_try_lock(sysMutex *mutex);
int sys_mutex_unlock(sysMutex *mutex);

sysSem *sys_sem_create(u32 initial_value);
void sys_sem_init(sysSem *sem, u32 initial_value);
int sys_sem_wait(sysSem *sem);
int sys_sem_wait_timeout(sysSem *sem, u32 timeout);
int sys_sem_try_wait(sysSem *sem);
u32 sys_sem_value(sysSem *sem);
int sys_sem_post(sysSem *sem);

sysCond *sys_cond_create(void);
void sys_cond_init(sysCond *cond);
void sys_cond_destroy(sysCond *cond);
int sys_cond_signal(sysCond *cond);
int sys_cond_broadcast(sysCond *cond);
int sys_cond_wait(sysCond *cond, sysMutex *mutex);

sysThread *sys_thread_create(int(*fn)(void *), const char *name, const size_t stacksize,
                           void *data IF_PRIO(, int priority) IF_COP(, unsigned int core));
void sys_run_thread(sysThread *thread);
void sys_wait_thread(sysThread *thread, int *status);
int sys_thread_id(void);
int sys_set_thread_priority(sysThread *thread, int priority);

#endif /* #ifndef __SYSTHREAD_H__ */

