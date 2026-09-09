/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

/* SoC glue for drivers shared with the X1000 (target/mips/ingenic) */
#ifndef __INGENIC_SOC_H__
#define __INGENIC_SOC_H__
#include "irq-x1600.h"
#include "gpio-x1600.h"
#include "nand-ingenic.h"
#include "i2c-ingenic.h"
#include "clk-x1600.h"
#include "spin-x1600.h"
#include "x1600/dma.h"
#include "x1600/dma_chn.h"
#include "x1600/cpm.h"
#include "x1600/ost.h"
#include "x1600/rtc.h"
#include "x1600/i2c.h"

#define SOC_CLK_PCLK X1600_CLK_PCLK
#define SOC_EXCLK_FREQ X1600_EXCLK_FREQ

/* The 18 KiB SPL slot ends before the vendor partition table. Use
 * bootloader2.ucl to distinguish the load address from the X1000 image
 * expected by older jztool versions. */
#define INSTALL_SPL_LENGTH      (18 * 1024)
#define INSTALL_BOOTLOADER_NAME "bootloader2.ucl"

/* Bounded: a wedged peripheral must not take the whole boot with it */
#define SOC_SPIN_WHILE(cond) ((void)x1600_spin_while(cond))
#define SOC_SPIN_UNTIL(cond) ((void)x1600_spin_until(cond))
/* Same, for waits that must yield to the kernel; needs the scheduler running */
#define SOC_SPIN_WHILE_DO(cond, body) ((void)x1600_spin_while_do(cond, body))

#define SOC_SPIN_GUARD_DECL(v)   uint32_t v = X1600_SPIN_GUARD
#define SOC_SPIN_GUARD_TICK(v)   (--(v))

/* The PM contradicts itself on how OSTFR clears, so do both: a flag left set
 * re-enters the tick interrupt immediately and stalls the boot. */
#define SOC_OST_ACK_RECHECK()                           \
    do {                                                \
        if(jz_readf(OST_1FLG, FFLAG))                   \
            jz_write(OST_1FLG, BM_OST_1FLG_FFLAG);      \
    } while(0)
#endif
