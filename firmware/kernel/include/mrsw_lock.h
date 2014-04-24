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
#ifndef MRSW_LOCK_H
#define MRSW_LOCK_H

/* Multi-reader, single-writer object that allows mutltiple readers or
 * a single writer thread access to a critical section. Recursion is not
 * supported. */
struct mrsw_lock
{
    int volatile count; /* rd/wr counter; >0 = reader(s), <0 = writer */
    struct thread_entry *reader_queue;
    struct thread_entry *writer_queue;
    struct thread_entry *thread;
    IF_COP( struct corelock cl; )
};

void mrsw_init(struct mrsw_lock *mrsw);
void mrsw_read_acquire(struct mrsw_lock *mrsw);
void mrsw_read_release(struct mrsw_lock *mrsw);
void mrsw_write_acquire(struct mrsw_lock *mrsw);
void mrsw_write_release(struct mrsw_lock *mrsw);

#endif /* MRSW_LOCK_H */
