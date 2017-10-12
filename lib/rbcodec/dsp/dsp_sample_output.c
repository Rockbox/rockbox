/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Miika Pekkarinen
 * Copyright (C) 2012 Michael Sevakis
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
#include "rbcodecconfig.h"
#include "platform.h"
#include "dsp_core.h"
#include "dsp_sample_io.h"
#include "dsp_proc_entry.h"
#include "dsp-util.h"
#include <string.h>

#if 0
#include <debug.h>
#else
#undef DEBUGF
#define DEBUGF(...)
#endif

static const dsp_pcm_format_t dsp_out_default_pcm_format =
    DSP_PCM_FORMAT_INIT(DSP_OUT_DEFAULT_PCM_FORMAT);

/* Functions may be implemented in here or externally
 *
 * If externally, they'll be macroed to the actual function names
 */

#ifdef CONFIG_DSP_OUT_2BYTE_INT
#ifdef CONFIG_DSP_OUT_CHNUM
#ifdef sample_output_1ch_1ch_2i
void sample_output_1ch_1ch_2i(struct sample_io_data *this,
                              struct dsp_buffer *src,
                              struct dsp_buffer *dst);
#endif /* sample_output_1ch_1ch_2i */
#endif /* CONFIG_DSP_OUT_CHNUM */

#ifdef sample_output_1ch_2ch_2i
void sample_output_1ch_2ch_2i(struct sample_io_data *this,
                              struct dsp_buffer *src,
                              struct dsp_buffer *dst);
#endif /* sample_output_1ch_2ch_2i */
#ifdef sample_output_2ch_2ch_2i
void sample_output_2ch_2ch_2i(struct sample_io_data *this,
                              struct dsp_buffer *src,
                              struct dsp_buffer *dst);
#endif /* sample_output_2ch_2ch_2i */

#ifdef CONFIG_DSP_OUT_CHNUM
#ifdef sample_output_dith_1ch_1ch_2i
void sample_output_dith_1ch_1ch_2i(struct sample_io_data *this,
                                   struct dsp_buffer *src,
                                   struct dsp_buffer *dst);
#else /* ndef sample_output_dith_1ch_1ch_2i */
#define sample_output_dith_1ch_1ch_2i sample_output_dith_nch_nch_2i
#define DSP_NEED_DEFAULT_DITH
#endif /* sample_output_dith_1ch_1ch_2i */
#endif /* CONFIG_DSP_OUT_CHNUM */

#ifdef sample_output_dith_1ch_2ch_2i
void sample_output_dith_1ch_2ch_2i(struct sample_io_data *this,
                                   struct dsp_buffer *src,
                                   struct dsp_buffer *dst);
#else /* ndef sample_output_dith_1ch_2ch_2i */
#define sample_output_dith_1ch_2ch_2i sample_output_dith_nch_nch_2i
#define DSP_NEED_DEFAULT_DITH
#endif /* sample_output_dith_1ch_2ch_2i */

#ifdef sample_output_dith_2ch_2ch_2i
void sample_output_dith_2ch_2ch_2i(struct sample_io_data *this,
                                   struct dsp_buffer *src,
                                   struct dsp_buffer *dst);
#else /* ndef sample_output_dith_2ch_2ch_2i */
#define sample_output_dith_2ch_2ch_2i sample_output_dith_nch_nch_2i
#define DSP_NEED_DEFAULT_DITH
#endif /* sample_output_dith_2ch_2ch_2i */
#endif /* CONFIG_DSP_OUT_2BYTE_INT */

#ifdef CONFIG_DSP_OUT_3BYTE_INT
#ifdef CONFIG_DSP_OUT_CHNUM
#ifdef sample_output_1ch_1ch_3i
void sample_output_1ch_1ch_3i(struct sample_io_data *this,
                              struct dsp_buffer *src,
                              struct dsp_buffer *dst);
#endif /* sample_output_1ch_1ch_3i */
#endif /* CONFIG_DSP_OUT_CHNUM */
#ifdef sample_output_1ch_2ch_3i
void sample_output_1ch_2ch_3i(struct sample_io_data *this,
                              struct dsp_buffer *src,
                              struct dsp_buffer *dst);
#endif /* sample_output_1ch_3ch_3i */
#ifdef sample_output_2ch_2ch_3i
void sample_output_2ch_2ch_3i(struct sample_io_data *this,
                              struct dsp_buffer *src,
                              struct dsp_buffer *dst);
