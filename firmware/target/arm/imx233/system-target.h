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
#ifndef SYSTEM_TARGET_H
#define SYSTEM_TARGET_H

#include "system-arm.h"
#include "mmu-arm.h"
#include "panic.h"

#include "clock-target.h" /* CPUFREQ_* are defined here */

#define HW_DIGCTL_BASE          0x8001C000
#define HW_DIGCTL_MICROSECONDS  (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0xC0))

#define INT_SRC_USB_CTRL    11
#define INT_SRC_TIMER(nr)   (28 + (nr))
#define INT_SRC_LCDIF_DMA   45
#define INT_SRC_LCDIF_ERROR 46
#define INT_SRC_NR_SOURCES  66

void imx233_enable_interrupt(int src, bool enable);
void imx233_softirq(int src, bool enable);
void udelay(unsigned us);
bool imx233_us_elapsed(uint32_t ref, unsigned us_delay);
void imx233_reset_block(volatile uint32_t *block_reg);
void power_off(void);

void udelay(unsigned usecs);

static inline void mdelay(unsigned msecs)
{
    udelay(1000 * msecs);
}

#endif /* SYSTEM_TARGET_H */
