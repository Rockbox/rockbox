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
#include "clkctrl-imx233.h"
#include "icoll-imx233.h"
#include "clock-target.h" /* CPUFREQ_* are defined here */

/* Digital control */
#define HW_DIGCTL_BASE          0x8001C000
#define HW_DIGCTL_CTRL          (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0))
#define HW_DIGCTL_CTRL__USB_CLKGATE (1 << 2)

#define HW_DIGCTL_HCLKCOUNT     (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0x20))

#define HW_DIGCTL_MICROSECONDS  (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0xC0))

/* USB Phy */
#define HW_USBPHY_BASE          0x8007C000 
#define HW_USBPHY_PWD           (*(volatile uint32_t *)(HW_USBPHY_BASE + 0))
#define HW_USBPHY_PWD__ALL      (7 << 10 | 0xf << 17)

#define HW_USBPHY_CTRL          (*(volatile uint32_t *)(HW_USBPHY_BASE + 0x30))

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

void udelay(unsigned us);
bool imx233_us_elapsed(uint32_t ref, unsigned us_delay);
void imx233_reset_block(volatile uint32_t *block_reg);
void power_off(void);
void imx233_enable_usb_controller(bool enable);
void imx233_enable_usb_phy(bool enable);

void udelay(unsigned usecs);

static inline void mdelay(unsigned msecs)
{
    udelay(1000 * msecs);
}

void usb_insert_int(void);
void usb_remove_int(void);

bool dbg_hw_target_info(void);

#endif /* SYSTEM_TARGET_H */
