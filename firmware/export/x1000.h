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

#ifndef __X1000_H__
#define __X1000_H__

#include "config.h"

/* Frequency of external oscillator EXCLK */
//#define X1000_EXCLK_FREQ 24000000

/* Maximum CPU frequency that can be achieved on the target */
//#define CPU_FREQ         1008000000

/* Only 24 MHz and 26 MHz external oscillators are supported by the X1000 */
#if X1000_EXCLK_FREQ == 24000000
# define X1000_EXCLK_24MHZ
#elif X1000_EXCLK_FREQ == 26000000
# define X1000_EXCLK_26MHZ
#else
# error "Unsupported EXCLK freq"
#endif

/* On-chip TCSM (tightly coupled shared memory), aka IRAM */
#define X1000_TCSM_BASE         0xf4000000
#define X1000_TCSM_SIZE         (16 * 1024)

/* External SDRAM */
#define X1000_SDRAM_BASE        0x80000000
#define X1000_SDRAM_SIZE        (MEMORYSIZE * 1024 * 1024)

/* Memory definitions for Rockbox */
#define X1000_IRAM_BASE         X1000_SDRAM_BASE
#define X1000_IRAM_SIZE         (16 * 1024)
#define X1000_IRAM_END          (X1000_IRAM_BASE + X1000_IRAM_SIZE)
#define X1000_DRAM_BASE         X1000_IRAM_END
#define X1000_DRAM_SIZE         (X1000_SDRAM_SIZE - X1000_IRAM_SIZE)
#define X1000_DRAM_END          (X1000_DRAM_BASE + X1000_DRAM_SIZE)
#define X1000_STACKSIZE         0x1e00
#define X1000_IRQSTACKSIZE      0x300

/* Required for pcm_rec_dma_get_peak_buffer(), doesn't do anything
 * except on targets with recording. */
#define HAVE_PCM_DMA_ADDRESS
#define HAVE_PCM_REC_DMA_ADDRESS

/* Convert kseg0 address to physical address or uncached address */
#define PHYSADDR(x)     ((unsigned long)(x) & 0x1fffffff)
#define UNCACHEDADDR(x) (PHYSADDR(x) | 0xa0000000)

/* Defines for usb-designware driver */
#define OTGBASE             0xb3500000
#define USB_NUM_ENDPOINTS   9

/* CPU cache parameters */
#define CACHEALIGN_BITS     5
#define CACHE_SIZE          (16 * 1024)

#endif /* __X1000_H__ */
