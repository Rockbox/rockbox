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

static int locked = 0;
static struct pcm_dma_command_t dac_dma;
static bool pcm_freezed = false;

/**
 * WARNING !
 * Never reset the dma channel, otherwise it will halt the DAC for some reason
 * */

static void play(const void *addr, size_t size)
{
    dac_dma.dma.next = NULL;
    dac_dma.dma.buffer = (void *)addr;
    dac_dma.dma.cmd = HW_APB_CHx_CMD__COMMAND__READ |
        HW_APB_CHx_CMD__IRQONCMPLT |
        HW_APB_CHx_CMD__SEMAPHORE |
        size << HW_APB_CHx_CMD__XFER_COUNT_BP;
    /* dma subsystem will make sure cached stuff is written to memory */
    imx233_dma_start_command(APB_AUDIO_DAC, &dac_dma.dma);
}

void INT_DAC_DMA(void)
{
    const void *start;
    size_t size;

    if(pcm_play_dma_complete_callback(PCM_DMAST_OK, &start, &size))
    {
        play(start, size);
        pcm_play_dma_status_callback(PCM_DMAST_STARTED);
    }

    imx233_dma_clear_channel_interrupt(APB_AUDIO_DAC);
}

void INT_DAC_ERROR(void)
{
    /* TODO: Inform of error through pcm_play_dma_complete_callback */
}

void pcm_play_lock(void)
{
    if(locked++ == 0)
        imx233_dma_enable_channel_interrupt(APB_AUDIO_DAC, false);
}

void pcm_play_unlock(void)
{
    if(--locked == 0)
        imx233_dma_enable_channel_interrupt(APB_AUDIO_DAC, true);
}

void pcm_play_dma_stop(void)
{
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    pcm_play_dma_stop();

    play(addr, size);
}

void pcm_play_dma_pause(bool pause)
{
    imx233_dma_freeze_channel(APB_AUDIO_DAC, pause);
    pcm_freezed = pause;
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
    imx233_dma_enable_channel_interrupt(APB_AUDIO_DAC, true);
}

void pcm_dma_apply_settings(void)
{
    audiohw_set_frequency(pcm_fsel);
}

size_t pcm_get_bytes_waiting(void)
{
    struct imx233_dma_info_t info = imx233_dma_get_info(APB_AUDIO_DAC, DMA_INFO_AHB_BYTES);
    return info.ahb_bytes;
}

const void *pcm_play_dma_get_peak_buffer(int *count)
{
    if(!pcm_freezed)
        imx233_dma_freeze_channel(APB_AUDIO_DAC, true);
    struct imx233_dma_info_t info = imx233_dma_get_info(APB_AUDIO_DAC, DMA_INFO_AHB_BYTES | DMA_INFO_BAR);
    if(!pcm_freezed)
        imx233_dma_freeze_channel(APB_AUDIO_DAC, false);
    *count = info.ahb_bytes;
    return (void *)info.bar;
}

/*
 * Recording
 */

void pcm_rec_lock(void)
{
}

void pcm_rec_unlock(void)
{
}

void pcm_rec_dma_init(void)
{
}

void pcm_rec_dma_close(void)
{
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    (void) addr;
    (void) size;
}

/*
void pcm_rec_dma_record_more(void *start, size_t size)
{
}
*/

void pcm_rec_dma_stop(void)
{
}

const void *pcm_rec_dma_get_peak_buffer(void)
{
    return NULL;
}
