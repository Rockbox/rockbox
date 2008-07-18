/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006,2007 by Marcoen Hirschberg
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
#include "config.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "power.h"
#include "panic.h"
#include "pcf50606.h"
#include "ata.h"
#include "ata-target.h"
#include "backlight-target.h"

/* ARESET on C7C68300 and RESET on ATA interface (Active Low) */
#define ATA_RESET_ENABLE GPGDAT &= ~(1 << 10)
#define ATA_RESET_DISABLE GPGDAT |= (1 << 10)

/* ATA_EN on C7C68300 */
#define USB_ATA_ENABLE   GPBDAT |=  (1 << 5)
#define USB_ATA_DISABLE  GPBDAT &= ~(1 << 5)

void ata_reset(void)
{
    ATA_RESET_ENABLE;
    sleep(1); /* > 25us */
    ATA_RESET_DISABLE;
    sleep(1); /* > 2ms */
}

/* This function is called before enabling the USB bus */
void ata_enable(bool on)
{
    GPBCON=( GPBCON&~(1<<11) ) | (1<<10); /* Make the pin an output */
    GPBUP|=1<<5;  /* Disable pullup in SOC as we are now driving */
    if(on)
        USB_ATA_DISABLE;
    else
        USB_ATA_ENABLE;
}

bool ata_is_coldstart(void)
{
    /* Check the pin configuration - return true when pin is unconfigured */
    return (GPGCON & 0x00300000) == 0;
}

void ata_device_init(void)
{
    GPGCON=( GPGCON&~(1<<21) ) | (1<<20); /* Make the pin an output */
    GPGUP |= 1<<10;  /* Disable pullup in SOC as we are now driving */
    /* ATA reset */
    ATA_RESET_DISABLE; /* Set the pin to disable an active low reset */
}

#if !defined(BOOTLOADER)
void copy_read_sectors(unsigned char* buf, int wordcount)
{
    __buttonlight_trigger();

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
    DIDST0 = (int) buf;
	if(DIDST0 < 0x30000000)
		DIDST0 += 0x30000000;
    DIDSTC0 = 0;

    /* DACK/DREQ Sync to AHB, Whole service, No reload, 16-bit transfers */
    DCON0 = ((1 << 30) | (1<<27) | (1<<22) | (1<<20)) | wordcount;

    /* Activate the channel */
    DMASKTRIG0 = 0x2;

    invalidate_dcache_range((void *)buf, wordcount*2);

    /* Start DMA */
    DMASKTRIG0 |= 0x1;

    /* Wait for transfer to complete */
    while((DSTAT0 & 0x000fffff))
        yield();
    /* Dump cache for the buffer  */
}
#endif
