/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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
#include "audiohw.h"
#include "pcm.h"
#include "pcm-internal.h"
#include "panic.h"
#include "dma-x1000.h"
#include "irq-x1000.h"
#include "x1000/aic.h"

#define AIC_STATE_STOPPED   0
#define AIC_STATE_PLAYING   1

volatile unsigned aic_tx_underruns = 0;

static int aic_state = AIC_STATE_STOPPED;

static int aic_lock = 0;
static volatile int aic_dma_pending_event = DMA_EVENT_NONE;

static dma_desc aic_dma_desc;

static void pcm_dma_int_cb(int event);

void pcm_play_dma_init(void)
{
    /* Let the target initialize its hardware and setup the AIC */
    audiohw_init();

    /* Set DMA callback */
    dma_set_callback(DMA_CHANNEL_AUDIO, pcm_dma_int_cb);

    /* Program FIFO threshold -- DMA settings must match */
    jz_writef(AIC_CFG, TFTH(16));

    /* Ensure all playback is disabled */
    jz_writef(AIC_CCR, ERPL(0));

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

static void pcm_dma_start(const void* addr, size_t size)
{
    aic_dma_desc.cm = jz_orf(DMA_CHN_CM, SAI(1), DAI(0), RDIL(9),
                             SP_V(32BIT), DP_V(32BIT), TSZ_V(AUTO),
                             STDE(0), TIE(1), LINK(0));
    aic_dma_desc.sa = PHYSADDR(addr);
    aic_dma_desc.ta = PHYSADDR(JA_AIC_DR);
    aic_dma_desc.tc = size;
    aic_dma_desc.sd = 0;
    aic_dma_desc.rt = jz_orf(DMA_CHN_RT, TYPE_V(I2S_TX));
    aic_dma_desc.pad0 = 0;
    aic_dma_desc.pad1 = 0;

    commit_dcache_range(&aic_dma_desc, sizeof(dma_desc));
    commit_dcache_range(addr, size);

    REG_DMA_CHN_DA(DMA_CHANNEL_AUDIO) = PHYSADDR(&aic_dma_desc);
    jz_writef(DMA_CHN_CS(DMA_CHANNEL_AUDIO), DES8(1), NDES(0));
    jz_set(DMA_DB, 1 << DMA_CHANNEL_AUDIO);
    jz_writef(DMA_CHN_CS(DMA_CHANNEL_AUDIO), CTE(1));

    pcm_play_dma_status_callback(PCM_DMAST_STARTED);
}

static void pcm_dma_handle_event(int event)
{
    if(event == DMA_EVENT_COMPLETE) {
        const void* addr;
        size_t size;
        if(pcm_play_dma_complete_callback(PCM_DMAST_OK, &addr, &size))
            pcm_dma_start(addr, size);
    } else if(event == DMA_EVENT_NONE) {
        /* ignored, so callers don't need to check for this */
    } else {
        pcm_play_dma_status_callback(PCM_DMAST_ERR_DMA);
    }
}

static void pcm_dma_int_cb(int event)
{
    if(aic_lock) {
        aic_dma_pending_event = event;
        return;
    } else {
        pcm_dma_handle_event(event);
    }
}

void pcm_play_dma_start(const void* addr, size_t size)
{
    aic_dma_pending_event = DMA_EVENT_NONE;
    aic_state = AIC_STATE_PLAYING;

    pcm_dma_start(addr, size);
    jz_writef(AIC_CCR, TDMS(1), ETUR(1), ERPL(1));
}

void pcm_play_dma_stop(void)
{
    jz_writef(AIC_CCR, TDMS(0), ETUR(0), ERPL(0));
    jz_writef(AIC_CCR, TFLUSH(1));

    aic_dma_pending_event = DMA_EVENT_NONE;
    aic_state = AIC_STATE_STOPPED;
}

void pcm_play_lock(void)
{
    ++aic_lock;
}

void pcm_play_unlock(void)
{
    int irq = disable_irq_save();
    if(--aic_lock == 0 && aic_state == AIC_STATE_PLAYING) {
        pcm_dma_handle_event(aic_dma_pending_event);
        aic_dma_pending_event = DMA_EVENT_NONE;
    }

    restore_irq(irq);
}

void AIC(void)
{
    if(jz_readf(AIC_SR, TUR)) {
        aic_tx_underruns += 1;
        jz_writef(AIC_SR, TUR(0));
    }
}
