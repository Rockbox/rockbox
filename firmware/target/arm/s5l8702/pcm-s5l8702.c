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

static unsigned char dblbuf[2][WATERMARK_BYTES] CACHEALIGN_ATTR;
static int active_dblbuf;
static int pcm_running = false;
size_t pcm_remaining;

/* Mask the DMA interrupt */
void pcm_play_dma_lock(void)
{
    dmac_ch_lock_int(&dma_play_ch);
}

/* Unmask the DMA interrupt if enabled */
void pcm_play_dma_unlock(void)
{
    dmac_ch_unlock_int(&dma_play_ch);
}

static inline void play_queue_dma(void *addr, size_t size, void *cb_data)
{
    commit_dcache_range(addr, size);
    dmac_ch_queue(&dma_play_ch, addr,
                (void*)S5L8702_DADDR_PERI_IIS0_TX, size, cb_data);
}

static void dma_play_transfer(const void *dataptr)
{
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
}

static void dma_play_callback(void *cb_data)
{
    if (!cb_data)
        return; /* dblbuf callback entered, nothing to do */

    if (pcm_remaining)
        dma_play_transfer(cb_data); /* continue with current */
    else
        pcm_play_dma_complete_callback(0);   /* possibly start new buffer */
}

void pcm_play_dma_send_frames(const void* addr, unsigned long frames)
{
    pcm_remaining = frames * 4;

    if (!pcm_running)
    {
        /* start from stopped state */
        pcm_running = true;
        I2STXCOM = 0xe;
    }

    dma_play_transfer(addr);
}

void pcm_play_dma_prepare(void)
{
    if (pcm_running)
        pcm_play_dma_stop();
}

void pcm_play_dma_stop(void)
{
    dmac_ch_stop(&dma_play_ch);
    I2STXCOM = 0xa;
    pcm_running = false;
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
void pcm_dma_apply_settings(const struct pcm_hw_settings *settings)
{
    static uint16_t last_clkcon3l = 0;
    uint16_t clkcon3l;
    int fsel = settings->fsel;

    /* For unknown reasons, s5l8702 I2S controller does not synchronize
     * with CS42L55 at 32000 Hz. To fix it, the CODEC is configured with
     * a sample rate of 48000 Hz and MCLK is decreased 1/3 to 8 Mhz,
     * obtaining 32 KHz in LRCK controller input and 8 MHz in SCLK input.
     * OF uses this trick.
     */
    if (fsel == HW_FREQ_32) {
        fsel = HW_FREQ_48;
        clkcon3l = 0x3028;  /* PLL2 / 3 / 9 -> 8 MHz */
    }
    else {
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

void pcm_dma_init(const struct pcm_hw_settings *settings)
{
    PWRCON(0) &= ~(1 << 4);
    PWRCON(1) &= ~(1 << 7);

    dmac_ch_init(&dma_play_ch, &dma_play_ch_cfg);

    I2STXCON = 0xb100019;
    I2SCLKCON = 1;

    audiohw_codec_init();
    pcm_dma_apply_settings();
}

unsigned long pcm_get_frames_waiting(void)
{
    size_t total_bytes;
    dmac_ch_get_info(&dma_play_ch, NULL, &total_bytes);
    return total_bytes / 4;
}

const void* pcm_play_dma_get_peak_buffer(unsigned long *frames_rem)
{
    size_t total_bytes;
    void *addr = dmac_ch_get_info(&dma_play_ch, &total_bytes, NULL);
    *frames_rem = total_bytes / 4; /* bytes to frames */
    return addr; /* aligned to dest burst */
}

/****************************************************************************
 ** Recording DMA transfer
 **/
#ifdef HAVE_RECORDING
static void *rec_dma_addr;
static size_t rec_dma_size;
static bool pcm_rec_initialized = false;
static int completed_task;

/* ahead capture buffer */
#define PCM_AHEADBUF_SAMPLES    128
#define AHEADBUF_SZ             (PCM_AHEADBUF_SAMPLES * 4)
static unsigned char ahead_buf[AHEADBUF_SZ] CACHEALIGN_ATTR;

/* DMA configuration */
static void dma_rec_callback(void *cb_data) ICODE_ATTR;

enum {  /* cb_data */
    TASK_AHEADBUF,
    TASK_RECBUF
};

#define DMA_REC_TSKBUF_SZ   2   /* N tasks, MUST be pow2 */
#define DMA_REC_LLIBUF_SZ   8   /* N LLIs, MUST be pow2 */
static struct dmac_tsk dma_rec_tskbuf[DMA_REC_TSKBUF_SZ];
static struct dmac_lli volatile \
            dma_rec_llibuf[DMA_REC_LLIBUF_SZ] CACHEALIGN_ATTR;

static struct dmac_ch dma_rec_ch = {
    .dmac = &s5l8702_dmac0,
    .prio = DMAC_CH_PRIO(1),
    .cb_fn = dma_rec_callback,

    .llibuf = dma_rec_llibuf,
    .llibuf_mask = DMA_REC_LLIBUF_SZ - 1,
    .llibuf_bus = DMAC_MASTER_AHB1,

    .tskbuf = dma_rec_tskbuf,
    .tskbuf_mask = DMA_REC_TSKBUF_SZ - 1,
    .queue_mode = QUEUE_LINK,
};

static struct dmac_ch_cfg dma_rec_ch_cfg = {
    .srcperi = S5L8702_DMAC0_PERI_IIS0_RX,
    .dstperi = S5L8702_DMAC0_PERI_MEM,
    .sbsize  = DMACCxCONTROL_BSIZE_4,
    .dbsize  = DMACCxCONTROL_BSIZE_4,
    .swidth  = DMACCxCONTROL_WIDTH_16,
    .dwidth  = DMACCxCONTROL_WIDTH_16,
    .sbus    = DMAC_MASTER_AHB1,
    .dbus    = DMAC_MASTER_AHB1,
    .sinc    = DMACCxCONTROL_INC_DISABLE,
    .dinc    = DMACCxCONTROL_INC_ENABLE,
    .prot    = DMAC_PROT_CACH | DMAC_PROT_BUFF | DMAC_PROT_PRIV,
    /* align LLI transfers to L-R pairs (samples) */
    .lli_xfer_max_count = DMAC_LLI_MAX_COUNT & ~1,
};

/* maximum and minimum supported block sizes in bytes */
#define MIN_SIZE ((size_t) (AHEADBUF_SZ * 2))
#define MAX_SIZE ((size_t) (AHEADBUF_SZ + ((DMA_REC_LLIBUF_SZ - 1) * \
             (dma_rec_ch_cfg.lli_xfer_max_count << dma_rec_ch_cfg.swidth))))

