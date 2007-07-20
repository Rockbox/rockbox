/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Will Robertson
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "power.h"
#include "panic.h"
#include "pcf50606.h"
#include "ata-target.h"
#include "mmu-imx31.h"
#include "backlight-target.h"

#define INTERRUPT_PENDING (*(volatile long*)0x43F8C024)
#define DMA_PENDING (1 << 3)
#define CONTROLLER_IDLE (1 << 4)
#define ATA_RST (1 << 6)

void ata_reset(void)
{
    ATA_CONTROL &= ~ATA_RST;
    sleep(1);
    ATA_CONTROL |= ATA_RST;
    sleep(1);
}

/* This function is called before enabling the USB bus */
void ata_enable(bool on)
{
    (void)on;
}

bool ata_is_coldstart(void)
{
    return 0;
}

void ata_device_init(void)
{
    bool reassert_dma_pending = false;

    if(ATA_CONTROL & DMA_PENDING)
        reassert_dma_pending = true;
    ATA_CONTROL &= ~DMA_PENDING;
    while(!(INTERRUPT_PENDING & CONTROLLER_IDLE))
        sleep(1);
    /* Setup the timing for PIO mode */
    /* TODO */

    /* If DMA_PENDING was set before we changed the timings
     * we need to reset it, otherwise a DMA read will fail */
    if(reassert_dma_pending)
        ATA_CONTROL |= DMA_PENDING;
    
}

#if !defined(BOOTLOADER)
void copy_read_sectors(unsigned char* buf, int wordcount)
{
    (void)buf;
    (void)wordcount;
}
#endif
