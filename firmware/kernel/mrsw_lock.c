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
#include "kernel-internal.h"
#include "mrsw-lock.h"

#ifdef HAVE_PRIORITY_SCHEDULING

static FORCE_INLINE void
mrsw_reader_claim(struct mrsw_lock *mrsw, struct thread_entry *current,
                  int count, unsigned int slotnum)
{
    /* no need to lock this; if a reader can claim, noone is in the queue */
    threadbit_set_bit(&mrsw->splay.mask, slotnum);
    mrsw->splay.blocker.thread = count == 1 ? current : NULL;
}

static FORCE_INLINE void
mrsw_reader_relinquish(struct mrsw_lock *mrsw, struct thread_entry *current,
                       int count, unsigned int slotnum)
{
    /* If no writer is queued or has ownership then noone is queued;
       if a writer owns it, then the reader would be blocked instead.
       Therefore, if the queue has threads, then the next after the
       owning readers is a writer and this is not the last reader. */
    if (mrsw->queue)
        corelock_lock(&mrsw->splay.cl);

    threadbit_clear_bit(&mrsw->splay.mask, slotnum);

    if (count == 0)
    {
        /* There is noone waiting; we'd be calling mrsw_wakeup_writer()
           at this time instead */
        mrsw->splay.blocker.thread = NULL;
        return;
    }

    if (count == 1)
    {
        KERNEL_ASSERT(threadbit_popcount(&mrsw->splay.mask) == 1,
                      "mrsw_reader_relinquish() - "
                      "threadbits has wrong popcount: %d\n",
                      threadbit_popcount(&mrsw->splay.mask));
        /* switch owner to sole remaining reader */
        slotnum = threadbit_ffs(&mrsw->splay.mask);
        mrsw->splay.blocker.thread = thread_id_entry(slotnum);
    }

    if (mrsw->queue)
    {
        priority_disinherit(current, &mrsw->splay.blocker);
        corelock_unlock(&mrsw->splay.cl);
    }
}

static FORCE_INLINE unsigned int
mrsw_reader_wakeup_writer(struct mrsw_lock *mrsw, unsigned int slotnum)
{
    threadbit_clear_bit(&mrsw->splay.mask, slotnum);
    return wakeup_thread(&mrsw->queue, WAKEUP_TRANSFER);
}

static FORCE_INLINE unsigned int
mrsw_writer_wakeup_writer(struct mrsw_lock *mrsw)
{
    return wakeup_thread(&mrsw->queue, WAKEUP_TRANSFER);
}

static FORCE_INLINE unsigned int
mrsw_writer_wakeup_readers(struct mrsw_lock *mrsw)
{
    unsigned int result = wakeup_thread(&mrsw->queue, WAKEUP_TRANSFER_MULTI);
    mrsw->count = thread_self_entry()->retval;
    return result;
}

#else /* !HAVE_PRIORITY_SCHEDULING */

#define mrsw_reader_claim(mrsw, current, count, slotnum) \
    do {} while (0)

#define mrsw_reader_relinquish(mrsw, current, count, slotnum) \
    do {} while (0)

static FORCE_INLINE unsigned int
mrsw_reader_wakeup_writer(struct mrsw_lock *mrsw)
{
    mrsw->splay.blocker.thread = mrsw->queue;
    return wakeup_thread(&mrsw->queue);
}

static FORCE_INLINE unsigned int
mrsw_writer_wakeup_writer(struct mrsw_lock *mrsw)
{
    mrsw->splay.blocker.thread = mrsw->queue;
    return wakeup_thread(&mrsw->queue);
}

static FORCE_INLINE unsigned int
mrsw_writer_wakeup_readers(struct mrsw_lock *mrsw)
{
    mrsw->splay.blocker.thread = NULL;
    int count = 0;

    while (mrsw->queue && mrsw->queue->retval != 0)
    {
        wakeup_thread(&mrsw->queue);
        count++;
    }

    mrsw->count = count;
    return THREAD_OK;
}

#endif /* HAVE_PRIORITY_SCHEDULING */

/** Public interface **/

/* Initializes a multi-reader, single-writer object */
void mrsw_init(struct mrsw_lock *mrsw)
{
    mrsw->count = 0;
    mrsw->queue = NULL;
    mrsw->splay.blocker.thread = NULL;
#ifdef HAVE_PRIORITY_SCHEDULING
    mrsw->splay.blocker.priority = PRIORITY_IDLE;
    threadbit_clear(&mrsw->splay.mask);
    corelock_init(&mrsw->splay.cl);
    memset(mrsw->rdrecursion, 0, sizeof (mrsw->rdrecursion));
#endif /* HAVE_PRIORITY_SCHEDULING */
    corelock_init(&mrsw->cl);
}

/* Request reader thread lock. Any number of reader threads may enter which
 * also locks-out all writer threads. Same thread may safely acquire read
 * access recursively. The current writer is ignored and gets access. */
