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
#include "irq-x1000.h"
#include "x1000/aic.h"

/* #define LOGF_ENABLE */
#include "logf.h"

/* #define UNDERRUN_LOGF logf */
#define UNDERRUN_LOGF panicf

/* TX amount = number of FIFO entries written after an interrupt
 * TX thresh = threshold to trigger a FIFO interrupt.
 *
 * Interrupt will trigger when number of entries in FIFO <= 2*AIC_TX_THRESH.
 * TX FIFO has 64 entries.
 */
#define AIC_TX_AMOUNT 32
#define AIC_TX_THRESH 16

static struct timeout aic_tur_reset_tmo;
static const void* aic_databuf = NULL;
static const void* aic_dataend = NULL;
static volatile int play_lock = 0;
static volatile int want_irq = 0;

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();
}

void pcm_play_dma_init(void)
{
    /* Let the target initialize its hardware and setup the AIC */
    audiohw_init();

    /* Program FIFO threshold */
    jz_writef(AIC_CFG, TFTH(AIC_TX_THRESH));

    /* Ensure all playback is disabled */
    jz_writef(AIC_CCR, ERPL(0));

    /* Enable the controller */
    jz_writef(AIC_CFG, ENABLE(1));
}

void pcm_dma_apply_settings(void)
{
    audiohw_set_frequency(pcm_fsel);
}

static bool aic_fill_fifo(size_t count)
{
    while(aic_databuf != aic_dataend) {
        REG_AIC_DR = *(unsigned*)aic_databuf;
        aic_databuf += 4;
        if(--count == 0)
            return false;
    }

    return true;
}

void pcm_play_dma_start(const void* addr, size_t size)
{
    if(size & 3)
        panicf("PCM buffer size must be a multiple of 4");

    aic_databuf = addr;
    aic_dataend = addr + size;
    aic_fill_fifo(64);

    want_irq = 1;
    if(play_lock == 0)
        system_enable_irq(IRQ_AIC);
    jz_writef(AIC_CCR, ETFS(1), ETUR(1), ERPL(1));
}

void pcm_play_dma_stop(void)
{
    int irq = disable_irq_save();
    want_irq = 0;
    system_disable_irq(IRQ_AIC);
    timeout_cancel(&aic_tur_reset_tmo);
    jz_writef(AIC_CCR, ETFS(0), ETUR(0), ERPL(0));
    jz_writef(AIC_CCR, TFLUSH(1));
    restore_irq(irq);
}

void pcm_play_lock(void)
{
    int irq = disable_irq_save();
    if(++play_lock == 1)
        system_disable_irq(IRQ_AIC);
    restore_irq(irq);
}

void pcm_play_unlock(void)
{
    int irq = disable_irq_save();
    if(--play_lock == 0 && want_irq)
        system_enable_irq(IRQ_AIC);
    restore_irq(irq);
}


static int aic_tur_reset(struct timeout* tmo)
{
    (void)tmo;
    jz_writef(AIC_CCR, ETUR(1));
    return 0;
}

void AIC0(void)
{
    if(jz_readf(AIC_SR, TUR)) {
        UNDERRUN_LOGF("AIC TX FIFO underrun at tick: %ld", current_tick);
        timeout_register(&aic_tur_reset_tmo, aic_tur_reset, 5, 0);
        jz_writef(AIC_CCR, ETUR(0));
    }

    if(aic_fill_fifo(AIC_TX_AMOUNT)) {
        const void* addr;
        size_t size;
        if(pcm_play_dma_complete_callback(PCM_DMAST_OK, &addr, &size)) {
            if(size & 3)
                panicf("PCM buffer size must be a multiple of 4");

            aic_databuf = addr;
            aic_dataend = addr + size;
            pcm_play_dma_status_callback(PCM_DMAST_STARTED);
        }
    }
}