#endif /* sample_output_2ch_2ch_3i */
#endif /* CONFIG_DSP_OUT_3BYTE_INT */

#ifdef CONFIG_DSP_OUT_4BYTE_INT
#ifdef CONFIG_DSP_OUT_CHNUM
#ifdef sample_output_1ch_1ch_4i
void sample_output_1ch_1ch_4i(struct sample_io_data *this,
                              struct dsp_buffer *src,
                              struct dsp_buffer *dst);
#endif /* sample_output_1ch_1ch_4i */
#endif /* CONFIG_DSP_OUT_CHNUM */
#ifdef sample_output_1ch_2ch_4i
void sample_output_1ch_2ch_4i(struct sample_io_data *this,
                              struct dsp_buffer *src,
                              struct dsp_buffer *dst);
#endif /* sample_output_1ch_2ch_4i */
#ifdef sample_output_2ch_2ch_4i
void sample_output_2ch_2ch_4i(struct sample_io_data *this,
                              struct dsp_buffer *src,
                              struct dsp_buffer *dst);
#endif /* sample_output_2ch_2ch_4i */
#endif /* CONFIG_DSP_OUT_4BYTE_INT */

/** Sample output **/

/* for when it doesn't like what you're doing */
static void sample_output_null(struct sample_io_data *this,
                               struct dsp_buffer *src,
                               struct dsp_buffer *dst)
{
    (void)this; (void)src; (void)dst;
}

#ifdef CONFIG_DSP_OUT_2BYTE_INT
#ifdef CONFIG_DSP_OUT_CHNUM
#ifndef sample_output_1ch_1ch_2i
/* write 1-channel internal format to 1-channel output format */
static void sample_output_1ch_1ch_2i(struct sample_io_data *this,
                                         struct dsp_buffer *src,
                                         struct dsp_buffer *dst)
{
    unsigned long count = this->frames_out;
    const int32_t *s0 = src->p32[0];
    int16_t *d = dst->pout;
    int scale = -this->out_pcm_shift;
    int32_t dc_bias = (long)(!!scale) << (scale - 1);

    do
    {
        *d++ = clip_sample_shr(*s0++, scale, 16, dc_bias);
    }
    while (--count);
}
#endif /* sample_output_1ch_1ch_2i */
#endif /* CONFIG_DSP_OUT_CHNUM */

#ifndef sample_output_1ch_2ch_2i
/* write 1-channel internal format to 2-channel interleaved output format */
static void sample_output_1ch_2ch_2i(struct sample_io_data *this,
                                     struct dsp_buffer *src,
                                     struct dsp_buffer *dst)
{
    unsigned long count = this->frames_out;
    const int32_t *s0 = src->p32[0];
    int16_t *d = dst->pout;
    int scale = -this->out_pcm_shift;
    int32_t dc_bias = (long)(!!scale) << (scale - 1);

    do
    {
        int32_t lr = clip_sample_shr(*s0++, scale, 16, dc_bias);
        *d++ = lr;
        *d++ = lr;
    }
    while (--count);
}
#endif /* sample_output_1ch_2ch_2i_shr */

#ifndef sample_output_2ch_2ch_2i
/* write 2-channel non-interleaved stereo internal format to 2-channel
   interleaved output format */
static void sample_output_2ch_2ch_2i(struct sample_io_data *this,
                                     struct dsp_buffer *src,
                                     struct dsp_buffer *dst)
{
    unsigned long count = this->frames_out;
    const int32_t *s0 = src->p32[0];
    const int32_t *s1 = src->p32[1];
    int16_t *d = dst->pout;
    int scale = -this->out_pcm_shift;
    int32_t dc_bias = (long)(!!scale) << (scale - 1);

    do
    {
        *d++ = clip_sample_shr(*s0++, scale, 16, dc_bias);
        *d++ = clip_sample_shr(*s1++, scale, 16, dc_bias);
    }
    while (--count);
}
#endif /* sample_output_2ch_2ch_2i */
#endif /* CONFIG_DSP_OUT_2BYTE_INT */

