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
#include "kernel.h"
#include "thread-internal.h"
#include "kernel-internal.h"

/* Initialize a mutex object - call before any use and do not call again once
 * the object is available to other threads */
void mutex_init(struct mutex *m)
{
    corelock_init(&m->cl);
    m->queue = NULL;
    m->recursion = 0;
    m->blocker.thread = NULL;
#ifdef HAVE_PRIORITY_SCHEDULING
    m->blocker.priority = PRIORITY_IDLE;
    m->no_preempt = false;
#endif
}

/* Gain ownership of a mutex object or block until it becomes free */
void mutex_lock(struct mutex *m)
{
    struct thread_entry *current = thread_self_entry();

    if(current == m->blocker.thread)
    {
        /* current thread already owns this mutex */
        m->recursion++;
        return;
    }

    /* lock out other cores */
    corelock_lock(&m->cl);

    /* must read thread again inside cs (a multiprocessor concern really) */
    if(LIKELY(m->blocker.thread == NULL))
    {
        /* lock is open */
        m->blocker.thread = current;
        corelock_unlock(&m->cl);
        return;
    }

    /* block until the lock is open... */
    IF_COP( current->obj_cl = &m->cl; )
    IF_PRIO( current->blocker = &m->blocker; )
    current->bqp = &m->queue;

    disable_irq();
    block_thread(current, TIMEOUT_BLOCK);

    corelock_unlock(&m->cl);

    /* ...and turn control over to next thread */
    switch_thread();
}

/* Release ownership of a mutex object - only owning thread must call this */
void mutex_unlock(struct mutex *m)
{
    /* unlocker not being the owner is an unlocking violation */
    KERNEL_ASSERT(m->blocker.thread == thread_self_entry(),
                  "mutex_unlock->wrong thread (%s != %s)\n",
                  m->blocker.thread->name,
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
        m->blocker.thread = NULL;
        corelock_unlock(&m->cl);
        return;
    }

    const int oldlevel = disable_irq_save();
    /* Tranfer of owning thread is handled in the wakeup protocol
     * if priorities are enabled otherwise just set it from the
     * queue head. */
    IFN_PRIO( m->blocker.thread = m->queue; )
    unsigned int result = wakeup_thread(&m->queue, WAKEUP_TRANSFER);
    restore_irq(oldlevel);

    corelock_unlock(&m->cl);

#ifdef HAVE_PRIORITY_SCHEDULING
    if((result & THREAD_SWITCH) && !m->no_preempt)
        switch_thread();
#endif
    (void)result;
}
