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

/* Identifier for each effect in DSP chain */
enum dsp_proc_ids
{
    /* No particular order of application is implied by these values */
    DSP_PROC_PGA = 0,       /* pre-gain amp */
#ifdef HAVE_PITCHSCREEN
    DSP_PROC_TIMESTRETCH,   /* time-stretching */
#endif
    DSP_PROC_RESAMPLE,      /* resampler that provides NATIVE_FREQUENCY */
    DSP_PROC_CROSSFEED,     /* stereo crossfeed */
    DSP_PROC_EQUALIZER,     /* n-band equalizer */
#ifdef HAVE_SW_TONE_CONTROLS
    DSP_PROC_TONE_CONTROLS, /* bass and treble */
#endif
    DSP_PROC_CHANNEL_MODE,  /* channel modes */
    DSP_PROC_COMPRESSOR,    /* dynamic-range compressor */
};

struct dsp_config;
struct dsp_data;
struct dsp_buffer;
struct dsp_proc_entry;

#include "channel_mode.h"
#include "compressor.h"
#include "crossfeed.h"
#include "eq.h"
#ifdef HAVE_SW_TONE_CONTROLS
#include "tone_controls.h"
#endif
#ifdef HAVE_PITCHSCREEN
#include "tdspeed.h"
#endif
#include "dsp_filter.h"
#include "lin_resample.h"
#include "pga.h"

enum dsp_ids
{
    CODEC_IDX_AUDIO,
    CODEC_IDX_VOICE,
    DSP_COUNT,
};

enum dsp_settings
{
    DSP_RESET = 1,
    DSP_SET_FREQUENCY,
    DSP_SWITCH_FREQUENCY,
    DSP_SET_SAMPLE_DEPTH,
    DSP_SET_STEREO_MODE,
    DSP_FLUSH,
    DSP_SET_REPLAY_GAIN,
    /* Processing stage-specific messages */
    DSP_PROC_INIT,
    DSP_PROC_CLOSE,
};

/* DSP sample transform function prototype */
typedef void (*dsp_proc_fn_type)(struct dsp_proc_entry *this,
                                 struct dsp_buffer **buf);

/* DSP transform configure function prototype */
typedef void (*dsp_proc_config_fn_type)(struct dsp_proc_entry *this,
                                        struct dsp_config *dsp,
                                        enum dsp_settings setting,
                                        intptr_t value);

struct dsp_proc_db_entry
{
    enum dsp_proc_ids id;              /* id of this stage */
    dsp_proc_config_fn_type configure; /* dsp_configure hook */
};

struct dsp_proc_entry
{
    intptr_t data;    /* 00h: any value, at beginning for easy asm use */
    uint32_t ip_mask; /* In-place id bit (0 or Id bit flag if IP) */
    dsp_proc_fn_type process[2]; /* Processing normal/format changes */
};

/* dsp_proc_entry defaults upon enabling:
 *  .data       = 0
 *  .ip_mask    = BIT_N(dsp_proc_db_entry.id)
 *  .process[0] = dsp_format_change_process // to crash creatively
 *  .process[1] = dsp_format_change_process
 *
 * DSP_PROC_INIT handler just has to change what it needs to change.
 */

#define NATIVE_FREQUENCY   44100 /* internal/output sample rate */

enum dsp_stereo_modes
{
    STEREO_INTERLEAVED,
    STEREO_NONINTERLEAVED,
    STEREO_MONO,
    STEREO_NUM_MODES,
};

/* Format into for the buffer (if .valid == true) */
struct sample_format
{
    uint8_t changed;         /* 00h: 0=no change, 1=changed (is also index) */
    uint8_t num_channels;    /* 01h: number of channels of data */
    uint8_t frac_bits;       /* 02h: number of fractional bits */
    uint8_t output_scale;    /* 03h: output scaling shift */
    int32_t frequency;       /* 04h: pitch-adjusted sample rate */
    int32_t codec_frequency; /* 08h: codec-specifed sample rate */
                             /* 0ch */
};

/* Compare format data only */
#define EQU_SAMPLE_FORMAT(f1, f2) \
    (!memcmp(&(f1).num_channels, &(f2).num_channels, \
             sizeof (f1) - sizeof ((f1).changed)))

static inline void format_change_set(struct sample_format *f)
    { f->changed = 1; }
static inline void format_change_ack(struct sample_format *f)
    { f->changed = 0; }

