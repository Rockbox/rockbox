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

struct dsp_config;

enum dsp_ids
{
    CODEC_IDX_AUDIO,
    CODEC_IDX_VOICE,
    DSP_COUNT,
};

enum dsp_settings
{
    DSP_INIT, /* For dsp_init */
    DSP_RESET,
    DSP_SET_FREQUENCY,
    DSP_SWITCH_FREQUENCY = DSP_SET_FREQUENCY, /* deprecated */
    DSP_SET_SAMPLE_DEPTH,
    DSP_SET_STEREO_MODE,
    DSP_FLUSH,
    DSP_PROC_INIT,
    DSP_PROC_CLOSE,
    DSP_PROC_SETTING, /* stage-specific should be this + id */
};

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

/** For use by processing stages **/

#define DSP_PRINT_FORMAT(name, id, format) \
    DEBUGF("DSP format- " #name "\n"                          \
           "  id:%d chg:%c ch:%u fb:%u os:%u hz:%u chz:%u\n", \
           (int)id,                                           \
           (format).changed ? 'y' : 'n',                      \
           (unsigned int)(format).num_channels,               \
           (unsigned int)(format).frac_bits,                  \
           (unsigned int)(format).output_scale,               \
           (unsigned int)(format).frequency,                  \
           (unsigned int)(format).codec_frequency);

/* Get DSP pointer */
struct dsp_config * dsp_get_config(enum dsp_ids id);

/* Get DSP id */
enum dsp_ids dsp_get_id(const struct dsp_config *dsp);

#if 0 /* Not needed now but enable if something must know this */
/* Is the DSP processing a buffer? */
bool dsp_is_busy(const struct dsp_config *dsp);
#endif /* 0 */

/** General DSP processing **/

/* Process the given buffer - see implementation in dsp.c for more */
void dsp_process(struct dsp_config *dsp, struct dsp_buffer *src,
                 struct dsp_buffer *dst);

/* Change DSP settings */
intptr_t dsp_configure(struct dsp_config *dsp, unsigned int setting,
                       intptr_t value);

/* One-time startup init that must come before settings reset/apply */
void dsp_init(void);

#endif /* _DSP_H */
