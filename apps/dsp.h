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

#ifndef _DSP_H
#define _DSP_H

#include <stdlib.h>
#include <stdbool.h>

#define NATIVE_FREQUENCY       44100

enum
{
    STEREO_INTERLEAVED = 0,
    STEREO_NONINTERLEAVED,
    STEREO_MONO,
    STEREO_NUM_MODES,
};

enum
{
    CODEC_IDX_AUDIO = 0,
    CODEC_IDX_VOICE,
};

enum
{
    DSP_MYDSP = 1,
    DSP_SET_FREQUENCY,
    DSP_SWITCH_FREQUENCY,
    DSP_SET_SAMPLE_DEPTH,
    DSP_SET_STEREO_MODE,
    DSP_RESET,
    DSP_FLUSH,
    DSP_SET_TRACK_GAIN,
    DSP_SET_ALBUM_GAIN,
    DSP_SET_TRACK_PEAK,
    DSP_SET_ALBUM_PEAK,
    DSP_CROSSFEED
};

struct dsp_config;

int dsp_process(struct dsp_config *dsp, char *dest,
                const char *src[], int count);
int dsp_input_count(struct dsp_config *dsp, int count);
int dsp_output_count(struct dsp_config *dsp, int count);
intptr_t dsp_configure(struct dsp_config *dsp, int setting,
                       intptr_t value);
int get_replaygain_mode(bool have_track_gain, bool have_album_gain);
void dsp_set_replaygain(void);
void dsp_set_crossfeed(bool enable);
void dsp_set_crossfeed_direct_gain(int gain);
void dsp_set_crossfeed_cross_params(long lf_gain, long hf_gain,
                                    long cutoff);
void dsp_set_eq(bool enable);
void dsp_set_eq_precut(int precut);
void dsp_set_eq_coefs(int band);
void dsp_dither_enable(bool enable);
void dsp_timestretch_enable(bool enable);
bool dsp_timestretch_available(void);
void sound_set_pitch(int32_t r);
int32_t sound_get_pitch(void);
void dsp_set_timestretch(int32_t percent);
int32_t dsp_get_timestretch(void);
int dsp_callback(int msg, intptr_t param);
void dsp_set_compressor(int c_threshold, int c_ratio, int c_gain,
                        int c_knee, int c_release);

#endif
