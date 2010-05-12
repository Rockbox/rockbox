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

static void* playback_address;
static inline void set_dma(const void *addr, size_t size)
{
    int burst_size;
    logf("%x %d %x", (unsigned int)addr, size, REG_AIC_SR);

    if(size % 16)
    {
        if(size % 4)
        {
            size /= 2;
            burst_size = DMAC_DCMD_DS_16BIT;
        }
        else
        {
            size /= 4;
            burst_size = DMAC_DCMD_DS_32BIT;
        }
    }
    else
    {
        size /= 16;
        burst_size = DMAC_DCMD_DS_16BYTE;
    }

    __dcache_writeback_all();
    REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) = DMAC_DCCSR_NDES;
    REG_DMAC_DSAR(DMA_AIC_TX_CHANNEL)  = PHYSADDR((unsigned long)addr);
    REG_DMAC_DTAR(DMA_AIC_TX_CHANNEL)  = PHYSADDR((unsigned long)AIC_DR);
    REG_DMAC_DTCR(DMA_AIC_TX_CHANNEL)  = size;
    REG_DMAC_DRSR(DMA_AIC_TX_CHANNEL)  = DMAC_DRSR_RS_AICOUT;
    REG_DMAC_DCMD(DMA_AIC_TX_CHANNEL)  = (DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | burst_size | DMAC_DCMD_DWDH_16 | DMAC_DCMD_TIE);

    playback_address = (void*)addr;
}

static inline void play_dma_callback(void)
{
    unsigned char *start = NULL;
    size_t size = 0;

    if(pcm_callback_for_more)
        pcm_callback_for_more(&start, &size);

    if(LIKELY(size > 0 && start))
    {
        set_dma(start, size);
        REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) |= DMAC_DCCSR_EN;
    }
    else
    {
        /* Error, callback missing or no more DMA to do */
        pcm_play_dma_stop();
        pcm_play_dma_stopped_callback();
    }
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
        play_dma_callback();
    }
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    dma_enable();

    set_dma(addr, size);

    __aic_enable_transmit_dma();
    __aic_enable_replay();

    REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) |= DMAC_DCCSR_EN;
}

void pcm_play_dma_stop(void)
{
    int flags = disable_irq_save();

    REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) = (REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) | DMAC_DCCSR_HLT) & ~DMAC_DCCSR_EN;

    dma_disable();

    __aic_disable_transmit_dma();
    __aic_disable_replay();

    restore_irq(flags);
}

static unsigned int play_lock = 0;
void pcm_play_lock(void)
{
    int flags = disable_irq_save();

    if (++play_lock == 1)
        __dmac_channel_disable_irq(DMA_AIC_TX_CHANNEL);

    restore_irq(flags);
}

void pcm_play_unlock(void)
{
    int flags = disable_irq_save();

    if (--play_lock == 0)
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

static int get_dma_count(void)
{
    int count = REG_DMAC_DTCR(DMA_AIC_TX_CHANNEL);
    switch(REG_DMAC_DCMD(DMA_AIC_TX_CHANNEL) & DMAC_DCMD_DS_MASK)
    {
        case DMAC_DCMD_DS_16BIT:
            count *= 2;
            break;
        case DMAC_DCMD_DS_32BIT:
            count *= 4;
            break;
        case DMAC_DCMD_DS_16BYTE:
            count *= 16;
            break;
    }

    return count;
}

size_t pcm_get_bytes_waiting(void)
{
    int bytes, flags = disable_irq_save();

    if(REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) & DMAC_DCCSR_EN)
        bytes = get_dma_count() & ~3;
    else
        bytes = 0;

    restore_irq(flags);

    return bytes;
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    int flags = disable_irq_save();

    const void* addr;
    if(REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) & DMAC_DCCSR_EN)
    {
        int bytes = get_dma_count();
        *count = bytes >> 2;
        addr = (const void*)((int)(playback_address + bytes + 2) & ~3);
    }
    else
    {
        *count = 0;
        addr = NULL;
    }

    restore_irq(flags);

    return addr;
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

const void * pcm_rec_dma_get_peak_buffer(void)
{
    return NULL;
}

void pcm_rec_dma_record_more(void *start, size_t size)
{
    (void) start;
    (void) size;
}
#endif
