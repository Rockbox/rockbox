/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 Bertrik Sikken
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

#include "s5l8700.h"
#include "dma-target.h"
#include "panic.h"
#include "system.h"

/*  Driver for the IODMA part of the s5l8700

    When requesting a DMA transfer the supplied callback is stored and called
    upon completion of the DMA transfer (callback runs in interrupt context).
 */


#define DMAC_BASE       0x38400000

#define DMABASE(c)  (*(volatile unsigned int*)(DMAC_BASE+0x00+(0x20*c)))
#define DMACON(c)   (*(volatile unsigned int*)(DMAC_BASE+0x04+(0x20*c)))
#define DMATCNT(c)  (*(volatile unsigned int*)(DMAC_BASE+0x08+(0x20*c)))
#define DMACADDR(c) (*(volatile unsigned int*)(DMAC_BASE+0x0C+(0x20*c)))
#define DMACTCNT(c) (*(volatile unsigned int*)(DMAC_BASE+0x10+(0x20*c)))
#define DMACOM(c)   (*(volatile unsigned int*)(DMAC_BASE+0x14+(0x20*c)))
#define DMANOFF(c)  (*(volatile unsigned int*)(DMAC_BASE+0x18+(0x20*c)))

#define DMACOM_HOLD             2   /* only allowed on channel 0 */
#define DMACOM_SKIP             3   /* only allowed on channel 0 */
#define DMACOM_CHAN_ON          4
#define DMACOM_CHAN_OFF         5
#define DMACOM_CLEAR_HCOM       6
#define DMACOM_CLEAR_HCOM_WCOM  7


/* one completion callback for each channel */
static void (*dma_callback[4])(void);


void dma_init(void)
{
    int i;
    
    for (i = 0; i < 4; i++) {
        dma_callback[i] = NULL;
        dma_disable_channel(i);
    }

    INTMSK |= (1 << 10);
}

/* setup a DMA transfer, but do not start it yet */
void dma_setup_channel(int channel, int sel, int dir, int dsize, int blen,
                       void *addr, size_t size, void (*callback)(void))
{
    dma_callback[channel] = callback;

    DMACON(channel) = (sel << 30) |     /* DEVSEL */
                      (dir << 29) |     /* DIR */
                      (0 << 24) |       /* SCHCNT */
                      (dsize << 22) |   /* DSIZE */
                      (blen << 19) |    /* BLEN */
                      (0 << 18) |       /* RELOAD */
                      (0 << 17) |       /* HCOMINT */
                      (1 << 16) |       /* WCOMINT */
                      (0 << 0);         /* OFFSET */
    DMABASE(channel) = (unsigned int)addr;
    DMATCNT(channel) = size - 1;
}

void dma_enable_channel(int channel)
{
    DMACOM(channel) = DMACOM_CHAN_ON;
}

void dma_disable_channel(int channel)
{
    DMACOM(channel) = DMACOM_CHAN_OFF;
}

/* interrupt handler for all DMA channels */
void INT_DMA(void)
{
    unsigned int mask;
    int channel;

    mask = (1 << 0) |   /* WCOMx interrupt bit */
           (1 << 1);    /* HCOMx interrupt bit */
    for (channel = 0; channel < 4; channel++) {
        if (DMAALLST & mask) {
            /* clear half and whole completion bits */
            DMACOM(channel) = DMACOM_CLEAR_HCOM_WCOM;

            if (dma_callback[channel]) {
                dma_callback[channel]();
            }
        }
        mask <<= 4;
    }
}

