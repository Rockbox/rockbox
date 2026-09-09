/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 *
 * Based on firmware/target/mips/ingenic_x1000/pcm-x1000.c,
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

/* Playback only; the receive DMA request is unverified. Avoid logging from
 * buffer and interrupt paths because the USB console affects transfer
 * timing. */
#define LOGF_ENABLE

#include "system.h"
#include "logf.h"
#include "kernel.h"
#include "audio.h"
#include "audiohw.h"
#include "pcm.h"
#include "pcm-internal.h"
#include "pcm_sink.h"
#include "aic-x1600.h"
#include "clk-x1600.h"
#include "dma-ingenic.h"
#include "irq-x1600.h"
#include "x1600/aic.h"
#include "x1600/cpm.h"

/* Exposed for a future debug screen, same as the X1000 port. */
volatile unsigned aic_tx_underruns = 0;

static bool aic_playing = false;

static int play_lock = 0;
static volatile int play_dma_pending_event = DMA_EVENT_NONE;
static dma_desc play_dma_desc;

static void pcm_play_dma_int_cb(int event);

/* Initialization */

static void sink_dma_init(void)
{
    /* Ungate clock -- two of them here, unlike the X1000 */
    jz_writef(CPM_CLKGR, AUDIO(0));
    jz_writef(CPM_CLKGR1, I2S0_TCLK(0));

    /* Configure AIC with some sane defaults. BIT_CLK must be stopped while
     * AUSEL changes (PM 9.7.2.1). */
    aic_enable_i2s_bit_clock(false);
    jz_writef(AIC_FR, ENB(0), MSB(0), DMODE(0), LSMP(0), AUSEL_V(I2S));

    /* Let the target initialize its hardware and setup the AIC */
    audiohw_init();

    /* A missing bit clock is silence with every other stage correct */
    if(clk_get(X1600_CLK_I2S_MCLK) == 0)
        logf("aic: MCLK IS 0 -- EPLL %lu Hz, nothing will play",
             (unsigned long)clk_get(X1600_CLK_EPLL));
    else
        logf("aic: MCLK %lu Hz, BCLK %lu Hz",
             (unsigned long)clk_get(X1600_CLK_I2S_MCLK),
             (unsigned long)clk_get(X1600_CLK_I2S_BCLK));

    /* Enable the controller, then reset it -- the PM's order (9.8.1) */
    aic_enable_i2s_bit_clock(true);
    jz_writef(AIC_FR, ENB(1));
    jz_writef(AIC_FR, RST(1));

    /* Immediate format writes can be cleared by the pending reset. RST
     * always reads zero, so allow reset to settle before programming them. */
    mdelay(1);

    /* Program audio format (stereo, packed 16 bit samples) */
    jz_writef(AIC_I2SCR, AMSL(0), RFIRST(0), SWLH(0));

#if (PCM_NATIVE_BITDEPTH > 16)
    /* 24-bit PCM is not enabled or tested on this target. */
    jz_writef(AIC_CR, PACK16(0), CHANNEL_V(STEREO),
              OSS_V(24BIT), ISS_V(24BIT), ENDSW(0), MONOCTR_V(BOTH));
#else
    jz_writef(AIC_CR, PACK16(1), CHANNEL_V(STEREO),
              OSS_V(16BIT), ISS_V(24BIT), ENDSW(0), MONOCTR_V(BOTH));
#endif

    /* Set FIFO thresholds. AICFR then reads 0x03100017, matching the device
     * under vendor firmware; if playback is gritty, try TFTH 15. */
    jz_writef(AIC_FR, TFTH(16), RFTH(3));

    dma_set_callback(DMA_CHANNEL_AUDIO, pcm_play_dma_int_cb);

    /* Mask all interrupts and disable playback/recording. The TFIFO-loop bits
     * are X1600-only and cleared so a warm restart cannot inherit them. */
    jz_writef(AIC_CR, ETFLOR(0), ETFLS(0), ETFL(0), TLDMS(0),
              RDMS(0), TDMS(0), EROR(0), ETUR(0), ERFS(0), ETFS(0),
              ENLBF(0), ERPL(0), EREC(0));

    /* Enable interrupts -- INTC source AUDIO here, not the X1000's IRQ_AIC */
    system_enable_irq(IRQ_AUDIO);
}

static void sink_set_freq(uint16_t freq)
{
    logf("aic: set_freq %u", (unsigned)freq);
    audiohw_set_frequency(freq);
}

