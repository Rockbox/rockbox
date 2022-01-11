/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021-2022 Aidan MacDonald
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
#include "audio.h"
#include "audiohw.h"
#include "pcm.h"
#include "pcm-internal.h"
#include "panic.h"
#include "dma-x1000.h"
#include "irq-x1000.h"
#include "x1000/aic.h"
#include "x1000/cpm.h"

#define AIC_STATE_STOPPED   0x00
#define AIC_STATE_PLAYING   0x01
#define AIC_STATE_RECORDING 0x02

volatile unsigned aic_tx_underruns = 0;

static int aic_state = AIC_STATE_STOPPED;

static int play_lock = 0;
static volatile int play_dma_pending_event = DMA_EVENT_NONE;
static dma_desc play_dma_desc;
static void pcm_play_dma_int_cb(int event);

#ifdef HAVE_RECORDING
volatile unsigned aic_rx_overruns = 0;
static int rec_lock = 0;
static volatile int rec_dma_pending_event = DMA_EVENT_NONE;
static dma_desc rec_dma_desc;

static void pcm_rec_dma_int_cb(int event);
#endif

void pcm_play_dma_init(void)
{
    /* Ungate clock */
    jz_writef(CPM_CLKGR, AIC(0));

    /* Configure AIC with some sane defaults */
    jz_writef(AIC_CFG, RST(1));
    jz_writef(AIC_I2SCR, STPBK(1));
    jz_writef(AIC_CFG, MSB(0), LSMP(0), ICDC(0), AUSEL(1), BCKD(0), SYNCD(0));
    jz_writef(AIC_CCR, ENDSW(0), ASVTSU(0));
    jz_writef(AIC_I2SCR, RFIRST(0), ESCLK(0), AMSL(0));
    jz_write(AIC_SPENA, 0);

    /* Let the target initialize its hardware and setup the AIC */
    audiohw_init();

#if (PCM_NATIVE_BITDEPTH > 16)
    /* Program audio format (stereo, 24 bit samples) */
    jz_writef(AIC_CCR, PACK16(0), CHANNEL_V(STEREO),
              OSS_V(24BIT), ISS_V(24BIT), M2S(0));
    jz_writef(AIC_I2SCR, SWLH(0));
#else
    /* Program audio format (stereo, packed 16 bit samples) */
    jz_writef(AIC_CCR, PACK16(1), CHANNEL_V(STEREO),
              OSS_V(16BIT), ISS_V(16BIT), M2S(0));
    jz_writef(AIC_I2SCR, SWLH(0));
#endif

    /* Set DMA settings */
    jz_writef(AIC_CFG, TFTH(16), RFTH(15));
    dma_set_callback(DMA_CHANNEL_AUDIO, pcm_play_dma_int_cb);
#ifdef HAVE_RECORDING
    dma_set_callback(DMA_CHANNEL_RECORD, pcm_rec_dma_int_cb);
#endif

    /* Mask all interrupts and disable playback/recording */
    jz_writef(AIC_CCR, EROR(0), ETUR(0), ERFS(0), ETFS(0),
              ENLBF(0), ERPL(0), EREC(0));

    /* Enable the controller */
    jz_writef(AIC_CFG, ENABLE(1));

    /* Enable interrupts */
    system_enable_irq(IRQ_AIC);
}

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();
}

void pcm_dma_apply_settings(void)
{
    audiohw_set_frequency(pcm_fsel);
}

static void play_dma_start(const void* addr, size_t size)
{
    play_dma_desc.cm = jz_orf(DMA_CHN_CM, SAI(1), DAI(0), RDIL(9),
                              SP_V(32BIT), DP_V(32BIT), TSZ_V(AUTO),
                              STDE(0), TIE(1), LINK(0));
    play_dma_desc.sa = PHYSADDR(addr);
    play_dma_desc.ta = PHYSADDR(JA_AIC_DR);
    play_dma_desc.tc = size;
    play_dma_desc.sd = 0;
    play_dma_desc.rt = jz_orf(DMA_CHN_RT, TYPE_V(I2S_TX));
    play_dma_desc.pad0 = 0;
    play_dma_desc.pad1 = 0;

    commit_dcache_range(&play_dma_desc, sizeof(dma_desc));
    commit_dcache_range(addr, size);

    REG_DMA_CHN_DA(DMA_CHANNEL_AUDIO) = PHYSADDR(&play_dma_desc);
    jz_writef(DMA_CHN_CS(DMA_CHANNEL_AUDIO), DES8(1), NDES(0));
    jz_set(DMA_DB, 1 << DMA_CHANNEL_AUDIO);
    jz_writef(DMA_CHN_CS(DMA_CHANNEL_AUDIO), CTE(1));

    pcm_play_dma_status_callback(PCM_DMAST_STARTED);
}

static void play_dma_handle_event(int event)
{
    if(event == DMA_EVENT_COMPLETE) {
        const void* addr;
        size_t size;
        if(pcm_play_dma_complete_callback(PCM_DMAST_OK, &addr, &size))
            play_dma_start(addr, size);
    } else if(event == DMA_EVENT_NONE) {
        /* ignored, so callers don't need to check for this */
    } else {
        pcm_play_dma_status_callback(PCM_DMAST_ERR_DMA);
    }
}

static void pcm_play_dma_int_cb(int event)
{
    if(play_lock) {
        play_dma_pending_event = event;
        return;
    } else {
        play_dma_handle_event(event);
    }
}

