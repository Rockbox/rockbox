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

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "thread.h"

struct semaphore
{
    struct __wait_queue queue;    /* Waiter list */
    int volatile        count;    /* # of waits remaining before unsignaled */
    int                 max;      /* maximum # of waits to remain signaled */
    IF_COP( struct corelock cl; ) /* multiprocessor sync */
};

extern void semaphore_init(struct semaphore *s, int max, int start);
extern int  semaphore_wait(struct semaphore *s, int timeout);
extern void semaphore_release(struct semaphore *s);

#endif /* SEMAPHORE_H */
