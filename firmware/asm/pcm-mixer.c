/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Michael Sevakis
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

#if defined(CPU_ARM) && !defined (CONFIG_PCM_MULTISIZE)
  #include "arm/pcm-mixer.c"
#elif defined(CPU_COLDFIRE) && !defined (CONFIG_PCM_MULTISIZE)
  #include "m68k/pcm-mixer.c"
#else /* Generic/multisize interfaces */

#include "dsp-util.h"
#include <string.h>

/* If sizes are locked at 16-bits, then most code in functions below will be
   optimized away as unreachable */

#if PCM_AMP_FRACBITS > 16
#warning 2-byte gained functions will require 64-bit integers
#endif

static FORCE_INLINE int32_t
sample_amplify_sized_shr(int32_t a, int32_t s, size_t size, int shift)
{
    if (size > 2) {
        return ((int64_t)a * s) >> shift;
    }
    else {
        return (a * s) >> shift;
    }
}

static FORCE_INLINE int32_t
sample_amplify_sized_shl(int32_t a, int32_t s, size_t size, int shift)
{
    /* right now, shl needed implies < 32 bits */
    return (a * s) << shift;
    (void)size;
}

/* SSI = Signed, Source Size, Interleaved */
/* SDI = Signed, Dest Size, Interleaved */

static FORCE_INLINE void
write_samples_unity_1ch_ssi_sdi(struct pcm_stream_desc *desc,
                                void *out,
                                unsigned long count,
                                size_t srcz)
{
    const void *src = desc->addr_tx;
    int shift = pcm_desc_sample_shift_unity(desc);
    int32_t sm;

    if (shift >= 0) {
        while (count--) {
            sm = read_intx_postidx32(&src, 1, srcz) << shift;
            pcm_dma_t_write_postidx(&out, sm, 1, 1);
            pcm_dma_t_write_postidx(&out, sm, 1, 1);
        }
    }
    else {
        shift = -shift;

        while (count--) {
            sm = read_intx_postidx32(&src, 1, srcz) >> shift;
            pcm_dma_t_write_postidx(&out, sm, 1, 1);
            pcm_dma_t_write_postidx(&out, sm, 1, 1);
        }
    }
}

static FORCE_INLINE void
write_samples_unity_2ch_ssi_sdi(struct pcm_stream_desc *desc,
                                void *out,
                                unsigned long count,
                                size_t srcz)
{
    const void *src = desc->addr_tx;
    int shift = pcm_desc_sample_shift_unity(desc);
    int32_t sl, sr;

    if (shift == 0 && pcm_desc_sample_size(desc) == PCM_DMA_T_SIZE) {
        memcpy(out, desc->addr_tx, count*PCM_DMA_T_FRAME_SIZE);
    }
    else if (shift >= 0) {
        while (count--) {
            sl = read_intx_postidx32(&src, 1, srcz) << shift;
            sr = read_intx_postidx32(&src, 1, srcz) << shift;
            pcm_dma_t_write_postidx(&out, sl, 1, 1);
            pcm_dma_t_write_postidx(&out, sr, 1, 1);
        }
    }
    else {
        shift = -shift;

        while (count--) {
            sl = read_intx_postidx32(&src, 1, srcz) >> shift;
            sr = read_intx_postidx32(&src, 1, srcz) >> shift;
            pcm_dma_t_write_postidx(&out, sl, 1, 1);
            pcm_dma_t_write_postidx(&out, sr, 1, 1);
        }
    }
}

static FORCE_INLINE void
write_samples_gain_1ch_ssi_sdi(struct pcm_stream_desc *desc,
                               void *out,
                               unsigned long count,
                               size_t srcz)
{

    const void *src = desc->addr_tx;
    int shift = pcm_desc_sample_shift_gain(desc);

    int32_t a = desc->amplitude_tx;
    int32_t sm;

    if (shift >= 0) {
        while (count--) {
            sm = read_intx_postidx32(&src, 1, srcz);
            sm = sample_amplify_sized_shl(a, sm, srcz, shift);
            pcm_dma_t_write_postidx(&out, sm, 1, 1);
            pcm_dma_t_write_postidx(&out, sm, 1, 1);
        }
    }
    else {
        shift = -shift;

        while (count--) {
            sm = read_intx_postidx32(&src, 1, srcz);
            sm = sample_amplify_sized_shr(a, sm, srcz, shift);
            pcm_dma_t_write_postidx(&out, sm, 1, 1);
            pcm_dma_t_write_postidx(&out, sm, 1, 1);
        }
    }
}

