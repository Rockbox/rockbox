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
#include "kernel-internal.h"
#include "mutex.h"

/* Initialize a mutex object - call before any use and do not call again once
 * the object is available to other threads */
void mutex_init(struct mutex *m)
{
    wait_queue_init(&m->queue);
    m->recursion = 0;
    blocker_init(&m->blocker);
    corelock_init(&m->cl);
}

/* Gain ownership of a mutex object or block until it becomes free */
void mutex_lock(struct mutex *m)
{
    ASSERT_CPU_MODE(CPU_MODE_THREAD_CONTEXT);

    struct thread_entry *current = __running_self_entry();

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
    disable_irq();
    block_thread(current, TIMEOUT_BLOCK, &m->queue, &m->blocker);

    corelock_unlock(&m->cl);

    /* ...and turn control over to next thread */
    switch_thread();
}

/* Release ownership of a mutex object - only owning thread must call this */
void mutex_unlock(struct mutex *m)
{
    /* unlocker not being the owner is an unlocking violation */
    KERNEL_ASSERT(m->blocker.thread == __running_self_entry(),
                  "mutex_unlock->wrong thread (%s != %s)\n",
                  m->blocker.thread->name,
                  __running_self_entry()->name);

    if(m->recursion > 0)
    {
        /* this thread still owns lock */
        m->recursion--;
        return;
    }

    /* lock out other cores */
    corelock_lock(&m->cl);

    /* transfer to next queued thread if any */
    struct thread_entry *thread = WQ_THREAD_FIRST(&m->queue);
    if(LIKELY(thread == NULL))
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
#ifndef HAVE_PRIORITY_SCHEDULING
    m->blocker.thread = thread;
#endif
    unsigned int result = wakeup_thread(thread, WAKEUP_TRANSFER);
    restore_irq(oldlevel);

    corelock_unlock(&m->cl);

#ifdef HAVE_PRIORITY_SCHEDULING
    if(result & THREAD_SWITCH)
        switch_thread();
#endif
    (void)result;
}
