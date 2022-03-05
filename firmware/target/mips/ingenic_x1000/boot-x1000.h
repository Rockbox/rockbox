/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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

#ifndef __BOOT_X1000_H__
#define __BOOT_X1000_H__

#include "x1000/cpm.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

enum {
    /* Set after running clk_init() and setting up system clocks */
    BOOT_FLAG_CLK_INIT = (1 << 31),

    /* Set by the SPL if it was loaded over USB boot */
    BOOT_FLAG_USB_BOOT = (1 << 30),
};

void x1000_boot_rockbox(const void* source, size_t length)
    __attribute__((section(".icode")));
void x1000_boot_linux(const void* source, size_t length,
                      void* load, void* entry, const char* args)
    __attribute__((section(".icode")));

/* dual boot support code */
void x1000_dualboot_cleanup(void);
void x1000_dualboot_init_clocktree(void);
void x1000_dualboot_init_uart2(void);
int x1000_dualboot_load_pdma_fw(void);

/* Note: these functions are inlined to minimize SPL code size.
 * They are private to the X1000 early boot code anyway... */

static inline void cpm_scratch_set(uint32_t value)
{
    /* TODO: see if this holds its state over a WDT reset */
    REG_CPM_SCRATCH_PROT = 0x5a5a;
    REG_CPM_SCRATCH = value;
    REG_CPM_SCRATCH_PROT = 0xa5a5;
}

static inline void init_boot_flags(void)
{
    cpm_scratch_set(0);
}

static inline bool get_boot_flag(uint32_t bit)
{
    return (REG_CPM_SCRATCH & bit) != 0;
}

static inline void set_boot_flag(uint32_t bit)
{
    cpm_scratch_set(REG_CPM_SCRATCH | bit);
}

static inline void clr_boot_flag(uint32_t bit)
{
    cpm_scratch_set(REG_CPM_SCRATCH & ~bit);
}

#endif /* __BOOT_X1000_H__ */
