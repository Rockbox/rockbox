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

#if 1
#include <debug.h>
#else
#undef DEBUGF
#define DEBUGF(...)
#endif

/* The internal format is 32-bit samples, non-interleaved, stereo. This
 * format is similar to the raw output from several codecs, so no copying is
 * needed for that case.
 *
 * Note that for mono, dst[0] equals dst[1], as there is no point in
 * processing the same data twice nor should it be done when modifying
 * samples in-place.
 *
 * When conversion is required:
 * Updates source buffer to point past the samples "consumed" also consuming
 * that portion of the input buffer and the destination is set to the buffer
 * of samples for later stages to consume.
 *
 * Input operates similarly to how an out-of-place processing stage should
 * behave.
 */

extern void dsp_sample_output_init(struct sample_io_data *this);
extern void dsp_sample_output_flush(struct sample_io_data *this);

#define SAMPLE_BUF_COUNT 128 /* Per channel, per DSP */
/* CODEC_IDX_AUDIO = left and right, CODEC_IDX_VOICE = mono */
static int32_t sample_bufs[3][SAMPLE_BUF_COUNT] IBSS_ATTR;

/* inline helper to setup buffers when conversion is required */
static FORCE_INLINE int sample_input_setup(struct sample_io_data *this,
                                           struct dsp_buffer **buf_p,
                                           int channels,
                                           struct dsp_buffer **src,
                                           struct dsp_buffer **dst)
{
    struct dsp_buffer *s = *buf_p;
    struct dsp_buffer *d = *dst = &this->sample_buf;

    *buf_p = d;

    if (d->remcount > 0)
        return 0; /* data still remains */

    *src = s;

    int count = MIN(s->remcount, SAMPLE_BUF_COUNT);

    d->remcount  = count;
    d->p32[0]    = this->sample_buf_arr[0];
    d->p32[1]    = this->sample_buf_arr[channels - 1];
    d->proc_mask = s->proc_mask;

    return count;
}

/* convert count 16-bit mono to 32-bit mono */
static void sample_input_mono16(struct sample_io_data *this,
                                struct dsp_buffer **buf_p)
{
    struct dsp_buffer *src, *dst;
    int count = sample_input_setup(this, buf_p, 1, &src, &dst);

    if (count <= 0)
        return;

    const int16_t *s = src->pin[0];
    int32_t *d = dst->p32[0];
    const int scale = WORD_SHIFT;

    dsp_advance_buffer_input(src, count, sizeof (int16_t));

    do
    {
        *d++ = *s++ << scale;
    }
    while (--count > 0);
}

/* convert count 16-bit interleaved stereo to 32-bit noninterleaved */
static void sample_input_i_stereo16(struct sample_io_data *this,
                                    struct dsp_buffer **buf_p)
{
    struct dsp_buffer *src, *dst;
    int count = sample_input_setup(this, buf_p, 2, &src, &dst);

    if (count <= 0)
        return;

    const int16_t *s = src->pin[0];
    int32_t *dl = dst->p32[0];
    int32_t *dr = dst->p32[1];
    const int scale = WORD_SHIFT;

    dsp_advance_buffer_input(src, count, 2*sizeof (int16_t));

    do
    {
        *dl++ = *s++ << scale;
        *dr++ = *s++ << scale;
    }
    while (--count > 0);
}

/* convert count 16-bit noninterleaved stereo to 32-bit noninterleaved */
static void sample_input_ni_stereo16(struct sample_io_data *this,
                                     struct dsp_buffer **buf_p)
{
    struct dsp_buffer *src, *dst;
    int count = sample_input_setup(this, buf_p, 2, &src, &dst);

    if (count <= 0)
        return;

    const int16_t *sl = src->pin[0];
    const int16_t *sr = src->pin[1];
    int32_t *dl = dst->p32[0];
    int32_t *dr = dst->p32[1];
    const int scale = WORD_SHIFT;

    dsp_advance_buffer_input(src, count, sizeof (int16_t));

    do
    {
        *dl++ = *sl++ << scale;
        *dr++ = *sr++ << scale;
    }
    while (--count > 0);
}

/* convert count 32-bit mono to 32-bit mono */
static void sample_input_mono32(struct sample_io_data *this,
                                struct dsp_buffer **buf_p)
{
    struct dsp_buffer *dst = &this->sample_buf;

    if (dst->remcount > 0)
    {
        *buf_p = dst;
        return; /* data still remains */
    }
    /* else no buffer switch */

    struct dsp_buffer *src = *buf_p;
    src->p32[1] = src->p32[0];
}


/* convert count 32-bit interleaved stereo to 32-bit noninterleaved stereo */
static void sample_input_i_stereo32(struct sample_io_data *this,
                                    struct dsp_buffer **buf_p)
{
    struct dsp_buffer *src, *dst;
    int count = sample_input_setup(this, buf_p, 2, &src, &dst);

