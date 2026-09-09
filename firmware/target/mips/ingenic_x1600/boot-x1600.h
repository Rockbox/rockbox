/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> >    <
 *   Player     |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 *
 * Based on firmware/target/mips/ingenic_x1000/boot-x1000.h
 * Copyright (C) 2021 Aidan MacDonald
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
#ifndef __BOOT_X1600_H__
#define __BOOT_X1600_H__

#include "x1600/cpm.h"
#include "x1600/gpio.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Boot flags in the CPM scratch-pad, which survives the jumps from SPL to
 * bootloader to Rockbox. CPSPR is writable only while CPSPPR holds 0x5a5a. */
enum {
    /* Set once clk_init() has run */
    BOOT_FLAG_CLK_INIT = (1 << 31),

    /* Set by the SPL if it was loaded over USB boot */
    BOOT_FLAG_USB_BOOT = (1 << 30),
};

/* Boot-method strap values on PC27/PC28. Sample before reconfiguring the
 * pins. The USB encoding is inferred and has not been verified through a
 * USB-booted SPL. */
enum {
    X1600_BOOT_SEL_MSC0 = 0,
    X1600_BOOT_SEL_SFC0 = 1,
    X1600_BOOT_SEL_NOR  = 2,
    X1600_BOOT_SEL_USB  = 3,
};

static inline unsigned x1600_boot_sel(void)
{
    /* port C = index 2; PIN is read-only */
    return (REG_GPIO_PIN(2) >> 27) & 0x3;
}

/* Inlined to keep the SPL small, exactly as the X1000 does it */
static inline void cpm_scratch_set(uint32_t value)
{
    REG_CPM_CPSPPR = 0x5a5a;    /* unlock: PM 11.1.2.22 */
    REG_CPM_CPSPR  = value;
    REG_CPM_CPSPPR = 0xa5a5;    /* re-lock */
}

static inline void init_boot_flags(void)
{
    cpm_scratch_set(0);
}

static inline bool get_boot_flag(uint32_t bit)
{
    return (REG_CPM_CPSPR & bit) != 0;
}

static inline void set_boot_flag(uint32_t bit)
{
    cpm_scratch_set(REG_CPM_CPSPR | bit);
}

static inline void clr_boot_flag(uint32_t bit)
{
    cpm_scratch_set(REG_CPM_CPSPR & ~bit);
}

/* Hand control to a Rockbox image; in IRAM because it copies over its DRAM */
void x1600_boot_rockbox(const void* source, size_t length)
    __attribute__((section(".icode.x1600_boot_rockbox")));

#endif /* __BOOT_X1600_H__ */
