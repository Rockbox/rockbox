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
#include "rbcodecconfig.h"

#define NATIVE_FREQUENCY       44100

#ifdef HAVE_PITCHSCREEN
/* Precision of the pitch and speed variables. 100 = 1%, 10000 = 100%. */
#define PITCH_SPEED_PRECISION 100L
#define PITCH_SPEED_100 (100L * PITCH_SPEED_PRECISION)  /* 100% speed */
#endif /* HAVE_PITCHSCREEN */

enum
{
    STEREO_INTERLEAVED = 0,
    STEREO_NONINTERLEAVED,
    STEREO_MONO,
    STEREO_NUM_MODES,
};

#ifdef RBCODEC_ENABLE_CHANNEL_MODES
enum Channel {
    SOUND_CHAN_STEREO,
    SOUND_CHAN_MONO,
    SOUND_CHAN_CUSTOM,
    SOUND_CHAN_MONO_LEFT,
    SOUND_CHAN_MONO_RIGHT,
    SOUND_CHAN_KARAOKE,
    SOUND_CHAN_NUM_MODES,
};
#endif

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

/****************************************************************************
 * NOTE: Any assembly routines that use these structures must be updated
 * if current data members are moved or changed.
 */
struct resample_data
{
    uint32_t delta;                     /* 00h */
    uint32_t phase;                     /* 04h */
    int32_t last_sample[2];             /* 08h */
                                        /* 10h */
};

/* This is for passing needed data to external dsp routines. If another
 * dsp parameter needs to be passed, add to the end of the structure
 * and remove from dsp_config.
 * If another function type becomes assembly/external and requires dsp
 * config info, add a pointer paramter of type "struct dsp_data *".
 * If removing something from other than the end, reserve the spot or
 * else update every implementation for every target.
 * Be sure to add the offset of the new member for easy viewing as well. :)
 * It is the first member of dsp_config and all members can be accessesed
 * through the main aggregate but this is intended to make a safe haven
 * for these items whereas the c part can be rearranged at will. dsp_data
 * could even moved within dsp_config without disurbing the order.
 */
struct dsp_data
{
    int output_scale;                   /* 00h */
    int num_channels;                   /* 04h */
    struct resample_data resample_data; /* 08h */
    int32_t clip_min;                   /* 18h */
    int32_t clip_max;                   /* 1ch */
    int32_t gain;                       /* 20h - Note that this is in S8.23 format. */
    int frac_bits;                      /* 24h */
                                        /* 28h */
};

enum {
    RBCODEC_REPLAYGAIN_NONE,
    RBCODEC_REPLAYGAIN_TRACK,
    RBCODEC_REPLAYGAIN_ALBUM,
};

struct dsp_config;

int dsp_process(struct dsp_config *dsp, char *dest,
                const char *src[], int count);
int dsp_input_count(struct dsp_config *dsp, int count);
int dsp_output_count(struct dsp_config *dsp, int count);
intptr_t dsp_configure(struct dsp_config *dsp, int setting,
                       intptr_t value);
void rbcodec_dsp_set_replaygain(int type, bool noclip, int preamp);
void dsp_set_crossfeed(bool enable);
void dsp_set_crossfeed_direct_gain(int gain);
void dsp_set_crossfeed_cross_params(long lf_gain, long hf_gain,
                                    long cutoff);
void dsp_set_eq(bool enable);
void dsp_set_eq_precut(int precut);
void dsp_set_eq_coefs(int band, unsigned long cutoff, unsigned long q,
                      long gain);
void dsp_dither_enable(bool enable);
#ifdef HAVE_PITCHSCREEN
void dsp_timestretch_enable(bool enable);
bool dsp_timestretch_available(void);
void sound_set_pitch(int32_t r);
int32_t sound_get_pitch(void);
void dsp_set_timestretch(int32_t percent);
int32_t dsp_get_timestretch(void);
#endif
void dsp_set_compressor(int c_threshold, int c_gain, int c_ratio,
                        int c_knee, int c_release);
#ifdef HAVE_SW_VOLUME_CONTROL
void dsp_set_volume(int vol);
#endif
#ifdef HAVE_SW_TONE_CONTROLS
void set_tone_controls(int bass, int treble, int prescale);
#endif
#ifdef RBCODEC_ENABLE_CHANNEL_MODES
void dsp_set_channel_config(int value);
void dsp_set_stereo_width(int value);
#endif

#endif
