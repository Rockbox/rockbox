/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008-2009 Rafaël Carré
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
#include "system.h"
#include "audio.h"
#include "string.h"
#include "as3525.h"
#include "pl081.h"
#include "dma-target.h"
#include "clock-target.h"
#include "panic.h"
#include "as3514.h"
#include "audiohw.h"
#include "mmu-arm.h"

#define MAX_TRANSFER (4*((1<<11)-1)) /* maximum data we can transfer via DMA
                                      * i.e. 32 bits at once (size of I2SO_DATA)
                                      * and the number of 32bits words has to
                                      * fit in 11 bits of DMA register */

static unsigned char *dma_start_addr;
static size_t      dma_size;   /* in 4*32 bits */
static void dma_callback(void);
static int locked = 0;

/* Mask the DMA interrupt */
void pcm_play_lock(void)
{
    if(++locked == 1)
        VIC_INT_EN_CLEAR = INTERRUPT_DMAC;
}

/* Unmask the DMA interrupt if enabled */
void pcm_play_unlock(void)
{
    if(--locked == 0)
        VIC_INT_ENABLE = INTERRUPT_DMAC;
}

static void play_start_pcm(void)
{
    const unsigned char* addr = dma_start_addr;
    size_t size = dma_size;
    if(size > MAX_TRANSFER)
        size = MAX_TRANSFER;

    dma_size -= size;
    dma_start_addr += size;

    clean_dcache_range((void*)addr, size);  /* force write back */
    dma_enable_channel(1, (void*)addr, (void*)I2SOUT_DATA, DMA_PERI_I2SOUT,
                DMAC_FLOWCTRL_DMAC_MEM_TO_PERI, true, false, size >> 2, DMA_S1,
                dma_callback);
}

static void dma_callback(void)
{
    if(!dma_size)
    {
        register pcm_more_callback_type get_more = pcm_callback_for_more;
        if(get_more)
            get_more(&dma_start_addr, &dma_size);
    }

    if(!dma_size)
    {
        pcm_play_dma_stop();
        pcm_play_dma_stopped_callback();
    }
    else
        play_start_pcm();
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    dma_size = size;
    dma_start_addr = (unsigned char*)addr;

    CGU_PERI |= CGU_I2SOUT_APB_CLOCK_ENABLE;
    CGU_AUDIO |= (1<<11);

    dma_retain();

    play_start_pcm();
}

void pcm_play_dma_stop(void)
{
    dma_disable_channel(1);
    dma_size = 0;

    dma_release();

    CGU_PERI &= ~CGU_I2SOUT_APB_CLOCK_ENABLE;
    CGU_AUDIO &= ~(1<<11);
}

void pcm_play_dma_pause(bool pause)
{
    if(pause)
        dma_disable_channel(1);
    else
        play_start_pcm();
}

void pcm_play_dma_init(void)
{
    CGU_PERI |= CGU_I2SOUT_APB_CLOCK_ENABLE;

    I2SOUT_CONTROL = (1<<6)|(1<<3)  /* enable dma, stereo */;

    audiohw_preinit();
}

void pcm_postinit(void)
{
    audiohw_postinit();
}

void pcm_dma_apply_settings(void)
{
    unsigned long frequency = pcm_sampr;

    /* TODO : use a table ? */
    const int divider = ((AS3525_MCLK_FREQ/128) + (frequency/2)) / frequency;

    int cgu_audio = CGU_AUDIO;  /* read register */
    cgu_audio &= ~(3 << 0);     /* clear i2sout MCLK_SEL */
    cgu_audio |=  (AS3525_MCLK_SEL << 0);  /* set i2sout MCLK_SEL */
    cgu_audio &= ~(511 << 2);   /* clear i2sout divider */
    cgu_audio |= (divider - 1) << 2;  /* set new i2sout divider */
    CGU_AUDIO = cgu_audio;      /* write back register */
}

size_t pcm_get_bytes_waiting(void)
{
    return dma_size;
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    *count = dma_size >> 2;
    return (const void*)dma_start_addr;
}

#ifdef HAVE_PCM_DMA_ADDRESS
void * pcm_dma_addr(void *addr)
{
    if (addr != NULL)
        addr = UNCACHED_ADDR(addr);
    return addr;
}
#endif


/****************************************************************************
 ** Recording DMA transfer
 **/
#ifdef HAVE_RECORDING
#define I2SIN_RECORDING_MASK ( I2SIN_MASK_POER | I2SIN_MASK_PUER | \
            I2SIN_MASK_POHF | I2SIN_MASK_POAF | I2SIN_MASK_POF )

static int rec_locked = 0;
static unsigned int *rec_start_addr;
static size_t      rec_size;

void pcm_rec_lock(void)
{
    if(++rec_locked == 1) {
        int vic_state = disable_irq_save();
        VIC_INT_EN_CLEAR = INTERRUPT_I2SIN;
        I2SIN_MASK = 0;
        restore_irq( vic_state );
    }
}