#ifdef CONFIG_DSP_OUT_3BYTE_INT
#ifdef CONFIG_DSP_OUT_CHNUM
#ifndef sample_output_1ch_1ch_3i
/* write 1-channel internal format to 1-channel output format */
static void sample_output_1ch_1ch_3i(struct sample_io_data *this,
                                     struct dsp_buffer *src,
                                     struct dsp_buffer *dst)
{
    unsigned long count = this->frames_out;
    const int32_t *s0 = src->p32[0];
    void *d = dst->pout;
    int scale = this->out_pcm_shift;

    if (scale >= 0)
    {
        unsigned int inbits = src->format.frac_bits + 1;

        do
        {
            int32_t s = clip_sample_shl(*s0++, scale, inbits);
            write_int24_postidx(&d, s, 1);
        }
        while (--count);
    }
    else
    {
        scale = -scale;
        int32_t dc_bias = 1L << (scale - 1);
        unsigned int outbits = this->out_pcm_bits;

        do
        {
            int32_t s = clip_sample_shr(*s0++, scale, outbits, dc_bias);
            write_int24_postidx(&d, s, 1);
        }
        while (--count);
    }
}
#endif /* sample_output_1ch_1ch_3i */
#endif /* CONFIG_DSP_OUT_CHNUM */

#ifndef sample_output_1ch_2ch_3i
/* write mono internal format to output format */
static void sample_output_1ch_2ch_3i(struct sample_io_data *this,
                                     struct dsp_buffer *src,
                                     struct dsp_buffer *dst)
{
    unsigned long count = this->frames_out;
    const int32_t *s0 = src->p32[0];
    void *d = dst->pout;
    int scale = this->out_pcm_shift;

    if (scale >= 0)
    {
        unsigned int inbits = src->format.frac_bits + 1;

        do
        {
            int32_t s = clip_sample_shl(*s0++, scale, inbits);
            write_int24_postidx(&d, s, 1);
            write_int24_postidx(&d, s, 1);
        }
        while (--count);
    }
    else
    {
        scale = -scale;
        int32_t dc_bias = 1L << (scale - 1);
        unsigned int outbits = this->out_pcm_bits;

        do
        {
            int32_t s = clip_sample_shr(*s0++, scale, outbits, dc_bias);
            write_int24_postidx(&d, s, 1);
            write_int24_postidx(&d, s, 1);
        }
        while (--count);
    }
}
#endif /* sample_output_1ch_2ch_3i */

#ifndef sample_output_2ch_2ch_3i
/* write 2-channel non-interleaved stereo internal format to 2-channel
   interleaved output format */
static void sample_output_2ch_2ch_3i(struct sample_io_data *this,
                                     struct dsp_buffer *src,
                                     struct dsp_buffer *dst)
{
    unsigned long count = this->frames_out;
    const int32_t *s0 = src->p32[0];
    const int32_t *s1 = src->p32[1];
    void *d = dst->pout;
    int scale = this->out_pcm_shift;

    if (scale >= 0)
    {
        unsigned int inbits = src->format.frac_bits + 1;

        do
        {
            int32_t l = clip_sample_shl(*s0++, scale, inbits);
            int32_t r = clip_sample_shl(*s1++, scale, inbits);
            write_int24_postidx(&d, l, 1);
            write_int24_postidx(&d, r, 1);
        }
        while (--count);
    }
    else
    {
        scale = -scale;
        int32_t dc_bias = 1L << (scale - 1);
        unsigned int outbits = this->out_pcm_bits;

        do
        {
            int32_t l = clip_sample_shr(*s0++, scale, outbits, dc_bias);
            int32_t r = clip_sample_shr(*s1++, scale, outbits, dc_bias);
            write_int24_postidx(&d, l, 1);
            write_int24_postidx(&d, r, 1);
        }
        while (--count);
    }
}
#endif /* sample_output_2ch_2ch_3i */
#endif /* CONFIG_DSP_OUT_3BYTE_INT */

#ifdef CONFIG_DSP_OUT_4BYTE_INT
#ifdef CONFIG_DSP_OUT_CHNUM
#ifndef sample_output_1ch_1ch_4i
/* write 1-channel internal format to 1-channel output format */
static void sample_output_1ch_1ch_4i(struct sample_io_data *this,
                                     struct dsp_buffer *src,
                                     struct dsp_buffer *dst)
{
    unsigned long count = this->frames_out;
    const int32_t *s0 = src->p32[0];
    int32_t *d = dst->pout;
    int scale = this->out_pcm_shift;

