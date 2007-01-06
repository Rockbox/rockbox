/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id $
 *
 * Copyright (C) 2006,2007 by Marcoen Hirschberg
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
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "panic.h"
#include "pcf50606.h"
#include "ata-target.h"
#include "mmu-meg-fx.h"

void ata_reset(void)
{
    GPGDAT &= ~(1 << 10);
    sleep(1); /* > 25us */
    GPGDAT |= (1 << 10);
    sleep(1); /* > 2ms */
}

void ata_enable(bool on)
{
    if(on)
        GPGDAT &= ~(1 << 12);
    else
        GPGDAT |= (1 << 12);
}

bool ata_is_coldstart(void)
{
    return true; /* TODO */
}

void ata_device_init(void)
{
}

void copy_read_sectors(unsigned char* buf, int wordcount)
{
    /* Unaligned transfer - slow copy */
    if ( (unsigned long)buf & 1)
    {   /* not 16-bit aligned, copy byte by byte */
        unsigned short tmp = 0;
        unsigned char* bufend = buf + wordcount*2;
        do
        {
            tmp = ATA_DATA;
            *buf++ = tmp & 0xff; /* I assume big endian */
            *buf++ = tmp >> 8;   /*  and don't use the SWAB16 macro */
        } while (buf < bufend); /* tail loop is faster */
        return;
    }
    /* This should never happen, but worth watching for */
    if(wordcount > (1 << 18))
        panicf("atd-meg-fx.c: copy_read_sectors: too many sectors per read!");

//#define GIGABEAT_DEBUG_ATA
#ifdef GIGABEAT_DEBUG_ATA
        static int line = 0;
        static char str[256];
        snprintf(str, sizeof(str), "ODD DMA to %08x, %d", buf, wordcount);
        lcd_puts(10, line, str);
        line = (line+1) % 32;
        lcd_update();
#endif
    /* Reset the channel */
    DMASKTRIG0 |= 4;
    /* Wait for DMA controller to be ready */
    while(DMASKTRIG0 & 0x2)
        ;
    while(DSTAT0 & (1 << 20))
        ;
    /* Source is ATA_DATA, on AHB Bus, Fixed */
    DISRC0 = (int) 0x18000000;
    DISRCC0 = 0x1;
    /* Dest mapped to physical address, on AHB bus, increment */
    DIDST0 = (int) (buf + 0x30000000);
    DIDSTC0 = 0;

    /* DACK/DREQ Sync to AHB, Int on Transfer complete, Whole service, No reload, 16-bit transfers */
    DCON0 = ((1 << 30) | (1<< 29) | (1<<27) | (1<<22) | (1<<20)) | wordcount;

    /* Activate the channel */
    DMASKTRIG0 = 0x2;

    invalidate_dcache_range((void *)buf, wordcount*2);

    INTMSK &= ~(1<<17);      /* unmask the interrupt */
    SRCPND = (1<<17);       /* clear any pending interrupts */
    /* Start DMA */
    DMASKTRIG0 |= 0x1;

    /* Wait for transfer to complete */
    while((DSTAT0 & 0x000fffff))
        CLKCON |= (1 << 2); /* set IDLE bit */
    /* Dump cache for the buffer  */
}

void dma0(void)
{
}


