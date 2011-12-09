/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Boris Gjenero
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

#ifndef __ATA_DRIVER_H__
#define __ATA_DRIVER_H__

#include "config.h" /* for HAVE_ATA_DMA */
#include "ata-target.h" /* for other target-specific defines */

 /* Returns true if the interface hasn't been initialised yet */
bool ata_is_coldstart(void) STORAGE_INIT_ATTR;
/* Initializes the interface */
void ata_device_init(void) STORAGE_INIT_ATTR;
/* ata_enable(true) is used after ata_device_init() to enable the interface
 * ata_enable(false) is used to disable the interface so 
 * an ATA to USB bridge chip can use it instead.*/
void ata_enable(bool on);
/* ATA hard reset: pulse the RESET pin */
#ifdef HAVE_ATA_POWER_OFF
void ata_reset(void);
#else
void ata_reset(void) STORAGE_INIT_ATTR;
#endif

/* Optional optimized target-specific PIO transfer */
#ifdef ATA_OPTIMIZED_READING
void copy_read_sectors(unsigned char* buf, int wordcount);
#endif
#ifdef ATA_OPTIMIZED_WRITING
void copy_write_sectors(const unsigned char* buf, int wordcount);
#endif

/* Optional target-specific waiting */
#ifdef ATA_TARGET_POLLING
int ata_wait_for_bsy(void);
int ata_wait_for_rdy(void);
#endif

/* Optional setting of controller timings for PIO mode */
#ifdef ATA_SET_PIO_TIMING
void ata_set_pio_timings(int mode);
#endif

#ifdef HAVE_ATA_DMA
/* Used to update last disk activity time while waiting for DMA */
void ata_keep_active(void);
/* Set DMA mode for ATA interface */
void ata_dma_set_mode(unsigned char mode);
/* Sets up DMA transfer */
bool ata_dma_setup(void *addr, unsigned long bytes, bool write);
/* Waits for DMA transfer completion */
bool ata_dma_finish(void);
#endif /* HAVE_ATA_DMA */

#endif /* __ATA_DRIVER_H__ */
