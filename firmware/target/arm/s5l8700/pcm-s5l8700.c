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
#include <string.h>

#include "config.h"
#include "system.h"
#include "audio.h"
#include "s5l8700.h"
#include "panic.h"
#include "audiohw.h"
#include "pcm.h"
#include "pcm-internal.h"
#include "pcm_sampr.h"
#include "dma-target.h"
#include "mmu-arm.h"

/*  Driver for the IIS/PCM part of the s5l8700 using DMA

    Notes:
    - pcm_play_dma_pause is untested, not sure if implemented the right way
    - pcm_play_dma_stop is untested, not sure if implemented the right way
    - recording is not implemented
*/

static volatile int locked = 0;
static const int zerosample = 0;
static unsigned char dblbuf[1024] IBSS_ATTR;
static const void* queuedbuf;
static size_t queuedsize;
static const void* nextbuf;
static size_t nextsize;

static const struct div_entry {
    int             pdiv, mdiv, sdiv, cdiv;
} div_table[HW_NUM_FREQ] = {
#ifdef IPOD_NANO2G
    [HW_FREQ_11] = {   0,   41,    3,    8},
    [HW_FREQ_22] = {   0,   41,    3,    4},
    [HW_FREQ_44] = {   0,   41,    3,    2},
    [HW_FREQ_88] = {   0,   41,    3,    1},
    [HW_FREQ_8 ] = {   0,    2,    1,    9},
    [HW_FREQ_16] = {   0,    2,    0,    9},
    [HW_FREQ_32] = {   2,    2,    0,    9},
    [HW_FREQ_64] = {   6,    2,    0,    9},
    [HW_FREQ_12] = {   0,    2,    2,    3},
    [HW_FREQ_24] = {   0,    2,    1,    3},
    [HW_FREQ_48] = {   0,    2,    0,    3},
    [HW_FREQ_96] = {   2,    2,    0,    3},
#else
/* table of recommended PLL/MCLK dividers for mode 256Fs from the datasheet */
    [HW_FREQ_11] = {  26,  189,    3,    8},
    [HW_FREQ_22] = {  50,   98,    2,    8},
    [HW_FREQ_44] = {  37,  151,    1,    9},
    [HW_FREQ_88] = {  50,   98,    1,    4},
#if 0   /* disabled because the codec driver does not support it (yet) */
    [HW_FREQ_8 ] = {  28,  192,    3,   12},
    [HW_FREQ_16] = {  28,  192,    3,    6},
    [HW_FREQ_32] = {  28,  192,    2,    6},
    [HW_FREQ_12] = {  28,  192,    3,    8},
    [HW_FREQ_24] = {  28,  192,    2,    8},
    [HW_FREQ_48] = {  28,  192,    2,    4},
    [HW_FREQ_96] = {  28,  192,    1,    4},
#endif
#endif
};

/* Mask the DMA interrupt */
void pcm_play_lock(void)
{
    if (locked++ == 0) {
        INTMSK &= ~(1 << 10);
    }
}

/* Unmask the DMA interrupt if enabled */
void pcm_play_unlock(void)
{
    if (--locked == 0) {
        INTMSK |= (1 << 10);
    }
}

void INT_DMA(void) ICODE_ATTR;
void INT_DMA(void)
{
    bool new_buffer = false;
    DMACOM0 = 7;
    while (!(DMACON0 & (1 << 18)))
    {
        if (queuedsize)
        {
            memcpy(dblbuf, queuedbuf, queuedsize);
            DMABASE0 = (unsigned int)dblbuf;
            DMATCNT0 = queuedsize / 2 - 1;
            queuedsize = 0;
        }
        else
        {
            if (!nextsize)
            {
                new_buffer = pcm_play_dma_complete_callback(
                                PCM_DMAST_OK, &nextbuf, &nextsize);
                if (!new_buffer)
                    break;
            }
            queuedsize = MIN(sizeof(dblbuf), nextsize / 2);
            nextsize -= queuedsize;
            queuedbuf = nextbuf + nextsize;
            DMABASE0 = (unsigned int)nextbuf;
            DMATCNT0 = nextsize / 2 - 1;
            nextsize = 0;
        }
        commit_dcache();
        DMACOM0 = 4;
        DMACOM0 = 7;

        if (new_buffer)
        {
            pcm_play_dma_status_callback(PCM_DMAST_STARTED);
            new_buffer = false;
        }
    }

}

void pcm_play_dma_start(const void* addr, size_t size)
{
    /* DMA channel on */
    nextbuf = addr;
    nextsize = size;
    queuedsize = 0;
    DMABASE0 = (unsigned int)(&zerosample);
    DMATCNT0 = 0;
    DMACOM0 = 4;

    /* IIS Tx clock on */
    I2SCLKCON = (1 << 0);   /* 1 = power on */
    
    /* IIS Tx on */
    I2STXCOM = (1 << 3) |   /* 1 = transmit mode on */
               (1 << 2) |   /* 1 = I2S interface enable */
               (1 << 1) |   /* 1 = DMA request enable */
               (0 << 0);    /* 0 = LRCK on */
}

