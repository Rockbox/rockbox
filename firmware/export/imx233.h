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
#ifndef __IMX233_H__
#define __IMX233_H__

/*
 * Chip Memory Map:
 *   0x00000000 - 0x00007fff: on chip ram
 *   0x40000000 - 0x5fffffff: dram (512Mb max)
 *   0x80000000 - 0x80100000: memory mapped registers
 * We use the following map:
 *   0x60000000 - 0x7fffffff: dram (cached)
 *   0x90000000 - 0xafffffff: dram (buffered)
 *   everything else        : identity mapped (uncached)
 *
 * As a side note it's important to notice that uncached dram is identity mapped
 */

#define IRAM_ORIG           0
#define IRAM_SIZE           0x8000
#define DRAM_ORIG           0x40000000
#define DRAM_SIZE           (MEMORYSIZE * 0x100000)

#define UNCACHED_DRAM_ADDR  0x40000000
#define CACHED_DRAM_ADDR    0x60000000
#define BUFFERED_DRAM_ADDR  0x90000000
#define CACHEALIGN_SIZE     32

#define NOCACHE_BASE        (UNCACHED_DRAM_ADDR - CACHED_DRAM_ADDR)

#define PHYSICAL_ADDR(a) \
    ((typeof(a))((uintptr_t)(a) >= BUFFERED_DRAM_ADDR ? \
        ((uintptr_t)(a) - BUFFERED_DRAM_ADDR + UNCACHED_DRAM_ADDR) \
        :(uintptr_t)(a) >= CACHED_DRAM_ADDR ? \
        ((uintptr_t)(a) - CACHED_DRAM_ADDR + UNCACHED_DRAM_ADDR) \
        :(uintptr_t)(a)))
#define UNCACHED_ADDR(a) PHYSICAL_ADDR(a)

#define TTB_BASE_ADDR   (DRAM_ORIG + DRAM_SIZE - TTB_SIZE)
#define TTB_SIZE        0x4000
#define TTB_BASE        ((unsigned long *)TTB_BASE_ADDR)
#define FRAME_SIZE      (LCD_WIDTH * LCD_HEIGHT * LCD_DEPTH / 8)
#define FRAME_PHYS_ADDR (DRAM_ORIG + DRAM_SIZE - TTB_SIZE - FRAME_SIZE)
#define FRAME           ((void *)(FRAME_PHYS_ADDR - UNCACHED_DRAM_ADDR + BUFFERED_DRAM_ADDR))

/* Timer runs at APBX speed which is derived from ref_xtal@24MHz */
#define TIMER_FREQ      24000000

#ifdef SANSA_FUZEPLUS
#define TICK_TIMER_NR   0
#define USER_TIMER_NR   1
#else
#error Select timers !
#endif

/* USBOTG */
#define USB_QHARRAY_ATTR    __attribute__((section(".qharray"),nocommon,aligned(2048)))
#define USB_NUM_ENDPOINTS   5
#define USB_DEVBSS_ATTR     NOCACHEBSS_ATTR
#define USB_BASE            0x80080000
/*
#define QHARRAY_SIZE        ((64*USB_NUM_ENDPOINTS*2 + 2047) & (0xffffffff - 2047))
#define QHARRAY_PHYS_ADDR   ((FRAME_PHYS_ADDR - QHARRAY_SIZE) & (0xffffffff - 2047))
*/

#define __REG_SET(reg)  (*((volatile uint32_t *)(&reg + 1)))
#define __REG_CLR(reg)  (*((volatile uint32_t *)(&reg + 2)))
#define __REG_TOG(reg)  (*((volatile uint32_t *)(&reg + 3)))

#define __BLOCK_SFTRST  (1 << 31)
#define __BLOCK_CLKGATE (1 << 30)

#define CACHEALIGN_BITS     4

#define __XTRACT(reg, field)    ((reg & reg##__##field##_BM) >> reg##__##field##_BP)
#define __XTRACT_EX(val, field)    (((val) & field##_BM) >> field##_BP)
#define __FIELD_SET(reg, field, val) reg = (reg & ~reg##__##field##_BM) | (val << reg##__##field##_BP)

#endif /* __IMX233_H__ */
