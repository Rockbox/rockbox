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
#include "pcm-internal.h"
#include "dsp-util.h"
#include "fixedpoint.h"
#include "pcm_sw_volume.h"

/* volume factors set by pcm_set_master_volume */
static int32_t vol_factor_l = 0, vol_factor_r = 0;

#ifdef AUDIOHW_HAVE_PRESCALER
/* prescale factor set by pcm_set_prescaler */
static int32_t prescale_factor = PCM_SW_VOLUME_FACTOR_UNITY;
#endif /* AUDIOHW_HAVE_PRESCALER */

/* final pcm scaling factors */
static int32_t pcm_new_factor_l = 0, pcm_new_factor_r = 0;
static int32_t pcm_factor_l = 0, pcm_factor_r = 0;
void (* pcm_sw_volume_buffer_amp_fn)(void *, unsigned long) = NULL;

#if PCM_SW_VOLUME_FACTOR_MAX > PCM_SW_VOLUME_FACTOR_UNITY
/* Sometimes used as a balance control only */
#define SW_VOL_NEED_BOOST
#endif

/***
 ** Volume scaling routines
 ** If unbuffered, called externally by pcm driver
 **/

/* Apply the amplification factor */
static FORCE_INLINE int32_t
sw_volume_sample_amp(int32_t f, int32_t s)
{
    return pcm_dma_t_amplify(s, f, PCM_SW_VOLUME_FRACBITS);
}

static FORCE_INLINE int32_t
sw_volume_sample_amp_clip(int32_t f, int32_t s)
{
#ifdef SW_VOL_NEED_BOOST
    return pcm_dma_t_amplify_clip(s, f, PCM_SW_VOLUME_FRACBITS);
#else
    return pcm_dma_t_amplify(s, f, PCM_SW_VOLUME_FRACBITS);
#endif
}

#ifdef SW_VOL_NEED_BOOST
/* Either boost (any > UNITY) requires clipping */
static void sw_volume_buffer_boost(void *buffer, unsigned long count)
{
    int32_t factor_l = pcm_factor_l, factor_r = pcm_factor_r;

    while (count--)
    {
        int32_t ch0 = pcm_dma_t_read_preidx((const void **)&buffer, 0, false);
        int32_t ch1 = pcm_dma_t_read_preidx((const void **)&buffer, 1, false);

        ch0 = sw_volume_sample_amp_clip(factor_l, ch0);
        ch1 = sw_volume_sample_amp_clip(factor_r, ch1);

        pcm_dma_t_write_postidx(&buffer, ch0, 1, 1);
        pcm_dma_t_write_postidx(&buffer, ch1, 1, 1);
    }
}
#endif /* SW_VOL_NEED_BOOST */

/* Either cut (both <= UNITY), no clipping needed */
static void sw_volume_buffer_cut(void *buffer, unsigned long count)
{
    int32_t factor_l = pcm_factor_l, factor_r = pcm_factor_r;

    while (count--)
    {
        int32_t ch0 = pcm_dma_t_read_preidx((const void **)&buffer, 0, false);
        int32_t ch1 = pcm_dma_t_read_preidx((const void **)&buffer, 1, false);

        ch0 = sw_volume_sample_amp(factor_l, ch0);
        ch1 = sw_volume_sample_amp(factor_r, ch1);

        pcm_dma_t_write_postidx(&buffer, ch0, 1, 1);
        pcm_dma_t_write_postidx(&buffer, ch1, 1, 1);
    }
}

/* Transition the volume change smoothly across a frame */
static void sw_volume_buffer_trans(void *buffer, unsigned long count)
{
    int32_t factor_l = pcm_factor_l, factor_r = pcm_factor_r;

    /* Transition from the old value to the new value using an inverted cosinus
       from PI..0 in order to minimize amplitude-modulated harmonics generation
       (zipper effects). */
    int32_t new_factor_l = pcm_new_factor_l;
    int32_t new_factor_r = pcm_new_factor_r;

    int32_t diff_l = new_factor_l - factor_l;
    int32_t diff_r = new_factor_r - factor_r;

    for (unsigned long done = 0; done < count; done++)
    {
        int32_t sweep = (1 << 14) - fp14_cos(180*done / count); /* 0.0..2.0 */
        int32_t f_l = fp_mul(sweep, diff_l, 15) + factor_l;
        int32_t f_r = fp_mul(sweep, diff_r, 15) + factor_r;

        int32_t ch0 = pcm_dma_t_read_preidx((const void **)&buffer, 0, false);
        int32_t ch1 = pcm_dma_t_read_preidx((const void **)&buffer, 1, false);

        ch0 = sw_volume_sample_amp_clip(f_l, ch0);
        ch1 = sw_volume_sample_amp_clip(f_r, ch1);

        pcm_dma_t_write_postidx(&buffer, ch0, 1, 1);
        pcm_dma_t_write_postidx(&buffer, ch1, 1, 1);
    }

    /* Select steady-state operation */
    pcm_sw_volume_sync_factors();
}


/** Internal **/

/* Assign the new scaling function for normal steady-state operation */
void pcm_sw_volume_sync_factors(void)
{
    int32_t new_factor_l = pcm_new_factor_l;
    int32_t new_factor_r = pcm_new_factor_r;

    pcm_factor_l = new_factor_l;
    pcm_factor_r = new_factor_r;

#ifdef SW_VOL_NEED_BOOST
    if (new_factor_l > PCM_SW_VOLUME_FACTOR_UNITY ||
        new_factor_r > PCM_SW_VOLUME_FACTOR_UNITY)
    {
        pcm_sw_volume_buffer_amp_fn = sw_volume_buffer_boost;
    }
    else
#endif /* SW_VOL_NEED_BOOST */
    if (new_factor_l < PCM_SW_VOLUME_FACTOR_UNITY ||
        new_factor_r < PCM_SW_VOLUME_FACTOR_UNITY)
    {
        pcm_sw_volume_buffer_amp_fn = sw_volume_buffer_cut;
    }
    else
    {
        pcm_sw_volume_buffer_amp_fn = NULL;
    }
}

/* Return the scale factor corresponding to the centibel level */
static int32_t pcm_centibels_to_factor(int volume)
{
    if (volume == PCM_MUTE_LEVEL)
        return 0; /* mute */

    /* Centibels -> fixedpoint */
    return (int32_t)fp_factor(fp_div(volume, 10, PCM_SW_VOLUME_FRACBITS),
                              PCM_SW_VOLUME_FRACBITS);
}

/* Produce final pcm scale factor */
static void pcm_sync_prescaler(void)
{
    int32_t factor_l = vol_factor_l;
    int32_t factor_r = vol_factor_r;
#ifdef AUDIOHW_HAVE_PRESCALER
    factor_l = fp_mul(prescale_factor, factor_l, PCM_SW_VOLUME_FRACBITS);
    factor_r = fp_mul(prescale_factor, factor_r, PCM_SW_VOLUME_FRACBITS);
#endif
    pcm_play_lock_device_callback();

    pcm_new_factor_l = MIN(factor_l, PCM_SW_VOLUME_FACTOR_MAX);
    pcm_new_factor_r = MIN(factor_r, PCM_SW_VOLUME_FACTOR_MAX);

    if (pcm_new_factor_l != pcm_factor_l || pcm_new_factor_r != pcm_factor_r)
        pcm_sw_volume_buffer_amp_fn = sw_volume_buffer_trans;

    pcm_play_unlock_device_callback();
}


/** Public functions **/

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
