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

#include "config.h"
#include "tick.h"
#include "general.h"
#include "panic.h"

/****************************************************************************
 * Timer tick
 *****************************************************************************/


/* List of tick tasks - final element always NULL for termination */
void (*tick_funcs[MAX_NUM_TICK_TASKS+1])(void);

#if !defined(CPU_PP) || !defined(BOOTLOADER) || \
    defined(HAVE_BOOTLOADER_USB_MODE)
volatile long current_tick SHAREDDATA_ATTR = 0;
#endif

/* - Timer initialization and interrupt handler is defined at
 * the target level: tick_start() is implemented in the target tree */

int tick_add_task(void (*f)(void))
{
    int oldlevel = disable_irq_save();
    void **arr = (void **)tick_funcs;
    void **p = find_array_ptr(arr, f);

    /* Add a task if there is room */
    if(p - arr < MAX_NUM_TICK_TASKS)
    {
        *p = f; /* If already in list, no problem. */
    }
    else
    {
        panicf("Error! tick_add_task(): out of tasks");
    }

    restore_irq(oldlevel);
    return 0;
}

int tick_remove_task(void (*f)(void))
{
    int oldlevel = disable_irq_save();
    int rc = remove_array_ptr((void **)tick_funcs, f);
    restore_irq(oldlevel);
    return rc;
}

void init_tick(void)
{
    tick_start(1000/HZ);
}
