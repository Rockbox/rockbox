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

#define HW_POWER_BASE           0x80044000
#define HW_POWER_STS            (*(volatile uint32_t *)(HW_POWER_BASE + 0xc0))
#define HW_POWER_STS__PSWITCH_BP    20
#define HW_POWER_STS__PSWITCH_BM    (3 << 20)

#define HW_POWER_RESET          (*(volatile uint32_t *)(HW_POWER_BASE + 0x100))
#define HW_POWER_RESET__UNLOCK  0x3E770000
#define HW_POWER_RESET__PWD     0x1

#define HW_ICOLL_BASE           0x80000000

#define HW_ICOLL_VECTOR         (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x0))

#define HW_ICOLL_LEVELACK       (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x10))
#define HW_ICOLL_LEVELACK__LEVEL0   0x1

#define HW_ICOLL_CTRL           (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x20))
#define HW_ICOLL_CTRL__IRQ_FINAL_ENABLE (1 << 16)
#define HW_ICOLL_CTRL__ARM_RSE_MODE     (1 << 18)

#define HW_ICOLL_VBASE          (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x40))
#define HW_ICOLL_INTERRUPT(i)   (*(volatile uint32_t *)(HW_ICOLL_BASE + 0x120 + (i) * 0x10))
#define HW_ICOLL_INTERRUPT__PRIORITY_BM 0x3
#define HW_ICOLL_INTERRUPT__ENABLE      0x4
#define HW_ICOLL_INTERRUPT__SOFTIRQ     0x8
#define HW_ICOLL_INTERRUPT__ENFIQ       0x10

#define INT_SRC_SSP2_ERROR  2
#define INT_SRC_USB_CTRL    11
#define INT_SRC_SSP1_DMA    14
#define INT_SRC_SSP1_ERROR  15
#define INT_SRC_SSP2_DMA    20
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