#if 0
#define SIZE_PANIC(sz) { \
    if (((sz) < MIN_SIZE) || ((sz) > MAX_SIZE)) \
        panicf("pcm record: unsupported size: %d", (sz)); \
}
#else
#define SIZE_PANIC(sz) {}
#endif


static void rec_dmac_ch_queue(void *addr, size_t size, int cb_data)
{
    discard_dcache_range(addr, size);
    dmac_ch_queue(&dma_rec_ch, (void*)S5L8702_DADDR_PERI_IIS0_RX,
                                            addr, size, (void *)cb_data);
}

static void dma_rec_callback(void *cb_data)
{
    completed_task = (int)cb_data;

    if (completed_task == TASK_AHEADBUF)
    {
        /* safety check */
        if (rec_dma_addr == NULL)
            return; /* capture finished */

        /* move ahead buffer to record buffer and queue
           next capture-ahead task */
        memcpy(rec_dma_addr, ahead_buf, AHEADBUF_SZ);
        rec_dmac_ch_queue(ahead_buf, AHEADBUF_SZ, TASK_AHEADBUF);
    }
    else /* TASK_RECBUF */
    {
        pcm_rec_dma_complete_callback(0); /* possibly start new buffer */
    }
}

void pcm_rec_dma_capture_frames(void *addr, unsigned long frames)
{
    size_t size = frames * 4;

    SIZE_PANIC(size);

    rec_dma_addr = addr;
    rec_dma_size = size;

    if (completed_task == -2)
    {
        /* launch first DMA transfer to capture into ahead buffer,
           link the second task to capture into record buffer */
        rec_dmac_ch_queue(ahead_buf, AHEADBUF_SZ, TASK_AHEADBUF);
        I2SRXCOM = 0x6; /* start Rx I2S */
        completed_task = -1;
    }

    rec_dmac_ch_queue(addr + AHEADBUF_SZ, size - AHEADBUF_SZ, TASK_RECBUF);
}

void pcm_rec_dma_lock(void)
{
    if (pcm_rec_initialized)
        dmac_ch_lock_int(&dma_rec_ch);
}

void pcm_rec_dma_unlock(void)
{
    if (pcm_rec_initialized)
        dmac_ch_unlock_int(&dma_rec_ch);
}

void pcm_rec_dma_stop(void)
{
    if (!pcm_rec_initialized)
        return;

    dmac_ch_stop(&dma_rec_ch);

    I2SRXCOM = 0x2; /* stop Rx I2S */
    completed_task = -1;
}

void pcm_rec_dma_prepare(void)
{
    pcm_rec_dma_stop();
    completed_task = -2;
}

void pcm_rec_dma_close(void)
{
    pcm_rec_dma_stop();
}

void pcm_rec_dma_init(void)
{
    if (pcm_rec_initialized)
        return;

    PWRCON(0) &= ~(1 << 4);
    PWRCON(1) &= ~(1 << 7);

    dmac_ch_init(&dma_rec_ch, &dma_rec_ch_cfg);

    /* synchronize lock status */
    dmac_ch_lock_int(&dma_rec_ch);

    I2SRXCON = 0x1000;
    I2SCLKCON = 1;

    pcm_rec_initialized = true;
}

const void * pcm_rec_dma_get_peak_buffer(unsigned long *frames_avail)
{
    void *addr, *dmaaddr;

    pcm_rec_lock();

    if (completed_task == TASK_AHEADBUF) {
        /* At this moment, interrupt for TASK_RECBUF is waiting to
           be handled. TASK_RECBUF is already finished and HW is
           transfering next TASK_AHEADBUF. Return whole block. */
        dmaaddr = dmac_ch_get_info(&dma_rec_ch, NULL, NULL);
        addr = rec_dma_addr;
        *frames_avail = (dmaaddr - addr) / 4;
    }
    else {
        /* Ahead buffer not yet captured _and_ moved to
           record buffer. Return nothing. */
        *frames_avail = 0;
        addr = NULL;
    }

    pcm_rec_unlock();

    return addr;
}
#endif /* HAVE_RECORDING */
