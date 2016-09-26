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

/**
 * Absolute maximum CPU speed: 454.74 MHz (STMP3780), 320.00 MHz (STMP3700)
 * Intermediate CPU speeds: 261.82 MHz, 64 MHz
 * Absolute minimum CPU speed: 24 MHz */
#define IMX233_CPUFREQ_454_MHz  454740000
#define IMX233_CPUFREQ_320_MHz  320000000
#define IMX233_CPUFREQ_261_MHz  261820000
#define IMX233_CPUFREQ_64_MHz    64000000
#define IMX233_CPUFREQ_24_MHz    24000000

#define CPUFREQ_DEFAULT     IMX233_CPUFREQ_64_MHz
#define CPUFREQ_NORMAL      IMX233_CPUFREQ_64_MHz
#if IMX233_SUBTARGET >= 3780
#define CPUFREQ_MAX         IMX233_CPUFREQ_454_MHz
#elif IMX233_SUBTARGET >= 3700
#define CPUFREQ_MAX         IMX233_CPUFREQ_320_MHz
#endif
#define CPUFREQ_SLEEP       IMX233_CPUFREQ_64_MHz

void system_prepare_fw_start(void);
void imx233_system_prepare_shutdown(void);
void udelay(unsigned us);
bool imx233_us_elapsed(uint32_t ref, unsigned us_delay);
void imx233_reset_block(volatile uint32_t *block_reg);
void imx233_enable_usb_controller(bool enable);
void imx233_enable_usb_phy(bool enable);
// NOTE: this is available even if HAVE_ADJUSTABLE_CPU_FREQ is undef
void imx233_set_cpu_frequency(long frequency);

void udelay(unsigned usecs);

static inline void mdelay(unsigned msecs)
{
    udelay(1000 * msecs);
}

void usb_insert_int(void);
void usb_remove_int(void);

bool dbg_hw_target_info(void);

/* for watchdog */
void imx233_keep_alive(void);

#endif /* SYSTEM_TARGET_H */
