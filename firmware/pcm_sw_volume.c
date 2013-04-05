/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Michael Sevakis
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
#include "system.h"
#include "pcm.h"
#include "pcm-internal.h"
#include "dsp-util.h"
#include "fixedpoint.h"
#include "pcm_sw_volume.h"

/* source buffer from client */
static const void * volatile src_buf_addr = NULL;
static size_t volatile src_buf_rem = 0;

#define PCM_PLAY_DBL_BUF_SIZE (PCM_PLAY_DBL_BUF_SAMPLE*PCM_SAMPLE_SIZE)

/* double buffer and frame length control */
static int16_t pcm_dbl_buf[2][PCM_PLAY_DBL_BUF_SAMPLES*2]
        PCM_DBL_BUF_BSS MEM_ALIGN_ATTR;
static size_t pcm_dbl_buf_size[2];
static int pcm_dbl_buf_num = 0;
static size_t frame_size;
static unsigned int frame_count, frame_err, frame_frac;

#ifdef AUDIOHW_HAVE_PRESCALER
static int32_t prescale_factor = PCM_FACTOR_UNITY;
static int32_t vol_factor_l = 0, vol_factor_r = 0;
#endif /* AUDIOHW_HAVE_PRESCALER */

/* pcm scaling factors */
static int32_t pcm_factor_l = 0, pcm_factor_r = 0;

#define PCM_FACTOR_CLIP(f) \
    MAX(MIN((f), PCM_FACTOR_MAX), PCM_FACTOR_MIN)
#define PCM_SCALE_SAMPLE(f, s) \
    (((f) * (s) + PCM_FACTOR_UNITY/2) >> PCM_FACTOR_BITS)


/* TODO: #include CPU-optimized routines and move this to /firmware/asm */
static inline void pcm_copy_buffer(int16_t *dst, const int16_t *src,
                                   size_t size)
{
    int32_t factor_l = pcm_factor_l;
    int32_t factor_r = pcm_factor_r;

    if (LIKELY(factor_l <= PCM_FACTOR_UNITY && factor_r <= PCM_FACTOR_UNITY))
    {
        /* All cut or unity */
        while (size)
        {
            *dst++ = PCM_SCALE_SAMPLE(factor_l, *src++);
            *dst++ = PCM_SCALE_SAMPLE(factor_r, *src++);
            size -= PCM_SAMPLE_SIZE;
        }
    }
    else
    {
        /* Any positive gain requires clipping */
        while (size)
        {
            *dst++ = clip_sample_16(PCM_SCALE_SAMPLE(factor_l, *src++));
            *dst++ = clip_sample_16(PCM_SCALE_SAMPLE(factor_r, *src++));
            size -= PCM_SAMPLE_SIZE;
        }
    }
}

bool pcm_play_dma_complete_callback(enum pcm_dma_status status,
                                    const void **addr, size_t *size)
{
    /* Check status callback first if error */
    if (status < PCM_DMAST_OK)
        status = pcm_play_call_status_cb(status);

    size_t sz = pcm_dbl_buf_size[pcm_dbl_buf_num];

    if (status >= PCM_DMAST_OK && sz)
    {
        /* Do next chunk */
        *addr = pcm_dbl_buf[pcm_dbl_buf_num];
        *size = sz;
        return true;
    }
    else
    {
        /* This is a stop chunk or error */
        pcm_play_stop_int();
        return false;
    }
}

/* Equitably divide large source buffers amongst double buffer frames;
   frames smaller than or equal to the double buffer chunk size will play
   in one chunk */
static void update_frame_params(size_t size)
{
    int count    = size / PCM_SAMPLE_SIZE;
    frame_count  = (count + PCM_PLAY_DBL_BUF_SAMPLES - 1) /
                   PCM_PLAY_DBL_BUF_SAMPLES;
    int perframe = count / frame_count;
    frame_size   = perframe * PCM_SAMPLE_SIZE;
    frame_frac   = count - perframe * frame_count;
    frame_err    = 0;
}

