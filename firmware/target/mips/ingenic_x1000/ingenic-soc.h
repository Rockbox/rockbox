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

/* SoC glue for drivers shared with the X1600 (firmware/target/mips/ingenic). */
#ifndef __INGENIC_SOC_H__
#define __INGENIC_SOC_H__
#include "irq-x1000.h"
#include "msc-x1000.h"
#include "uart-x1000.h"
#include "gpio-ingenic.h"
#include "sfc-x1000.h"
#include "nand-ingenic.h"
#include "i2c-ingenic.h"
#include "clk-x1000.h"
#include "x1000/dma.h"
#include "x1000/dma_chn.h"
#include "x1000/cpm.h"
#include "x1000/ost.h"
#include "x1000/rtc.h"
#include "x1000/i2c.h"

#define SOC_CLK_PCLK X1000_CLK_PCLK
#define SOC_EXCLK_FREQ X1000_EXCLK_FREQ

/* Bootloader install map. */
#define INSTALL_SPL_LENGTH      (12 * 1024)
#define INSTALL_BOOTLOADER_NAME "bootloader.ucl"

#define SOC_SPIN_WHILE(cond) do { while(cond); } while(0)
#define SOC_SPIN_UNTIL(cond) do { while(!(cond)); } while(0)
#define SOC_SPIN_WHILE_DO(cond, body) do { while(cond) { body; } } while(0)

#define SOC_SPIN_GUARD_DECL(v)
#define SOC_SPIN_GUARD_TICK(v)   1

/* The X1000's OSTFR clears on the write; no recheck needed. */
#define SOC_OST_ACK_RECHECK()    do { } while(0)
#endif
