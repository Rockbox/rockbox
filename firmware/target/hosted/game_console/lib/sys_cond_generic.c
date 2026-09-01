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

/* sysCond */
void sys_cond_init(sysCond *cond)
{
    if (cond) {
        memset(cond, 0, sizeof(sysCond));
        sys_mutex_init(&cond->lock);
        sys_sem_init(&cond->wait_sem, 0);
        sys_sem_init(&cond->wait_done, 0);
        cond->waiting = cond->signals = 0;
    }
}

sysCond *sys_cond_create(void)
{
    sysCond *cond;

    cond = (sysCond *) malloc(sizeof(sysCond));
    if (cond) {
        sys_cond_init(cond);
    } else {
        DEBUGF("sys_cond_create: out of memory.\n");
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
    sys_mutex_lock(&cond->lock);
    if (cond->waiting > cond->signals) {
        ++cond->signals;
        sys_sem_post(&cond->wait_sem);
        sys_mutex_unlock(&cond->lock);
        sys_sem_wait(&cond->wait_done);
    } else {
        sys_mutex_unlock(&cond->lock);
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
    sys_mutex_lock(&cond->lock);
    if (cond->waiting > cond->signals) {
        int i, num_waiting;

        num_waiting = (cond->waiting - cond->signals);
        cond->signals = cond->waiting;
        for (i = 0; i < num_waiting; ++i) {
            sys_sem_post(&cond->wait_sem);
        }
        /* Now all released threads are blocked here, waiting for us.
           Collect them all (and win fabulous prizes!) :-)
         */
        sys_mutex_unlock(&cond->lock);
        for (i = 0; i < num_waiting; ++i) {
            sys_sem_wait(&cond->wait_done);
        }
    } else {
        sys_mutex_unlock(&cond->lock);
    }

    return 0;
}

int sys_cond_wait(sysCond *cond, sysMutex *mutex)
{
    if (!cond) {
        DEBUGF("sys_cond_signal: Invalid param 'cond'\n");
        return -1;
    }

    sys_mutex_lock(&cond->lock);
    ++cond->waiting;
    sys_mutex_unlock(&cond->lock);

    /* Unlock the mutex, as is required by condition variable semantics */
    sys_mutex_unlock(mutex);

    /* Wait for a signal */
    int retval = sys_sem_wait(&cond->wait_sem);

    sys_mutex_lock(&cond->lock);
    if (cond->signals > 0) {
        /* We always notify the signal thread that we are done */
        sys_sem_post(&cond->wait_done);

        /* Signal handshake complete */
        --cond->signals;
    }
    --cond->waiting;
    sys_mutex_unlock(&cond->lock);

    /* Lock the mutex, as is required by condition variable semantics */
    sys_mutex_lock(mutex);

    return retval;
}
