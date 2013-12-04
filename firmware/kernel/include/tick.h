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
#ifndef TICK_H
#define TICK_H

#include "config.h"
#include "system.h" /* for NULL */
extern void init_tick(void);

#define HZ      100 /* number of ticks per second */

#define MAX_NUM_TICK_TASKS 8

/* global tick variable */
#if defined(CPU_PP) && defined(BOOTLOADER) && \
    !defined(HAVE_BOOTLOADER_USB_MODE)
/* We don't enable interrupts in the PP bootloader unless USB mode is
   enabled for it, so we need to fake the current_tick variable */
#define current_tick (signed)(USEC_TIMER/10000)

static inline void call_tick_tasks(void)
{
}
#else
extern volatile long current_tick;

/* inline helper for implementing target interrupt handler */
static inline void call_tick_tasks(void)
{
    extern void (*tick_funcs[MAX_NUM_TICK_TASKS+1])(void);
    void (**p)(void) = tick_funcs;
    void (*fn)(void);

    current_tick++;

    for(fn = *p; fn != NULL; fn = *(++p))
    {
        fn();
    }
}
#endif

/* implemented in target tree */
extern void tick_start(unsigned int interval_in_ms) INIT_ATTR;

extern int tick_add_task(void (*f)(void));
extern int tick_remove_task(void (*f)(void));

#endif /* TICK_H */
