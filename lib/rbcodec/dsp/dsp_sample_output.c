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
#include "config.h"
#include "system.h"
#include "dsp_core.h"
#include "dsp_sample_io.h"
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

#if !defined(CPU_COLDFIRE) && !defined(CPU_ARM)
/* write mono internal format to output format */
void sample_output_mono(struct sample_io_data *this,
                        struct dsp_buffer *src, struct dsp_buffer *dst)
{
    int count = this->outcount;
    const int32_t *s0 = src->p32[0];
    int16_t *d = dst->p16out;
    int scale = src->format.output_scale;
    int32_t dc_bias = 1L << (scale - 1);

    do
    {
        int32_t lr = clip_sample_16((*s0++ + dc_bias) >> scale);
        *d++ = lr;
        *d++ = lr;
    }
    while (--count > 0);
}

/* write stereo internal format to output format */
void sample_output_stereo(struct sample_io_data *this,
                          struct dsp_buffer *src, struct dsp_buffer *dst)
{
    int count = this->outcount;
    const int32_t *s0 = src->p32[0];
    const int32_t *s1 = src->p32[1];
    int16_t *d = dst->p16out;
    int scale = src->format.output_scale;
    int32_t dc_bias = 1L << (scale - 1);

    do
    {
        *d++ = clip_sample_16((*s0++ + dc_bias) >> scale);
        *d++ = clip_sample_16((*s1++ + dc_bias) >> scale);
    }
    while (--count > 0);
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
    int count = this->outcount;
    int channels = src->format.num_channels;
    int scale = src->format.output_scale;
    int32_t dc_bias = 1L << (scale - 1); /* 1/2 bit of significance */
    int32_t mask = (1L << scale) - 1; /* Mask of bits quantized away */

    for (int ch = 0; ch < channels; ch++)
    {
        struct dither_state *dither = &dither_data.state[ch];

        const int32_t *s = src->p32[ch];
        int16_t *d = &dst->p16out[ch];

        for (int i = 0; i < count; i++, s++, d += 2)
        {
            /* Noise shape and bias (for correct rounding later) */
            int32_t sample = *s;

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
        }
    }

    if (channels > 1)
        return;

    /* Have to duplicate left samples into the right channel since
       output is interleaved stereo */
    int16_t *d = dst->p16out;

    do
    {
        int16_t s = *d++;
        *d++ = s;
    }
    while (--count > 0);
}

/* Initialize the output function for settings and format */
static void dsp_sample_output_format_change(struct sample_io_data *this,
                                            struct dsp_buffer *src,
                                            struct dsp_buffer *dst)
{
    static const sample_output_fn_type fns[2][2] =
    {
        { sample_output_mono,        /* DC-biased quantizing */
          sample_output_stereo },
        { sample_output_dithered,    /* Tri-PDF dithering */
          sample_output_dithered },
    };

    struct sample_format *format = &src->format;
    bool dither = dsp_get_id((void *)this) == CODEC_IDX_AUDIO &&
                  dither_data.enabled;
    int channels = format->num_channels;

    DSP_PRINT_FORMAT(DSP Output, -1, *format);

    this->output_samples[0] = fns[dither ? 1 : 0][channels - 1];
    format_change_ack(format); /* always ack, we're last */

    /* The real function mustn't be called with no data */
    if (this->outcount > 0)
        this->output_samples[0](this, src, dst);
}

void dsp_sample_output_init(struct sample_io_data *this)
{
    this->output_samples[0] = sample_output_stereo;
    this->output_samples[1] = dsp_sample_output_format_change;
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

    struct sample_io_data *data = (void *)dsp_get_config(CODEC_IDX_AUDIO);
    dsp_sample_output_flush(data);
    dither_data.enabled = enable;
    data->output_samples[0] = dsp_sample_output_format_change;
}
