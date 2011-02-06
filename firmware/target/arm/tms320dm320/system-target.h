/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007, 2009 by Karl Kurbjun
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

#define CPUFREQ_SLEEP       32768
#define CPUFREQ_DEFAULT  87500000
#define CPUFREQ_NORMAL   87500000
#define CPUFREQ_MAX     175000000

void udelay(int usec);

#if defined(CREATIVE_ZVx) && defined(BOOTLOADER)
    /* hacky.. */
#define SLEEP_KERNEL_HOOK(ticks) \
    ({                                              \
    long _sleep_ticks = current_tick + (ticks) + 1; \
    while (TIME_BEFORE(current_tick, _sleep_ticks)) \
        switch_thread(); \
    true; }) /* handled here */
#endif

#endif /* SYSTEM_TARGET_H */