    if (scale >= 0)
    {
        unsigned int inbits = src->format.frac_bits + 1;

        do
        {
            *d++ = clip_sample_shl(*s0++, scale, inbits);
        }
        while (--count);
    }
    else
    {
        scale = -scale;
        int32_t dc_bias = 1L << (scale - 1);
        unsigned int outbits = this->out_pcm_bits;

        do
        {
            *d++ = clip_sample_shr(*s0++, scale, outbits, dc_bias);
        }
        while (--count);
    }
}
#endif /* sample_output_1ch_1ch_4i */
#endif /* CONFIG_DSP_OUT_CHNUM */

#ifndef sample_output_1ch_2ch_4i
/* write mono internal format to output format */
static void sample_output_1ch_2ch_4i(struct sample_io_data *this,
                                     struct dsp_buffer *src,
                                     struct dsp_buffer *dst)
{
    unsigned long count = this->frames_out;
    const int32_t *s0 = src->p32[0];
    int32_t *d = dst->pout;
    int scale = this->out_pcm_shift;

    if (scale >= 0)
    {
        unsigned int inbits = src->format.frac_bits + 1;

        do
        {
            int32_t s = clip_sample_shl(*s0++, scale, inbits);
            *d++ = s;
            *d++ = s;
        }
        while (--count);
    }
    else
    {
        scale = -scale;
        int32_t dc_bias = 1L << (scale - 1);
        unsigned int outbits = this->out_pcm_bits;

        do
        {
            *d++ = clip_sample_shr(*s0++, scale, outbits, dc_bias);
        }
        while (--count);
    }
}
#endif /* sample_output_1ch_2ch_4i */

#ifndef sample_output_2ch_2ch_4i
/* write 2-channel non-interleaved stereo internal format to 2-channel
   interleaved output format */
static void sample_output_2ch_2ch_4i(struct sample_io_data *this,
                                     struct dsp_buffer *src,
                                     struct dsp_buffer *dst)
{
    unsigned long count = this->frames_out;
    const int32_t *s0 = src->p32[0];
    const int32_t *s1 = src->p32[1];
    int32_t *d = dst->pout;
    int scale = this->out_pcm_shift;

    if (scale >= 0)
    {
        unsigned int inbits = src->format.frac_bits + 1;

        do
        {
            *d++ = clip_sample_shl(*s0++, scale, inbits);
            *d++ = clip_sample_shl(*s1++, scale, inbits);
        }
        while (--count);
    }
    else
    {
        scale = -scale;
        int32_t dc_bias = 1L << (scale - 1);
        unsigned int outbits = this->out_pcm_bits;

        do
        {
            *d++ = clip_sample_shr(*s0++, scale, outbits, dc_bias);
            *d++ = clip_sample_shr(*s1++, scale, outbits, dc_bias);
        }
        while (--count);
    }
}
#endif /* sample_output_2ch_2ch_4i */
#endif /* CONFIG_DSP_OUT_4BYTE_INT */

/**
 * The "dither" code to convert the 24-bit samples produced by libmad was
 * taken from the coolplayer project - coolplayer.sourceforge.net
 *
 * This function handles mono and stereo outputs.
 *
 * Only used when downscaling, if enabled
 */
static struct dither_data
{
    struct dither_state
    {
        long error[3];  /* 00h: error term history */
        long random;    /* 0ch: last random value */
    } state[2];         /* 0=left, 1=right */
    bool enabled;       /* 20h: dithered output enabled */
                        /* 24h */
} dither_data IBSS_ATTR;

#ifdef DSP_NEED_DEFAULT_DITH
static void sample_output_dith_nch_nch_2i(struct sample_io_data *this,
                                          struct dsp_buffer *src,
                                          struct dsp_buffer *dst)
{
    unsigned long count = this->frames_out;
    unsigned int in_chnum = src->format.num_channels;
#ifdef CONFIG_DSP_OUT_CHNUM
    unsigned int out_chnum = this->out_pcm_chnum;
#else
    unsigned int out_chnum = 2;
#endif

    int scale = -this->out_pcm_shift;
    int32_t dc_bias = 1L << (scale - 1); /* 1/2 bit of significance */
    int32_t mask = (1L << scale) - 1; /* Mask of bits quantized away */

