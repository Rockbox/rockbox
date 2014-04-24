/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
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
#include "config.h"
#include "system.h"
#include "thread.h"
#include "kernel.h"
#include "kernel-internal.h"
#include "mrsw_lock.h"

/* Initializes a multi-reader, single-writer object */
void mrsw_init(struct mrsw_lock *mrsw)
{
    mrsw->count = 0;
    mrsw->reader_queue = NULL;
    mrsw->writer_queue = NULL;
    corelock_init(&mrsw->cl);
}

/* Request reader thread lock. Any number of reader threads may enter which
 * also locks-out all writer threads. Same thread may safely acquire read
 * access recursively. The current writer is ignored and gets access. */
void mrsw_read_acquire(struct mrsw_lock *mrsw)
{
    struct thread_entry *current = thread_self_entry();

    if (current == mrsw->thread)
        return; /* Read request while holding write access; pass */

    corelock_lock(&mrsw->cl);

    int count = mrsw->count;

    if (LIKELY(count >= 0))
    {
        /* Lock open to readers */
        mrsw->count = count + 1;
        corelock_unlock(&mrsw->cl);
        return;
    }

    /* Writer is present; block... */
    IF_COP( current->obj_cl = &mrsw->cl; )
    current->bqp = &mrsw->reader_queue;

    disable_irq();
    block_thread(current);

    corelock_unlock(&mrsw->cl);

    /* ...and turn control over to next thread */
    switch_thread();
}

/* Release reader thread lockout of writer thread. The last reader to
 * leave opens up access to writer threads. The current writer is ignored. */
void mrsw_read_release(struct mrsw_lock *mrsw)
{
    struct thread_entry *current = thread_self_entry();

    if (current == mrsw->thread)
        return; /* Read release while holding write access; ignore */

    corelock_lock(&mrsw->cl);

    unsigned int result = THREAD_NONE;

    int count = mrsw->count;

    if (count > 0)
    {
        if (--count == 0 && UNLIKELY(mrsw->writer_queue != NULL))
        {
            /* Writer waiting */
            mrsw->count = -1;
            mrsw->thread = mrsw->writer_queue;
            const int oldlevel = disable_irq_save();
            result = wakeup_thread(&mrsw->writer_queue);
            restore_irq(oldlevel);
        }
        else
        {
            mrsw->count = count;
        }
    }

    corelock_unlock(&mrsw->cl);


#ifdef HAVE_PRIORITY_SCHEDULING
    if (result & THREAD_SWITCH)
        switch_thread();
#endif

    (void)result;
}

/* Acquire writer thread lock which provides exclusive access. If a thread
 * that is holding read access calls this it will deadlock. The writer may
 * safely call recursively. */
void mrsw_write_acquire(struct mrsw_lock *mrsw)
{
    struct thread_entry *current = thread_self_entry();

    if (current == mrsw->thread)
    {
        /* Current thread already has write access */
        mrsw->count--;
        return;
    }

    corelock_lock(&mrsw->cl);

    int count = mrsw->count;

    if (LIKELY(count == 0))
    {
        /* Lock open to writers */
        mrsw->thread = current;
        mrsw->count = -1;
        corelock_unlock(&mrsw->cl);
        return;
    }

    /* Reader(s) and/or writer(s) present - block... */
    IF_COP( current->obj_cl = &mrsw->cl; )
    current->bqp = &mrsw->writer_queue;

    disable_irq();
    block_thread(current);

    corelock_unlock(&mrsw->cl);

    /* ...and turn control over to next thread */
    switch_thread();
}

/* Release writer thread lock and open the lock to readers and writers */
void mrsw_write_release(struct mrsw_lock *mrsw)
{
    KERNEL_ASSERT(current == mrsw->thread,
                  "mrsw_write_release->wrong thread (%s != %s)\n",
                  thread_self()->name, mrsw->thread->name);

    int count = mrsw->count;
    if (count < -1)
    {
        /* This thread still owns write lock */
        mrsw->count = count + 1;
        return;
    }

    corelock_lock(&mrsw->cl);

    unsigned int result = THREAD_NONE;

    /* By default for now, prefer writers over readers */

    mrsw->thread = mrsw->writer_queue;

    const int oldlevel = disable_irq_save();

    if (mrsw->writer_queue != NULL)      /* count stays -1 */
        result = wakeup_thread(&mrsw->writer_queue);
    else if (mrsw->reader_queue != NULL) /* count = num waiting readers */
        result = thread_queue_wake(&mrsw->reader_queue, &mrsw->count);
    else                                 /* count = 0; noone waiting */
        mrsw->count = 0;

    restore_irq(oldlevel);

    corelock_unlock(&mrsw->cl);

#ifdef HAVE_PRIORITY_SCHEDULING
    if (result & THREAD_SWITCH)
        switch_thread();
#endif
    (void)result;
}
