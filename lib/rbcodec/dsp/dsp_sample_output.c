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

/* May be implemented in here or externally.*/
void sample_output_mono(struct sample_io_data *this,
                        struct dsp_buffer *src, struct dsp_buffer *dst);
void sample_output_stereo(struct sample_io_data *this,
                          struct dsp_buffer *src, struct dsp_buffer *dst);
void sample_output_dithered(struct sample_io_data *this,
                            struct dsp_buffer *src, struct dsp_buffer *dst);

/** Sample output **/

#if (!defined(CPU_COLDFIRE) && !defined(CPU_ARM)) \
    || NATIVE_DEPTH > WORD_DEPTH
/* write mono internal format to output format */
void sample_output_mono(struct sample_io_data *this,
                        struct dsp_buffer *src, struct dsp_buffer *dst)
{
    unsigned long count = this->frames_out;
    const int32_t *s0 = src->p32[0];
    pcm_dma_t *d = dst->pout;
    int scale = src->format.output_scale;

#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_4_BYTE_CAPS)
    if (scale <= 0)
    {
        int depth = src->format.frac_bits + 1;
        scale = -scale;

        do
        {
            int32_t lr = clip_sample(*s0++, depth) << scale;
            pcm_dma_t_write_incr(&d, lr, 1);
            pcm_dma_t_write_incr(&d, lr, 1);
        }
        while (--count > 0);

        return;
    }
#endif /* CONFIG_PCM_FORMAT_CAPS */

    int32_t dc_bias = 1L << (scale - 1);

    do
    {
        int32_t lr = mix_clip_sample_t(*s0++, dc_bias, scale);
        pcm_dma_t_write_incr(&d, lr, 1);
        pcm_dma_t_write_incr(&d, lr, 1);
    }
    while (--count);
}

/* write stereo internal format to output format */
void sample_output_stereo(struct sample_io_data *this,
                          struct dsp_buffer *src, struct dsp_buffer *dst)
{
    unsigned long count = this->frames_out;
    const int32_t *s0 = src->p32[0];
    const int32_t *s1 = src->p32[1];
    pcm_dma_t *d = dst->pout;
    int scale = src->format.output_scale;

#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_4_BYTE_CAPS)
    if (scale <= 0)
    {
        int depth = src->format.frac_bits + 1;
        scale = -scale;

        do
        {
            int32_t l = clip_sample(*s0++, depth) << scale;
            int32_t r = clip_sample(*s1++, depth) << scale;
            pcm_dma_t_write_incr(&d, l, 1);
            pcm_dma_t_write_incr(&d, r, 1);
        }
        while (--count > 0);

        return;
    }
#endif /* CONFIG_PCM_FORMAT_CAPS */

    int32_t dc_bias = 1L << (scale - 1);

    do
    {
        int32_t l = mix_clip_sample_t(*s0++, dc_bias, scale);
        int32_t r = mix_clip_sample_t(*s1++, dc_bias, scale);
        pcm_dma_t_write_incr(&d, l, 1);
        pcm_dma_t_write_incr(&d, r, 1);
    }
    while (--count);
}
#endif /* CPU */

/**
 * The "dither" code to convert the 24-bit samples produced by libmad was
 * taken from the coolplayer project - coolplayer.sourceforge.net
 *
 * This function handles mono and stereo outputs.
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

void sample_output_dithered(struct sample_io_data *this,
                            struct dsp_buffer *src, struct dsp_buffer *dst)
{
    unsigned long count = this->frames_out;
    unsigned int channels = src->format.num_channels;
    int scale = src->format.output_scale;
    int32_t dc_bias = 1L << (scale - 1); /* 1/2 bit of significance */
    int32_t mask = (1L << scale) - 1; /* Mask of bits quantized away */

    for (unsigned int ch = 0; ch < channels; ch++)
    {
        struct dither_state *dither = &dither_data.state[ch];

        const int32_t *s = src->p32[ch];
        pcm_dma_t *d = &dst->pout[ch];

        for (int i = 0; i < count; i++)
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
            output = clip_sample_t(output);
            pcm_dma_t_write_incr(&d, output, 2);
        }
    }

    if (channels > 1)
        return;

    /* Have to duplicate left samples into the right channel since
       output is interleaved stereo */
    pcm_dma_t *d = dst->pout;

    do
    {
        pcm_dma_t s = pcm_dma_t_read_incr(&d);
        pcm_dma_t_write_incr(&d, s);
    }
    while (--count);
}

/* Initialize the output function for settings and format */
void dsp_sample_output_format_change(struct sample_io_data *this,
                                     struct sample_format *format)
{
    static const sample_output_fn_type fns[2][2] =
    {
        { sample_output_mono,        /* DC-biased quantizing */
          sample_output_stereo },
        { sample_output_dithered,    /* Tri-PDF dithering */
          sample_output_dithered },
    };

    bool dither = true;

#if NATIVE_DEPTH > WORD_DEPTH
    /* No dithering unless downscaling */
    dither = format->output_scale > 0;
#endif

    if (dither) {
        dither = dsp_get_id((void *)this) == CODEC_IDX_AUDIO &&
                 dither_data.enabled;
    }

    int channels = format->num_channels;

    DSP_PRINT_FORMAT(DSP Output, *format);

    this->output_samples = fns[dither ? 1 : 0][channels - 1];
    this->output_version = format->version;
}

void INIT_ATTR dsp_sample_output_init(struct sample_io_data *this)
{
    this->output_version = 0;
    this->output_samples = sample_output_stereo;
}

/* Flush the dither history */
void dsp_sample_output_flush(struct sample_io_data *this)
{
    if (dsp_get_id((void *)this) == CODEC_IDX_AUDIO)
        memset(dither_data.state, 0, sizeof (dither_data.state));
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
        dsp_sample_output_flush(data);

    data->output_version = 0; /* Force format update */
}