    if (count <= 0)
        return;

    const int32_t *s = src->pin[0];
    int32_t *dl = dst->p32[0];
    int32_t *dr = dst->p32[1];

    dsp_advance_buffer_input(src, count, 2*sizeof (int32_t));

    do
    {
        *dl++ = *s++;
        *dr++ = *s++;
    }
    while (--count > 0);
}

/* convert 32 bit-noninterleaved stereo to 32-bit noninterleaved stereo */
static void sample_input_ni_stereo32(struct sample_io_data *this,
                                     struct dsp_buffer **buf_p)
{
    struct dsp_buffer *dst = &this->sample_buf;

    if (dst->remcount > 0)
        *buf_p = dst; /* data still remains */
    /* else no buffer switch */
}

/* set the to-native sample conversion function based on dsp sample
 * parameters */
static void dsp_sample_input_format_change(struct sample_io_data *this,
                                           struct dsp_buffer **buf_p)
{
    static const sample_input_fn_type fns[STEREO_NUM_MODES][2] =
    {
        [STEREO_INTERLEAVED] =
            { sample_input_i_stereo16,
              sample_input_i_stereo32 },
        [STEREO_NONINTERLEAVED] =
            { sample_input_ni_stereo16,
              sample_input_ni_stereo32 },
        [STEREO_MONO] =
            { sample_input_mono16,
              sample_input_mono32 },
    };

    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &this->sample_buf;

    /* Ack configured format change */
    format_change_ack(&this->format);

    if (dst->remcount > 0)
    {
        *buf_p = dst;
        return; /* data still remains */
    }

    DSP_PRINT_FORMAT(DSP Input, -1, src->format);

    /* new format - remember it and pass it along */
    dst->format = src->format;
    this->input_samples[0] = fns[this->stereo_mode]
                                [this->sample_depth > NATIVE_DEPTH ? 1 : 0];

    this->input_samples[0](this, buf_p);

    if (*buf_p == dst) /* buffer switch? */
        format_change_ack(&src->format);
}

static void dsp_sample_input_init(struct sample_io_data *this,
                                  enum dsp_ids dsp_id)
{
    int32_t *lbuf, *rbuf;

    switch (dsp_id)
    {
    case CODEC_IDX_AUDIO:
        lbuf = sample_bufs[0];
        rbuf = sample_bufs[1];
        break;

    case CODEC_IDX_VOICE:
        lbuf = rbuf = sample_bufs[2]; /* Always mono */
        break;

    default:
        /* orly */
        DEBUGF("DSP Input- unknown dsp %d\n", (int)dsp_id);
        return;
    }

    this->sample_buf_arr[0] = lbuf;
    this->sample_buf_arr[1] = rbuf;

    this->input_samples[0] = sample_input_ni_stereo32;
    this->input_samples[1] = dsp_sample_input_format_change;
}

/* discard the sample buffer */
static void dsp_sample_input_flush(struct sample_io_data *this)
{
    this->sample_buf.remcount = 0;
}

void dsp_sample_io_configure(struct sample_io_data *this,
                             unsigned int setting,
                             intptr_t value)
{
    switch (setting)
    {
    case DSP_INIT:
        dsp_sample_input_init(this, (enum dsp_ids)value);
        dsp_sample_output_init(this);
        break;

    case DSP_RESET:
        /* Reset all sample descriptions to default */
        format_change_set(&this->format);
        this->format.num_channels = 2;
        this->format.frac_bits = WORD_FRACBITS;
        this->format.output_scale = WORD_FRACBITS + 1 - NATIVE_DEPTH;
        this->format.frequency = NATIVE_FREQUENCY;
        this->format.codec_frequency = NATIVE_FREQUENCY;
        this->sample_depth = NATIVE_DEPTH;
        this->stereo_mode = STEREO_NONINTERLEAVED;
        break;

    case DSP_SET_FREQUENCY:
        value = value > 0 ? value : NATIVE_FREQUENCY;
        format_change_set(&this->format);
        this->format.frequency = value;
        this->format.codec_frequency = value;
        break;

    case DSP_SET_SAMPLE_DEPTH:
        format_change_set(&this->format);
        this->format.frac_bits =
            value <= NATIVE_DEPTH ? WORD_FRACBITS : value;
        this->format.output_scale =
            this->format.frac_bits + 1 - NATIVE_DEPTH;
        this->sample_depth = value;
        break;

    case DSP_SET_STEREO_MODE:
        format_change_set(&this->format);
        this->format.num_channels = value == STEREO_MONO ? 1 : 2;
        this->stereo_mode = value;
        break;

    case DSP_FLUSH:
        dsp_sample_input_flush(this);
        dsp_sample_output_flush(this);
        break;
    }
}
