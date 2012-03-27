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
    CODEC_IDX_AUDIO = 0,
    CODEC_IDX_VOICE,
    DSP_COUNT,
};

enum
{
    STEREO_INTERLEAVED = 0,
    STEREO_NONINTERLEAVED,
    STEREO_MONO,
    STEREO_NUM_MODES,
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
    DSP_SET_REPLAY_GAIN,
    DSP_CROSSFEED,
    DSP_IS_BUSY,
};

/* Identifier for each effect in DSP chain */
enum dsp_proc_masks
{
    DSP_PROC_GAIN          = 0x00000001,
    DSP_PROC_TIMESTRETCH   = 0x00000002,
    DSP_PROC_RESAMPLE      = 0x00000004,
    DSP_PROC_CROSSFEED     = 0x00000008,
    DSP_PROC_EQUALIZER     = 0x00000010,
    DSP_PROC_TONE_CONTROLS = 0x00000020,
    DSP_PROC_CHANNELS      = 0x00000040,
    DSP_PROC_COMPRESSOR    = 0x00000080,
    DSP_PROC_MASK_USE_INIT_CB = 0xffffffff, /* Initialize when activated */
};

/* Used by ASM routines - keep field order or else fix the functions */
struct dsp_buffer
{
    int remcount;           /* 00h: Samples in buffer */
    union
    {
        const void *pin[2]; /* 04h: Channel pointers (input) */
        int32_t *p32[2];    /* 04h: Channel pointers (internal) */
        int16_t *p16out;    /* 04h: DSP output buffer (output) */
    };
    union
    {
        uint32_t proc_mask; /* 0Ch: In-place effects already appled to buffer
                                    in order to avoid double-processing. Set
                                    to zero on new buffer before passing to
                                    DSP. */
        int bufcount;       /* 0Ch: Buffer length/dest buffer remaining
                                    Basically, pay no attention unless it's
                                    *your* new buffer and is used internally
                                    or is specifically the final output
                                    buffer. */
    };
};

static inline void dsp_advance_buffer_input(struct dsp_buffer *buf,
                                            int by_count,
                                            size_t size_each)
{
    buf->remcount -= by_count;
    buf->pin[0] += by_count * size_each;
    buf->pin[1] += by_count * size_each;
}

static inline void dsp_advance_buffer_output(struct dsp_buffer *buf,
                                             int by_count)
{
    buf->bufcount -= by_count;
    buf->remcount += by_count;
    buf->p16out += 2 * by_count; /* Interleaved stereo */
}

static inline void dsp_advance_buffer32(struct dsp_buffer *buf,
                                        int by_count)
{
    buf->remcount -= by_count;
    buf->p32[0] += by_count;
    buf->p32[1] += by_count;
}

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
    int32_t gain;                       /* 18h - Note that this is in S8.23 format. */
    int frac_bits;                      /* 1ch */
                                        /* 20h */
};

/* Structure used with DSP_SET_REPLAY_GAIN message */
struct dsp_replay_gains
{
    long track_gain;
    long album_gain;
    long track_peak;
    long album_peak;
};

struct dsp_config;

#define CONFIG_FROM_DATA(x) \
    TYPE_FROM_MEMBER(struct dsp_config, x, data)

int dsp_output_count(struct dsp_config *dsp, int count);
void dsp_process(struct dsp_config *dsp, struct dsp_buffer *src,
                 struct dsp_buffer *dst);
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
void dsp_set_compressor(void);

#endif