void pcm_rec_unlock(void)
{
    if(--rec_locked == 0) {
        int vic_state = disable_irq_save();
        VIC_INT_ENABLE = INTERRUPT_I2SIN;
        I2SIN_MASK = I2SIN_RECORDING_MASK;
        restore_irq( vic_state );
    }
}


void pcm_rec_dma_record_more(void *start, size_t size)
{
    rec_start_addr = start;
    rec_size = size;
}


void pcm_rec_dma_stop(void)
{
    int vic_state = disable_irq_save();
    VIC_INT_EN_CLEAR = INTERRUPT_I2SIN;
    I2SIN_MASK = 0;
    restore_irq( vic_state );

    I2SOUT_CONTROL &= ~(1<<5); /* source = i2soutif fifo */
    CGU_AUDIO &= ~((1<<23)|(1<<11));
    CGU_PERI &= ~(CGU_I2SIN_APB_CLOCK_ENABLE|CGU_I2SOUT_APB_CLOCK_ENABLE);
}


void INT_I2SIN(void)
{
    register int status;
    register pcm_more_callback_type2 more_ready;

    status = I2SIN_STATUS;

    if ( status & ((1<<6)|(1<<0)) ) /* errors */
        panicf("i2sin error: 0x%x = %s %s", status,
            (status & (1<<6)) ? "push" : "",
            (status & (1<<0)) ? "pop" : ""
        );

    /* called at half full so it's safe to pull 16 FIFO reads in one chunk */
    if( rec_size >= 16*4 )
    {
        /* unrolled loop */
        *rec_start_addr++ = *I2SIN_DATA;
        *rec_start_addr++ = *I2SIN_DATA;
        *rec_start_addr++ = *I2SIN_DATA;
        *rec_start_addr++ = *I2SIN_DATA;

        *rec_start_addr++ = *I2SIN_DATA;
        *rec_start_addr++ = *I2SIN_DATA;
        *rec_start_addr++ = *I2SIN_DATA;
        *rec_start_addr++ = *I2SIN_DATA;

        *rec_start_addr++ = *I2SIN_DATA;
        *rec_start_addr++ = *I2SIN_DATA;
        *rec_start_addr++ = *I2SIN_DATA;
        *rec_start_addr++ = *I2SIN_DATA;

        *rec_start_addr++ = *I2SIN_DATA;
        *rec_start_addr++ = *I2SIN_DATA;
        *rec_start_addr++ = *I2SIN_DATA;
        *rec_start_addr++ = *I2SIN_DATA;

        rec_size -= 16*4; /* 16x4byte reads */
    }

    /* read out any odd samples left */
    while (((I2SIN_RAW_STATUS & (1<<5)) == 0) && rec_size)
    {
        /* 14 bits per sample = 1 32 bits word */
        *rec_start_addr++ = *I2SIN_DATA;
        rec_size -= 4;
    }

    I2SIN_CLEAR = status;

    if(!rec_size)
    {
        more_ready = pcm_callback_more_ready;
        if(!more_ready || more_ready(0) < 0)
        {
            /* Finished recording */
            pcm_rec_dma_stop();
            pcm_rec_dma_stopped_callback();
        }
    }
}


void pcm_rec_dma_start(void *addr, size_t size)
{
    rec_start_addr = addr;
    rec_size = size;

    CGU_PERI |= CGU_I2SIN_APB_CLOCK_ENABLE|CGU_I2SOUT_APB_CLOCK_ENABLE;
    CGU_AUDIO |= ((1<<23)|(1<<11));

    I2SOUT_CONTROL |= 1<<5; /* source = loopback from i2sin fifo */

    /* 14 bits samples, i2c clk src = I2SOUTIF, sdata src = AFE,
     * data valid at positive edge of SCLK */
    I2SIN_CONTROL = (1<<5) | (1<<2);

    unsigned long tmp;
    while ( ( I2SIN_RAW_STATUS & ( 1<<5 ) ) == 0 )
        tmp = *I2SIN_DATA; /* FLUSH FIFO */
    I2SIN_CLEAR = (1<<6)|(1<<0);    /* push error, pop error */
    I2SIN_MASK = I2SIN_RECORDING_MASK;

    VIC_INT_ENABLE = INTERRUPT_I2SIN;
}


void pcm_rec_dma_close(void)
{
    pcm_rec_dma_stop();
}


void pcm_rec_dma_init(void)
{
    unsigned long frequency = pcm_sampr;

    /* TODO : use a table ? */
    const int divider = ((AS3525_MCLK_FREQ/128) + (frequency/2)) / frequency;

    int cgu_audio = CGU_AUDIO;  /* read register */
    cgu_audio &= ~(3 << 12);    /* clear i2sin MCLK_SEL */
    cgu_audio |=  (AS3525_MCLK_SEL << 12);  /* set i2sin MCLK_SEL */
    cgu_audio &= ~(511 << 14);  /* clear i2sin divider */
    cgu_audio |= (divider - 1) << 14; /* set new i2sin divider */
    CGU_AUDIO = cgu_audio;      /* write back register */
}


const void * pcm_rec_dma_get_peak_buffer(void)
{
    return (const void*)rec_start_addr;
}

#endif /* HAVE_RECORDING */
