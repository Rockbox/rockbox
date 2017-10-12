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
#include "pcm.h"
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

static int dac_locked = 0;
static struct pcm_dma_command_t dac_dma;
static bool dac_freezed = false;

static const void *dac_buf;      /* current buffer */
static unsigned long dac_frames; /* remaining frames */
static unsigned long dac_xfer;   /* current transfer count */

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
    dac_xfer = MIN(dac_frames, dma_max_frames);

    dac_dma.dma.next = NULL;
    dac_dma.dma.buffer = (void *)dac_buf;
    dac_dma.dma.cmd = BF_OR(APB_CHx_CMD, COMMAND_V(READ),
        IRQONCMPLT(1), SEMAPHORE(1), XFER_COUNT(dac_xfer*4));
    /* dma subsystem will make sure cached stuff is written to memory */
    dac_state = DAC_PLAYING;
    imx233_dma_start_command(APB_AUDIO_DAC, &dac_dma.dma);
}

void INT_DAC_DMA(void)
{
    /* if stop is pending, ackonowledge stop
     * otherwise try to get some more and stop if there is none */
    if(dac_state == DAC_STOP_PENDING)
    {
        dac_state = DAC_STOPPED;
    }
    else if(dac_state == DAC_PLAYING)
    {
        /* advance buffer (post-increment!) */
        dac_buf += dac_xfer*4;
        dac_frames -= dac_xfer;

        /* continue if buffer is not done, otherwise try to get some new data */
        if(dac_frames || pcm_play_dma_complete_callback(PCM_DMAST_OK, &dac_buf, &dac_frames))
        {
            play();
            pcm_play_dma_status_callback(PCM_DMAST_STARTED);
        }
        else
            dac_state = DAC_STOPPED;
    }

    imx233_dma_clear_channel_interrupt(APB_AUDIO_DAC);
}

void INT_DAC_ERROR(void)
{
    dac_state = DAC_STOPPED;
    pcm_play_dma_status_callback(PCM_DMAST_ERR_DMA);
    imx233_dma_clear_channel_interrupt(APB_AUDIO_DAC);
}

void pcm_play_lock(void)
{
    if(dac_locked++ == 0)
        imx233_dma_enable_channel_interrupt(APB_AUDIO_DAC, false);
}

void pcm_play_unlock(void)
{
    if(--dac_locked == 0)
        imx233_dma_enable_channel_interrupt(APB_AUDIO_DAC, true);
}

void pcm_play_dma_stop(void)
{
    /* do not interrupt the current transaction because resetting the dma
     * would halt the DAC and clearing RUN causes sound havoc so simply
     * wait for the end of transfer */
    dac_buf = NULL;
    dac_frames = 0;
    dac_state = DAC_STOP_PENDING;
}

void pcm_play_dma_start(const void *addr, unsigned long frames)
{
    /* update pending buffer */
    dac_buf = addr;
    dac_frames = frames;
    /* if we are stopped restart playback, otherwise IRQ will pick up */
    if(dac_state == DAC_STOPPED)
        play();
    else
        dac_state = DAC_PLAYING;
}

void pcm_play_dma_pause(bool pause)
{
    imx233_dma_freeze_channel(APB_AUDIO_DAC, pause);
    dac_freezed = pause;
}

void pcm_play_dma_init(void)
{
    audiohw_preinit();
}

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();
    imx233_icoll_enable_interrupt(INT_SRC_DAC_DMA, true);
    imx233_icoll_enable_interrupt(INT_SRC_DAC_ERROR, true);
    imx233_icoll_set_priority(INT_SRC_DAC_DMA, ICOLL_PRIO_AUDIO);
    imx233_dma_enable_channel_interrupt(APB_AUDIO_DAC, true);
}

void pcm_dma_apply_settings(void)
{
    pcm_play_lock();
    /* update frequency */
    audiohw_set_frequency(pcm_fsel);
    /* compute maximum transfer size: aim at ~1/100s stop time maximum, make sure
     * the resulting value is a multiple of cache line. At sample rate F we
     * transfer two samples (2 x 2 bytes) F times per second = 4F b/s */
    dma_max_frames = CACHEALIGN_UP(4 * pcm_sampr / 100) / 4;
    pcm_play_unlock();
}

unsigned long pcm_get_frames_waiting(void)
{
    unsigned long frames = 0, xfer = 0, ahb_bytes = 0;

    int oldlevel = disable_irq_save();

    if(dac_state == DAC_PLAYING && !dac_freezed)
    {
        struct imx233_dma_info_t info = imx233_dma_get_info(APB_AUDIO_DAC,
                                            DMA_INFO_AHB_BYTES);
        frames = dac_frames;
        xfer = dac_xfer;
        ahb_bytes = info.ahb_bytes;
    }

    restore_irq(oldlevel);

    return frames - (xfer - ahb_bytes / 4);
}

