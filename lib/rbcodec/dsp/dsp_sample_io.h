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
#define WORD_SHIFT      12
#define WORD_FRACBITS   27
#define NATIVE_DEPTH    16

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
    sample_input_fn_type input_samples[2]; /* input functions */
    struct dsp_buffer sample_buf; /* Buffer descriptor for converted samples */
    int32_t *sample_buf_arr[2];   /* Internal format buffer pointers */
    sample_output_fn_type output_samples[2]; /* Final output functions */
};

/* Sample IO watches the format setting from the codec */
void dsp_sample_io_configure(struct sample_io_data *this,
                             unsigned int setting,
                             intptr_t value);

#endif /* DSP_SAMPLE_IO_H */