/* Used by ASM routines - keep field order or else fix the functions */
struct dsp_buffer
{
    int32_t remcount;       /* 00h: Samples in buffer (In, Int, Out) */
    union
    {
        const void *pin[2]; /* 04h: Channel pointers (In) */
        int32_t *p32[2];    /* 04h: Channel pointers (Int) */
        int16_t *p16out;    /* 04h: DSP output buffer (Out) */
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
    struct sample_format format; /* 10h: Buffer format data */
                                 /* 1ch */
};

/* Remove samples from input buffer (In). Sample size is specified.
   Provided to dsp_process(). */
static inline void dsp_advance_buffer_input(struct dsp_buffer *buf,
                                            int by_count,
                                            size_t size_each)
{
    buf->remcount -= by_count;
    buf->pin[0] += by_count * size_each;
    buf->pin[1] += by_count * size_each;
}

/* Add samples to output buffer and update remaining space (Out).
   Provided to dsp_process() */
static inline void dsp_advance_buffer_output(struct dsp_buffer *buf,
                                             int by_count)
{
    buf->bufcount -= by_count;
    buf->remcount += by_count;
    buf->p16out += 2 * by_count; /* Interleaved stereo */
}

/* Remove samples from internal input buffer (In, Int).
   Provided to dsp_process() or by another processing stage. */
static inline void dsp_advance_buffer32(struct dsp_buffer *buf,
                                        int by_count)
{
    buf->remcount -= by_count;
    buf->p32[0] += by_count;
    buf->p32[1] += by_count;
}

/* Structure used with DSP_SET_REPLAY_GAIN message */
struct dsp_replay_gains
{
    long track_gain;
    long album_gain;
    long track_peak;
    long album_peak;
};

/** For use by processing stages **/

#define DSP_PRINT_FORMAT(name, id, format) \
    DEBUGF("DSP format- " #name "\n"                          \
           "  id:%d chg:%c ch:%u fb:%u os:%u hz:%u chz:%u\n", \
           (int)id,                                           \
           (format).changed ? 'y' : 'n',                      \
           (unsigned)(format).num_channels,                   \
           (unsigned)(format).frac_bits,                      \
           (unsigned)(format).output_scale,                   \
           (unsigned)(format).frequency,                      \
           (unsigned)(format).codec_frequency);

/* Enable/disable a processing stage - not to be called during processing
 * by processing code! */
void dsp_proc_enable(struct dsp_config *dsp, enum dsp_proc_ids id,
                     bool enable);
/* Activate/deactivate processing stage, doesn't affect enabled status
 * thus will not enable anything -
 * may be called during processing to activate/deactivate for format
 * changes */
void dsp_proc_activate(struct dsp_config *dsp, enum dsp_proc_ids id,
                       bool activate);

/* Is the specified stage active on the DSP? */
bool dsp_proc_active(struct dsp_config *dsp, enum dsp_proc_ids id);

/* Call this->process[fmt] according to the rules */
bool dsp_proc_call(struct dsp_proc_entry *this, struct dsp_buffer **buf_p,
                   unsigned fmt);

/* Generic handler for this->process[1] */
void dsp_format_change_process(struct dsp_proc_entry *this,
                               struct dsp_buffer **buf_p);

/** General DSP processing **/

/* Process the given buffer - see implementation in dsp.c for more */
void dsp_process(struct dsp_config *dsp, struct dsp_buffer *src,
                 struct dsp_buffer *dst);

/* Change DSP settings */
intptr_t dsp_configure(struct dsp_config *dsp, enum dsp_settings setting,
                       intptr_t value);

/* Get DSP pointer */
struct dsp_config * dsp_get_config(enum dsp_ids);

/* Get DSP id */
enum dsp_ids dsp_get_id(const struct dsp_config *dsp);

#if 0 /* Not needed now but enable if something must know this */
/* Is the DSP processing a buffer? */
#ifdef HAVE_PITCHSCREEN
bool dsp_is_busy(const struct dsp_config *dsp);
#endif
#endif /* 0 */

/** DSP-side settings that are not intrinsicly part of a particular stage **/

/* Set the tri-pdf dithered output */
void dsp_dither_enable(bool enable);

void sound_set_pitch(int32_t ratio);
int32_t sound_get_pitch(void);

int get_replaygain_mode(bool have_track_gain, bool have_album_gain);
void dsp_set_replaygain(void);


/* Callback for firmware layers to interface */
int dsp_callback(int msg, intptr_t param);

/* One-time startup init that must come before settings reset/apply */
void dsp_init(void);

#endif /* _DSP_H */