/* Obtain the next buffer and prepare it for pcm driver playback */
enum pcm_dma_status
pcm_play_dma_status_callback_int(enum pcm_dma_status status)
{
    if (status != PCM_DMAST_STARTED)
        return status;

    size_t size = pcm_dbl_buf_size[pcm_dbl_buf_num];
    const void *addr = src_buf_addr + size;

    size = src_buf_rem - size;

    if (size == 0 && pcm_get_more_int(&addr, &size))
    {
        update_frame_params(size);
        pcm_play_call_status_cb(PCM_DMAST_STARTED);
    }

    src_buf_addr = addr;
    src_buf_rem = size;

    if (size != 0)
    {
        size = frame_size;

        if ((frame_err += frame_frac) >= frame_count)
        {
            frame_err -= frame_count;
            size += PCM_SAMPLE_SIZE;
        }
    }

    pcm_dbl_buf_num ^= 1;
    pcm_dbl_buf_size[pcm_dbl_buf_num] = size;
    pcm_copy_buffer(pcm_dbl_buf[pcm_dbl_buf_num], addr, size);

    return PCM_DMAST_OK;
}

/* Prefill double buffer and start pcm driver */
static void start_pcm(bool reframe)
{
    pcm_dbl_buf_num = 0;
    pcm_dbl_buf_size[0] = 0;

    if (reframe)
        update_frame_params(src_buf_rem);

    pcm_play_dma_status_callback(PCM_DMAST_STARTED);
    pcm_play_dma_status_callback(PCM_DMAST_STARTED);

    pcm_play_dma_start(pcm_dbl_buf[1], pcm_dbl_buf_size[1]);
}

void pcm_play_dma_start_int(const void *addr, size_t size)
{
    src_buf_addr = addr;
    src_buf_rem = size;
    start_pcm(true);
}

void pcm_play_dma_pause_int(bool pause)
{
    if (pause)
        pcm_play_dma_pause(true);
    else if (src_buf_rem)
        start_pcm(false);    /* Reprocess in case volume level changed */
    else
        pcm_play_stop_int(); /* Playing frame was last frame */
}

void pcm_play_dma_stop_int(void)
{
    pcm_play_dma_stop();
    src_buf_addr = NULL;
    src_buf_rem = 0;
}

/* Return playing buffer from the source buffer */
const void * pcm_play_dma_get_peak_buffer_int(int *count)
{
    const void *addr = src_buf_addr;
    size_t size = src_buf_rem;
    const void *addr2 = src_buf_addr;

    if (addr == addr2 && size)
    {
        *count = size / PCM_SAMPLE_SIZE;
        return addr;
    }

    *count = 0;
    return NULL;
}

/* Return the scale factor corresponding to the centibel level */
static int32_t pcm_centibels_to_factor(int volume)
{
    if (volume == PCM_MUTE_LEVEL)
        return 0; /* mute */

    /* Centibels -> fixedpoint */
    return fp_factor(PCM_FACTOR_UNITY*volume / 10, PCM_FACTOR_BITS);
}

#ifdef AUDIOHW_HAVE_PRESCALER
/* Produce final pcm scale factor */
static void pcm_sync_prescaler(void)
{
    int32_t factor_l = fp_mul(prescale_factor, vol_factor_l, PCM_FACTOR_BITS);
    int32_t factor_r = fp_mul(prescale_factor, vol_factor_r, PCM_FACTOR_BITS);
    pcm_factor_l = PCM_FACTOR_CLIP(factor_l);
    pcm_factor_r = PCM_FACTOR_CLIP(factor_r);
}

/* Set the prescaler value for all PCM playback */
void pcm_set_prescaler(int prescale)
{
    prescale_factor = pcm_centibels_to_factor(-prescale);
    pcm_sync_prescaler();
}

/* Set the per-channel volume cut/gain for all PCM playback */
void pcm_set_master_volume(int vol_l, int vol_r)
{
    vol_factor_l = pcm_centibels_to_factor(vol_l);
    vol_factor_r = pcm_centibels_to_factor(vol_r);
    pcm_sync_prescaler();
}

#else /* ndef AUDIOHW_HAVE_PRESCALER */

/* Set the per-channel volume cut/gain for all PCM playback */
void pcm_set_master_volume(int vol_l, int vol_r)
{
    int32_t factor_l = pcm_centibels_to_factor(vol_l);
    int32_t factor_r = pcm_centibels_to_factor(vol_r);
    pcm_factor_l = PCM_FACTOR_CLIP(factor_l);
    pcm_factor_r = PCM_FACTOR_CLIP(factor_r);
}
#endif /* AUDIOHW_HAVE_PRESCALER */
