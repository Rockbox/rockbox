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
#include <string.h>
#include "config.h"
#include "system.h"
#include "thread.h"
#include "kernel.h"
#include "kernel-internal.h"

#if   MRSW_ACC_POL == MRSW_POL_RR
#elif MRSW_ACC_POL == MRSW_POL_WRT
#else
#error unrecognized access policy
#endif

/**
 * Queue format:
 * A single circular wait queue with waiting writer writers and waiting
 * readers sorted together with their own kind. This makes queue scanning for
 * the next highest priority a simple operation (and compatible without
 * modifying the scheduler internals).
 *
 *   <<...W0->W1->W2->R0->R1->R2...>>
 * writer0^    reader0^
 *
 * Blocking uses "insert last" thus when blocking, the queue head is set to
 * the first of the other type, if any exists, to get same types sorted
 * together at the current tail. Since the list is circular, flipping the
 * order is trivial.
 *
 * Waking uses "remove first" thus when waking, the queue head is set to the
 * first of the type being woken, if any exist, to remove that type.
 */

static FORCE_INLINE void
mrsw_reader_claim(struct mrsw_lock *mrsw, struct thread_entry *current
                  IF_PRIO(, int count, unsigned int slotnum, uint8_t *tcountp))
{
#ifdef HAVE_PRIORITY_SCHEDULING
    threadbit_set_bit(&mrsw->splay.mask, slotnum);
    *tcountp = 1;
    mrsw->splay.blocker.thread = count == 1 ? current : NULL;
#endif /* HAVE_PRIORITY_SCHEDULING */
    (void)mrsw; (void)current;
}

static FORCE_INLINE void
mrsw_reader_relinquish(struct mrsw_lock *mrsw, struct thread_entry *current
                       IF_PRIO(, int count, unsigned int slotnum, uint8_t *tcountp))
{
#ifdef HAVE_PRIORITY_SCHEDULING
    if (mrsw->queue)
    {
        corelock_lock(&mrsw->splay.cl);
    }

    threadbit_clear_bit(&mrsw->splay.mask, slotnum);
    *tcountp = 0;

    if (count == 0)
    {
        /* Last reader so no writer is present or else we'd be inside
           mrsw_reader_wakeup_writer() instead */
        mrsw->splay.blocker.thread = NULL;
        return;
    }

    if (count == 1)
    {
        KERNEL_ASSERT(threadbit_popcount(&mrsw->splay.mask) == 1,
                      "mrsw_reader_relinquish() - "
                      "threadbits has wrong popcount: %d\n",
                      threadbit_popcount(&mrsw->splay.mask));
        unsigned int index = threadbit_ffs(&mrsw->splay.mask);
        mrsw->splay.blocker.thread = thread_id_entry(index);
    }

    if (mrsw->queue)
    {
        priority_disinherit(current, &mrsw->splay.blocker);
        corelock_unlock(&mrsw->splay.cl);
    }

#endif /* HAVE_PRIORITY_SCHEDULING */
    (void)mrsw; (void)current;
}

static FORCE_INLINE unsigned int
mrsw_reader_wakeup_writer(struct mrsw_lock *mrsw
                          IF_PRIO(, unsigned int slotnum, uint8_t *tcountp))
{
#if MRSW_ACC_POL == MRSW_POL_WRT
    mrsw->queue = mrsw->writer0;
    struct thread_entry *next = mrsw->writer0->l.next;
    mrsw->writer0 = (next == mrsw->writer0 || next == mrsw->reader0)
                        ? NULL : next; /* writer0 = next queued writer */
#endif /* MRSW_ACC_POL */
    IFN_PRIO( mrsw->splay.blocker.thread = mrsw->queue; )
#ifdef HAVE_PRIORITY_SCHEDULING
    corelock_lock(&mrsw->splay.cl);
    threadbit_clear_bit(&mrsw->splay.mask, slotnum);
    *tcountp = 0;
    corelock_unlock(&mrsw->splay.cl);
#endif /* HAVE_PRIORITY_SCHEDULING */
    return wakeup_thread(&mrsw->queue, WAKEUP_TRANSFER);
}

static FORCE_INLINE unsigned int
mrsw_writer_wakeup_writer(struct mrsw_lock *mrsw)
{
#if MRSW_ACC_POL == MRSW_POL_WRT
    mrsw->queue = mrsw->writer0;
    struct thread_entry *next = mrsw->writer0->l.next;
    mrsw->writer0 = (next == mrsw->writer0 || next == mrsw->reader0)
                        ? NULL : next; /* writer0 = next queued writer */
#endif /* MRSW_ACC_POL */
    IFN_PRIO( mrsw->splay.blocker.thread = mrsw->queue; )
    return wakeup_thread(&mrsw->queue, WAKEUP_TRANSFER);
}

static FORCE_INLINE unsigned int
mrsw_writer_wakeup_readers(struct mrsw_lock *mrsw)
{
#if MRSW_ACC_POL == MRSW_POL_WRT
    mrsw->queue = mrsw->reader0;
    mrsw->reader0 = NULL; /* Only readers: waking everyone */
#endif /* MRSW_ACC_POL */
#ifdef HAVE_PRIORITY_SCHEDULING
    struct thread_entry *current = thread_self_entry();
    current->retval = MRSW_ACC_POL;
    int result = wakeup_thread(&mrsw->queue, WAKEUP_TRANSFER_MULTI);
    mrsw->count = current->retval;
    return result;
#else /* !HAVE_PRIORITY_SCHEDULING */
#if MRSW_ACC_POL == MRSW_POL_RR
    #error RR not yet supported without priority
#endif
    mrsw->splay.blocker.thread = NULL;
    return thread_queue_wake(&mrsw->queue, &mrsw->count);
#endif /* HAVE_PRIORITY_SCHEDULING */
}