static FORCE_INLINE void
write_samples_gain_2ch_ssi_sdi(struct pcm_stream_desc *desc,
                               void *out,
                               unsigned long count,
                               size_t srcz)
{
    const void *src = desc->addr_tx;
    int shift = pcm_desc_sample_shift_gain(desc);
    int32_t a = desc->amplitude_tx;
    int32_t sl, sr;

    if (shift >= 0) {
        while (count--) {
            sl = read_intx_postidx32(&src, 1, srcz);
            sr = read_intx_postidx32(&src, 1, srcz);
            sl = sample_amplify_sized_shl(a, sl, srcz, shift);
            sr = sample_amplify_sized_shl(a, sr, srcz, shift);
            pcm_dma_t_write_postidx(&out, sl, 1, 1);
            pcm_dma_t_write_postidx(&out, sr, 1, 1);
        }

    }
    else {
        shift = -shift;

        while (count--) {
            sl = read_intx_postidx32(&src, 1, srcz);
            sr = read_intx_postidx32(&src, 1, srcz);
            sl = sample_amplify_sized_shr(a, sl, srcz, shift);
            sr = sample_amplify_sized_shr(a, sr, srcz, shift);
            pcm_dma_t_write_postidx(&out, sl, 1, 1);
            pcm_dma_t_write_postidx(&out, sr, 1, 1);
        }
    }
}

static FORCE_INLINE void
mix_samples_unity_1ch_ssi_sdi(struct pcm_stream_desc *desc,
                              void *out,
                              unsigned long count,
                              size_t srcz)
{
    const void *src = desc->addr_tx;
    int shift = pcm_desc_sample_shift_unity(desc);
    int32_t sm, dl, dr;

    if (shift >= 0) {
        while (count--) {
            sm = read_intx_postidx32(&src, 1, srcz) << shift;
            dl = pcm_dma_t_read_preidx((const void **)&out, 0, false);
            dr = pcm_dma_t_read_preidx((const void **)&out, 1, false);
            dl = pcm_dma_t_mix_clip(dl, sm);
            dr = pcm_dma_t_mix_clip(dr, sm);
            pcm_dma_t_write_postidx(&out, dl, 1, 1);
            pcm_dma_t_write_postidx(&out, dr, 1, 1);
        }
    }
    else {
        while (count--) {
            sm = read_intx_postidx32(&src, 1, srcz) >> shift;
            dl = pcm_dma_t_read_preidx((const void **)&out, 0, false);
            dr = pcm_dma_t_read_preidx((const void **)&out, 1, false);
            dl = pcm_dma_t_mix_clip(dl, sm);
            dr = pcm_dma_t_mix_clip(dr, sm);
            pcm_dma_t_write_postidx(&out, dl, 1, 1);
            pcm_dma_t_write_postidx(&out, dr, 1, 1);
        }
    }
}

static FORCE_INLINE void
mix_samples_unity_2ch_ssi_sdi(struct pcm_stream_desc *desc,
                              void *out,
                              unsigned long count,
                              size_t srcz)
{
    const void *src = desc->addr_tx;
    int shift = pcm_desc_sample_shift_unity(desc);
    int32_t sl, sr, dl, dr;

    if (shift >= 0) {
        while (count--) {
            sl = read_intx_postidx32(&src, 1, srcz) << shift;
            sr = read_intx_postidx32(&src, 1, srcz) << shift;
            dl = pcm_dma_t_read_preidx((const void **)&out, 0, false);
            dr = pcm_dma_t_read_preidx((const void **)&out, 1, false);
            dl = pcm_dma_t_mix_clip(dl, sl);
            dr = pcm_dma_t_mix_clip(dr, sr);
            pcm_dma_t_write_postidx(&out, dl, 1, 1);
            pcm_dma_t_write_postidx(&out, dr, 1, 1);
        }
    }
    else {
        while (count--) {
            sl = read_intx_postidx32(&src, 1, srcz) >> shift;
            sr = read_intx_postidx32(&src, 1, srcz) >> shift;
            dl = pcm_dma_t_read_preidx((const void **)&out, 0, false);
            dr = pcm_dma_t_read_preidx((const void **)&out, 1, false);
            dl = pcm_dma_t_mix_clip(dl, sl);
            dr = pcm_dma_t_mix_clip(dr, sr);
            pcm_dma_t_write_postidx(&out, dl, 1, 1);
            pcm_dma_t_write_postidx(&out, dr, 1, 1);
        }
    }
}

static FORCE_INLINE void
mix_samples_gain_1ch_ssi_sdi(struct pcm_stream_desc *desc,
                             void *out,
                             unsigned long count,
                             size_t srcz)
{
    const void *src = desc->addr_tx;
    int shift = pcm_desc_sample_shift_gain(desc);
    int32_t a = desc->amplitude_tx;
    int32_t sm, dl, dr;

    if (shift >= 0) {
        while (count--) {
            sm = read_intx_postidx32(&src, 1, srcz);
            sm = sample_amplify_sized_shl(a, sm, srcz, shift);
            dl = pcm_dma_t_read_preidx((const void **)&out, 0, false);
            dr = pcm_dma_t_read_preidx((const void **)&out, 1, false);
            dl = pcm_dma_t_mix_clip(dl, sm);
            dr = pcm_dma_t_mix_clip(dr, sm);
            pcm_dma_t_write_postidx(&out, dl, 1, 1);
            pcm_dma_t_write_postidx(&out, dr, 1, 1);
        }
    }
    else {
        shift = -shift;

        while (count--) {
            sm = read_intx_postidx32(&src, 1, srcz);
            sm = sample_amplify_sized_shr(a, sm, srcz, shift);
            dl = pcm_dma_t_read_preidx((const void **)&out, 0, false);
            dr = pcm_dma_t_read_preidx((const void **)&out, 1, false);
            dl = pcm_dma_t_mix_clip(dl, sm);
            dr = pcm_dma_t_mix_clip(dr, sm);
            pcm_dma_t_write_postidx(&out, dl, 1, 1);
            pcm_dma_t_write_postidx(&out, dr, 1, 1);
        }
    }
}

