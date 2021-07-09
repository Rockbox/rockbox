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

#ifdef SANSA_CONNECT
#define CPUFREQ_DEFAULT  74250000
#define CPUFREQ_NORMAL   74250000
#define CPUFREQ_MAX     148500000
#else
#define CPUFREQ_DEFAULT  87500000
#define CPUFREQ_NORMAL   87500000
#define CPUFREQ_MAX     175000000
#endif

void udelay(int usec);
void mdelay(int msec);

#if defined(CREATIVE_ZVx) && defined(BOOTLOADER)
    /* hacky.. */
#define SLEEP_KERNEL_HOOK(ticks) \
    ({                                              \
    long _sleep_ticks = current_tick + (ticks) + 1; \
    while (TIME_BEFORE(current_tick, _sleep_ticks)) \
        switch_thread(); \
    true; }) /* handled here */
#endif

#ifdef BOOTLOADER
void tick_stop(void);
void system_prepare_fw_start(void);
#endif

#endif /* SYSTEM_TARGET_H */
