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
    struct __wait_queue queue;     /* waiter list */
    int                 recursion; /* lock owner recursion count */
    struct blocker      blocker;   /* priority inheritance info
                                      for waiters and owner*/
    IF_COP( struct corelock cl; )  /* multiprocessor sync */
};

extern void mutex_init(struct mutex *m);
extern void mutex_lock(struct mutex *m);
extern void mutex_unlock(struct mutex *m);

#endif /* MUTEX_H */