    for (unsigned int ch = 0; ch < in_chnum; ch++)
    {
        struct dither_state *dither = &dither_data.state[ch];

        const int32_t *s = src->p32[ch];
        int16_t *d = (int16_t *)dst->pout + ch;

        for (unsigned long i = 0; i < count; i++)
        {
            /* Noise shape and bias (for correct rounding later) */
            int32_t sample = *s++;

            sample += dither->error[0] - dither->error[1] + dither->error[2];
            dither->error[2] = dither->error[1];
            dither->error[1] = dither->error[0] / 2;

            int32_t output = sample + dc_bias;

            /* Dither, highpass triangle PDF */
            int32_t random = dither->random*0x0019660dL + 0x3c6ef35fL;
            output += (random & mask) - (dither->random & mask);
            dither->random = random;

            /* Quantize sample to output range */
            output >>= scale;

            /* Error feedback of quantization */
            dither->error[0] = sample - (output << scale);

            /* Clip and store */
            *d = clip_sample_16(output);
            d += out_chnum;
        }
    }

    if (in_chnum == 1 && out_chnum == 2)
    {
        /* Have to duplicate left samples into the right channel since
           output is interleaved stereo */
        int16_t *p = dst->pout;

        do
        {
            int32_t s = *p++;
            *p++ = s;
        }
        while (--count);
    }
}
#endif /* DSP_NEED_DEFAULT_DITH */

static inline void force_output_format_change(struct sample_io_data *this)
{
    this->output_version = 0;
}

static void INIT_ATTR init_sample_output(struct sample_io_data *this)
{
    const dsp_pcm_format_t *format = &dsp_out_default_pcm_format;
    this->output_sampr  = DSP_OUT_DEFAULT_HZ;
    this->out_pcm_chnum = DSP_PCM_FORMAT_GET_CHNUM(*format);
    this->out_pcm_bits  = DSP_PCM_FORMAT_GET_BITS(*format);
    this->out_pcm_size  = DSP_PCM_FORMAT_GET_SIZE(*format);
    this->out_pcm_shift = (int)this->out_pcm_bits -
                          (int)(this->format.frac_bits + 1);
    force_output_format_change(this);
}

static void flush_sample_output(struct sample_io_data *this)
{
    if (dsp_get_id((void *)this) == CODEC_IDX_AUDIO)
        memset(dither_data.state, 0, sizeof (dither_data.state));
}

static int set_out_pcm_format(struct sample_io_data *this,
                              const dsp_pcm_format_t *out_format)
{
    if (!out_format)
        out_format = &dsp_out_default_pcm_format;

    /* NOTE: This isn't checking all fields at this time. Adding unsigned
     * or float formats will require remembering a couple other details. */
    unsigned int bits  = DSP_PCM_FORMAT_GET_BITS(*out_format);
    unsigned int chnum = DSP_PCM_FORMAT_GET_CHNUM(*out_format);
    size_t       size  = DSP_PCM_FORMAT_GET_SIZE(*out_format);

    if (size * 8 < bits)
        return -1; 

    if (bits  != this->out_pcm_bits ||
        chnum != this->out_pcm_chnum ||
        size  != this->out_pcm_size)
    {
        this->out_pcm_bits  = bits;
        this->out_pcm_chnum = chnum;
        this->out_pcm_size  = size;

        force_output_format_change(this);
    }

    return 0;
}

static bool set_out_frequency(struct sample_io_data *this,
                              unsigned long samplerate,
                              intptr_t *outval)
{
    samplerate = MIN(DSP_OUT_MAX_HZ, MAX(DSP_OUT_MIN_HZ, samplerate));

    *outval = samplerate;

    if (samplerate == this->output_sampr)
        return true; /* don't broadcast */

    this->output_sampr = samplerate;
    return false;
}

