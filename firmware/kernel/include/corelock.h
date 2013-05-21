/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Ulf Ralberg
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


#ifndef CORELOCK_H
#define CORELOCK_H

#include "config.h"

#ifdef LIBROCKPLAY
#include <pthread.h>

struct corelock
{
    pthread_mutex_t mutex;
};
#else
/* No reliable atomic instruction available - use Peterson's algorithm */
struct corelock
{
    volatile unsigned char myl[NUM_CORES];
    volatile unsigned char turn;
} __attribute__((packed));
#endif

/* Too big to inline everywhere */
extern void corelock_init(struct corelock *cl);
extern void corelock_lock(struct corelock *cl);
extern int  corelock_try_lock(struct corelock *cl);
extern void corelock_unlock(struct corelock *cl);

#endif /* CORELOCK_H */
