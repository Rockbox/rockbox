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
#define WORD_SHIFT              12
#define WORD_FRACBITS           27
#define NATIVE_DEPTH            16

#define SAMPLE_BUF_COUNT 128 /* Per channel, per DSP */

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
    int outcount;                /* 00h: Output count */
    struct sample_format format; /* General format info */
    int sample_depth; /* Codec-specified sample depth */           
    int stereo_mode;  /* Codec-specified input format */
    sample_input_fn_type input_samples[2]; /* input fn */
    struct dsp_buffer sample_buf; /* Buffer descriptor for converted samples */
    int32_t sample_buf_arr[2][SAMPLE_BUF_COUNT]; /* Internal format */
    sample_output_fn_type output_samples[2];
};

void dsp_sample_input_init(struct sample_io_data *this);
void dsp_sample_input_flush(struct sample_io_data *this);
void dsp_sample_output_init(struct sample_io_data *this);
void dsp_sample_output_flush(struct sample_io_data *this);

/* in dsp_misc.c */
#ifdef HAVE_PITCHSCREEN
void dsp_pitch_update(struct sample_io_data *this);
#else
static inline void dsp_pitch_update(struct sample_io_data *this)
{ (void)this; }
#endif

#endif /* DSP_SAMPLE_IO_H */
