/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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
        VIC_INT_EN_CLEAR |= INTERRUPT_DMAC;
}

/* Unmask the DMA interrupt if enabled */
void pcm_play_unlock(void)
{
    if(--locked == 0)
        VIC_INT_ENABLE |= INTERRUPT_DMAC;
}

static void play_start_pcm(void)
{
    const unsigned char* addr = dma_start_addr;
    size_t size = dma_size;
    if(size > MAX_TRANSFER)
        size = MAX_TRANSFER;

    if((unsigned int)dma_start_addr & 3)
        panicf("unaligned pointer!");

    dma_size -= size;
    dma_start_addr += size;

    CGU_PERI |= CGU_I2SOUT_APB_CLOCK_ENABLE;
    CGU_AUDIO |= (1<<11);

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
        pcm_play_dma_stop();
    else
        play_start_pcm();
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    dma_size = size;
    dma_start_addr = (unsigned char*)addr;

    play_start_pcm();
}

void pcm_play_dma_stop(void)
{
    dma_disable_channel(1);
    dma_size = 0;

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

    /* clock source PLLA, minimal frequency */
    CGU_AUDIO |= (511<<2) | (1<<0);

    I2SOUT_CONTROL |= (1<<6) ;  /* enable dma */
    I2SOUT_CONTROL |= (1<<3) ;  /* stereo */
    I2SOUT_CONTROL &= ~(1<<2);  /* 16 bit samples */

    audiohw_preinit();
}

void pcm_postinit(void)
{
    audiohw_postinit();
    pcm_apply_settings();
}

void pcm_set_frequency(unsigned int frequency)
{
    const int divider = (((AS3525_PLLA_FREQ/128) + (frequency/2)) / frequency) - 1;
    if(divider < 0 || divider > 511)
        panicf("unsupported frequency %d", frequency);

    CGU_AUDIO &= ~(((511 ^ divider) << 2) /* I2SOUT */
            /*| ((511 ^ divider) << 14) */ /* I2SIN */
            );

    pcm_curr_sampr = frequency;
}

void pcm_apply_settings(void)
{
    pcm_set_frequency(HW_SAMPR_DEFAULT);
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

void pcm_record_more(void *start, size_t size)
{
    (void)start;
    (void)size;
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


const void * pcm_rec_dma_get_peak_buffer(int *count)
{
    (void)count;
}

#endif /* HAVE_RECORDING */
