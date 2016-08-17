/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#ifndef __JZ4760B_H__
#define __JZ4760B_H__

/*
 * Chip Memory Map:
 *   0x00000000 - 0x10000000: dram (physical address, remapped to 0x20000000 by EMC)
 *   0x132b0000 - 0x132b3fff: tcsm0 (physical address)
 *   0x132c0000 - 0x132c7fff: tcsm1 (physical address)
 *   0x132d0000 - 0x132d2fff: sram (physical address)
 *   0x20000000 - 0x30000000: dram (physical address)
 *   0x80000000 - 0x90000000: dram (virtual address, cached)
 *   0x932b0000 - 0x132d3fff: tcsm0 (virtual address, cached)
 *   0x932c0000 - 0x132d7fff: tcsm1 (virtual address, cached)
 *   0x932d0000 - 0x132d2fff: sram (virtual address, cached)
 *   0xb32b0000 - 0xb32d3fff: tcsm0 (virtual address, uncached)
 *   0xb32c0000 - 0xb32d7fff: tcsm1 (virtual address, uncached)
 *   0xb32d0000 - 0x132d2fff: sram (virtual address, uncached)
 *   0xf4000000 - 0xf4003fff: tcsm0 (cpu virtual address, always uncached)
 *
 * We use the following map:
 *   0x00000000 - 0x10000000: dram (physical address)
 *   0x132b0000 - 0x132b3fff: tcsm0 (physical address)
 *   0x80000000 - 0x87ffffff: dram (virtual address, cached)
 *   0x88000000 - 0x88003fff: tcsm0 (mmu mapped, uncached)
 *
 * The advantage of this setting is that dram and tcsm0 are in the same 28-bit
 * segment so that jumps can access any code in the system.
 *
 * WARNING When accessing the TCSM0 via its physical address, the access it done
 * through the AHB1 bus (yeah that means the CPU emits a AHB read to reads its
 * tighly-coupled memory...). Thus the AHB1 needs to be ungated or it will simply
 * not work.
 */

#define DRAM_ORIG           0x80000000
#define DRAM_SIZE           (MEMORYSIZE * 0x100000)

#define IRAM_ORIG           0x88000000
#define IRAM_SIZE           0x4000

#define PHYSICAL_DRAM_ADDR  0x20000000
#define PHYSICAL_IRAM_ADDR  0x132b0000

/* 32 bytes per cache line */
#define CACHEALIGN_SIZE     32
#define CACHEALIGN_BITS     5

#define ___IN_RANGE(type, a) (type##_ORIG <= (a) && (a) < (type##_ORIG + type##_SIZE))
#define __IN_RANGE(type, a) ___IN_RANGE(type, (uintptr_t)(a))
#define __REMAP(type, a)    ((uintptr_t)(a) - type##_ORIG + PHYSICAL_##type##_ADDR)
#define PHYSICAL_ADDR(a) \
    ((typeof(a)) \
        __IN_RANGE(DRAM, a) ? __REMAP(DRAM, a) \
        : __IN_RANGE(IRAM, a) ? __REMAP(IRAM, a) \
        : (uintptr_t)(a))

/* framebuffer: we put it at the end of the dram (see linker files) */
#define JZ_FRAMEBUFFER_SIZE (LCD_WIDTH * LCD_HEIGHT * LCD_DEPTH / 8)
/* align frame size to cache line */
#define FRAME_SIZE      ((JZ_FRAMEBUFFER_SIZE + CACHEALIGN_SIZE - 1) & ~(CACHEALIGN_SIZE - 1))
#define FRAME           ((void *)(DRAM_ORIG + DRAM_SIZE - FRAME_SIZE))

/* USBOTG */
#define USB_NUM_ENDPOINTS  3
#define USB_DEVBSS_ATTR    IBSS_ATTR

#endif /* __JZ4760B_H__ */
 
