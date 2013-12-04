/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
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


/****************************************************************************
 * Simple mutex functions ;)
 ****************************************************************************/

#include <stdbool.h>
#include "config.h"
#include "system.h"
#include "mutex.h"
#include "corelock.h"
#include "thread-internal.h"
#include "kernel-internal.h"

static inline void __attribute__((always_inline))
mutex_set_thread(struct mutex *mtx, struct thread_entry *td)
{
#ifdef HAVE_PRIORITY_SCHEDULING
    mtx->blocker.thread = td;
#else
    mtx->thread = td;
#endif
}

static inline struct thread_entry * __attribute__((always_inline))
mutex_get_thread(volatile struct mutex *mtx)
{
#ifdef HAVE_PRIORITY_SCHEDULING
    return mtx->blocker.thread;
#else
    return mtx->thread;
#endif
}

/* Initialize a mutex object - call before any use and do not call again once
 * the object is available to other threads */
void mutex_init(struct mutex *m)
{
    corelock_init(&m->cl);
    m->queue = NULL;
    m->recursion = 0;
    mutex_set_thread(m, NULL);
#ifdef HAVE_PRIORITY_SCHEDULING
    m->blocker.priority = PRIORITY_IDLE;
    m->blocker.wakeup_protocol = wakeup_priority_protocol_transfer;
    m->no_preempt = false;
#endif
}

/* Gain ownership of a mutex object or block until it becomes free */
void mutex_lock(struct mutex *m)
{
    struct thread_entry *current = thread_self_entry();

    if(current == mutex_get_thread(m))
    {
        /* current thread already owns this mutex */
        m->recursion++;
        return;
    }

    /* lock out other cores */
    corelock_lock(&m->cl);

    /* must read thread again inside cs (a multiprocessor concern really) */
    if(LIKELY(mutex_get_thread(m) == NULL))
    {
        /* lock is open */
        mutex_set_thread(m, current);
        corelock_unlock(&m->cl);
        return;
    }

    /* block until the lock is open... */
    IF_COP( current->obj_cl = &m->cl; )
    IF_PRIO( current->blocker = &m->blocker; )
    current->bqp = &m->queue;

    disable_irq();
    block_thread(current);

    corelock_unlock(&m->cl);

    /* ...and turn control over to next thread */
    switch_thread();
}

/* Release ownership of a mutex object - only owning thread must call this */
void mutex_unlock(struct mutex *m)
{
    /* unlocker not being the owner is an unlocking violation */
    KERNEL_ASSERT(mutex_get_thread(m) == thread_self_entry(),
                  "mutex_unlock->wrong thread (%s != %s)\n",
                  mutex_get_thread(m)->name,
                  thread_self_entry()->name);

    if(m->recursion > 0)
    {
        /* this thread still owns lock */
        m->recursion--;
        return;
    }

    /* lock out other cores */
    corelock_lock(&m->cl);

    /* transfer to next queued thread if any */
    if(LIKELY(m->queue == NULL))
    {
        /* no threads waiting - open the lock */
        mutex_set_thread(m, NULL);
        corelock_unlock(&m->cl);
        return;
    }
    else
    {
        const int oldlevel = disable_irq_save();
        /* Tranfer of owning thread is handled in the wakeup protocol
         * if priorities are enabled otherwise just set it from the
         * queue head. */
        IFN_PRIO( mutex_set_thread(m, m->queue); )
        IF_PRIO( unsigned int result = ) wakeup_thread(&m->queue);
        restore_irq(oldlevel);

        corelock_unlock(&m->cl);

#ifdef HAVE_PRIORITY_SCHEDULING
        if((result & THREAD_SWITCH) && !m->no_preempt)
            switch_thread();
#endif
    }
}