const void *pcm_play_dma_get_peak_buffer(unsigned long *frames_rem)
{
    unsigned long frames = 0, xfer = 0, bar = 0, ahb_bytes = 0;

    int oldlevel = disable_irq_save();

    if(dac_state == DAC_PLAYING && !dac_freezed)
    {
        struct imx233_dma_info_t info = imx233_dma_get_info(APB_AUDIO_DAC,
                                            DMA_INFO_AHB_BYTES | DMA_INFO_BAR);
        frames = dac_frames;
        xfer = dac_xfer;
        bar = info.bar;
        ahb_bytes = info.ahb_bytes;
    }

    restore_irq(oldlevel);

    *frames_rem = frames - (xfer - ahb_bytes / 4);
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
static int adc_locked = 0;
static struct pcm_dma_command_t adc_dma;

static void *adc_buf;            /* current buffer */
static unsigned long adc_frames; /* remaining frames */
static unsigned long adc_xfer;   /* current transfer size */
static unsigned long adc_peaks;  /* next peak readout position */

enum
{
    ADC_RECORDING,
    ADC_STOP_PENDING,
    ADC_STOPPED,
}adc_state = ADC_STOPPED;

void pcm_rec_lock(void)
{
    if(adc_locked++ == 0)
        imx233_dma_enable_channel_interrupt(APB_AUDIO_ADC, false);
}


void pcm_rec_unlock(void)
{
    if(--adc_locked == 0)
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
    adc_xfer = MIN(adc_frames, dma_max_frames);

    adc_dma.dma.next = NULL;
    adc_dma.dma.buffer = (void *)adc_buf;
    adc_dma.dma.cmd = BF_OR(APB_CHx_CMD, COMMAND_V(WRITE),
        IRQONCMPLT(1), SEMAPHORE(1), XFER_COUNT(adc_xfer*4));
    /* dma subsystem will make sure cached stuff is written to memory */
    adc_state = ADC_RECORDING;
    imx233_dma_start_command(APB_AUDIO_ADC, &adc_dma.dma);
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
        /* advance buffer (post-increment!) */
        adc_buf += adc_xfer*4;
        adc_frames -= adc_xfer;

        /* continue if buffer is not done, otherwise try to get some new data */
        if(!adc_frames)
        {
            if(pcm_rec_dma_complete_callback(PCM_DMAST_OK, &adc_buf, &adc_frames))
                adc_peaks = (unsigned long)PHYSICAL_ADDR(adc_buf);
            else
                adc_state = ADC_STOPPED;
        }

        if(adc_state == ADC_RECORDING)
        {
            rec();
            pcm_rec_dma_status_callback(PCM_DMAST_STARTED);
        }
    }

    imx233_dma_clear_channel_interrupt(APB_AUDIO_ADC);
}

void INT_ADC_ERROR(void)
{
    adc_state = ADC_STOPPED;
    pcm_rec_dma_status_callback(PCM_DMAST_ERR_DMA);
    imx233_dma_clear_channel_interrupt(APB_AUDIO_ADC);
}

void pcm_rec_dma_start(void *addr, unsigned long frames)
{
    /* update pending buffer */
    adc_buf = addr;
    adc_frames = frames;
    adc_peaks = (unsigned long)PHYSICAL_ADDR(addr);
    /* if we are stopped restart recording, otherwise IRQ will pick up */
    if(adc_state == ADC_STOPPED)
        rec();
    else
        adc_state = ADC_RECORDING;
}

void pcm_rec_dma_stop(void)
{
    /* do not interrupt the current transaction because resetting the dma
     * would halt the ADC and clearing RUN causes sound havoc so simply
     * wait for the end of transfer */
    adc_buf = NULL;
    adc_frames = 0;
    adc_state = ADC_STOP_PENDING;
    adc_peaks = 0;
}

const void *pcm_rec_dma_get_peak_buffer(unsigned long *frames_avail)
{
    unsigned long bar = 0, peaks = 0;

    int oldlevel = disable_irq_save();

    if(adc_state == ADC_RECORDING)
    {
        struct imx233_dma_info_t info = imx233_dma_get_info(APB_AUDIO_ADC,
                                            DMA_INFO_BAR);
        bar = info.bar & ~3;
        peaks = adc_peaks;
        adc_peaks = bar;
    }

    restore_irq(oldlevel);

    *frames_avail = (bar - peaks) / 4;
    return (void *)bar;
}
#endif /* HAVE_RECORDING */
