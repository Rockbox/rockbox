/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: pcm-s5l8700.c 28600 2010-11-14 19:49:20Z Buschel $
 *
 * Copyright © 2011 Michael Sparmann
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
#include "s5l8702.h"
#include "panic.h"
#include "audiohw.h"
#include "pcm.h"
#include "pcm-internal.h"
#include "pcm_sampr.h"
#include "mmu-arm.h"
#include "pcm-target.h"
#include "dma-s5l8702.h"

/* DMA configuration */

/* 3 DMA tasks needed, one chunk task and two dblbuf tasks */
#define DMA_PLAY_TSKBUF_SZ  4   /* N tasks, MUST be pow2 */
#define DMA_PLAY_LLIBUF_SZ  4   /* N LLIs, MUST be pow2 */

static struct dmac_tsk dma_play_tskbuf[DMA_PLAY_TSKBUF_SZ];
static struct dmac_lli volatile \
            dma_play_llibuf[DMA_PLAY_LLIBUF_SZ] CACHEALIGN_ATTR;

static void dma_play_callback(void *data) ICODE_ATTR;

static struct dmac_ch dma_play_ch = {
    .dmac = &s5l8702_dmac0,
    .prio = DMAC_CH_PRIO(2),
    .cb_fn = dma_play_callback,

    .tskbuf = dma_play_tskbuf,
    .tskbuf_mask = DMA_PLAY_TSKBUF_SZ - 1,
    .queue_mode = QUEUE_LINK,

    .llibuf = dma_play_llibuf,
    .llibuf_mask = DMA_PLAY_LLIBUF_SZ - 1,
    .llibuf_bus = DMAC_MASTER_AHB1,
};

static struct dmac_ch_cfg dma_play_ch_cfg = {
    .srcperi = S5L8702_DMAC0_PERI_MEM,
    .dstperi = S5L8702_DMAC0_PERI_IIS0_TX,
    .sbsize  = DMACCxCONTROL_BSIZE_8,
    .dbsize  = DMACCxCONTROL_BSIZE_4,
    .swidth  = DMACCxCONTROL_WIDTH_16,
    .dwidth  = DMACCxCONTROL_WIDTH_16,
    .sbus    = DMAC_MASTER_AHB1,
    .dbus    = DMAC_MASTER_AHB1,
    .sinc    = DMACCxCONTROL_INC_ENABLE,
    .dinc    = DMACCxCONTROL_INC_DISABLE,
    .prot    = DMAC_PROT_CACH | DMAC_PROT_BUFF | DMAC_PROT_PRIV,
    /* align LLI transfers to L-R pairs (samples) */
    .lli_xfer_max_count = DMAC_LLI_MAX_COUNT & ~1,
};
#define LLI_MAX_BYTES       8188  /* lli_xfer_max_count << swidth */

/* Use all available LLIs for chunk */
/*#define CHUNK_MAX_BYTES     (LLI_MAX_BYTES * (DMA_PLAY_LLIBUF_SZ - 2))*/
#define CHUNK_MAX_BYTES     (LLI_MAX_BYTES * 1)
#define WATERMARK_BYTES     (PCM_WATERMARK * 4)

static volatile int locked = 0;
static unsigned char dblbuf[2][WATERMARK_BYTES] CACHEALIGN_ATTR;
static int active_dblbuf;
size_t pcm_remaining;

/* Mask the DMA interrupt */
void pcm_play_lock(void)
{
    if (locked++ == 0)
        dmac_ch_lock_int(&dma_play_ch);
}

/* Unmask the DMA interrupt if enabled */
void pcm_play_unlock(void)
{
    if (--locked == 0)
        dmac_ch_unlock_int(&dma_play_ch);
}

static inline void play_queue_dma(void *addr, size_t size, void *cb_data)
{
    commit_dcache_range(addr, size);
    dmac_ch_queue(&dma_play_ch, addr,
                (void*)S5L8702_DADDR_PERI_IIS0_TX, size, cb_data);
}

