/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021-2022 Aidan MacDonald
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

/* On-chip TCSM (tightly coupled shared memory), aka IRAM. The SPL runs from
 * here, but the rest of Rockbox doesn't use it - it is too difficult to use
 * as a normal memory region because it's not in KSEG0. */
#define X1000_TCSM_BASE         0xf4000000
#define X1000_TCSM_SIZE         (16 * 1024)

/* SPL load and entry point addresses, this is defined by the HW boot ROM.
 * First 4K is used by mask ROM for stack + variables, and the next 2K are
 * occupied by SPL header. Usable code+data size is 10K. */
#define X1000_SPL_LOAD_ADDR     (X1000_TCSM_BASE + 0x1000)
#define X1000_SPL_EXEC_ADDR     (X1000_TCSM_BASE + 0x1800)
#define X1000_SPL_SIZE          (X1000_TCSM_SIZE - 0x1800)

/* External SDRAM - just one big linear mapping in KSEG0. */
#define X1000_SDRAM_BASE        0x80000000
#define X1000_SDRAM_SIZE        (MEMORYSIZE * 1024 * 1024)
#define X1000_SDRAM_END         (X1000_SDRAM_BASE + X1000_SDRAM_SIZE)

/* Memory definitions for Rockbox
 *
 * IRAM - Contains the exception handlers and acts as a safe stub area
 *        from which you can overwrite the rest of DRAM (used by RoLo).
 *
 * DRAM - This is the main RAM area used for code, data, and bss sections.
 *        The audio, codec, and plugin buffers also reside in here.
 *
 * X1000_IRAM_BASE is the base of the exception vectors and must be set to
 * the base of kseg0 (0x80000000). The X1000 supports the EBase register so
 * the vectors can be remapped, allowing IRAM to be moved to any 4K-aligned
 * address, but it would introduce more complexity and there's currently no
 * good reason to do this.
 *
 * X1000_DRAM_BASE doubles as the entry point address. There is some legacy
 * baggage surrounding this value so be careful when changing it.
 *
 * - Rockbox's DRAM_BASE should always equal X1000_STANDARD_DRAM_BASE because
 *   this value is hardcoded by old bootloaders released in 2021. This can be
 *   changed if truly necessary, but it should be avoided.
 * - The bootloader's DRAM_BASE can be changed freely but if it isn't equal
 *   to X1000_STANDARD_DRAM_BASE, the update package generation *must* be
 *   updated to use the "bootloader2.ucl" filename to ensure old jztools do
 *   not try to incorrectly boot the binary at the wrong load address.
 *
 * The bootloader DRAM_BASE is also hardcoded in the SPL, but the SPL is
 * considered as part of the bootloader to avoid introducing unnecessary
 * ABI boundaries. Therefore this hardcoded use can safely be ignored.
 *
 * There is no requirement that IRAM and DRAM are contiguous, but they must
 * reside in the same segment (ie. upper 3 address bits must be identical),
 * otherwise we need long calls to go between the two.
 */
#define X1000_IRAM_BASE         X1000_SDRAM_BASE
#define X1000_IRAM_SIZE         (16 * 1024)
#define X1000_IRAM_END          (X1000_IRAM_BASE + X1000_IRAM_SIZE)
#define X1000_DRAM_BASE         X1000_IRAM_END
#define X1000_DRAM_SIZE         (X1000_SDRAM_SIZE - X1000_IRAM_SIZE)
#define X1000_DRAM_END          (X1000_DRAM_BASE + X1000_DRAM_SIZE)

/* Stacks are placed in IRAM to avoid various annoying issues in boot code. */
#define X1000_STACKSIZE         0x1e00
#define X1000_IRQSTACKSIZE      0x300

/* Standard DRAM base address for backward compatibility */
#define X1000_STANDARD_DRAM_BASE 0x80004000

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
