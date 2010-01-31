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

/* ATA stuff was taken from the iPod code */

#include <stdbool.h>
#include "system.h"
#include "ata.h"
#include "ata-target.h"

void ata_reset() 
{

}

void ata_enable(bool on)
{
    /* TODO: Implement ata_enable() */
    (void)on;
}

bool ata_is_coldstart()
{
    return false;
    /* TODO: Implement coldstart variable */
}

void ata_device_init()
{
#ifdef SAMSUNG_YH920
    CPU_INT_DIS = (1<<IDE_IRQ);
#endif
#ifdef HAVE_ATA_DMA
    IDE_DMA_CONTROL |= 2;
    IDE_DMA_CONTROL &= ~1;
    IDE0_CFG &= ~0x8010;
    IDE0_CFG |= 0x20; 
#else

    /* From ipod-ide.c:ipod_ide_register() */
    IDE0_CFG |= (1<<5);
#ifdef IPOD_NANO
    IDE0_CFG |= (0x10000000); /* cpu > 65MHz */
#else
    IDE0_CFG &=~(0x10000000); /* cpu < 65MHz */
#endif
#endif

    IDE0_PRI_TIMING0 = 0x10;
    IDE0_PRI_TIMING1 = 0x80002150;
}

/* These are PIO timings for 80 Mhz.  At 24 Mhz, 
   the first value is 0 but the rest are the same.
   They go in IDE0_PRI_TIMING0.
   
   Rockbox used 0x10, and test_disk shows that leads to faster PIO.   
   If 0x10 is incorrect, these timings may be needed with some devices.
static const unsigned long pio80mhz[] = {
    0xC293, 0x43A2, 0x11A1, 0x7232, 0x3131
}; 
*/

#ifdef HAVE_ATA_DMA
/* Timings for multi-word and ultra DMA modes.
   These go in IDE0_PRI_TIMING1
 */
static const unsigned long tm_mwdma[] = {
    0xF9F92, 0x56562, 0x45451 
};

static const unsigned long tm_udma[] = { 
    0x800037C1, 0x80003491, 0x80003371, 
#if ATA_MAX_UDMA > 2       
    0x80003271, 0x80003071 
#endif
};

#if ATA_MAX_UDMA > 2
static bool dma_boosted = false;
static bool dma_needs_boost;
#endif

/* This function sets up registers for 80 Mhz.
   Ultra DMA mode 2 works at 30 Mhz.
 */
void ata_dma_set_mode(unsigned char mode) {
    int modeidx;

    (*(volatile unsigned long *)(0x600060C4)) = 0xC0000000; /* 80 Mhz */
    IDE0_CFG &= ~0x10000000;

    modeidx = mode & 7;
    mode &= 0xF8;
    if (mode == 0x40 && modeidx <= ATA_MAX_UDMA) {
        IDE0_PRI_TIMING1 = tm_udma[modeidx];
#if ATA_MAX_UDMA > 2
        if (modeidx > 2)
            dma_needs_boost = true;
        else
            dma_needs_boost = false;
#endif
    } else if (mode == 0x20 && modeidx <= ATA_MAX_MWDMA)
        IDE0_PRI_TIMING1 = tm_mwdma[modeidx];

    IDE0_CFG |= 0x20000000; /* >= 50 Mhz */
} 

#define IDE_CFG_INTRQ           8
#define IDE_DMA_CONTROL_READ    8

/* This waits for an ATA interrupt using polling.
   In ATA_CONTROL, CONTROL_nIEN must be cleared.
 */
STATICIRAM ICODE_ATTR int ata_wait_intrq(void)
{
    long timeout = current_tick + HZ*10;
    
    do 
    {
        if (IDE0_CFG & IDE_CFG_INTRQ)
            return 1;
        ata_keep_active();
        yield();
    } while (TIME_BEFORE(current_tick, timeout));

    return 0; /* timeout */
}

/* This function checks if parameters are appropriate for DMA, 
   and if they are, it sets up for DMA.
   
   If return value is false, caller may use PIO for this transfer.
   
   If return value is true, caller must issue a DMA ATA command
   and then call ata_dma_finish().
 */
bool ata_dma_setup(void *addr, unsigned long bytes, bool write) {
    /* Require cacheline alignment for reads to prevent interference. */
    if (!write && ((unsigned long)addr & 15))
        return false;

    /* Writes only need to be word-aligned, but by default DMA
     * is not used for writing as it appears to be slower.
     */
#ifdef ATA_DMA_WRITES
    if (write && ((unsigned long)addr & 3))
        return false;
#else
    if (write)
        return false;
#endif

#if ATA_MAX_UDMA > 2
    if (dma_needs_boost && !dma_boosted) {
        cpu_boost(true);
        dma_boosted = true;
    }
#endif

    if (write) {
        /* If unflushed, old data may be written to disk */
        cpucache_flush();
    }
    else {
        /* Invalidate cache because new data may be present in RAM */
        cpucache_invalidate();
    }

    /* Clear pending interrupts so ata_dma_finish() can wait for an
       interrupt from this transfer
     */
    IDE0_CFG |= IDE_CFG_INTRQ;

    IDE_DMA_CONTROL |= 2;
    IDE_DMA_LENGTH = bytes - 4;

#ifndef BOOTLOADER
    if ((unsigned long)addr < DRAM_START)
        /* Rockbox remaps DRAM to start at 0 */
        IDE_DMA_ADDR = (unsigned long)addr + DRAM_START;
    else
#endif
        IDE_DMA_ADDR = (unsigned long)addr;

    if (write)
        IDE_DMA_CONTROL &= ~IDE_DMA_CONTROL_READ;
    else
        IDE_DMA_CONTROL |= IDE_DMA_CONTROL_READ;

    IDE0_CFG |= 0x8000;

    return true;
}

/* This function waits for a DMA transfer to end.
   It must be called to finish what ata_dma_setup started.
   
   Return value is true if DMA completed before the timeout, and false
   if a timeout happened.
 */
bool ata_dma_finish(void) {
    bool res;
    
    /* It may be okay to put this at the end of setup */
    IDE_DMA_CONTROL |= 1;

    /* Wait for end of transfer.
       Reading standard ATA status while DMA is in progress causes 
       failures and hangs.  Because of that, another wait is used.
     */
    res = ata_wait_intrq();

    IDE0_CFG &= ~0x8000;
    IDE_DMA_CONTROL &= ~0x80000001;

#if ATA_MAX_UDMA > 2
    if (dma_boosted) {
        cpu_boost(false);
        dma_boosted = false;
    }
#endif

    return res;
}

#endif /* HAVE_ATA_DMA */
