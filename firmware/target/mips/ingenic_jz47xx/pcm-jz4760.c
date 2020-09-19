/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Roman Stolyarov
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
#define LOGF_ENABLE

#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "audio.h"
#include "sound.h"
#include "pcm.h"
#include "pcm-internal.h"
#include "cpu.h"

//#undef HW_NEEDS_PCM_DOUBLE_BUFFER

#ifdef HW_NEEDS_PCM_DOUBLE_BUFFER
static volatile uint8_t read_idx = 0;
static volatile uint8_t write_idx = 0;
static volatile uint8_t last_cdoa = 0;

struct jz4760_dma_desc4 {
    uint32_t des0; // control
    uint32_t des1; // source addr
    uint32_t des2; // target addr
    uint32_t des3; // descriptor offset and xfer len
} __attribute__((packed));

struct jz4760_dma_desc4 txdesc[2] __attribute__((aligned(16))); // XXX put into uncached region?

#endif

/****************************************************************************
 ** Playback DMA transfer
 **/

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();

    /* Flush FIFO */
    __aic_flush_tfifo();
}

void pcm_play_dma_init(void)
{
#ifdef HW_NEEDS_PCM_DOUBLE_BUFFER
    /* Set up fixed portions of DMA descriptor chains */
    txdesc[0].des2 = PHYSADDR((unsigned long)AIC_DR);
    txdesc[0].des3 = (PHYSADDR((unsigned long)&txdesc[1]) << 20 & 0xFF000000);
    txdesc[1].des2 = PHYSADDR((unsigned long)AIC_DR);
    txdesc[1].des3 = (PHYSADDR((unsigned long)&txdesc[0]) << 20 & 0xFF000000);
#endif

    /* Make sure engine is idle */
    __dmac_channel_enable_clk(DMA_AIC_TX_CHANNEL);
    REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) = 0;
    __dmac_channel_disable_irq(DMA_AIC_TX_CHANNEL);
    __dmac_channel_disable_clk(DMA_AIC_TX_CHANNEL);

    /* Initialize default register values. */
    audiohw_init();

    system_enable_irq(DMA_IRQ(DMA_AIC_TX_CHANNEL));
}

void pcm_dma_apply_settings(void)
{
    audiohw_set_frequency(pcm_fsel);
}

static const void* playback_address;
static inline void set_dma(const void *addr, size_t size)
{
    int burst_size;
    commit_discard_dcache_range(addr, size);

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

#ifdef HW_NEEDS_PCM_DOUBLE_BUFFER
    logf("+%d %x %d %x", write_idx, (unsigned int)addr, size, REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL));
    logf("< %x %x %x %x", txdesc[write_idx].des0, txdesc[write_idx].des1, txdesc[write_idx].des2, txdesc[write_idx].des3);
    txdesc[write_idx].des0 = DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | burst_size | DMAC_DCMD_DWDH_16 | DMAC_DCMD_TIE | DMAC_DCMD_LINK;
    txdesc[write_idx].des1 = PHYSADDR((unsigned long)addr);
    txdesc[write_idx].des3 = (txdesc[write_idx].des3 & 0xff000000) | (size & 0xffffff);
    logf("> %x %x %x %x", txdesc[write_idx].des0, txdesc[write_idx].des1, txdesc[write_idx].des2, txdesc[write_idx].des3);
    commit_discard_dcache_range(&txdesc[write_idx], sizeof(txdesc[write_idx]));
    write_idx ^= 1;

    /* Start engine if it's not already active */
    if (!(REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) & DMAC_DCCSR_EN)) {
        last_cdoa = 0;
        REG_DMAC_DDA(DMA_AIC_TX_CHANNEL) = PHYSADDR((unsigned long)&txdesc[0]);
        REG_DMAC_DMADBSR(0) = 1 << DMA_AIC_TX_CHANNEL;
        REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) = DMAC_DCCSR_EN | BDMAC_DCCSR_DES4;
        logf("K %x %x", REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL), &txdesc[0]);
    }
#else
    REG_DMAC_DSAR(DMA_AIC_TX_CHANNEL)  = PHYSADDR((unsigned long)addr);
    REG_DMAC_DTAR(DMA_AIC_TX_CHANNEL)  = PHYSADDR((unsigned long)AIC_DR);
    REG_DMAC_DTCR(DMA_AIC_TX_CHANNEL)  = size;
    REG_DMAC_DCMD(DMA_AIC_TX_CHANNEL)  = (DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | burst_size | DMAC_DCMD_DWDH_16 | DMAC_DCMD_TIE);
    REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) = DMAC_DCCSR_NDES | DMAC_DCCSR_EN;
