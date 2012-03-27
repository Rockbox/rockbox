/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Thom Johansen
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

#include <config.h>

#ifndef _DSP_ASM_H
#define _DSP_ASM_H

struct dsp_proc_entry;

/* Set the appropriate #defines based on CPU or whatever matters */
#if defined(CPU_ARM)
#define DSP_HAVE_ASM_APPLY_GAIN
#define DSP_HAVE_ASM_RESAMPLING
#define DSP_HAVE_ASM_CROSSFEED
#define DSP_HAVE_ASM_SOUND_CHAN_MONO
#define DSP_HAVE_ASM_SOUND_CHAN_CUSTOM
#define DSP_HAVE_ASM_SOUND_CHAN_KARAOKE
#define DSP_HAVE_ASM_SAMPLE_OUTPUT_MONO
#define DSP_HAVE_ASM_SAMPLE_OUTPUT_STEREO
#elif defined (CPU_COLDFIRE)
#define DSP_HAVE_ASM_APPLY_GAIN
#define DSP_HAVE_ASM_RESAMPLING
#define DSP_HAVE_ASM_CROSSFEED
#define DSP_HAVE_ASM_SOUND_CHAN_MONO
#define DSP_HAVE_ASM_SOUND_CHAN_CUSTOM
#define DSP_HAVE_ASM_SOUND_CHAN_KARAOKE
#define DSP_HAVE_ASM_SAMPLE_OUTPUT_MONO
#define DSP_HAVE_ASM_SAMPLE_OUTPUT_STEREO
#endif /* CPU_COLDFIRE */

/* Declare prototypes based upon what's #defined above */
#ifdef DSP_HAVE_ASM_CROSSFEED
void apply_crossfeed(struct dsp_data *data,
                     struct dsp_buffer **buf_p);
#else
static void apply_crossfeed(struct dsp_data *data,
                            struct dsp_buffer **buf_p);
#endif /* DSP_HAVE_ASM_CROSSFEED */

#ifdef DSP_HAVE_ASM_APPLY_GAIN
void dsp_apply_gain(struct dsp_data *data,
                    struct dsp_buffer **buf_p);
#else
static void dsp_apply_gain(struct dsp_data *data,
                           struct dsp_buffer **buf_p);
#endif /* DSP_HAVE_ASM_APPLY_GAIN* */

#ifdef DSP_HAVE_ASM_RESAMPLING
int dsp_lin_resample(struct dsp_data *data,
                     struct dsp_buffer *src, struct dsp_buffer *dst);
#else
static int dsp_lin_resample(struct dsp_data *data,
                            struct dsp_buffer *src, struct dsp_buffer *dst);
#endif /* DSP_HAVE_ASM_RESAMPLING */

static void resample(struct dsp_data *data, struct dsp_buffer **buf_p);

static void channels_process_init_cb(struct dsp_proc_entry *ent);

#ifdef DSP_HAVE_ASM_SOUND_CHAN_MONO
void channels_process_sound_chan_mono(struct dsp_data *data,
                                      struct dsp_buffer **buf_p);
#else
static void channels_process_sound_chan_mono(struct dsp_data *data,
                                             struct dsp_buffer **buf_p);
#endif

static void channels_process_sound_chan_mono_left(struct dsp_data *data,
                                                  struct dsp_buffer **buf_p);

static void channels_process_sound_chan_mono_right(struct dsp_data *data,
                                                   struct dsp_buffer **buf_p);

#ifdef DSP_HAVE_ASM_SOUND_CHAN_CUSTOM
void channels_process_sound_chan_custom(struct dsp_data *data,
                                        struct dsp_buffer **buf_p);
#else
static void channels_process_sound_chan_custom(struct dsp_data *data,
                                               struct dsp_buffer **buf_p);
#endif

#ifdef DSP_HAVE_ASM_SOUND_CHAN_KARAOKE
void channels_process_sound_chan_karaoke(struct dsp_data *data,
                                         struct dsp_buffer **buf_p);
#else
static void channels_process_sound_chan_karaoke(struct dsp_data *data,
                                                struct dsp_buffer **buf_p);
#endif

#ifdef DSP_HAVE_ASM_SAMPLE_OUTPUT_STEREO
void sample_output_stereo(int count, struct dsp_data *data,
                          int32_t const * const src[], int16_t *dst);
#else
static void sample_output_stereo(int count, struct dsp_data *data,
                                 int32_t const * const src[], int16_t *dst);
#endif

#ifdef DSP_HAVE_ASM_SAMPLE_OUTPUT_MONO
void sample_output_mono(int count, struct dsp_data *data,
                        int32_t const * const src[], int16_t *dst);
#else
static void sample_output_mono(int count, struct dsp_data *data,
                               int32_t const * const src[], int16_t *dst);
#endif

static void eq_process(struct dsp_data *data,struct dsp_buffer **buf_p);
#ifdef HAVE_SW_TONE_CONTROLS
static void tone_process(struct dsp_data *data, struct dsp_buffer **buf_p);
#endif

#endif /* _DSP_ASM_H */
