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

#ifndef MUTEX_H
#define MUTEX_H

#include "thread.h"

struct mutex
{
    struct thread_entry *queue;   /* waiter list */
    int recursion;                /* lock owner recursion count */
    struct blocker blocker;       /* priority inheritance info
                                     for waiters and owner*/
    IF_COP( struct corelock cl; ) /* multiprocessor sync */
#ifdef HAVE_PRIORITY_SCHEDULING
    bool no_preempt;
#endif
};

extern void mutex_init(struct mutex *m);
extern void mutex_lock(struct mutex *m);
extern void mutex_unlock(struct mutex *m);
#ifdef HAVE_PRIORITY_SCHEDULING
/* Deprecated temporary function to disable mutex preempting a thread on
 * unlock - firmware/drivers/fat.c and a couple places in apps/buffering.c -
 * reliance on it is a bug! */
static inline void mutex_set_preempt(struct mutex *m, bool preempt)
    { m->no_preempt = !preempt; }
#else
/* Deprecated but needed for now - firmware/drivers/ata_mmc.c */
static inline bool mutex_test(const struct mutex *m)
    { return m->blocker.thread != NULL; }
#endif /* HAVE_PRIORITY_SCHEDULING */

#endif /* MUTEX_H */
