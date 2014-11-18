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

static volatile int locked = 0;
static const int zerosample = 0;
static unsigned char dblbuf[2][PCM_WATERMARK * 4];
static int active_dblbuf;
struct dma_lli pcm_lli[PCM_LLICOUNT] __attribute__((aligned(16)));
static struct dma_lli* lastlli;
static const void* dataptr;
size_t pcm_remaining;
size_t pcm_chunksize;

/* Mask the DMA interrupt */
void pcm_play_lock(void)
{
    if (locked++ == 0) {
        //TODO: Urgh, I don't like that at all...
        VIC0INTENCLEAR = 1 << IRQ_DMAC0;
    }
}

/* Unmask the DMA interrupt if enabled */
void pcm_play_unlock(void)
{
    if (--locked == 0) {
        VIC0INTENABLE = 1 << IRQ_DMAC0;
    }
}

void INT_DMAC0C0(void) ICODE_ATTR;
void INT_DMAC0C0(void)
{
    DMAC0INTTCCLR = 1;
    if (!pcm_remaining)
    {
        pcm_play_dma_complete_callback(PCM_DMAST_OK, &dataptr, &pcm_remaining);
        pcm_chunksize = pcm_remaining;
    }
    if (!pcm_remaining)
    {
        pcm_lli->nextlli = NULL;
        pcm_lli->control = 0x7524a000;
        commit_dcache();
        return;
    }
    uint32_t lastsize = MIN(PCM_WATERMARK * 4, pcm_remaining / 2 + 1) & ~1;
    pcm_remaining -= lastsize;
    if (pcm_remaining) lastlli = &pcm_lli[ARRAYLEN(pcm_lli) - 1];
    else lastlli = pcm_lli;
    uint32_t chunksize = MIN(PCM_CHUNKSIZE * 4 - lastsize, pcm_remaining);
    if (pcm_remaining > chunksize && chunksize > pcm_remaining - PCM_WATERMARK * 8)
        chunksize = pcm_remaining - PCM_WATERMARK * 8;
    pcm_remaining -= chunksize;
    bool last = !chunksize;
    int i = 0;
    while (chunksize)
    {
        uint32_t thislli = MIN(PCM_LLIMAX * 4, chunksize);
        chunksize -= thislli;
        pcm_lli[i].srcaddr = (void*)dataptr;
        pcm_lli[i].dstaddr = (void*)((int)&I2STXDB0);
        pcm_lli[i].nextlli = chunksize ? &pcm_lli[i + 1] : lastlli;
        pcm_lli[i].control = (chunksize ? 0x7524a000 : 0xf524a000) | (thislli / 2);
        dataptr += thislli;
        i++;
    }
    if (!pcm_remaining)
    {
        memcpy(dblbuf[active_dblbuf], dataptr, lastsize);
        lastlli->srcaddr = dblbuf[active_dblbuf];
        active_dblbuf ^= 1;
    }
    else lastlli->srcaddr = dataptr;
    lastlli->dstaddr = (void*)((int)&I2STXDB0);
    lastlli->nextlli = last ? NULL : pcm_lli;
    lastlli->control = (last ? 0xf524a000 : 0x7524a000) | (lastsize / 2);
    dataptr += lastsize;
    commit_dcache();
    if (!(DMAC0C0CONFIG & 1) && (pcm_lli[0].control & 0xfff))
    {
        DMAC0C0LLI = pcm_lli[0];
        DMAC0C0CONFIG = 0x8a81;
    }
    else DMAC0C0NEXTLLI = pcm_lli;

    pcm_play_dma_status_callback(PCM_DMAST_STARTED);
}

void pcm_play_dma_start(const void* addr, size_t size)
{
    dataptr = addr;
    pcm_remaining = size;
    I2STXCOM = 0xe;
    INT_DMAC0C0();
}

void pcm_play_dma_stop(void)
{
    DMAC0C0CONFIG = 0x8a80;
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
    I2STXCON = 0xb100019;
    I2SCLKCON = 1;
    VIC0INTENABLE = 1 << IRQ_DMAC0;

    audiohw_preinit();
    pcm_dma_apply_settings();
}

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();
}

size_t pcm_get_bytes_waiting(void)
{
    int bytes = pcm_remaining;
    const struct dma_lli* lli = (const struct dma_lli*)((int)&DMAC0C0LLI);
    while (lli)
    {
        bytes += (lli->control & 0xfff) * 2;
        if (lli == lastlli) break;
        lli = lli->nextlli;
    }
    return bytes;
}

const void* pcm_play_dma_get_peak_buffer(int *count)
{
    *count = (DMAC0C0LLI.control & 0xfff) * 2;
    return (void*)(((uint32_t)DMAC0C0LLI.srcaddr) & ~3);
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