void pcm_play_dma_stop(void)
{
    /* DMA channel off */
    DMACOM0 = 5;
    
    /* IIS Tx off */
    I2STXCOM = (1 << 3) |   /* 1 = transmit mode on */
               (0 << 2) |   /* 1 = I2S interface enable */
               (1 << 1) |   /* 1 = DMA request enable */
               (0 << 0);    /* 0 = LRCK on */
}

/* pause playback by disabling the I2S interface */
void pcm_play_dma_pause(bool pause)
{
    if (pause) {
        I2STXCOM |= (1 << 0);   /* LRCK off */
    }
    else {
        I2STXCOM &= ~(1 << 0);  /* LRCK on */
    }
}

static void pcm_dma_set_freq(enum hw_freq_indexes idx)
{
    struct div_entry div = div_table[idx];

    /* configure PLL1 and MCLK for the desired sample rate */
    PLL1PMS = (div.pdiv << 16) |
              (div.mdiv << 8) |
              (div.sdiv << 0);
    PLL1LCNT = 280;    /* 150 microseconds */

    /* enable PLL1 and wait for lock */
    PLLCON |= 1 << 1;
    while ((PLLLOCK & (1 << 1)) == 0);

    /* configure MCLK */
    CLKCON = (CLKCON & ~(0xFF)) | 
             (0 << 7) |         /* MCLK_MASK */
             (2 << 5) |         /* MCLK_SEL = PLL1 */
             (1 << 4) |         /* MCLK_DIV_ON */
             (div.cdiv - 1);    /* MCLK_DIV_VAL */
}

void pcm_play_dma_init(void)
{
    /* configure IIS pins */
#ifdef IPOD_NANO2G
    PCON5 = (PCON5 & ~(0xFFFF0000)) | 0x77720000;
    PCON6 = (PCON6 & ~(0x0F000000)) | 0x02000000;
#else
    PCON7 = (PCON7 & ~(0x0FFFFF00)) | 0x02222200;
#endif

    /* configure DMA channel */
    DMACON0 = (0 << 30) |       /* DEVSEL */
              (1 << 29) |       /* DIR */
              (0 << 24) |       /* SCHCNT */
              (1 << 22) |       /* DSIZE */
              (0 << 19) |       /* BLEN */
              (0 << 18) |       /* RELOAD */
              (0 << 17) |       /* HCOMINT */
              (1 << 16) |       /* WCOMINT */
              (0 << 0);         /* OFFSET */

    /* Enable the DMA IRQ */
    INTMSK |= (1 << 10);

    /* setup PLL */
    pcm_dma_set_freq(HW_FREQ_44);
    
    /* enable clock to the IIS module */
    PWRCON &= ~(1 << 6);

    /* configure IIS core */
#ifdef IPOD_NANO2G
    I2STXCON = (1 << 20) |  /* undocumented */
               (0 << 16) |  /* burst length */
               (0 << 15) |  /* 0 = falling edge */
               (0 << 13) |  /* 0 = basic I2S format */
               (0 << 12) |  /* 0 = MSB first */
               (0 << 11) |  /* 0 = left channel for low polarity */
               (3 << 8) |   /* MCLK divider */
               (0 << 5) |   /* 0 = 16-bit */
               (2 << 3) |   /* bit clock per frame */
               (1 << 0);    /* channel index */
#else
    I2STXCON = (DMA_IISOUT_BLEN << 16) |  /* burst length */
               (0 << 15) |  /* 0 = falling edge */
               (0 << 13) |  /* 0 = basic I2S format */
               (0 << 12) |  /* 0 = MSB first */
               (0 << 11) |  /* 0 = left channel for low polarity */
               (3 << 8) |   /* MCLK divider */
               (0 << 5) |   /* 0 = 16-bit */
               (0 << 3) |   /* bit clock per frame */
               (1 << 0);    /* channel index */
#endif

    audiohw_preinit();
}

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();
}

/* set the configured PCM frequency */
void pcm_dma_apply_settings(void)
{
    pcm_dma_set_freq(pcm_fsel);    
}

size_t pcm_get_bytes_waiting(void)
{
    return (nextsize + DMACTCNT0 + 2) << 1;
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    *count = DMACTCNT0 >> 1;
    return (void *)(((DMACADDR0 + 2) & ~3) | 0x40000000);
}

#ifdef HAVE_PCM_DMA_ADDRESS
void * pcm_dma_addr(void *addr)
{
    if (addr != NULL)
        addr = (void*)((uintptr_t)addr | 0x40000000);
    return addr;
}
#endif


/****************************************************************************
 ** Recording DMA transfer
 **/
#ifdef HAVE_RECORDING
void pcm_rec_lock(void)
{
}

void pcm_rec_unlock(void)
{
}

void pcm_rec_dma_stop(void)
{
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    (void)addr;
    (void)size;
}

void pcm_rec_dma_close(void)
{
}


void pcm_rec_dma_init(void)
{
}


const void * pcm_rec_dma_get_peak_buffer(void)
{
    return NULL;
}

#endif /* HAVE_RECORDING */
