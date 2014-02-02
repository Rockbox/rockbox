/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#ifndef __KERNEL_IMX233__
#define __KERNEL_IMX233__

#include "kernel.h"

/* The i.MX233 uses in several places virtual channels to multiplex the work.
 * To arbiter the use of the different channels, we use a simple channel arbiter
 * based on a semaphore to count the number of channels in use, and a bitmask
 * protected by a mutex */
struct channel_arbiter_t
{
    struct semaphore sema;
    struct mutex mutex;
    unsigned free_bm;
    int count;
};

void arbiter_init(struct channel_arbiter_t *a, unsigned count);
// doesn't check in use !
void arbiter_reserve(struct channel_arbiter_t *a, unsigned channel);
/* return channel on success and OBJ_WAIT_TIMEOUT on failure */
int arbiter_acquire(struct channel_arbiter_t *a, int timeout);
void arbiter_release(struct channel_arbiter_t *a, int channel);
bool arbiter_acquired(struct channel_arbiter_t *a, int channel);

#endif /* __KERNEL_IMX233__ */