void mrsw_read_acquire(struct mrsw_lock *mrsw)
{
    struct thread_entry *current = thread_self_entry();

    if (current == mrsw->splay.blocker.thread IF_PRIO( && mrsw->count < 0 ))
        return; /* Read request while holding write access; pass */

#ifdef HAVE_PRIORITY_SCHEDULING
    /* Track recursion counts for each thread:
       IF_PRIO, mrsw->count just tracks unique owners  */
    unsigned int slotnum = THREAD_ID_SLOT(current->id);
    if (threadbit_test_bit(&mrsw->splay.mask, slotnum))
    {
        KERNEL_ASSERT(mrsw->rdrecursion[slotnum] < UINT8_MAX,
                      "mrsw_read_acquire() - "
                      "Thread %s did too many claims\n",
                      current->name);
        mrsw->rdrecursion[slotnum]++;
        return;
    }
#endif /* HAVE_PRIORITY_SCHEDULING */

    corelock_lock(&mrsw->cl);

    int count = mrsw->count;

    if (LIKELY(count >= 0 && !mrsw->queue))
    {
        /* Lock open to readers:
           IFN_PRIO, mrsw->count tracks reader recursion */
        mrsw->count = ++count;
        mrsw_reader_claim(mrsw, current, count, slotnum);
        corelock_unlock(&mrsw->cl);
        return;
    }

    /* A writer owns it or is waiting; block... */
    IF_COP( current->obj_cl = &mrsw->cl; )
    IF_PRIO( current->blocker = &mrsw->splay.blocker; )
    current->bqp = &mrsw->queue;
    current->retval = 1; /* indicate multi-wake candidate */

    disable_irq();
    block_thread(current, TIMEOUT_BLOCK);

    corelock_unlock(&mrsw->cl);

    /* ...and turn control over to next thread */
    switch_thread();
}

/* Release reader thread lockout of writer thread. The last reader to
 * leave opens up access to writer threads. The current writer is ignored. */
void mrsw_read_release(struct mrsw_lock *mrsw)
{
    struct thread_entry *current = thread_self_entry();

    if (current == mrsw->splay.blocker.thread IF_PRIO( && mrsw->count < 0 ))
        return; /* Read release while holding write access; ignore */

#ifdef HAVE_PRIORITY_SCHEDULING
    unsigned int slotnum = THREAD_ID_SLOT(current->id);
    KERNEL_ASSERT(threadbit_test_bit(&mrsw->splay.mask, slotnum),
                  "mrsw_read_release() -"
                  " thread '%s' not reader\n",
                  current->name);

    uint8_t *rdcountp = &mrsw->rdrecursion[slotnum];
    unsigned int rdcount = *rdcountp;
    if (rdcount > 0)
    {
        /* Reader is releasing recursive claim */
        *rdcountp = rdcount - 1;
        return;
    }
#endif /* HAVE_PRIORITY_SCHEDULING */

    corelock_lock(&mrsw->cl);
    int count = mrsw->count;

    KERNEL_ASSERT(count > 0, "mrsw_read_release() - no readers!\n");

    unsigned int result = THREAD_NONE;
    const int oldlevel = disable_irq_save();

    if (--count == 0 && mrsw->queue)
    {
        /* No readers remain and a writer is waiting */
        mrsw->count = -1;
        result = mrsw_reader_wakeup_writer(mrsw IF_PRIO(, slotnum));
    }
    else
    {
        /* Giving up readership; we may be the last, or not */
        mrsw->count = count;
        mrsw_reader_relinquish(mrsw, current, count, slotnum);
    }

    restore_irq(oldlevel);
    corelock_unlock(&mrsw->cl);

#ifdef HAVE_PRIORITY_SCHEDULING
    if (result & THREAD_SWITCH)
        switch_thread();
#endif /* HAVE_PRIORITY_SCHEDULING */
    (void)result;
}

/* Acquire writer thread lock which provides exclusive access. If a thread
 * that is holding read access calls this it will deadlock. The writer may
 * safely call recursively. */
void mrsw_write_acquire(struct mrsw_lock *mrsw)
{
    struct thread_entry *current = thread_self_entry();

    if (current == mrsw->splay.blocker.thread)
    {
        /* Current thread already has write access */
        mrsw->count--;
        return;
    }

    corelock_lock(&mrsw->cl);

    int count = mrsw->count;

    if (LIKELY(count == 0))
    {
        /* Lock is open to a writer */
        mrsw->count = -1;
        mrsw->splay.blocker.thread = current;
        corelock_unlock(&mrsw->cl);
        return;
    }

    /* Readers present or a writer owns it - block... */
    IF_COP( current->obj_cl = &mrsw->cl; )
    IF_PRIO( current->blocker = &mrsw->splay.blocker; )
    current->bqp = &mrsw->queue;
    current->retval = 0; /* indicate single-wake candidate */

    disable_irq();
    block_thread(current, TIMEOUT_BLOCK);

    corelock_unlock(&mrsw->cl);

    /* ...and turn control over to next thread */
    switch_thread();
}

/* Release writer thread lock and open the lock to readers and writers */
void mrsw_write_release(struct mrsw_lock *mrsw)
{
    KERNEL_ASSERT(thread_self_entry() == mrsw->splay.blocker.thread,
                  "mrsw_write_release->wrong thread (%s != %s)\n",
                  thread_self_entry()->name,
                  mrsw->splay.blocker.thread->name);

    int count = mrsw->count;
    if (count < -1)
    {
        /* This thread still owns write lock */
        mrsw->count = count + 1;
        return;
    }

    unsigned int result = THREAD_NONE;

    corelock_lock(&mrsw->cl);
    const int oldlevel = disable_irq_save();

    if (mrsw->queue == NULL)           /* 'count' becomes zero */
    {
        mrsw->splay.blocker.thread = NULL;
        mrsw->count = 0;
    }
    else if (mrsw->queue->retval == 0) /* 'count' stays -1 */
        result = mrsw_writer_wakeup_writer(mrsw);
    else                               /* 'count' becomes # of readers */
        result = mrsw_writer_wakeup_readers(mrsw);

    restore_irq(oldlevel);
    corelock_unlock(&mrsw->cl);

#ifdef HAVE_PRIORITY_SCHEDULING
    if (result & THREAD_SWITCH)
        switch_thread();
#endif /* HAVE_PRIORITY_SCHEDULING */
    (void)result;
}