/* Initializes a multi-reader, single-writer object */
void mrsw_init(struct mrsw_lock *mrsw)
{
    mrsw->count   = 0;
    mrsw->queue   = NULL;
#if MRSW_ACC_POL == MRSW_POL_WRT
    mrsw->reader0 = NULL;
    mrsw->writer0 = NULL;
#endif /* MRSW_ACC_POL */
    mrsw->splay.blocker.thread = NULL;
#ifdef HAVE_PRIORITY_SCHEDULING
    mrsw->splay.blocker.priority = PRIORITY_IDLE;
    memset(mrsw->splay.tcounts, 0, sizeof (mrsw->splay.tcounts));
    threadbit_clear(&mrsw->splay.mask);
    corelock_init(&mrsw->splay.cl);
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
    uint8_t *tcountp = &mrsw->splay.tcounts[slotnum];
    unsigned int tcount = *tcountp;

    if (tcount > 0)
    {
        /* Reader recursion */
        KERNEL_ASSERT(tcount < UINT8_MAX,
                      "mrsw_read_acquire() - "
                      "Thread %s did too many claims\n",
                      current->name);
        *tcountp = tcount + 1;
        return;
    }
#endif /* HAVE_PRIORITY_SCHEDULING */

    corelock_lock(&mrsw->cl);

    int count = mrsw->count;

#if MRSW_ACC_POL == MRSW_POL_WRT
    if (LIKELY(count >= 0 && !mrsw->writer0))
#elif MRSW_ACC_POL == MRSW_POL_RR
    if (LIKELY(count >= 0 && !mrsw->queue))
#endif /* MRSW_ACC_POL */
    {
        /* Lock open to readers:
           IFN_PRIO, mrsw->count tracks reader recursion */
        mrsw->count = ++count;
        mrsw_reader_claim(mrsw, current IF_PRIO(, count, slotnum, tcountp));
        corelock_unlock(&mrsw->cl);
        return;
    }

    /* A writer owns it or is waiting; block... */
    IF_COP( current->obj_cl = &mrsw->cl; )
    IF_PRIO( current->blocker = &mrsw->splay.blocker; )
    current->bqp = &mrsw->queue;
    current->retval = 1; /* indicate multi-wake candidate */

#if MRSW_ACC_POL == MRSW_POL_WRT
    if (mrsw->writer0)
        mrsw->queue = mrsw->writer0;

    if (!mrsw->reader0)
        mrsw->reader0 = current;
#endif /* MRSW_ACC_POL */

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
    KERNEL_ASSERT(mrsw->splay.tcounts[THREAD_ID_SLOT(current->id)] > 0,
                  "mrsw_read_release() - thread '%s' not reader\n",
                  current->name);

    unsigned int slotnum = THREAD_ID_SLOT(current->id);
    uint8_t *tcountp = &mrsw->splay.tcounts[slotnum];
    unsigned int tcount = *tcountp;

    if (tcount > 1)
    {
        /* Reader is releasing recursive claim */
        *tcountp = tcount - 1;
        return;
    }
#endif /* HAVE_PRIORITY_SCHEDULING */

    corelock_lock(&mrsw->cl);
    int count = mrsw->count;

    KERNEL_ASSERT(count > 0, "mrsw_read_release() - no readers!\n");

    unsigned int result = THREAD_NONE;
    const int oldlevel = disable_irq_save();

#if MRSW_ACC_POL == MRSW_POL_WRT
    if (--count == 0 && mrsw->writer0)
#elif MRSW_ACC_POL == MRSW_POL_RR
    if (--count == 0 && mrsw->queue)
#endif /* MRSW_ACC_POL */
    {
        /* No readers remain and a writer is waiting */
        mrsw->count = -1;
        result = mrsw_reader_wakeup_writer(mrsw IF_PRIO(, slotnum, tcountp));
    }
    else
    {
        /* Giving up readership; we may be the last, or not */
        mrsw->count = count;
        mrsw_reader_relinquish(mrsw, current
                               IF_PRIO(, count, slotnum, tcountp));
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
    IF_PRIO( current->blocker = &mrsw->splay.blocker );
    current->bqp = &mrsw->queue;
    current->retval = 0; /* indicate single-wake candidate */

#if MRSW_ACC_POL == MRSW_POL_WRT
    if (mrsw->reader0)
        mrsw->queue = mrsw->reader0;

    if (!mrsw->writer0)
        mrsw->writer0 = current;
#endif /* MRSW_ACC_POL */

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

    /* Writers get first dibs */
    if (mrsw->queue)
    {
#if MRSW_ACC_POL == MRSW_POL_WRT
        if (mrsw->writer0)  /* 'count' stays at -1 */
#elif MRSW_ACC_POL == MRSW_POL_RR
        if (mrsw->queue->retval == 0)
#endif /* MRSW_ACC_POL */
            result = mrsw_writer_wakeup_writer(mrsw);
        else                /* 'count' becomes # of readers granted */
            result = mrsw_writer_wakeup_readers(mrsw);
    }
    else                    /* 'count' becomes zero; noone waiting */
    {
        mrsw->splay.blocker.thread = NULL;
        mrsw->count = 0;
    }

    restore_irq(oldlevel);
    corelock_unlock(&mrsw->cl);

#ifdef HAVE_PRIORITY_SCHEDULING
    if (result & THREAD_SWITCH)
        switch_thread();
#endif /* HAVE_PRIORITY_SCHEDULING */
    (void)result;
}
