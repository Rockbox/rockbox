/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Michael Sevakis
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
#include <stdlib.h>
#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "audio.h"
#include "sound.h"
#include "file.h"

/* PCM interrupt routine lockout */
static struct
{
    int locked;
    unsigned long state;
} dma_play_lock =
{
    .locked = 0,
    .state  = 0,
};

#define FIFO_COUNT ((IISFCON >> 6) & 0x3F)

/* Setup for the DMA controller */
#define DMA_CONTROL_SETUP  ((1<<31) | (1<<29) | (1<<23) | (1<<22) | (1<<20))

/* DMA count has hit zero - no more data */
/* Get more data from the callback and top off the FIFO */
void fiq_handler(void) __attribute__((interrupt ("FIQ")));

/* Mask the DMA interrupt */
void pcm_play_lock(void)
{
    if (++dma_play_lock.locked == 1)
        s3c_regset32(&INTMSK, DMA2_MASK);
}

/* Unmask the DMA interrupt if enabled */
void pcm_play_unlock(void)
{
    if (--dma_play_lock.locked == 0)
        s3c_regclr32(&INTMSK, dma_play_lock.state);
}

void pcm_play_dma_init(void)
{
    /* There seem to be problems when changing the IIS interface configuration
     * when a clock is not present.
     */
    s3c_regset32(&CLKCON, 1<<17);
    /* slave, transmit mode, 16 bit samples - MCLK 384fs - use 16.9344Mhz -
       BCLK 32fs */
    IISMOD = (1<<9) | (1<<8) | (2<<6) | (1<<3) | (1<<2) | (1<<0);

    /* RX,TX off,on */
    IISCON |= (1<<3) | (1<<2);

    s3c_regclr32(&CLKCON, 1<<17);

    audiohw_init();

    /* init GPIO */
    GPCCON = (GPCCON & ~(3<<14)) | (1<<14);
    GPCDAT |= (1<<7);
    /* GPE4=I2SDO, GPE3=I2SDI, GPE2=CDCLK, GPE1=I2SSCLK, GPE0=I2SLRCK */
    GPECON = (GPECON & ~0x3ff) | 0x2aa;

    /* Do not service DMA requests, yet */

    /* clear any pending int and mask it */
    s3c_regset32(&INTMSK, DMA2_MASK);
    SRCPND = DMA2_MASK;

    /* connect to FIQ */
    s3c_regset32(&INTMOD, DMA2_MASK);
}

void pcm_postinit(void)
{
    audiohw_postinit();
    pcm_apply_settings();
}

void pcm_dma_apply_settings(void)
{
    audiohw_set_frequency(pcm_fsel);
}

/* Connect the DMA and start filling the FIFO */
static void play_start_pcm(void)
{
    /* clear pending DMA interrupt */
    SRCPND = DMA2_MASK;

    pcm_apply_settings();

    /* Flush any pending writes */
    clean_dcache_range((void*)DISRC2, (DCON2 & 0xFFFFF) * 2);

    /* unmask DMA interrupt when unlocking */
    dma_play_lock.state = DMA2_MASK;

    /* turn on the request */
    IISCON |= (1<<5);

    /* Activate the channel */
    DMASKTRIG2 = 0x2;

    /* turn off the idle */
    IISCON &= ~(1<<3);

    /* start the IIS */
    IISCON |= (1<<0);
}

/* Disconnect the DMA and wait for the FIFO to clear */
static void play_stop_pcm(void)
{
    /* Mask DMA interrupt */
    s3c_regset32(&INTMSK, DMA2_MASK);

    /* De-Activate the DMA channel */
    DMASKTRIG2 = 0x4;

    /* are we playing? wait for the chunk to finish */
    if (dma_play_lock.state != 0)
    {
        /* wait for the FIFO to empty and DMA to stop */
        while ((IISCON & (1<<7)) || (DMASKTRIG2 & 0x2));
    }

    /* Keep interrupt masked when unlocking */
    dma_play_lock.state = 0;

    /* turn off the request */
    IISCON &= ~(1<<5);

    /* turn on the idle */
    IISCON |= (1<<3);

    /* stop the IIS */
    IISCON &= ~(1<<0);
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    /* Enable the IIS clock */
    s3c_regset32(&CLKCON, 1<<17);

    /* stop any DMA in progress - idle IIS */
    play_stop_pcm();

    /* connect DMA to the FIFO and enable the FIFO */
    IISFCON = (1<<15) | (1<<13);

    /* set DMA dest */
    DIDST2 = (unsigned int)&IISFIFO;

    /* IIS is on the APB bus, INT when TC reaches 0, fixed dest addr */
    DIDSTC2 = 0x03;

    /* set DMA source and options */
    DISRC2 = (unsigned int)addr + 0x30000000;
    /* How many transfers to make - we transfer half-word at a time = 2 bytes */
    /* DMA control: CURR_TC int, single service mode, I2SSDO int, HW trig */
    /*     no auto-reload, half-word (16bit) */
    DCON2 = DMA_CONTROL_SETUP | (size / 2);
    DISRCC2 = 0x00;  /* memory is on AHB bus, increment addresses */

    play_start_pcm();
}

/* Promptly stop DMA transfers and stop IIS */
void pcm_play_dma_stop(void)
{
    play_stop_pcm();

    /* Disconnect the IIS clock */
    s3c_regclr32(&CLKCON, 1<<17);
}

void pcm_play_dma_pause(bool pause)
{
    if (pause)
    {
        /* pause playback on current buffer */
        play_stop_pcm();
    }
    else
    {
        /* restart playback on current buffer */
        /* make sure we're aligned on left channel - skip any right
           channel sample left waiting */
        DISRC2 = (DCSRC2 + 2) & ~0x3;
        DCON2  = DMA_CONTROL_SETUP | (DSTAT2 & 0xFFFFE);
        play_start_pcm();
    }
}

void fiq_handler(void)
{
    static unsigned char *start;
    static size_t         size;
    register pcm_more_callback_type get_more;   /* No stack for this */

    /* clear any pending interrupt */
    SRCPND = DMA2_MASK;

    /* Buffer empty.  Try to get more. */
    get_more = pcm_callback_for_more;
    size = 0;

    if (get_more == NULL || (get_more(&start, &size), size == 0))
    {
        /* Callback missing or no more DMA to do */
        pcm_play_dma_stop();
        pcm_play_dma_stopped_callback();
    }
    else
    {
        /* Flush any pending cache writes */
        clean_dcache_range(start, size);

        /* set the new DMA values */
        DCON2 = DMA_CONTROL_SETUP | (size >> 1);
        DISRC2 = (unsigned int)start + 0x30000000;

        /* Re-Activate the channel */
        DMASKTRIG2 = 0x2;
    }
}

size_t pcm_get_bytes_waiting(void)
{
    /* lie a little and only return full pairs */
    return (DSTAT2 & 0xFFFFE) * 2;
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    unsigned long addr = DCSRC2;
    int cnt = DSTAT2;
    *count = (cnt & 0xFFFFF) >> 1;
    return (void *)((addr + 2) & ~3);
}
