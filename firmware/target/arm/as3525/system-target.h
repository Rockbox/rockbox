/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Rafaël Carré
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
#ifndef SYSTEM_TARGET_H
#define SYSTEM_TARGET_H

#include "system-arm.h"
#include "mmu-arm.h"
#include "panic.h"

#include "clock-target.h" /* CPUFREQ_* are defined here */

#define STORAGE_WANTS_ALIGN

/* We can use a interrupt-based mechanism on the fuzev2 */
#define INCREASED_SCROLLWHEEL_POLLING \
    (defined(HAVE_SCROLLWHEEL) && (CONFIG_CPU == AS3525))

#if INCREASED_SCROLLWHEEL_POLLING
/* let the timer interrupt twice as often for the scrollwheel polling */
#define KERNEL_TIMER_FREQ (TIMER_FREQ/2)
#else
#define KERNEL_TIMER_FREQ TIMER_FREQ
#endif

#define AS3525_UNCACHED_ADDR(a) ((typeof(a)) ((uintptr_t)(a) + 0x10000000))
#define AS3525_PHYSICAL_ADDR(a) \
    ((typeof(a)) ((((uintptr_t)(a)) & (MEMORYSIZE*0x100000)) \
        ? (((uintptr_t)(a)) - IRAM_ORIG) \
        : ((uintptr_t)(a))))

#if defined(SANSA_FUZEV2) || defined(SANSA_CLIPPLUS)
extern int amsv2_variant;
#endif

#ifdef SANSA_C200V2
/* 0: Backlight on A5, 1: Backlight on A7 */
extern int c200v2_variant;
/* c200v2 changes the timer interval often due to software pwm */
#define TIMER_PERIOD TIMER2_BGLOAD
#else
#define TIMER_PERIOD (KERNEL_TIMER_FREQ/HZ)
#endif

void udelay(unsigned usecs);
static inline void mdelay(unsigned msecs)
{
    udelay(1000 * msecs);
}
#endif /* SYSTEM_TARGET_H */
