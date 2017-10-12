/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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
#ifndef DSP_SAMPLE_IO_H
#define DSP_SAMPLE_IO_H

/* 16-bit samples are scaled based on these constants. The shift should be
 * no more than 15.
 */
#define DSP_WORD_INTL_SHIFT     12
#define DSP_WORD_INTL_FRACBITS  27
#define DSP_WORD_INTL_BITS      28
#define DSP_WORD_BITS           16

struct sample_io_data;

/* DSP initial buffer input function call prototype */
typedef void (*sample_input_fn_type)(struct sample_io_data *this,
                                     struct dsp_buffer **buf_p);

/* DSP final buffer output function call prototype */
typedef void (*sample_output_fn_type)(struct sample_io_data *this,
                                      struct dsp_buffer *src,
                                      struct dsp_buffer *dst);

/* This becomes part of the DSP aggregate */
struct sample_io_data
{
    unsigned long frames_out;     /* 00h: Output frame count */
    int8_t  out_pcm_shift;        /* 04h: Output factor exponent */
#ifdef CONFIG_DSP_OUT_CHNUM
    uint8_t out_pcm_chnum;        /* 05h: Number of channels in output */
#endif
    uint8_t out_pcm_bits;         /* Output sample depth in bits */
    uint8_t out_pcm_size;         /* Number of bytes in output sample */
    uint8_t out_pcm_format;       /* Current output format code */
    uint8_t format_dirty;         /* Format change set, avoids superfluous
                                     increments before carrying it out */
    uint8_t output_version;       /* Format version of src buffer at output */
    uint8_t in_pcm_stmode;        /* Codec-specified channel format */
    uint8_t in_pcm_bits;          /* Bit depth of codec input */
    struct sample_format format;  /* Format for next dsp_process call */
    sample_input_fn_type input_samples; /* Initial input function */
    struct dsp_buffer sample_buf; /* Buffer descriptor for converted samples */
    int32_t *sample_buf_p[2];     /* Internal format buffer pointers */
    sample_output_fn_type output_samples; /* Final output function */
    unsigned long output_sampr;   /* Master output samplerate */
};

void dsp_sample_input_format_change(struct sample_io_data *this,
                                    struct sample_format *format);

void dsp_sample_output_format_change(struct sample_io_data *this,
                                     struct sample_format *format);

/* Sample IO watches the format setting from the codec */
bool dsp_sample_io_configure(struct sample_io_data *this,
                             unsigned int setting,
                             intptr_t *value_p);

/* Add samples to output buffer and update remaining space (Out).
   Provided to dsp_process() */
static inline void dsp_advance_buffer_output(struct dsp_buffer *buf,
                                             unsigned long by_count,
                                             unsigned int numchannels,
                                             size_t sampsize)
{
    buf->frames -= by_count;
    buf->frames_rem += by_count;
#ifndef CONFIG_DSP_OUT_MULTISIZE
    sampsize = DSP_PCM_FORMAT_GET_SIZE(DSP_OUT_DEFAULT_PCM_FORMAT);
#endif
    buf->pout = PTR_ADD(buf->pout, by_count*numchannels*sampsize);
}

#endif /* DSP_SAMPLE_IO_H */
