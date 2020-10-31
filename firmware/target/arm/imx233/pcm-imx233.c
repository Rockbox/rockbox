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

static const void *dac_buf; /* current buffer */
static size_t dac_size; /* remaining size */

/* for both recording and playback: maximum transfer size, see
 * pcm_dma_apply_settings */
static size_t dma_max_size = CACHEALIGN_UP(1600);

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
    size_t xfer = MIN(dac_size, dma_max_size);

    dac_dma.dma.next = NULL;
    dac_dma.dma.buffer = (void *)dac_buf;
    dac_dma.dma.cmd = BF_OR(APB_CHx_CMD, COMMAND_V(READ),
        IRQONCMPLT(1), SEMAPHORE(1), XFER_COUNT(xfer));
    /* dma subsystem will make sure cached stuff is written to memory */
    dac_state = DAC_PLAYING;
    imx233_dma_start_command(APB_AUDIO_DAC, &dac_dma.dma);
    /* advance buffer */
    dac_buf += xfer;
    dac_size -= xfer;
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
        /* continue if buffer is not done, otherwise try to get some new data */
        if(dac_size != 0 || pcm_play_dma_complete_callback(PCM_DMAST_OK, &dac_buf, &dac_size))
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
    pcm_play_lock();
    dac_buf = NULL;
    dac_size = 0;
    dac_state = DAC_STOP_PENDING;
    pcm_play_unlock();
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    pcm_play_lock();
    /* update pending buffer */
    dac_buf = addr;
    dac_size = size;
    /* if we are stopped restart playback, otherwise IRQ will pick up */
    if(dac_state == DAC_STOPPED)
        play();
    else
        dac_state = DAC_PLAYING;
    pcm_play_unlock();
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
    dma_max_size = CACHEALIGN_UP(4 * pcm_sampr / 100);
    pcm_play_unlock();
}

size_t pcm_get_bytes_waiting(void)
{
    struct imx233_dma_info_t info = imx233_dma_get_info(APB_AUDIO_DAC, DMA_INFO_AHB_BYTES);
    return info.ahb_bytes;
}

const void *pcm_play_dma_get_peak_buffer(int *count)
{
    if(!dac_freezed)
        imx233_dma_freeze_channel(APB_AUDIO_DAC, true);
    struct imx233_dma_info_t info = imx233_dma_get_info(APB_AUDIO_DAC, DMA_INFO_AHB_BYTES | DMA_INFO_BAR);
    if(!dac_freezed)
        imx233_dma_freeze_channel(APB_AUDIO_DAC, false);
    *count = info.ahb_bytes;
    return (void *)info.bar;
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

static void *adc_buf; /* current buffer */
static size_t adc_size; /* remaining size */

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
    size_t xfer = MIN(adc_size, dma_max_size);

    adc_dma.dma.next = NULL;
    adc_dma.dma.buffer = (void *)adc_buf;
    adc_dma.dma.cmd = BF_OR(APB_CHx_CMD, COMMAND_V(WRITE),
        IRQONCMPLT(1), SEMAPHORE(1), XFER_COUNT(xfer));
    /* dma subsystem will make sure cached stuff is written to memory */
    adc_state = ADC_RECORDING;
    imx233_dma_start_command(APB_AUDIO_ADC, &adc_dma.dma);
    /* advance buffer */
    adc_buf += xfer;
    adc_size -= xfer;
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
        if(adc_size != 0 || pcm_rec_dma_complete_callback(PCM_DMAST_OK, &adc_buf, &adc_size))
        {
            rec();
            pcm_rec_dma_status_callback(PCM_DMAST_STARTED);
        }
        else
            adc_state = ADC_STOPPED;
    }

    imx233_dma_clear_channel_interrupt(APB_AUDIO_ADC);
}

void INT_ADC_ERROR(void)
{
    adc_state = ADC_STOPPED;
    pcm_rec_dma_status_callback(PCM_DMAST_ERR_DMA);
    imx233_dma_clear_channel_interrupt(APB_AUDIO_ADC);
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    pcm_rec_lock();
    /* update pending buffer */
    adc_buf = addr;
    adc_size = size;
    /* if we are stopped restart recording, otherwise IRQ will pick up */
    if(adc_state == ADC_STOPPED)
        rec();
    else
        adc_state = ADC_RECORDING;
    pcm_rec_unlock();
}

void pcm_rec_dma_stop(void)
{
    /* do not interrupt the current transaction because resetting the dma
     * would halt the ADC and clearing RUN causes sound havoc so simply
     * wait for the end of transfer */
    pcm_rec_lock();
    adc_buf = NULL;
    adc_size = 0;
    adc_state = ADC_STOP_PENDING;
    pcm_rec_unlock();
}

const void *pcm_rec_dma_get_peak_buffer(void)
{
    struct imx233_dma_info_t info = imx233_dma_get_info(APB_AUDIO_ADC, DMA_INFO_BAR);
    return (void *)info.bar;
}
#endif /* HAVE_RECORDING */
