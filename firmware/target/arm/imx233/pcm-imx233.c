/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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

#include "config.h"
#include "audiohw.h"
#include "dma-imx233.h"
#include "pcm-internal.h"
#include "audioout-imx233.h"

struct pcm_dma_command_t
{
    struct apb_dma_command_t dma;
    /* padded to next multiple of cache line size (32 bytes) */
    uint32_t pad[5];
} __attribute__((packed)) CACHEALIGN_ATTR;

__ENSURE_STRUCT_CACHE_FRIENDLY(struct pcm_dma_command_t)

/* Because we have no way of stopping the DMA properly (see below), we can only
 * let the tranfer finish on stop. However if the transfer is very long it could
 * take a while. We work around this by splitting big transfers into small burst
 * to make sure we can stop quickly. */

static struct pcm_dma_command_t dac_dma;
static bool dac_freezed = false;

static const void *dac_buf;      /* current buffer */
static unsigned long dac_frames; /* remaining frames */

/* for both recording and playback: maximum transfer size, see
 * pcm_dma_apply_settings */
static unsigned long dma_max_frames = CACHEALIGN_UP(1600) / 4;

enum
{
    DAC_PLAYING,
    DAC_STOP_PENDING,
    DAC_STOPPED,
}dac_state = DAC_STOPPED;

/**
 * WARNING !
 * Never reset the dma channel, otherwise it will halt the DAC for some reason
 * and I don't know how to recover from this state
 * */

static void play(void)
{
    /* split transfer if needed */
    unsigned long xfer = MIN(dac_frames, dma_max_frames);

    dac_dma.dma.next = NULL;
    dac_dma.dma.buffer = (void *)dac_buf;
    dac_dma.dma.cmd = BF_OR(APB_CHx_CMD, COMMAND_V(READ),
        IRQONCMPLT(1), SEMAPHORE(1), XFER_COUNT(xfer*4));
    /* dma subsystem will make sure cached stuff is written to memory */
    dac_state = DAC_PLAYING;
    imx233_dma_start_command(APB_AUDIO_DAC, &dac_dma.dma);
    /* advance buffer */
    dac_buf += xfer*4;
    dac_frames -= xfer;
}

void INT_DAC_DMA(void)
{
    /* if stop is pending, ackonowledge stop */
    if(dac_state == DAC_STOP_PENDING)
    {
        dac_state = DAC_STOPPED;
    }
    else if(dac_state == DAC_PLAYING)
    {
        /* continue if buffer is not done */
        if (dac_frames)
            play();
        else
            pcm_play_dma_complete_callback(0);
    }

    imx233_dma_clear_channel_interrupt(APB_AUDIO_DAC);
}

void INT_DAC_ERROR(void)
{
    pcm_play_dma_complete_callback(-EIO);
    dac_state = DAC_STOPPED;
    imx233_dma_clear_channel_interrupt(APB_AUDIO_DAC);
}

void pcm_play_dma_lock(void)
{
    imx233_dma_enable_channel_interrupt(APB_AUDIO_DAC, false);
}

void pcm_play_dma_unlock(void)
{
    imx233_dma_enable_channel_interrupt(APB_AUDIO_DAC, true);
}

void pcm_play_dma_stop(void)
{
    /* do not interrupt the current transaction because resetting the dma
     * would halt the DAC and clearing RUN causes sound havoc so simply
     * wait for the end of transfer */
    dac_buf = NULL;
    dac_frames = 0;

    if(dac_state != DAC_STOPPED)
        dac_state = DAC_STOP_PENDING;
}

void pcm_play_dma_send_frames(const void *addr, unsigned long frames)
{
    /* update pending buffer */
    dac_buf = addr;
    dac_frames = frames;
    dac_state = DAC_PLAYING;
    play();
}

void pcm_play_dma_prepare(void)
{
    /* nothing to do */
}

void pcm_play_dma_pause(bool pause)
{
    imx233_dma_freeze_channel(APB_AUDIO_DAC, pause);
    dac_freezed = pause;
}

void pcm_dma_init(const struct pcm_hw_settings *settings)
{
    audiohw_init();
    imx233_icoll_enable_interrupt(INT_SRC_DAC_DMA, true);
    imx233_icoll_enable_interrupt(INT_SRC_DAC_ERROR, true);
    imx233_icoll_set_priority(INT_SRC_DAC_DMA, ICOLL_PRIO_AUDIO);
    imx233_dma_enable_channel_interrupt(APB_AUDIO_DAC, true);
    (void)settings;
}

void pcm_dma_apply_settings(const struct pcm_hw_settings *settings)
{
    pcm_play_dma_lock();
    /* update frequency */
    audiohw_set_frequency(settings->fsel);
    /* compute maximum transfer size: aim at ~1/100s stop time maximum, make sure
     * the resulting value is a multiple of cache line. At sample rate F we
     * transfer two samples (2 x 2 bytes) F times per second = 4F b/s */
    dma_max_frames = CACHEALIGN_UP(4 * pcm_sampr / 100) / 4;
    pcm_play_dma_unlock();
}