/* Playback DMA */

static void play_dma_start(const void* addr, size_t size)
{
    /* Set DMA settings */
    play_dma_desc.cm = jz_orf(DMA_CHN_CM, SAI(1), DAI(0), RDIL(9),
                              SP_V(32BIT), DP_V(32BIT), TSZ_V(AUTO),
                              STDE(0), TIE(1), LINK(0));
    /* The engine reads the caller's buffer directly. */
    play_dma_desc.sa = PHYSADDR(addr);
    play_dma_desc.ta = PHYSADDR(JA_AIC_DR);
    play_dma_desc.tc = size;
    play_dma_desc.sd = 0;

    /* The AIC_TX request code is inferred from the PM AIC range. */
    play_dma_desc.rt = jz_orf(DMA_CHN_RT, TYPE_V(AIC_TX));

    play_dma_desc.pad0 = 0;
    play_dma_desc.pad1 = 0;

    /* Write the descriptor through KSEG1. Commit sample buffers before
     * DMA starts. */
    {
        volatile dma_desc* ud =
            (volatile dma_desc*)UNCACHEDADDR(&play_dma_desc);
        ud->cm = play_dma_desc.cm;
        ud->sa = play_dma_desc.sa;
        ud->ta = play_dma_desc.ta;
        ud->tc = play_dma_desc.tc;
        ud->sd = play_dma_desc.sd;
        ud->rt = play_dma_desc.rt;
        ud->pad0 = play_dma_desc.pad0;
        ud->pad1 = play_dma_desc.pad1;
    }
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

static void sink_dma_start(const void* addr, size_t size)
{
    play_dma_pending_event = DMA_EVENT_NONE;
    aic_playing = true;

    play_dma_start(addr, size);

    /* ERPL last (PM 9.8.3 step 9): with it clear the AIC sends zeroes
     * regardless of the FIFO, which is what we want mid-burst. */
    jz_writef(AIC_CR, TDMS(1), ETUR(1), ERPL(1));
}

static void sink_dma_stop(void)
{
    /* Stop feeding the FIFO, and stop counting under-runs -- the drain below
     * empties it deliberately. */
    jz_writef(AIC_CR, TDMS(0), ETUR(0));

    /* Bound the FIFO-drain wait by iterations because this can run in
     * interrupt context. */
    if(aic_i2s_bit_clock_enabled()) {
        int timeout = 1000000;
        while(jz_readf(AIC_SR, TFL) != 0 && --timeout > 0)
            ;
    }

    /* PM 9.8.3 step 11 (line 10280). */
    jz_writef(AIC_CR, ERPL(0));

    play_dma_pending_event = DMA_EVENT_NONE;
    aic_playing = false;
}

static void sink_lock(void)
{
    int irq = disable_irq_save();
    ++play_lock;
    restore_irq(irq);
}

static void sink_unlock(void)
{
    int irq = disable_irq_save();
    if(--play_lock == 0 && aic_playing) {
        play_dma_handle_event(play_dma_pending_event);
        play_dma_pending_event = DMA_EVENT_NONE;
    }

    restore_irq(irq);
}

struct pcm_sink builtin_pcm_sink = {
    .caps = {
        .samprs       = hw_freq_sampr,
        .num_samprs   = HW_NUM_FREQ,
        .default_freq = HW_FREQ_DEFAULT,
    },
    .ops = {
        .init     = sink_dma_init,
        .postinit = audiohw_postinit,
        .set_freq = sink_set_freq,
        .lock     = sink_lock,
        .unlock   = sink_unlock,
        .play     = sink_dma_start,
        .stop     = sink_dma_stop,
    },
};

#ifdef HAVE_PCM_DMA_ADDRESS
void* pcm_dma_addr(void* addr)
{
    return (void*)UNCACHEDADDR(addr);
}
#endif

/* Interrupt */

/* INTC source 0; the name must match the system-x1600.c vector table. */
void AUDIO(void)
{
    /* AIC error flags are cleared by writing zero. RMW preserves other
     * asserted flags (PM 9.7). */
    if(jz_readf(AIC_SR, TUR)) {
        aic_tx_underruns += 1;
        jz_writef(AIC_SR, TUR(0));
    }
}

/* Anti-pop pre-roll is not implemented; the vendor leaves the TFIFO loop
 * disabled. */
