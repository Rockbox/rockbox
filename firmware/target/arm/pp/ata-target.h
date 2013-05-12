/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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

#ifndef ATA_TARGET_H
#define ATA_TARGET_H

#include "config.h"

/* primary channel */
#define ATA_DATA        (*((volatile unsigned short*)(IDE_BASE + 0x1e0)))
#define ATA_ERROR       (*((volatile unsigned char*)(IDE_BASE + 0x1e4)))
#define ATA_NSECTOR     (*((volatile unsigned char*)(IDE_BASE + 0x1e8)))
#define ATA_SECTOR      (*((volatile unsigned char*)(IDE_BASE + 0x1ec)))
#define ATA_LCYL        (*((volatile unsigned char*)(IDE_BASE + 0x1f0)))
#define ATA_HCYL        (*((volatile unsigned char*)(IDE_BASE + 0x1f4)))
#define ATA_SELECT      (*((volatile unsigned char*)(IDE_BASE + 0x1f8)))
#define ATA_COMMAND     (*((volatile unsigned char*)(IDE_BASE + 0x1fc)))
#define ATA_CONTROL     (*((volatile unsigned char*)(IDE_BASE + 0x3f8)))

#if (CONFIG_CPU == PP5002)

#define ATA_OUT8(reg,val) do { reg = (val); \
                              while (!(IDE_CFG_STATUS & 0x40)); \
                          } while (0)

/* Plain C reading and writing. See comment in ata-as-arm.S */

#elif defined CPU_PP502x

/* asm optimized reading and writing */
#define ATA_OPTIMIZED_READING
#define ATA_OPTIMIZED_WRITING

#define ATA_SET_PIO_TIMING

#endif /* CONFIG_CPU */

#ifdef HAVE_ATA_DMA

/* IDE DMA controller registers */
#define IDE_DMA_CONTROL (*(volatile unsigned long *)(0xc3000400))
#define IDE_DMA_LENGTH  (*(volatile unsigned long *)(0xc3000408))
#define IDE_DMA_ADDR    (*(volatile unsigned long *)(0xc300040C))

/* Maximum multi-word DMA mode supported by the controller */
#define ATA_MAX_MWDMA 2

#ifndef BOOTLOADER    
/* The PP5020 supports UDMA 4, but it needs cpu boosting and only
 * improves performance by ~10% with a stock disk.
 * UDMA 2 is stable at 30 Mhz.
 * UDMA 1 is stable at 24 Mhz.
 */
#if CPUFREQ_NORMAL >= 30000000
#define ATA_MAX_UDMA 2
#elif CPUFREQ_NORMAL >= 24000000
#define ATA_MAX_UDMA 1
#else
#error "CPU speeds under 24Mhz have not been tested with DMA"
#endif
#else
/* The bootloader runs at 24 Mhz and needs a slower mode */
#define ATA_MAX_UDMA 1
#endif

#endif /* HAVE_ATA_DMA */

#endif /* ATA_TARGET_H */