#endif
}

static inline void play_dma_callback(void)
{
    const void *start;
    size_t size;

#ifdef HW_NEEDS_PCM_DOUBLE_BUFFER
    if (REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) & DMAC_DCCSR_EN) {
        logf("-%d", read_idx);
        read_idx ^= 1;
    }
    if (read_idx == write_idx) {
        logf("DBLBUF Underrun");
        REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) = 0;
        write_idx = read_idx = 0;
    }
#endif

    if (pcm_play_dma_complete_callback(PCM_DMAST_OK, &start, &size))
    {
        set_dma(start, size);
	playback_address = start;
        pcm_play_dma_status_callback(PCM_DMAST_STARTED);

#ifdef HW_NEEDS_PCM_DOUBLE_BUFFER
	/* Make sure HW always has two buffers queued up */
        if ((read_idx ^ write_idx) && pcm_play_dma_complete_callback(PCM_DMAST_OK, &start, &size)) {
            set_dma(start, size);
        }
#endif
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
        logf("PCM DMA TT");
        /* Called upon *completion* of descriptor chain */
        REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) &= ~DMAC_DCCSR_TT;
#ifndef HW_NEEDS_PCM_DOUBLE_BUFFER
        play_dma_callback();
#endif
    }

#ifdef HW_NEEDS_PCM_DOUBLE_BUFFER
    /* Check to see if we moved to a different descriptor since the last interrupt. */
    int cdoa = REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) >> DMAC_DCCSR_CDOA_BIT & 0xff;
    // XXX what if we underrun..?
    if (cdoa != last_cdoa) {
        logf("CDOA %x %x", cdoa, last_cdoa);
        last_cdoa = cdoa;
        play_dma_callback();
    }
#endif
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    pcm_play_dma_stop();
    logf("DMA start");

    __dmac_channel_enable_clk(DMA_AIC_TX_CHANNEL);

    /* Only need to do this init once...  */
    REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) = 0;
    REG_DMAC_DRSR(DMA_AIC_TX_CHANNEL)  = DMAC_DRSR_RS_AICOUT;
    REG_DMAC_DMACR(0) = DMAC_DMACR_FAIC | DMAC_DMACR_DMAE;  /* Fast DMA mode for AIC */

    set_dma(addr, size);

#ifdef HW_NEEDS_PCM_DOUBLE_BUFFER
    pcm_play_dma_status_callback(PCM_DMAST_STARTED);
    /* Ensure second HW buffer is fulled */
    if (pcm_play_dma_complete_callback(PCM_DMAST_OK, &addr, &size)) {
        set_dma(addr, size);
    }
#endif
    __aic_enable_replay();

    __dmac_channel_enable_irq(DMA_AIC_TX_CHANNEL);
}

void pcm_play_dma_stop(void)
{
    int flags = disable_irq_save();
    logf("DMA stop");

    REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) = (REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) | DMAC_DCCSR_HLT) & ~DMAC_DCCSR_EN;

    __dmac_channel_disable_irq(DMA_AIC_TX_CHANNEL);
    __dmac_channel_disable_clk(DMA_AIC_TX_CHANNEL);

    __aic_disable_replay();

    playback_address = NULL;
#ifdef HW_NEEDS_PCM_DOUBLE_BUFFER
    read_idx = write_idx = 0;
#endif

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

    if (pause)
        REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) &= ~DMAC_DCCSR_EN;
    else
        REG_DMAC_DCCSR(DMA_AIC_TX_CHANNEL) |= DMAC_DCCSR_EN;

    restore_irq(flags);
}

static int get_dma_count(void)
{
    int count;

    if (playback_address == NULL)
        return 0;

#ifdef HW_NEEDS_PCM_DOUBLE_BUFFER
    // XXX discard dcache?
    count = txdesc[read_idx].des3 & 0xffffff;
    if (read_idx == write_idx)
	count += txdesc[read_idx ^ 1].des3 & 0xffffffff;
#else
    count = REG_DMAC_DTCR(DMA_AIC_TX_CHANNEL);
#endif
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
