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

/* volume factors set by pcm_set_master_volume */
static uint32_t vol_factor_l = 0, vol_factor_r = 0;

#ifdef AUDIOHW_HAVE_PRESCALER
/* prescale factor set by pcm_set_prescaler */
static uint32_t prescale_factor = PCM_FACTOR_UNITY;
#endif /* AUDIOHW_HAVE_PRESCALER */

/* final pcm scaling factors */
static uint32_t pcm_new_factor_l = 0, pcm_new_factor_r = 0;
static uint32_t pcm_factor_l = 0, pcm_factor_r = 0;
static typeof (memcpy) *pcm_scaling_fn = NULL;

/***
 ** Volume scaling routines
 ** If unbuffered, called externally by pcm driver
 **/

/* TODO: #include CPU-optimized routines and move this to /firmware/asm */
#if PCM_SW_VOLUME_FRACBITS <= 16
#define PCM_F_T int32_t
#else
#define PCM_F_T int64_t /* Requires large integer math */
#endif /* PCM_SW_VOLUME_FRACBITS */

/* Scale and round sample by PCM factor */
static inline int32_t pcm_scale_sample(PCM_F_T f, int32_t s)
{
    return (f * s + (PCM_F_T)PCM_FACTOR_UNITY/2) >> PCM_SW_VOLUME_FRACBITS;
}

/* Both UNITY, use direct copy */
/* static void * memcpy(void *dst, const void *src, size_t size); */

/* Either cut (both <= UNITY), no clipping needed */
static void * pcm_scale_buffer_cut(void *dst, const void *src, size_t size)
{
    int16_t *d = dst;
    const int16_t *s = src;
    uint32_t factor_l = pcm_factor_l, factor_r = pcm_factor_r;

    while (size)
    {
        *d++ = pcm_scale_sample(factor_l, *s++);
        *d++ = pcm_scale_sample(factor_r, *s++);
        size -= PCM_SAMPLE_SIZE;
    }

    return dst;
}

/* Either boost (any > UNITY) requires clipping */
static void * pcm_scale_buffer_boost(void *dst, const void *src, size_t size)
{
    int16_t *d = dst;
    const int16_t *s = src;
    uint32_t factor_l = pcm_factor_l, factor_r = pcm_factor_r;

    while (size)
    {
        *d++ = clip_sample_16(pcm_scale_sample(factor_l, *s++));
        *d++ = clip_sample_16(pcm_scale_sample(factor_r, *s++));
        size -= PCM_SAMPLE_SIZE;
    }

    return dst;
}

/* Transition the volume change smoothly across a frame */
static void * pcm_scale_buffer_trans(void *dst, const void *src, size_t size)
{
    int16_t *d = dst;
    const int16_t *s = src;
    uint32_t factor_l = pcm_factor_l, factor_r = pcm_factor_r;

    /* Transition from the old value to the new value using an inverted cosinus
       from PI..0 in order to minimize amplitude-modulated harmonics generation
       (zipper effects). */
    uint32_t new_factor_l = pcm_new_factor_l;
    uint32_t new_factor_r = pcm_new_factor_r;

    int32_t diff_l = (int32_t)new_factor_l - (int32_t)factor_l;
    int32_t diff_r = (int32_t)new_factor_r - (int32_t)factor_r;

    for (size_t done = 0; done < size; done += PCM_SAMPLE_SIZE)
    {
        int32_t sweep = (1 << 14) - fp14_cos(180*done / size); /* 0.0..2.0 */
        uint32_t f_l = fp_mul(sweep, diff_l, 15) + factor_l;
        uint32_t f_r = fp_mul(sweep, diff_r, 15) + factor_r;
        *d++ = clip_sample_16(pcm_scale_sample(f_l, *s++));
        *d++ = clip_sample_16(pcm_scale_sample(f_r, *s++));
    }

    /* Select steady-state operation */
    pcm_sync_pcm_factors();

    return dst;
}

/* Called by completion routine to scale the next buffer of samples */
#ifndef PCM_SW_VOLUME_UNBUFFERED
static inline
#endif
void pcm_sw_volume_copy_buffer(void *dst, const void *src, size_t size)
{
    pcm_scaling_fn(dst, src, size);
}

/* Assign the new scaling function for normal steady-state operation */
void pcm_sync_pcm_factors(void)
{
    uint32_t new_factor_l = pcm_new_factor_l;
    uint32_t new_factor_r = pcm_new_factor_r;

    pcm_factor_l = new_factor_l;
    pcm_factor_r = new_factor_r;

    if (new_factor_l == PCM_FACTOR_UNITY &&
        new_factor_r == PCM_FACTOR_UNITY)
    {
        pcm_scaling_fn = memcpy;
    }
    else if (new_factor_l <= PCM_FACTOR_UNITY &&
             new_factor_r <= PCM_FACTOR_UNITY)
    {
        pcm_scaling_fn = pcm_scale_buffer_cut;
    }
    else
    {
        pcm_scaling_fn = pcm_scale_buffer_boost;
    }
}

#ifndef PCM_SW_VOLUME_UNBUFFERED
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

/** Overrides of certain functions in pcm.c and pcm-internal.h **/

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
    pcm_sw_volume_copy_buffer(pcm_dbl_buf[pcm_dbl_buf_num], addr, size);

    return PCM_DMAST_OK;
}

/* Prefill double buffer and start pcm driver */
static void start_pcm(bool reframe)
{
    /* Smoothed transition might not have happened so sync now */
    pcm_sync_pcm_factors();

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

#endif /* PCM_SW_VOLUME_UNBUFFERED */


/** Internal **/

/* Return the scale factor corresponding to the centibel level */
static uint32_t pcm_centibels_to_factor(int volume)
{
    if (volume == PCM_MUTE_LEVEL)
        return 0; /* mute */

    /* Centibels -> fixedpoint */
    return (uint32_t)fp_factor(fp_div(volume, 10, PCM_SW_VOLUME_FRACBITS),
                               PCM_SW_VOLUME_FRACBITS);
}


/** Public functions **/

/* Produce final pcm scale factor */
static void pcm_sync_prescaler(void)
{
    uint32_t factor_l = vol_factor_l;
    uint32_t factor_r = vol_factor_r;
#ifdef AUDIOHW_HAVE_PRESCALER
    factor_l = fp_mul(prescale_factor, factor_l, PCM_SW_VOLUME_FRACBITS);
    factor_r = fp_mul(prescale_factor, factor_r, PCM_SW_VOLUME_FRACBITS);
#endif
    pcm_play_lock();

    pcm_new_factor_l = MIN(factor_l, PCM_FACTOR_MAX);
    pcm_new_factor_r = MIN(factor_r, PCM_FACTOR_MAX);

    if (pcm_new_factor_l != pcm_factor_l || pcm_new_factor_r != pcm_factor_r)
        pcm_scaling_fn = pcm_scale_buffer_trans;

    pcm_play_unlock();
}

#ifdef AUDIOHW_HAVE_PRESCALER
/* Set the prescaler value for all PCM playback */
void pcm_set_prescaler(int prescale)
{
    prescale_factor = pcm_centibels_to_factor(-prescale);
    pcm_sync_prescaler();
}
#endif /* AUDIOHW_HAVE_PRESCALER */

/* Set the per-channel volume cut/gain for all PCM playback */
void pcm_set_master_volume(int vol_l, int vol_r)
{
    vol_factor_l = pcm_centibels_to_factor(vol_l);
    vol_factor_r = pcm_centibels_to_factor(vol_r);
    pcm_sync_prescaler();
}
