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

#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "audio.h"
#include "sound.h"
#include "pcm.h"
#include "jz4740.h"


/****************************************************************************
 ** Playback DMA transfer
 **/

void pcm_postinit(void)
{
    audiohw_postinit();
    
    /* playback sample:16 bits, burst:16 bytes */
    __i2s_set_transmit_trigger(4);
    __i2s_set_oss_sample_size(16);
    
    pcm_apply_settings();
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    /* TODO */
    *count = 0;
    return NULL;
}

void pcm_play_dma_init(void)
{
    /* TODO */

    /* Initialize default register values. */
    audiohw_init();
}

void pcm_set_frequency(unsigned int frequency)
{
    (void) frequency;
    /* TODO */
    
    /*
    __i2s_set_oss_sample_size(frequency);
    i2s_codec_set_samplerate(frequency);
    */
}

static void play_start_pcm(void)
{
    __i2s_enable_transmit_dma();
    __i2s_enable_replay();
    
    REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) |= DMAC_DCCSR_EN;
}

static void play_stop_pcm(void)
{
    REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) = (REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) | DMAC_DCCSR_HLT) & ~DMAC_DCCSR_EN;
    
    __i2s_disable_transmit_dma();
    __i2s_disable_replay();
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) = DMAC_DCCSR_NDES;
    REG_DMAC_DSAR(DMA_AIC_TX_CHANNEL)  = PHYSADDR((unsigned long)addr);
    REG_DMAC_DTAR(DMA_AIC_TX_CHANNEL)  = PHYSADDR((unsigned long)AIC_DR);
    REG_DMAC_DTCR(DMA_AIC_TX_CHANNEL)  = size;
    REG_DMAC_DRSR(DMA_AIC_TX_CHANNEL)  = DMAC_DRSR_RS_AICOUT;
    REG_DMAC_DCMD(DMA_AIC_TX_CHANNEL)  = ( DMAC_DCMD_SAI| DMAC_DCMD_SWDH_32 | DMAC_DCMD_DS_32BIT | DMAC_DCMD_DWDH_32
                                         | DMAC_DCMD_TIE);

    play_start_pcm();
}

void DMA_CALLBACK(DMA_AIC_TX_CHANNEL)(void)
{
    if( REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) & DMAC_DCCSR_TT )
        __aic_disable_transmit_dma();
}

size_t pcm_get_bytes_waiting(void)
{
    return REG_DMAC_DTCR(DMA_AIC_TX_CHANNEL);
}

void pcm_play_dma_stop(void)
{
    play_stop_pcm();
    
    /* TODO */
}

void pcm_play_lock(void)
{
    /* TODO */
}

void pcm_play_unlock(void)
{
   /* TODO */
}

void pcm_play_dma_pause(bool pause)
{
    if(pause)
        play_stop_pcm();
    else
        play_start_pcm();
}

#ifdef HAVE_RECORDING
/* TODO */
void pcm_rec_dma_init(void)
{
}

void pcm_rec_dma_close(void)
{
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    (void) addr;
    (void) size;
}

void pcm_rec_dma_stop(void)
{
}

void pcm_rec_lock(void)
{
}

void pcm_rec_unlock(void)
{
}

const void * pcm_rec_dma_get_peak_buffer(int *count)
{
    *count = 0;
    return NULL;
}

void pcm_record_more(void *start, size_t size)
{
    (void) start;
    (void) size;
}
#endif