static void dma_play_callback(void *cb_data)
{
    if (!cb_data)
        return; /* dblbuf callback entered, nothing to do */

    const void *dataptr = cb_data;

    if (!pcm_remaining)
        if (!pcm_play_dma_complete_callback(
                     PCM_DMAST_OK, &dataptr, &pcm_remaining))
            return;

    uint32_t lastsize = MIN(WATERMARK_BYTES, pcm_remaining >> 1);
    pcm_remaining -= lastsize;
    uint32_t chunksize = MIN(CHUNK_MAX_BYTES, pcm_remaining);

    /* last chunk should be at least 2*WATERMARK_BYTES in size */
    if ((pcm_remaining > chunksize) &&
                (pcm_remaining < chunksize + WATERMARK_BYTES * 2))
        chunksize = pcm_remaining - WATERMARK_BYTES * 2;

    pcm_remaining -= chunksize;

    /* first part */
    play_queue_dma((void*)dataptr, chunksize,
                (void*)dataptr + chunksize + lastsize); /* cb_data */

    /* second part */
    memcpy(dblbuf[active_dblbuf], dataptr + chunksize, lastsize);
    play_queue_dma(dblbuf[active_dblbuf], lastsize, NULL);
    active_dblbuf ^= 1;

    pcm_play_dma_status_callback(PCM_DMAST_STARTED);
}

void pcm_play_dma_start(const void* addr, size_t size)
{
    pcm_play_dma_stop();

    pcm_remaining = size;
    I2STXCOM = 0xe;
    dma_play_callback((void*)addr);
}

void pcm_play_dma_stop(void)
{
    dmac_ch_stop(&dma_play_ch);
    I2STXCOM = 0xa;
}

/* pause playback by disabling LRCK */
void pcm_play_dma_pause(bool pause)
{
    if (pause) I2STXCOM |= 1;
    else  I2STXCOM &= ~1;
}

/* MCLK = 12MHz (MCLKDIV2=1), [CS42L55 DS, s4.8] */
#define MCLK_FREQ     12000000

/* set the configured PCM frequency */
void pcm_dma_apply_settings(void)
{
    static uint16_t last_clkcon3l = 0;
    uint16_t clkcon3l;
    int fsel;

    /* For unknown reasons, s5l8702 I2S controller does not synchronize
     * with CS42L55 at 32000 Hz. To fix it, the CODEC is configured with
     * a sample rate of 48000 Hz and MCLK is decreased 1/3 to 8 Mhz,
     * obtaining 32 KHz in LRCK controller input and 8 MHz in SCLK input.
     * OF uses this trick.
     */
    if (pcm_fsel == HW_FREQ_32) {
        fsel = HW_FREQ_48;
        clkcon3l = 0x3028;  /* PLL2 / 3 / 9 -> 8 MHz */
    }
    else {
        fsel = pcm_fsel;
        clkcon3l = 0;  /* OSC0 -> 12 MHz */
    }

    /* configure MCLK */
    /* TODO: maybe all CLKCON management should be moved to
       cscodec-ipod6g.c and system-s5l8702.c */
    if (last_clkcon3l != clkcon3l) {
        CLKCON3 = (CLKCON3 & ~0xffff) | 0x8000 | clkcon3l;
        udelay(100);
        CLKCON3 &= ~0x8000;  /* CLKCON3L on */
        last_clkcon3l = clkcon3l;
    }

    /* configure I2S clock ratio */
    I2SCLKDIV = MCLK_FREQ / hw_freq_sampr[fsel];
    /* select CS42L55 sample rate */
    audiohw_set_frequency(fsel);
}

void pcm_play_dma_init(void)
{
    PWRCON(0) &= ~(1 << 4);
    PWRCON(1) &= ~(1 << 7);

    dmac_ch_init(&dma_play_ch, &dma_play_ch_cfg);

    I2STXCON = 0xb100019;
    I2SCLKCON = 1;

    audiohw_preinit();
    pcm_dma_apply_settings();
}

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();
}

size_t pcm_get_bytes_waiting(void)
{
    size_t total_bytes;
    dmac_ch_get_info(&dma_play_ch, NULL, &total_bytes);
    return total_bytes;
}

const void* pcm_play_dma_get_peak_buffer(int *count)
{
    void *addr = dmac_ch_get_info(&dma_play_ch, count, NULL);
    *count >>= 2; /* bytes to samples */
    return addr; /* aligned to dest burst */
}

#ifdef HAVE_PCM_DMA_ADDRESS
void * pcm_dma_addr(void *addr)
{
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