unsigned long pcm_play_dma_get_frames_waiting(void)
{
    unsigned long frames = 0, ahb_bytes = 0;

    int oldlevel = disable_irq_save();

    if(dac_state == DAC_PLAYING && !dac_freezed)
    {
        struct imx233_dma_info_t info = imx233_dma_get_info(APB_AUDIO_DAC,
                                            DMA_INFO_AHB_BYTES);
        frames = dac_frames;
        ahb_bytes = info.ahb_bytes;
    }

    restore_irq(oldlevel);

    return frames + ahb_bytes / 4;
}

const void *pcm_play_dma_get_peak_buffer(unsigned long *frames_rem)
{
    unsigned long frames = 0, bar = 0, ahb_bytes = 0;

    int oldlevel = disable_irq_save();

    if(dac_state == DAC_PLAYING && !dac_freezed)
    {
        struct imx233_dma_info_t info = imx233_dma_get_info(APB_AUDIO_DAC,
                                            DMA_INFO_AHB_BYTES | DMA_INFO_BAR);
        frames = dac_frames;
        bar = info.bar;
        ahb_bytes = info.ahb_bytes;
    }

    restore_irq(oldlevel);

    *frames_rem = frames + ahb_bytes / 4;
    return (void *)(bar & ~3);
}

/*
 * Recording
 */

/* Because we have no way of stopping the DMA properly (like for the DAC),
 * we can only let the tranfer finish on stop. However if the transfer is very
 * long it could take a while. We work around this by splitting big transfers
 * into small burst to make sure we can stop quickly. */
#ifdef HAVE_RECORDING
static struct pcm_dma_command_t adc_dma;

static void *adc_buf;            /* current buffer (physical address) */
static void *adc_addr;           /* current subtransfer buffer pointer */
static unsigned long adc_frames; /* remaining frames */

enum
{
    ADC_RECORDING,
    ADC_STOP_PENDING,
    ADC_STOPPED,
}adc_state = ADC_STOPPED;

void pcm_rec_dma_lock(void)
{
    imx233_dma_enable_channel_interrupt(APB_AUDIO_ADC, false);
}

void pcm_rec_dma_unlock(void)
{
    imx233_dma_enable_channel_interrupt(APB_AUDIO_ADC, true);
}

void pcm_rec_dma_init(void)
{
    imx233_icoll_enable_interrupt(INT_SRC_ADC_DMA, true);
    imx233_icoll_enable_interrupt(INT_SRC_ADC_ERROR, true);
    imx233_dma_enable_channel_interrupt(APB_AUDIO_ADC, true);
}

void pcm_rec_dma_close(void)
{
    pcm_rec_dma_stop();
}

static void rec(void)
{
    /* split transfer if needed */
    unsigned long xfer = MIN(adc_frames, dma_max_frames);

    adc_dma.dma.next = NULL;
    adc_dma.dma.buffer = (void *)adc_addr;
    adc_dma.dma.cmd = BF_OR(APB_CHx_CMD, COMMAND_V(WRITE),
        IRQONCMPLT(1), SEMAPHORE(1), XFER_COUNT(xfer*4));
    /* dma subsystem will make sure cached stuff is written to memory */
    adc_state = ADC_RECORDING;
    imx233_dma_start_command(APB_AUDIO_ADC, &adc_dma.dma);
    /* advance buffer */
    adc_addr += xfer*4;
    adc_frames -= xfer;
}

void INT_ADC_DMA(void)
{
    /* if stop is pending, ackonowledge stop
     * otherwise try to get some more and stop if there is none */
    if(adc_state == ADC_STOP_PENDING)
    {
        adc_state = ADC_STOPPED;
    }
    else if(adc_state == ADC_RECORDING)
    {
        /* continue if buffer is not done, otherwise try to get some new data */
        if(adc_frames)
            rec();
        else
            pcm_rec_dma_complete_callback(0);
    }

    imx233_dma_clear_channel_interrupt(APB_AUDIO_ADC);
}

void INT_ADC_ERROR(void)
{
    pcm_rec_dma_complete_callback(-EIO);
    adc_state = ADC_STOPPED;
    imx233_dma_clear_channel_interrupt(APB_AUDIO_ADC);
}

void pcm_rec_dma_capture_frames(void *addr, unsigned long frames)
{
    /* update pending buffer */
    adc_buf = PHYSICAL_ADDR(addr);
    adc_addr = addr;
    adc_frames = frames;
    adc_state = ADC_RECORDING;
    rec();
}

void pcm_rec_dma_prepare(void)
{
    /* nothing to do */
}

void pcm_rec_dma_stop(void)
{
    /* do not interrupt the current transaction because resetting the dma
     * would halt the ADC and clearing RUN causes sound havoc so simply
     * wait for the end of transfer */
    adc_buf = NULL;
    adc_addr = NULL;
    adc_frames = 0;

    if(adc_state != ADC_STOPPED)
        adc_state = ADC_STOP_PENDING;
}

const void *pcm_rec_dma_get_peak_buffer(unsigned long *frames_avail)
{
    unsigned long buf = 0;
    struct imx233_dma_info_t info;

    info.bar = 0;

    int oldlevel = disable_irq_save();

    if(adc_state == ADC_RECORDING)
    {
        info = imx233_dma_get_info(APB_AUDIO_ADC, DMA_INFO_BAR);
        buf = (unsigned long)adc_buf;
    }

    restore_irq(oldlevel);

    *frames_avail = (info.bar - buf) / 4;
    return (void *)buf;
}
#endif /* HAVE_RECORDING */