void pcm_play_dma_start(const void* addr, size_t size)
{
    play_dma_pending_event = DMA_EVENT_NONE;
    aic_state |= AIC_STATE_PLAYING;

    play_dma_start(addr, size);
    jz_writef(AIC_CCR, TDMS(1), ETUR(1), ERPL(1));
}

void pcm_play_dma_stop(void)
{
    jz_writef(AIC_CCR, TDMS(0), ETUR(0), ERPL(0));
    jz_writef(AIC_CCR, TFLUSH(1));

    play_dma_pending_event = DMA_EVENT_NONE;
    aic_state &= ~AIC_STATE_PLAYING;
}

void pcm_play_lock(void)
{
    int irq = disable_irq_save();
    ++play_lock;
    restore_irq(irq);
}

void pcm_play_unlock(void)
{
    int irq = disable_irq_save();
    if(--play_lock == 0 && (aic_state & AIC_STATE_PLAYING)) {
        play_dma_handle_event(play_dma_pending_event);
        play_dma_pending_event = DMA_EVENT_NONE;
    }

    restore_irq(irq);
}

#ifdef HAVE_RECORDING
/*
 * Recording
 */

static void rec_dma_start(void* addr, size_t size)
{
    /* NOTE: Rockbox always records in stereo and the AIC pushes in the
     * sample for each channel separately. One frame therefore requires
     * two 16-bit transfers from the AIC. */
    rec_dma_desc.cm = jz_orf(DMA_CHN_CM, SAI(0), DAI(1), RDIL(6),
                             SP_V(16BIT), DP_V(16BIT), TSZ_V(16BIT),
                             STDE(0), TIE(1), LINK(0));
    rec_dma_desc.sa = PHYSADDR(JA_AIC_DR);
    rec_dma_desc.ta = PHYSADDR(addr);
    rec_dma_desc.tc = size / 2;
    rec_dma_desc.sd = 0;
    rec_dma_desc.rt = jz_orf(DMA_CHN_RT, TYPE_V(I2S_RX));
    rec_dma_desc.pad0 = 0;
    rec_dma_desc.pad1 = 0;

    commit_dcache_range(&rec_dma_desc, sizeof(dma_desc));
    if((unsigned long)addr < 0xa0000000ul)
        discard_dcache_range(addr, size);

    REG_DMA_CHN_DA(DMA_CHANNEL_RECORD) = PHYSADDR(&rec_dma_desc);
    jz_writef(DMA_CHN_CS(DMA_CHANNEL_RECORD), DES8(1), NDES(0));
    jz_set(DMA_DB, 1 << DMA_CHANNEL_RECORD);
    jz_writef(DMA_CHN_CS(DMA_CHANNEL_RECORD), CTE(1));

    pcm_rec_dma_status_callback(PCM_DMAST_STARTED);
}

static void rec_dma_handle_event(int event)
{
    if(event == DMA_EVENT_COMPLETE) {
        void* addr;
        size_t size;
        if(pcm_rec_dma_complete_callback(PCM_DMAST_OK, &addr, &size))
            rec_dma_start(addr, size);
    } else if(event == DMA_EVENT_NONE) {
        /* ignored, so callers don't need to check for this */
    } else {
        pcm_rec_dma_status_callback(PCM_DMAST_ERR_DMA);
    }
}

static void pcm_rec_dma_int_cb(int event)
{
    if(rec_lock) {
        rec_dma_pending_event = event;
        return;
    } else {
        rec_dma_handle_event(event);
    }
}

void pcm_rec_dma_init(void)
{
}

void pcm_rec_dma_close(void)
{
}

void pcm_rec_dma_start(void* addr, size_t size)
{
    rec_dma_pending_event = DMA_EVENT_NONE;
    aic_state |= AIC_STATE_RECORDING;

    rec_dma_start(addr, size);
    jz_writef(AIC_CCR, RDMS(1), EROR(1), EREC(1));
}

void pcm_rec_dma_stop(void)
{
    jz_writef(AIC_CCR, RDMS(0), EROR(0), EREC(0));
    jz_writef(AIC_CCR, RFLUSH(1));

    rec_dma_pending_event = DMA_EVENT_NONE;
    aic_state &= ~AIC_STATE_RECORDING;
}

void pcm_rec_lock(void)
{
    int irq = disable_irq_save();
    ++rec_lock;
    restore_irq(irq);
}

void pcm_rec_unlock(void)
{
    int irq = disable_irq_save();
    if(--rec_lock == 0 && (aic_state & AIC_STATE_RECORDING)) {
        rec_dma_handle_event(rec_dma_pending_event);
        rec_dma_pending_event = DMA_EVENT_NONE;
    }

    restore_irq(irq);
}

const void* pcm_rec_dma_get_peak_buffer(void)
{
    return (const void*)UNCACHEDADDR(REG_DMA_CHN_TA(DMA_CHANNEL_RECORD));
}
#endif /* HAVE_RECORDING */

#ifdef HAVE_PCM_DMA_ADDRESS
void* pcm_dma_addr(void* addr)
{
    return (void*)UNCACHEDADDR(addr);
}
#endif

void AIC(void)
{
    if(jz_readf(AIC_SR, TUR)) {
        aic_tx_underruns += 1;
        jz_writef(AIC_SR, TUR(0));
    }

#ifdef HAVE_RECORDING
    if(jz_readf(AIC_SR, ROR)) {
        aic_rx_overruns += 1;
        jz_writef(AIC_SR, ROR(0));
    }
#endif /* HAVE_RECORDING */
}