static FORCE_INLINE void
mix_samples_gain_2ch_ssi_sdi(struct pcm_stream_desc *desc,
                             void *out,
                             unsigned long count,
                             size_t srcz)
{
    const void *src = desc->addr_tx;
    int shift = pcm_desc_sample_shift_gain(desc);
    int32_t a = desc->amplitude_tx;
    int32_t sl, sr, dl, dr;

    if (shift >= 0) {
        while (count--) {
            sl = read_intx_postidx32(&src, 1, srcz);
            sr = read_intx_postidx32(&src, 1, srcz);
            sl = sample_amplify_sized_shl(a, sl, srcz, shift);
            sr = sample_amplify_sized_shl(a, sr, srcz, shift);
            dl = pcm_dma_t_read_preidx((const void **)&out, 0, false);
            dr = pcm_dma_t_read_preidx((const void **)&out, 1, false);
            dl = pcm_dma_t_mix_clip(dl, sl);
            dr = pcm_dma_t_mix_clip(dr, sr);
            pcm_dma_t_write_postidx(&out, dl, 1, 1);
            pcm_dma_t_write_postidx(&out, dr, 1, 1);
        }
    }
    else {
        shift = -shift;

        while (count--) {
            sl = read_intx_postidx32(&src, 1, srcz);
            sr = read_intx_postidx32(&src, 1, srcz);
            sl = sample_amplify_sized_shr(a, sl, srcz, shift);
            sr = sample_amplify_sized_shr(a, sr, srcz, shift);
            dl = pcm_dma_t_read_preidx((const void **)&out, 0, false);
            dr = pcm_dma_t_read_preidx((const void **)&out, 1, false);
            dl = pcm_dma_t_mix_clip(dl, sl);
            dr = pcm_dma_t_mix_clip(dr, sr);
            pcm_dma_t_write_postidx(&out, dl, 1, 1);
            pcm_dma_t_write_postidx(&out, dr, 1, 1);
        }
    }
}

#define PCM_MIXER_CONV_FN_DECL_SSI_SDI(wrmx, ug, nch, st, dt, sz) \
    static void wrmx##_samples_##ug##_##nch##ch_##st##_##dt( \
                    struct pcm_stream_desc *desc, void *out, unsigned long count)  \
        { wrmx##_samples_##ug##_##nch##ch_ssi_sdi(desc, out, count, sz); }

#ifdef CONFIG_PCM_MULTISIZE
#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_2_BYTE_INT)
/* Functions to handle 2-byte sources */
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, unity, 1, s2i, sdi, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, unity, 2, s2i, sdi, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, gain , 1, s2i, sdi, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, gain , 2, s2i, sdi, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , unity, 1, s2i, sdi, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , unity, 2, s2i, sdi, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , gain , 1, s2i, sdi, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , gain , 2, s2i, sdi, 2)
#endif /* (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_2_BYTE_INT) */

#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_3_BYTE_INT)
/* Functions to handle 3-byte sources */
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, unity, 1, s3i, sdi, 3)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, unity, 2, s3i, sdi, 3)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, gain , 1, s3i, sdi, 3)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, gain , 2, s3i, sdi, 3)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , unity, 1, s3i, sdi, 3)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , unity, 2, s3i, sdi, 3)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , gain , 1, s3i, sdi, 3)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , gain , 2, s3i, sdi, 3)
#endif /* (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_3_BYTE_INT) */

#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_4_BYTE_INT)
/* Functions to handle 4-byte sources */
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, unity, 1, s4i, sdi, 4)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, unity, 2, s4i, sdi, 4)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, gain , 1, s4i, sdi, 4)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, gain , 2, s4i, sdi, 4)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , unity, 1, s4i, sdi, 4)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , unity, 2, s4i, sdi, 4)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , gain , 1, s4i, sdi, 4)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , gain , 2, s4i, sdi, 4)
#endif /* (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_4_BYTE_INT) */

#else /* ndef CONFIG_PCM_MULTISIZE */

/* Functions locked to signed 16-bit interleaved in and out */
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, unity, 1, s16i, s16i, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, unity, 2, s16i, s16i, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, gain , 1, s16i, s16i, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(write, gain , 2, s16i, s16i, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , unity, 1, s16i, s16i, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , unity, 2, s16i, s16i, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , gain , 1, s16i, s16i, 2)
PCM_MIXER_CONV_FN_DECL_SSI_SDI(mix  , gain , 2, s16i, s16i, 2)

#endif /* CONFIG_PCM_MULTISIZE */

#endif /* generic */

#ifndef mixer_buffer_cleanup
#define mixer_buffer_cleanup() do {} while (0)
#endif