/* Initialize the output function for settings and format */
void dsp_sample_output_format_change(struct sample_io_data *this,
                                     struct sample_format *format)
{
    /* Database of possible internal to output conversions */
    static const struct sample_out_fn_desc
    {
        unsigned long sample_size  : 3;
        unsigned long in_chnum     : 2;
        unsigned long out_chnum    : 2;
        unsigned long dithered     : 1;
        sample_output_fn_type fn;
    } funcs[] =
    {
#ifdef CONFIG_DSP_OUT_2BYTE_INT
#ifdef CONFIG_DSP_OUT_CHNUM
        { 2, 1, 1, 0, sample_output_1ch_1ch_2i      },
        { 2, 1, 1, 1, sample_output_dith_1ch_1ch_2i },
#endif /* CONFIG_DSP_OUT_CHNUM */
        { 2, 1, 2, 0, sample_output_1ch_2ch_2i      },
        { 2, 1, 2, 1, sample_output_dith_1ch_2ch_2i },
        { 2, 2, 2, 0, sample_output_2ch_2ch_2i      },
        { 2, 2, 2, 1, sample_output_dith_2ch_2ch_2i },
#endif /* CONFIG_DSP_OUT_2BYTE_INT */
#ifdef CONFIG_DSP_OUT_3BYTE_INT
#ifdef CONFIG_DSP_OUT_CHNUM
        { 3, 1, 1, 0, sample_output_1ch_1ch_3i      },
#endif /* CONFIG_DSP_OUT_CHNUM */
        { 3, 1, 2, 0, sample_output_1ch_2ch_3i      },
        { 3, 2, 2, 0, sample_output_2ch_2ch_3i      },
#endif /* CONFIG_DSP_OUT_3BYTE_INT */
#ifdef CONFIG_DSP_OUT_4BYTE_INT
#ifdef CONFIG_DSP_OUT_CHNUM
        { 4, 1, 1, 0, sample_output_1ch_1ch_4i      },
#endif /* CONFIG_DSP_OUT_CHNUM */
        { 4, 1, 2, 0, sample_output_1ch_2ch_4i      },
        { 4, 2, 2, 0, sample_output_2ch_2ch_4i      },
#endif /* CONFIG_DSP_OUT_4BYTE_INT */
    };

    /* Assume NULL output if a suitable interface isn't found */
    this->output_samples = sample_output_null;
    this->output_version = format->version;
    this->out_pcm_shift  = (int)this->out_pcm_bits -
                           (int)(format->frac_bits + 1);

    unsigned int out_size  = this->out_pcm_size;
    unsigned int in_chnum  = format->num_channels;
#ifdef CONFIG_DSP_OUT_CHNUM
    unsigned int out_chnum = this->out_pcm_chnum;
#endif /* CONFIG_DSP_OUT_CHNUM */
    unsigned int dithered  = dither_data.enabled;

    if (dithered)
    {
        dithered = dsp_get_id((void *)this) == CODEC_IDX_AUDIO;
#ifdef CONFIG_DSP_OUT_MULTISIZE
       if (dithered)
       {
           /* More formats has some other considerations */
           dithered = this->out_pcm_size == 2;
       }
#endif /* CONFIG_DSP_OUT_MULTISIZE */
    }

    /* Find one that matches the spec */
    for (unsigned int i = 0; i < ARRAYLEN(funcs); i++)
    {
        const struct sample_out_fn_desc *desc = &funcs[i];

        if (desc->sample_size == out_size &&
            desc->in_chnum    == in_chnum &&
        #ifdef CONFIG_DSP_OUT_CHNUM
            desc->out_chnum   == out_chnum &&
        #endif /* CONFIG_DSP_OUT_CHNUM */
            desc->dithered    == dithered)
        {
            this->output_samples = desc->fn;
            break;
        }
    }

    DSP_PRINT_FORMAT(DSP Output, *format);
}

bool dsp_sample_output_configure(struct sample_io_data *this,
                                 unsigned int setting,
                                 intptr_t value,
                                 intptr_t *outval)
{
    switch (setting)
    {
    case DSP_INIT:
        init_sample_output(this);
        break;

    case DSP_FLUSH:
        flush_sample_output(this);
        break;

    case DSP_SET_OUT_FREQUENCY:
        return set_out_frequency(this, value, outval);

    case DSP_GET_OUT_FREQUENCY:
        *outval = this->output_sampr;
        return true;
 
    case DSP_SET_OUT_PCM_FORMAT:
        *outval = 0;
        set_out_pcm_format(this, (const dsp_pcm_format_t *)value);
        return true;
    }

    return false;
}

/** Output settings **/

/* Set the tri-pdf dithered output */
void dsp_dither_enable(bool enable)
{
    if (enable == dither_data.enabled)
        return;

    dither_data.enabled = enable;
    struct sample_io_data *data = (void *)dsp_get_config(CODEC_IDX_AUDIO);

    if (enable)
        flush_sample_output(data);

    force_output_format_change(data);
}
