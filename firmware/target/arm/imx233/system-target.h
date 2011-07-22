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

#define HW_POWER_CTRL           (*(volatile uint32_t *)(HW_POWER_BASE + 0x0))

#define HW_POWER_5VCTRL         (*(volatile uint32_t *)(HW_POWER_BASE + 0x10))

#define HW_POWER_MINPWR         (*(volatile uint32_t *)(HW_POWER_BASE + 0x20))

#define HW_POWER_CHARGE         (*(volatile uint32_t *)(HW_POWER_BASE + 0x30))

#define HW_POWER_VDDDCTRL       (*(volatile uint32_t *)(HW_POWER_BASE + 0x40))
#define HW_POWER_VDDDCTRL__TRG_BP   0
#define HW_POWER_VDDDCTRL__TRG_BM   0x1f
#define HW_POWER_VDDDCTRL__TRG_STEP 25 /* mV */
#define HW_POWER_VDDDCTRL__TRG_MIN  800 /* mV */

#define HW_POWER_VDDACTRL       (*(volatile uint32_t *)(HW_POWER_BASE + 0x50))

#define HW_POWER_VDDIOCTRL      (*(volatile uint32_t *)(HW_POWER_BASE + 0x60))

#define HW_POWER_VDDMEMCTRL     (*(volatile uint32_t *)(HW_POWER_BASE + 0x70))

#define HW_POWER_MISC           (*(volatile uint32_t *)(HW_POWER_BASE + 0x90))

#define HW_POWER_STS            (*(volatile uint32_t *)(HW_POWER_BASE + 0xc0))
#define HW_POWER_STS__PSWITCH_BP    20
#define HW_POWER_STS__PSWITCH_BM    (3 << 20)

#define HW_POWER_BATTMONITOR    (*(volatile uint32_t *)(HW_POWER_BASE + 0xe0))

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
#define INT_SRC_GPIO0       16
#define INT_SRC_GPIO1       17
#define INT_SRC_GPIO2       18
#define INT_SRC_GPIO(i)     (INT_SRC_GPIO0 + (i))
#define INT_SRC_SSP2_DMA    20
#define INT_SRC_I2C_DMA     26
#define INT_SRC_I2C_ERROR   27
#define INT_SRC_TIMER(nr)   (28 + (nr))
#define INT_SRC_LCDIF_DMA   45
#define INT_SRC_LCDIF_ERROR 46
#define INT_SRC_NR_SOURCES  66

/**
 * Absolute maximum CPU speed: 454.74 MHz
 * Intermediate CPU speeds: 392.73 MHz, 360MHz, 261.82 MHz, 64 MHz
 * Absolute minimum CPU speed: 24 MHz */
#define IMX233_CPUFREQ_454_MHz  454740000
#define IMX233_CPUFREQ_392_MHz  392730000
#define IMX233_CPUFREQ_360_MHz  360000000
#define IMX233_CPUFREQ_261_MHz  261820000
#define IMX233_CPUFREQ_64_MHz    64000000
#define IMX233_CPUFREQ_24_MHz    24000000

#define CPUFREQ_DEFAULT     IMX233_CPUFREQ_454_MHz
#define CPUFREQ_NORMAL      IMX233_CPUFREQ_454_MHz
#define CPUFREQ_MAX         IMX233_CPUFREQ_454_MHz
#define CPUFREQ_SLEEP       IMX233_CPUFREQ_454_MHz

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
