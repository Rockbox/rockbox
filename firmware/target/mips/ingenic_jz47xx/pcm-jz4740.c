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
#include "pcm-internal.h"
#include "jz4740.h"


/****************************************************************************
 ** Playback DMA transfer
 **/

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();

    /* playback sample: 16 bits burst: 16 bytes */
    __i2s_set_iss_sample_size(16);
    __i2s_set_oss_sample_size(16);
    __i2s_set_transmit_trigger(10);
    __i2s_set_receive_trigger(1);

    /* Flush FIFO */
    __aic_flush_fifo();
}

void pcm_play_dma_init(void)
{
    /* TODO */

    system_enable_irq(DMA_IRQ(DMA_AIC_TX_CHANNEL));

    /* Initialize default register values. */
    audiohw_init();
}

void pcm_dma_apply_settings(void)
{
    /* TODO */
    audiohw_set_frequency(pcm_sampr);
}

static const void *playback_address;
static inline void set_dma(const void *addr, unsigned long frames)
{
    int burst_size;
    logf("%p %lu %x", addr, frames, REG_AIC_SR);

    if(frames % 4)
    {
        burst_size = DMAC_DCMD_DS_32BIT;
    }
    else
    {
        frames /= 4;
        burst_size = DMAC_DCMD_DS_16BYTE;
    }

    __dcache_writeback_all();
    REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) = DMAC_DCCSR_NDES;
    REG_DMAC_DSAR(DMA_AIC_TX_CHANNEL)  = PHYSADDR((unsigned long)addr);
    REG_DMAC_DTAR(DMA_AIC_TX_CHANNEL)  = PHYSADDR((unsigned long)AIC_DR);
    REG_DMAC_DTCR(DMA_AIC_TX_CHANNEL)  = frames;
    REG_DMAC_DRSR(DMA_AIC_TX_CHANNEL)  = DMAC_DRSR_RS_AICOUT;
    REG_DMAC_DCMD(DMA_AIC_TX_CHANNEL)  = (DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | burst_size | DMAC_DCMD_DWDH_16 | DMAC_DCMD_TIE);

    playback_address = addr;
}

void DMA_CALLBACK(DMA_AIC_TX_CHANNEL)(void) __attribute__ ((section(".icode")));
void DMA_CALLBACK(DMA_AIC_TX_CHANNEL)(void)
{
    if (REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) & DMAC_DCCSR_AR)
    {
        logf("PCM DMA address error");
        REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) &= ~DMAC_DCCSR_AR;
    }

    if (REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) & DMAC_DCCSR_HLT)
    {
        logf("PCM DMA halt");
        REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) &= ~DMAC_DCCSR_HLT;
    }

    if (REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) & DMAC_DCCSR_TT)
    {
        REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) &= ~DMAC_DCCSR_TT;
        pcm_play_dma_complete_callback(0);
    }
}

void pcm_play_dma_send_frames(const void *addr, unsigned long frames)
{
    set_dma(addr, frames);
    REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) |= DMAC_DCCSR_EN;
}

void pcm_play_dma_prepare(void)
{
    dma_enable();
    __aic_enable_transmit_dma();
    __aic_enable_replay();
}

void pcm_play_dma_stop(void)
{
    int flags = disable_irq_save();

    REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) = (REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) | DMAC_DCCSR_HLT) & ~DMAC_DCCSR_EN;

    dma_disable();

    __aic_disable_transmit_dma();
    __aic_disable_replay();

    playback_address = NULL;

    restore_irq(flags);
}

void pcm_play_dma_lock(void)
{
    int flags = disable_irq_save();
    __dmac_channel_disable_irq(DMA_AIC_TX_CHANNEL);
    restore_irq(flags);
}

void pcm_play_dma_unlock(void)
{
    int flags = disable_irq_save();
    __dmac_channel_enable_irq(DMA_AIC_TX_CHANNEL);
    restore_irq(flags);
}

void pcm_play_dma_pause(bool pause)
{
    int flags = disable_irq_save();

    if(pause)
        REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) &= ~DMAC_DCCSR_EN;
    else
        REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) |= DMAC_DCCSR_EN;

    restore_irq(flags);
}

static unsigned long get_dma_count(void)
{
    if (!(REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) & DMAC_DCCSR_EN))
        return 0;

    unsigned long count = REG_DMAC_DTCR(DMA_AIC_TX_CHANNEL);
    switch(REG_DMAC_DCMD(DMA_AIC_TX_CHANNEL) & DMAC_DCMD_DS_MASK)
    {
        case DMAC_DCMD_DS_32BIT:
            break;
        case DMAC_DCMD_DS_16BYTE:
            count *= 4;
            break;
    }

    return count;
}

unsigned long pcm_get_frames_waiting(void)
{
    int flags = disable_irq_save();
    unsigned long frames = get_dma_count();
    restore_irq(flags);

    return frames;
}

const void * pcm_play_dma_get_peak_buffer(unsigned long *frames_rem)
{
    int flags = disable_irq_save();
    unsigned long addr = REG_DMAC_DSAR(DMA_AIC_TX_CHANNEL);
    unsigned long frames = get_dma_count();
    restore_irq(flags);

    *frames_rem = frames;
    return (void *)(addr + frames*4);
}

void audiohw_close(void)
{
    /* TODO: prevent pop */
}

#ifdef HAVE_RECORDING
/* TODO */
void pcm_rec_dma_init(void)
{
}

void pcm_rec_dma_close(void)
{
}

void pcm_rec_dma_capture_frames(void *addr, unsigned long frames)
{
    (void) addr;
    (void) frames;
}

void pcm_rec_dma_prepare(void)
{
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

const void * pcm_rec_dma_get_peak_buffer(unsigned long *frames_avail)
{
    *frames_avail = 0;
    return NULL;
}
#endif
