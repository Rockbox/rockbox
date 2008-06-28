/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "dma-target.h"
#include "dm320.h"
#include "ata-target.h"
#include <stdbool.h>

#define CS1_START       0x50000000
#define CS2_START       0x60000000
#define SDRAM_START     0x00900000
#define FLASH_START     0x00100000
#define CF_START        0x40000000
#define SSFDC_START     0x48000000

static struct wakeup transfer_completion_signal;

static bool dma_in_progress = false;

void MTC0(void)
{
    IO_INTC_IRQ1 = INTR_IRQ1_MTC0;
    wakeup_signal(&transfer_completion_signal);
    dma_in_progress = false;
}

void dma_start(const void* addr, size_t size)
{
  /* Compatibility with Gigabeat S in dma_start.c */
  (void) addr;
  (void) size;
}

#define ATA_DEST        (ATA_IOBASE-CS1_START)
void dma_ata_read(unsigned char* buf, int shortcount)
{   
    if(dma_in_progress)
        wakeup_wait(&transfer_completion_signal, TIMEOUT_BLOCK);
    
    while((unsigned long)buf & 0x1F)
    {
        unsigned short tmp;
        tmp = ATA_DATA;
        *buf++ = tmp & 0xFF;
        *buf++ = tmp >> 8;
        shortcount--;
    }
    
    if (!shortcount)
        return;
    
    IO_SDRAM_SDDMASEL = 0x0820; /* 32-byte burst mode transfer */
    IO_EMIF_DMAMTCSEL = 1; /* Select CS1 */
    IO_EMIF_AHBADDH = ((unsigned long)buf >> 16) & 0x7FFF; /* Set variable address */
    IO_EMIF_AHBADDL = (unsigned long)buf & 0xFFFF;
    IO_EMIF_MTCADDH = ( (1 << 15) | (ATA_DEST >> 16) ); /* Set fixed address */
    IO_EMIF_MTCADDL = ATA_DEST & 0xFFFF;
    IO_EMIF_DMASIZE = shortcount/2; /* 16-bits *2 = 1 word */
    IO_EMIF_DMACTL = 3; /* Select MTC->AHB and start transfer */
    
    dma_in_progress = true;
    wakeup_wait(&transfer_completion_signal, TIMEOUT_BLOCK);
    
    if(shortcount % 2)
    {
        unsigned short tmp;
        tmp = ATA_DATA;
        *buf++ = tmp & 0xFF;
        *buf++ = tmp >> 8;
    }
}

void dma_ata_write(unsigned char* buf, int wordcount)
{
    if(dma_in_progress)
        wakeup_wait(&transfer_completion_signal, TIMEOUT_BLOCK);
    
    while((unsigned long)buf & 0x1F)
    {
        unsigned short tmp;
        tmp = (unsigned short) *buf++;
        tmp |= (unsigned short) *buf++ << 8;
        SET_16BITREG(ATA_DATA, tmp);
        wordcount--;
    }
    
    if (!wordcount)
        return;
    
    IO_SDRAM_SDDMASEL = 0x0830; /* 32-byte burst mode transfer */
    IO_EMIF_DMAMTCSEL = 1; /* Select CS1 */
    IO_EMIF_AHBADDH = ((unsigned long)buf >> 16) & ~(1 << 15); /* Set variable address */
    IO_EMIF_AHBADDL = (unsigned long)buf & 0xFFFF;
    IO_EMIF_MTCADDH = ( (1 << 15) | (ATA_DEST >> 16) ); /* Set fixed address */
    IO_EMIF_MTCADDL = ATA_DEST & 0xFFFF;
    IO_EMIF_DMASIZE = (wordcount+1)/2;
    IO_EMIF_DMACTL = 1; /* Select AHB->MTC and start transfer */
    
    dma_in_progress = true;
    wakeup_wait(&transfer_completion_signal, TIMEOUT_BLOCK);
}

void dma_init(void)
{
    IO_INTC_EINT1 |= INTR_EINT1_MTC0;     /* enable MTC interrupt */
    wakeup_init(&transfer_completion_signal);
    dma_in_progress = false;
}
