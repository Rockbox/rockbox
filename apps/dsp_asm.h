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

/* Set the appropriate #defines based on CPU or whatever matters */
#if defined(CPU_ARM)
#define DSP_HAVE_ASM_RESAMPLING
#define DSP_HAVE_ASM_CROSSFEED
#define DSP_HAVE_ASM_SOUND_CHAN_MONO
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
void apply_crossfeed(int count, int32_t *buf[]);
#endif

#ifdef DSP_HAVE_ASM_APPLY_GAIN
void dsp_apply_gain(int count, struct dsp_data *data, int32_t *buf[]);
#endif /* DSP_HAVE_ASM_APPLY_GAIN* */

#ifdef DSP_HAVE_ASM_RESAMPLING
int dsp_upsample(int count, struct dsp_data *data,
                 int32_t *src[], int32_t *dst[]);
int dsp_downsample(int count, struct dsp_data *data,
                   int32_t *src[], int32_t *dst[]);
#endif /* DSP_HAVE_ASM_RESAMPLING */

#ifdef DSP_HAVE_ASM_SOUND_CHAN_MONO
void channels_process_sound_chan_mono(int count, int32_t *buf[]);
#endif

#ifdef DSP_HAVE_ASM_SOUND_CHAN_CUSTOM
void channels_process_sound_chan_custom(int count, int32_t *buf[]);
#endif

#ifdef DSP_HAVE_ASM_SOUND_CHAN_KARAOKE
void channels_process_sound_chan_karaoke(int count, int32_t *buf[]);
#endif

#ifdef DSP_HAVE_ASM_SAMPLE_OUTPUT_STEREO
void sample_output_stereo(int count, struct dsp_data *data,
                          int32_t *src[], int16_t *dst);
#endif

#ifdef DSP_HAVE_ASM_SAMPLE_OUTPUT_MONO
void sample_output_mono(int count, struct dsp_data *data,
                        int32_t *src[], int16_t *dst);
#endif

#endif /* _DSP_ASM_H */
