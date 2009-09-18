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
#include "config.h"
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <sound.h>
#include "dsp.h"
#include "eq.h"
#include "kernel.h"
#include "playback.h"
#include "system.h"
#include "settings.h"
#include "replaygain.h"
#include "misc.h"
#include "tdspeed.h"
#include "buffer.h"
#include "fixedpoint.h"
#include "fracmul.h"
#include "pcmbuf.h"

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"

/* 16-bit samples are scaled based on these constants. The shift should be
 * no more than 15.
 */
#define WORD_SHIFT              12
#define WORD_FRACBITS           27

#define NATIVE_DEPTH            16
/* If the small buffer size changes, check the assembly code! */
#define SMALL_SAMPLE_BUF_COUNT  256
#define DEFAULT_GAIN            0x01000000

/* enums to index conversion properly with stereo mode and other settings */
enum
{
    SAMPLE_INPUT_LE_NATIVE_I_STEREO  = STEREO_INTERLEAVED,
    SAMPLE_INPUT_LE_NATIVE_NI_STEREO = STEREO_NONINTERLEAVED,
    SAMPLE_INPUT_LE_NATIVE_MONO      = STEREO_MONO,
    SAMPLE_INPUT_GT_NATIVE_I_STEREO  = STEREO_INTERLEAVED + STEREO_NUM_MODES,
    SAMPLE_INPUT_GT_NATIVE_NI_STEREO = STEREO_NONINTERLEAVED + STEREO_NUM_MODES,
    SAMPLE_INPUT_GT_NATIVE_MONO      = STEREO_MONO + STEREO_NUM_MODES,
    SAMPLE_INPUT_GT_NATIVE_1ST_INDEX = STEREO_NUM_MODES
};

enum
{
    SAMPLE_OUTPUT_MONO = 0,
    SAMPLE_OUTPUT_STEREO,
    SAMPLE_OUTPUT_DITHERED_MONO,
    SAMPLE_OUTPUT_DITHERED_STEREO
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

/* This is for passing needed data to assembly dsp routines. If another
 * dsp parameter needs to be passed, add to the end of the structure
 * and remove from dsp_config.
 * If another function type becomes assembly optimized and requires dsp
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
                                        /* 24h */
};

/* No asm...yet */
struct dither_data
{
    long error[3];  /* 00h */
    long random;    /* 0ch */
                    /* 10h */
};

struct crossfeed_data
{
    int32_t gain;           /* 00h - Direct path gain */
    int32_t coefs[3];       /* 04h - Coefficients for the shelving filter */
    int32_t history[4];     /* 10h - Format is x[n - 1], y[n - 1] for both channels */
    int32_t delay[13][2];   /* 20h */
    int32_t *index;         /* 88h - Current pointer into the delay line */
                            /* 8ch */
};

/* Current setup is one lowshelf filters three peaking filters and one
 *  highshelf filter. Varying the number of shelving filters make no sense,
 *  but adding peaking filters is possible.
 */
struct eq_state
{
    char enabled[5];            /* 00h - Flags for active filters */
    struct eqfilter filters[5]; /* 08h - packing is 4? */
                                /* 10ch */
};

/* Include header with defines which functions are implemented in assembly
   code for the target */
#include <dsp_asm.h>

/* Typedefs keep things much neater in this case */
typedef void (*sample_input_fn_type)(int count, const char *src[],
                                     int32_t *dst[]);
typedef int (*resample_fn_type)(int count, struct dsp_data *data,
                                const int32_t *src[], int32_t *dst[]);
typedef void (*sample_output_fn_type)(int count, struct dsp_data *data,
                                      const int32_t *src[], int16_t *dst);

/* Single-DSP channel processing in place */
typedef void (*channels_process_fn_type)(int count, int32_t *buf[]);
/* DSP local channel processing in place */
typedef void (*channels_process_dsp_fn_type)(int count, struct dsp_data *data,
                                             int32_t *buf[]);
/* DSP processes that return a value */
typedef int (*return_fn_type)(int count, int32_t *buf[]);

/*
 ***************************************************************************/

struct dsp_config
{
    struct dsp_data data; /* Config members for use in asm routines */
    long codec_frequency; /* Sample rate of data coming from the codec */
    long frequency;       /* Effective sample rate after pitch shift (if any) */
    int  sample_depth;
    int  sample_bytes;
    int  stereo_mode;
    int32_t  tdspeed_percent; /* Speed% * PITCH_SPEED_PRECISION */
    bool tdspeed_active;  /* Timestretch is in use */
    int  frac_bits;
    long limiter_preamp;     /* limiter amp gain in S7.24 format */
#ifdef HAVE_SW_TONE_CONTROLS
    /* Filter struct for software bass/treble controls */
    struct eqfilter tone_filter;
#endif
    /* Functions that change depending upon settings - NULL if stage is
       disabled */
    sample_input_fn_type         input_samples;
    resample_fn_type             resample;
    sample_output_fn_type        output_samples;
    /* These will be NULL for the voice codec and is more economical that
       way */
    channels_process_dsp_fn_type apply_gain;
    channels_process_fn_type     apply_crossfeed;
    channels_process_fn_type     eq_process;
    channels_process_fn_type     channels_process;
    return_fn_type               limiter_process;
};

/* General DSP config */
static struct dsp_config dsp_conf[2] IBSS_ATTR;     /* 0=A, 1=V */
/* Dithering */
static struct dither_data dither_data[2] IBSS_ATTR; /* 0=left, 1=right */
static long   dither_mask IBSS_ATTR;
static long   dither_bias IBSS_ATTR;
/* Crossfeed */
struct crossfeed_data crossfeed_data IDATA_ATTR =    /* A */
{
    .index = (int32_t *)crossfeed_data.delay
};

/* Equalizer */
static struct eq_state eq_data;                     /* A */

/* Software tone controls */
#ifdef HAVE_SW_TONE_CONTROLS
static int prescale;                                /* A/V */
static int bass;                                    /* A/V */
static int treble;                                  /* A/V */
#endif

/* Settings applicable to audio codec only */
static int32_t  pitch_ratio = PITCH_SPEED_100;
static int  channels_mode;
       long dsp_sw_gain;
       long dsp_sw_cross;
static bool dither_enabled;
static long eq_precut;
static long track_gain;
static bool new_gain;
static long album_gain;
static long track_peak;
static long album_peak;
static long replaygain;
static bool crossfeed_enabled;

/* limiter */
static int      count_adjust;
static bool     limiter_buffer_active;
static bool     limiter_buffer_full;
static bool     limiter_buffer_emptying;
static int32_t  limiter_buffer[2][LIMITER_BUFFER_SIZE] IBSS_ATTR;
static int32_t  *start_lim_buf[2] IBSS_ATTR,
                *end_lim_buf[2] IBSS_ATTR;
static uint16_t lim_buf_peak[LIMITER_BUFFER_SIZE] IBSS_ATTR;
static uint16_t *start_peak IBSS_ATTR,
                *end_peak IBSS_ATTR;
static uint16_t out_buf_peak[LIMITER_BUFFER_SIZE] IBSS_ATTR;
static uint16_t *out_buf_peak_index IBSS_ATTR;
static uint16_t release_peak IBSS_ATTR;
static int32_t  in_samp IBSS_ATTR,
                samp0 IBSS_ATTR;

static void     reset_limiter_buffer(struct dsp_config *dsp);
static int      limiter_buffer_count(bool buf_count);
static int      limiter_process(int count, int32_t *buf[]);
static uint16_t get_peak_value(int32_t sample);

 /* The clip_steps array essentially stores the results of fp_factor from
 * 0 to 12 dB, in 48 equal steps, in S3.28 format. */
const  long     clip_steps[49] ICONST_ATTR = {      0x10000000,
    0x10779AFA, 0x10F2B409, 0x1171654C, 0x11F3C9A0, 0x1279FCAD,
    0x13041AE9, 0x139241A2, 0x14248EF9, 0x14BB21F9, 0x15561A92,
    0x15F599A0, 0x1699C0F9, 0x1742B36B, 0x17F094CE, 0x18A38A01,
    0x195BB8F9, 0x1A1948C5, 0x1ADC619B, 0x1BA52CDC, 0x1C73D51D,
    0x1D488632, 0x1E236D3A, 0x1F04B8A1, 0x1FEC982C, 0x20DB3D0E,
    0x21D0D9E2, 0x22CDA2BE, 0x23D1CD41, 0x24DD9099, 0x25F12590,
    0x270CC693, 0x2830AFD3, 0x295D1F37, 0x2A925471, 0x2BD0911F,
    0x2D1818B3, 0x2E6930AD, 0x2FC42095, 0x312931EC, 0x3298B072,
    0x3412EA24, 0x35982F3A, 0x3728D22E, 0x38C52808, 0x3A6D8847,
    0x3C224CD9, 0x3DE3D264, 0x3FB2783F};
/* The gain_steps array essentially stores the results of fp_factor from
 * 0 to -12 dB, in 48 equal steps, in S3.28 format. */
const  long     gain_steps[49] ICONST_ATTR = { 0x10000000,
    0xF8BC9C0, 0xF1ADF94, 0xEAD2988, 0xE429058, 0xDDAFD68,
    0xD765AC1, 0xD149309, 0xCB59186, 0xC594210, 0xBFF9112,
    0xBA86B88, 0xB53BEF5, 0xB017965, 0xAB18964, 0xA63DDFE,
    0xA1866BA, 0x9CF1397, 0x987D507, 0x9429BEE, 0x8FF599E,
    0x8BDFFD3, 0x87E80B0, 0x840CEBE, 0x804DCE8, 0x7CA9E76,
    0x792070E, 0x75B0AB0, 0x7259DB2, 0x6F1B4BF, 0x6BF44D5,
    0x68E4342, 0x65EA5A0, 0x63061D6, 0x6036E15, 0x5D7C0D3,
    0x5AD50CE, 0x5841505, 0x55C04B8, 0x535176A, 0x50F44D9,
    0x4EA84FE, 0x4C6D00E, 0x4A41E78, 0x48268DF, 0x461A81C,
    0x441D53E, 0x422E985, 0x404DE62};

#define AUDIO_DSP (dsp_conf[CODEC_IDX_AUDIO])
#define VOICE_DSP (dsp_conf[CODEC_IDX_VOICE])

/* The internal format is 32-bit samples, non-interleaved, stereo. This
 * format is similar to the raw output from several codecs, so the amount
 * of copying needed is minimized for that case.
 */

#define RESAMPLE_RATIO          4 /* Enough for 11,025 Hz -> 44,100 Hz */

static int32_t small_sample_buf[SMALL_SAMPLE_BUF_COUNT] IBSS_ATTR;
static int32_t small_resample_buf[SMALL_SAMPLE_BUF_COUNT * RESAMPLE_RATIO] IBSS_ATTR;

static int32_t *big_sample_buf = NULL;
static int32_t *big_resample_buf = NULL;
static int big_sample_buf_count = -1;  /* -1=unknown, 0=not available */

static int sample_buf_count;
static int32_t *sample_buf;
static int32_t *resample_buf;

#define SAMPLE_BUF_LEFT_CHANNEL 0
#define SAMPLE_BUF_RIGHT_CHANNEL (sample_buf_count/2)
#define RESAMPLE_BUF_LEFT_CHANNEL 0
#define RESAMPLE_BUF_RIGHT_CHANNEL (sample_buf_count/2 * RESAMPLE_RATIO)


/* Clip sample to signed 16 bit range */
static inline int32_t clip_sample_16(int32_t sample)
{
    if ((int16_t)sample != sample)
        sample = 0x7fff ^ (sample >> 31);
    return sample;
}

int32_t sound_get_pitch(void)
{
    return pitch_ratio;
}

void sound_set_pitch(int32_t percent)
{
    pitch_ratio = percent;
    dsp_configure(&AUDIO_DSP, DSP_SWITCH_FREQUENCY,
                  AUDIO_DSP.codec_frequency);
}

static void tdspeed_setup(struct dsp_config *dspc)
{
    /* Assume timestretch will not be used */
    dspc->tdspeed_active = false;
    sample_buf = small_sample_buf;
    resample_buf = small_resample_buf;
    sample_buf_count = SMALL_SAMPLE_BUF_COUNT;

    if(!dsp_timestretch_available())
        return; /* Timestretch not enabled or buffer not allocated */
    if (dspc->tdspeed_percent == 0)
        dspc->tdspeed_percent = PITCH_SPEED_100;
    if (!tdspeed_config(
        dspc->codec_frequency == 0 ? NATIVE_FREQUENCY : dspc->codec_frequency,
        dspc->stereo_mode != STEREO_MONO,
        dspc->tdspeed_percent))
        return; /* Timestretch not possible or needed with these parameters */

    /* Timestretch is to be used */
    dspc->tdspeed_active = true;
    sample_buf = big_sample_buf;
    sample_buf_count = big_sample_buf_count;
    resample_buf = big_resample_buf;
}

void dsp_timestretch_enable(bool enabled)
{
    /* Hook to set up timestretch buffer on first call to settings_apply() */
    if (big_sample_buf_count < 0) /* Only do something on first call */
    {
        if (enabled)
        {
            /* Set up timestretch buffers */
            big_sample_buf_count = SMALL_SAMPLE_BUF_COUNT * RESAMPLE_RATIO;
            big_sample_buf = small_resample_buf;
            big_resample_buf = (int32_t *) buffer_alloc(big_sample_buf_count * RESAMPLE_RATIO * sizeof(int32_t));
        }
        else
        {
            /* Not enabled at startup, "big" buffers will never be available */
            big_sample_buf_count = 0;
        }
        tdspeed_setup(&AUDIO_DSP);
    }
}

void dsp_set_timestretch(int32_t percent)
{
    AUDIO_DSP.tdspeed_percent = percent;
    tdspeed_setup(&AUDIO_DSP);
}

int32_t dsp_get_timestretch()
{
    return AUDIO_DSP.tdspeed_percent;
}

bool dsp_timestretch_available()
{
    return (global_settings.timestretch_enabled && big_sample_buf_count > 0);
}

/* Convert count samples to the internal format, if needed.  Updates src
 * to point past the samples "consumed" and dst is set to point to the
 * samples to consume. Note that for mono, dst[0] equals dst[1], as there
 * is no point in processing the same data twice.
 */

/* convert count 16-bit mono to 32-bit mono */
static void sample_input_lte_native_mono(
    int count, const char *src[], int32_t *dst[])
{
    const int16_t *s = (int16_t *) src[0];
    const int16_t * const send = s + count;
    int32_t *d = dst[0] = dst[1] = &sample_buf[SAMPLE_BUF_LEFT_CHANNEL];
    int scale = WORD_SHIFT;

    while (s < send)
    {
        *d++ = *s++ << scale;
    }

    src[0] = (char *)s;
}

/* convert count 16-bit interleaved stereo to 32-bit noninterleaved */
static void sample_input_lte_native_i_stereo(
    int count, const char *src[], int32_t *dst[])
{
    const int32_t *s = (int32_t *) src[0];
    const int32_t * const send = s + count;
    int32_t *dl = dst[0] = &sample_buf[SAMPLE_BUF_LEFT_CHANNEL];
    int32_t *dr = dst[1] = &sample_buf[SAMPLE_BUF_RIGHT_CHANNEL];
    int scale = WORD_SHIFT;

    while (s < send)
    {
        int32_t slr = *s++;
#ifdef ROCKBOX_LITTLE_ENDIAN
        *dl++ = (slr >> 16) << scale;
        *dr++ = (int32_t)(int16_t)slr << scale;
#else  /* ROCKBOX_BIG_ENDIAN */
        *dl++ = (int32_t)(int16_t)slr << scale;
        *dr++ = (slr >> 16) << scale;
#endif
    }

    src[0] = (char *)s;
}

/* convert count 16-bit noninterleaved stereo to 32-bit noninterleaved */
static void sample_input_lte_native_ni_stereo(
    int count, const char *src[], int32_t *dst[])
{
    const int16_t *sl = (int16_t *) src[0];
    const int16_t *sr = (int16_t *) src[1];
    const int16_t * const slend = sl + count;
    int32_t *dl = dst[0] = &sample_buf[SAMPLE_BUF_LEFT_CHANNEL];
    int32_t *dr = dst[1] = &sample_buf[SAMPLE_BUF_RIGHT_CHANNEL];
    int scale = WORD_SHIFT;

    while (sl < slend)
    {
        *dl++ = *sl++ << scale;
        *dr++ = *sr++ << scale;
    }

    src[0] = (char *)sl;
    src[1] = (char *)sr;
}

/* convert count 32-bit mono to 32-bit mono */
static void sample_input_gt_native_mono(
    int count, const char *src[], int32_t *dst[])
{
    dst[0] = dst[1] = (int32_t *)src[0];
    src[0] = (char *)(dst[0] + count);
}

/* convert count 32-bit interleaved stereo to 32-bit noninterleaved stereo */
static void sample_input_gt_native_i_stereo(
    int count, const char *src[], int32_t *dst[])
{
    const int32_t *s = (int32_t *)src[0];
    const int32_t * const send = s + 2*count;
    int32_t *dl = dst[0] = &sample_buf[SAMPLE_BUF_LEFT_CHANNEL];
    int32_t *dr = dst[1] = &sample_buf[SAMPLE_BUF_RIGHT_CHANNEL];

    while (s < send)
    {
        *dl++ = *s++;
        *dr++ = *s++;
    }

    src[0] = (char *)send;
}

/* convert 32 bit-noninterleaved stereo to 32-bit noninterleaved stereo */
static void sample_input_gt_native_ni_stereo(
    int count, const char *src[], int32_t *dst[])
{
    dst[0] = (int32_t *)src[0];
    dst[1] = (int32_t *)src[1];
    src[0] = (char *)(dst[0] + count);
    src[1] = (char *)(dst[1] + count);
}

/**
 * sample_input_new_format()
 *
 * set the to-native sample conversion function based on dsp sample parameters
 *
 * !DSPPARAMSYNC
 * needs syncing with changes to the following dsp parameters:
 *  * dsp->stereo_mode (A/V)
 *  * dsp->sample_depth (A/V)
 */
static void sample_input_new_format(struct dsp_config *dsp)
{
    static const sample_input_fn_type sample_input_functions[] =
    {
        [SAMPLE_INPUT_LE_NATIVE_I_STEREO]  = sample_input_lte_native_i_stereo,
        [SAMPLE_INPUT_LE_NATIVE_NI_STEREO] = sample_input_lte_native_ni_stereo,
        [SAMPLE_INPUT_LE_NATIVE_MONO]      = sample_input_lte_native_mono,
        [SAMPLE_INPUT_GT_NATIVE_I_STEREO]  = sample_input_gt_native_i_stereo,
        [SAMPLE_INPUT_GT_NATIVE_NI_STEREO] = sample_input_gt_native_ni_stereo,
        [SAMPLE_INPUT_GT_NATIVE_MONO]      = sample_input_gt_native_mono,
    };

    int convert = dsp->stereo_mode;

    if (dsp->sample_depth > NATIVE_DEPTH)
        convert += SAMPLE_INPUT_GT_NATIVE_1ST_INDEX;

    dsp->input_samples = sample_input_functions[convert];
}


#ifndef DSP_HAVE_ASM_SAMPLE_OUTPUT_MONO
/* write mono internal format to output format */
static void sample_output_mono(int count, struct dsp_data *data,
                               const int32_t *src[], int16_t *dst)
{
    const int32_t *s0 = src[0];
    const int scale = data->output_scale;
    const int dc_bias = 1 << (scale - 1);

    while (count-- > 0)
    {
        int32_t lr = clip_sample_16((*s0++ + dc_bias) >> scale);
        *dst++ = lr;
        *dst++ = lr;
    }
}
#endif /* DSP_HAVE_ASM_SAMPLE_OUTPUT_MONO */

/* write stereo internal format to output format */
#ifndef DSP_HAVE_ASM_SAMPLE_OUTPUT_STEREO
static void sample_output_stereo(int count, struct dsp_data *data,
                                 const int32_t *src[], int16_t *dst)
{
    const int32_t *s0 = src[0];
    const int32_t *s1 = src[1];
    const int scale = data->output_scale;
    const int dc_bias = 1 << (scale - 1);

    while (count-- > 0)
    {
        *dst++ = clip_sample_16((*s0++ + dc_bias) >> scale);
        *dst++ = clip_sample_16((*s1++ + dc_bias) >> scale);
    }
}
#endif /* DSP_HAVE_ASM_SAMPLE_OUTPUT_STEREO */

/**
 * The "dither" code to convert the 24-bit samples produced by libmad was
 * taken from the coolplayer project - coolplayer.sourceforge.net
 *
 * This function handles mono and stereo outputs.
 */
static void sample_output_dithered(int count, struct dsp_data *data,
                                   const int32_t *src[], int16_t *dst)
{
    const int32_t mask = dither_mask;
    const int32_t bias = dither_bias;
    const int scale = data->output_scale;
    const int32_t min = data->clip_min;
    const int32_t max = data->clip_max;
    const int32_t range = max - min;
    int ch;
    int16_t *d;

    for (ch = 0; ch < data->num_channels; ch++)
    {
        struct dither_data * const dither = &dither_data[ch];
        const int32_t *s = src[ch];
        int i;

        for (i = 0, d = &dst[ch]; i < count; i++, s++, d += 2)
        {
            int32_t output, sample;
            int32_t random;

            /* Noise shape and bias (for correct rounding later) */
            sample = *s;
            sample += dither->error[0] - dither->error[1] + dither->error[2];
            dither->error[2] = dither->error[1];
            dither->error[1] = dither->error[0]/2;

            output = sample + bias;

            /* Dither, highpass triangle PDF */
            random = dither->random*0x0019660dL + 0x3c6ef35fL;
            output += (random & mask) - (dither->random & mask);
            dither->random = random;

            /* Round sample to output range */
            output &= ~mask;

            /* Error feedback */
            dither->error[0] = sample - output;

            /* Clip */
            if ((uint32_t)(output - min) > (uint32_t)range)
            {
                int32_t c = min;
                if (output > min)
                    c += range;
                output = c;
            }

            /* Quantize and store */
            *d = output >> scale;
        }
    }

    if (data->num_channels == 2)
        return;

    /* Have to duplicate left samples into the right channel since
       pcm buffer and hardware is interleaved stereo */
    d = &dst[0];

    while (count-- > 0)
    {
        int16_t s = *d++;
        *d++ = s;
    }
}

/**
 * sample_output_new_format()
 *
 * set the from-native to ouput sample conversion routine
 *
 * !DSPPARAMSYNC
 * needs syncing with changes to the following dsp parameters:
 *  * dsp->stereo_mode (A/V)
 *  * dither_enabled (A)
 */
static void sample_output_new_format(struct dsp_config *dsp)
{
    static const sample_output_fn_type sample_output_functions[] =
    {
        sample_output_mono,
        sample_output_stereo,
        sample_output_dithered,
        sample_output_dithered
    };

    int out = dsp->data.num_channels - 1;

    if (dsp == &AUDIO_DSP && dither_enabled)
        out += 2;

    dsp->output_samples = sample_output_functions[out];
}

/**
 * Linear interpolation resampling that introduces a one sample delay because
 * of our inability to look into the future at the end of a frame.
 */
#ifndef DSP_HAVE_ASM_RESAMPLING
static int dsp_downsample(int count, struct dsp_data *data,
                          const int32_t *src[], int32_t *dst[])
{
    int ch = data->num_channels - 1;
    uint32_t delta = data->resample_data.delta;
    uint32_t phase, pos;
    int32_t *d;

    /* Rolled channel loop actually showed slightly faster. */
    do
    {
        /* Just initialize things and not worry too much about the relatively
         * uncommon case of not being able to spit out a sample for the frame.
         */
        const int32_t *s = src[ch];
        int32_t last = data->resample_data.last_sample[ch];

        data->resample_data.last_sample[ch] = s[count - 1];
        d = dst[ch];
        phase = data->resample_data.phase;
        pos = phase >> 16;

        /* Do we need last sample of previous frame for interpolation? */
        if (pos > 0)
            last = s[pos - 1];

        while (pos < (uint32_t)count)
        {
            *d++ = last + FRACMUL((phase & 0xffff) << 15, s[pos] - last);
            phase += delta;
            pos = phase >> 16;
            last = s[pos - 1];
        }
    }
    while (--ch >= 0);

    /* Wrap phase accumulator back to start of next frame. */
    data->resample_data.phase = phase - (count << 16);
    return d - dst[0];
}

static int dsp_upsample(int count, struct dsp_data *data,
                        const int32_t *src[], int32_t *dst[])
{
    int  ch = data->num_channels - 1;
    uint32_t delta = data->resample_data.delta;
    uint32_t phase, pos;
    int32_t *d;

    /* Rolled channel loop actually showed slightly faster. */
    do
    {
        /* Should always be able to output a sample for a ratio up to RESAMPLE_RATIO */
        const int32_t *s = src[ch];
        int32_t last = data->resample_data.last_sample[ch];

        data->resample_data.last_sample[ch] = s[count - 1];
        d = dst[ch];
        phase = data->resample_data.phase;
        pos = phase >> 16;

        while (pos == 0)
        {
            *d++ = last + FRACMUL((phase & 0xffff) << 15, s[0] - last);
            phase += delta;
            pos = phase >> 16;
        }

        while (pos < (uint32_t)count)
        {
            last = s[pos - 1];
            *d++ = last + FRACMUL((phase & 0xffff) << 15, s[pos] - last);
            phase += delta;
            pos = phase >> 16;
        }
    }
    while (--ch >= 0);

    /* Wrap phase accumulator back to start of next frame. */
    data->resample_data.phase = phase & 0xffff;
    return d - dst[0];
}
#endif /* DSP_HAVE_ASM_RESAMPLING */

static void resampler_new_delta(struct dsp_config *dsp)
{
    dsp->data.resample_data.delta = (unsigned long)
        dsp->frequency * 65536LL / NATIVE_FREQUENCY;

    if (dsp->frequency == NATIVE_FREQUENCY)
    {
        /* NOTE: If fully glitch-free transistions from no resampling to
           resampling are desired, last_sample history should be maintained
           even when not resampling. */
        dsp->resample = NULL;
        dsp->data.resample_data.phase = 0;
        dsp->data.resample_data.last_sample[0] = 0;
        dsp->data.resample_data.last_sample[1] = 0;
    }
    else if (dsp->frequency < NATIVE_FREQUENCY)
        dsp->resample = dsp_upsample;
    else
        dsp->resample = dsp_downsample;
}

/* Resample count stereo samples. Updates the src array, if resampling is
 * done, to refer to the resampled data. Returns number of stereo samples
 * for further processing.
 */
static inline int resample(struct dsp_config *dsp, int count, int32_t *src[])
{
    int32_t *dst[2] =
    {
        &resample_buf[RESAMPLE_BUF_LEFT_CHANNEL],
        &resample_buf[RESAMPLE_BUF_RIGHT_CHANNEL],
    };

    count = dsp->resample(count, &dsp->data, (const int32_t **)src, dst);

    src[0] = dst[0];
    src[1] = dst[dsp->data.num_channels - 1];

    return count;
}

static void dither_init(struct dsp_config *dsp)
{
    memset(dither_data, 0, sizeof (dither_data));
    dither_bias = (1L << (dsp->frac_bits - NATIVE_DEPTH));
    dither_mask = (1L << (dsp->frac_bits + 1 - NATIVE_DEPTH)) - 1;
}

void dsp_dither_enable(bool enable)
{
    struct dsp_config *dsp = &AUDIO_DSP;
    dither_enabled = enable;
    sample_output_new_format(dsp);
}

/* Applies crossfeed to the stereo signal in src.
 * Crossfeed is a process where listening over speakers is simulated. This
 * is good for old hard panned stereo records, which might be quite fatiguing
 * to listen to on headphones with no crossfeed.
 */
#ifndef DSP_HAVE_ASM_CROSSFEED
static void apply_crossfeed(int count, int32_t *buf[])
{
    int32_t *hist_l = &crossfeed_data.history[0];
    int32_t *hist_r = &crossfeed_data.history[2];
    int32_t *delay = &crossfeed_data.delay[0][0];
    int32_t *coefs = &crossfeed_data.coefs[0];
    int32_t gain = crossfeed_data.gain;
    int32_t *di = crossfeed_data.index;

    int32_t acc;
    int32_t left, right;
    int i;

    for (i = 0; i < count; i++)
    {
        left = buf[0][i];
        right = buf[1][i];

        /* Filter delayed sample from left speaker */
        acc = FRACMUL(*di, coefs[0]);
        acc += FRACMUL(hist_l[0], coefs[1]);
        acc += FRACMUL(hist_l[1], coefs[2]);
        /* Save filter history for left speaker */
        hist_l[1] = acc;
        hist_l[0] = *di;
        *di++ = left;
        /* Filter delayed sample from right speaker */
        acc = FRACMUL(*di, coefs[0]);
        acc += FRACMUL(hist_r[0], coefs[1]);
        acc += FRACMUL(hist_r[1], coefs[2]);
        /* Save filter history for right speaker */
        hist_r[1] = acc;
        hist_r[0] = *di;
        *di++ = right;
        /* Now add the attenuated direct sound and write to outputs */
        buf[0][i] = FRACMUL(left, gain) + hist_r[1];
        buf[1][i] = FRACMUL(right, gain) + hist_l[1];

        /* Wrap delay line index if bigger than delay line size */
        if (di >= delay + 13*2)
            di = delay;
    }
    /* Write back local copies of data we've modified */
    crossfeed_data.index = di;
}
#endif /* DSP_HAVE_ASM_CROSSFEED */

/**
 * dsp_set_crossfeed(bool enable)
 *
 * !DSPPARAMSYNC
 * needs syncing with changes to the following dsp parameters:
 *  * dsp->stereo_mode (A)
 */
void dsp_set_crossfeed(bool enable)
{
    crossfeed_enabled = enable;
    AUDIO_DSP.apply_crossfeed = (enable && AUDIO_DSP.data.num_channels > 1)
                                    ? apply_crossfeed : NULL;
}

void dsp_set_crossfeed_direct_gain(int gain)
{
    crossfeed_data.gain = get_replaygain_int(gain * 10) << 7;
    /* If gain is negative, the calculation overflowed and we need to clamp */
    if (crossfeed_data.gain < 0)
        crossfeed_data.gain = 0x7fffffff;
}

/* Both gains should be below 0 dB */
void dsp_set_crossfeed_cross_params(long lf_gain, long hf_gain, long cutoff)
{
    int32_t *c = crossfeed_data.coefs;
    long scaler = get_replaygain_int(lf_gain * 10) << 7;

    cutoff = 0xffffffff/NATIVE_FREQUENCY*cutoff;
    hf_gain -= lf_gain;
    /* Divide cutoff by sqrt(10^(hf_gain/20)) to place cutoff at the -3 dB
     * point instead of shelf midpoint. This is for compatibility with the old
     * crossfeed shelf filter and should be removed if crossfeed settings are
     * ever made incompatible for any other good reason.
     */
    cutoff = fp_div(cutoff, get_replaygain_int(hf_gain*5), 24);
    filter_shelf_coefs(cutoff, hf_gain, false, c);
    /* Scale coefs by LF gain and shift them to s0.31 format. We have no gains
     * over 1 and can do this safely
     */
    c[0] = FRACMUL_SHL(c[0], scaler, 4);
    c[1] = FRACMUL_SHL(c[1], scaler, 4);
    c[2] <<= 4;
}

/* Apply a constant gain to the samples (e.g., for ReplayGain).
 * Note that this must be called before the resampler.
 */
#ifndef DSP_HAVE_ASM_APPLY_GAIN
static void dsp_apply_gain(int count, struct dsp_data *data, int32_t *buf[])
{
    const int32_t gain = data->gain;
    int ch;

    for (ch = 0; ch < data->num_channels; ch++)
    {
        int32_t *d = buf[ch];
        int i;

        for (i = 0; i < count; i++)
            d[i] = FRACMUL_SHL(d[i], gain, 8);
    }
}
#endif /* DSP_HAVE_ASM_APPLY_GAIN */

/* Combine all gains to a global gain. */
static void set_gain(struct dsp_config *dsp)
{
    /* gains are in S7.24 format */
    dsp->data.gain = DEFAULT_GAIN;

    /* Replay gain not relevant to voice */
    if (dsp == &AUDIO_DSP && replaygain)
    {
        dsp->data.gain = replaygain;
    }

    if (dsp->eq_process && eq_precut)
    {
        dsp->data.gain = fp_mul(dsp->data.gain, eq_precut, 24);
    }

    /* only preamp for the limiter if limiter is active and sample depth
     * allows safe pre-amping (12 dB is OK with 29 or less frac bits) */
    if ((dsp->limiter_preamp) && (dsp->frac_bits <= 29))
    {
        dsp->data.gain = fp_mul(dsp->data.gain, dsp->limiter_preamp, 24);
    }

#ifdef HAVE_SW_VOLUME_CONTROL
    if (global_settings.volume < SW_VOLUME_MAX ||
        global_settings.volume > SW_VOLUME_MIN)
    {
        int vol_gain = get_replaygain_int(global_settings.volume * 100);
        dsp->data.gain = (long) (((int64_t) dsp->data.gain * vol_gain) >> 24);
    }
#endif

    if (dsp->data.gain == DEFAULT_GAIN)
    {
        dsp->data.gain = 0;
    }
    else
    {
        dsp->data.gain >>= 1;   /* convert gain to S8.23 format */
    }

    dsp->apply_gain = dsp->data.gain != 0 ? dsp_apply_gain : NULL;
}

/**
 * Update the amount to cut the audio before applying the equalizer.
 *
 * @param precut to apply in decibels (multiplied by 10)
 */
void dsp_set_eq_precut(int precut)
{
    eq_precut = get_replaygain_int(precut * -10);
    set_gain(&AUDIO_DSP);
}

/**
 * Synchronize the equalizer filter coefficients with the global settings.
 *
 * @param band the equalizer band to synchronize
 */
void dsp_set_eq_coefs(int band)
{
    const int *setting;
    long gain;
    unsigned long cutoff, q;

    /* Adjust setting pointer to the band we actually want to change */
    setting = &global_settings.eq_band0_cutoff + (band * 3);

    /* Convert user settings to format required by coef generator functions */
    cutoff = 0xffffffff / NATIVE_FREQUENCY * (*setting++);
    q = *setting++;
    gain = *setting++;

    if (q == 0)
        q = 1;

    /* NOTE: The coef functions assume the EMAC unit is in fractional mode,
       which it should be, since we're executed from the main thread. */

    /* Assume a band is disabled if the gain is zero */
    if (gain == 0)
    {
        eq_data.enabled[band] = 0;
    }
    else
    {
        if (band == 0)
            eq_ls_coefs(cutoff, q, gain, eq_data.filters[band].coefs);
        else if (band == 4)
            eq_hs_coefs(cutoff, q, gain, eq_data.filters[band].coefs);
        else
            eq_pk_coefs(cutoff, q, gain, eq_data.filters[band].coefs);

        eq_data.enabled[band] = 1;
    }
}

/* Apply EQ filters to those bands that have got it switched on. */
static void eq_process(int count, int32_t *buf[])
{
    static const int shifts[] =
    {
        EQ_SHELF_SHIFT,  /* low shelf  */
        EQ_PEAK_SHIFT,   /* peaking    */
        EQ_PEAK_SHIFT,   /* peaking    */
        EQ_PEAK_SHIFT,   /* peaking    */
        EQ_SHELF_SHIFT,  /* high shelf */
    };
    unsigned int channels = AUDIO_DSP.data.num_channels;
    int i;

    /* filter configuration currently is 1 low shelf filter, 3 band peaking
       filters and 1 high shelf filter, in that order. we need to know this
       so we can choose the correct shift factor.
     */
    for (i = 0; i < 5; i++)
    {
        if (!eq_data.enabled[i])
            continue;
        eq_filter(buf, &eq_data.filters[i], count, channels, shifts[i]);
    }
}

/**
 * Use to enable the equalizer.
 *
 * @param enable true to enable the equalizer
 */
void dsp_set_eq(bool enable)
{
    AUDIO_DSP.eq_process = enable ? eq_process : NULL;
    set_gain(&AUDIO_DSP);
}

static void dsp_set_stereo_width(int value)
{
    long width, straight, cross;

    width = value * 0x7fffff / 100;

    if (value <= 100)
    {
        straight = (0x7fffff + width) / 2;
        cross = straight - width;
    }
    else
    {
        /* straight = (1 + width) / (2 * width) */
        straight = ((int64_t)(0x7fffff + width) << 22) / width;
        cross = straight - 0x7fffff;
    }

    dsp_sw_gain  = straight << 8;
    dsp_sw_cross = cross << 8;
}

/**
 * Implements the different channel configurations and stereo width.
 */

/* SOUND_CHAN_STEREO mode is a noop so has no function - just outline one for
 * completeness. */
#if 0
static void channels_process_sound_chan_stereo(int count, int32_t *buf[])
{
    /* The channels are each just themselves */
    (void)count; (void)buf;
}
#endif

#ifndef DSP_HAVE_ASM_SOUND_CHAN_MONO
static void channels_process_sound_chan_mono(int count, int32_t *buf[])
{
    int32_t *sl = buf[0], *sr = buf[1];

    while (count-- > 0)
    {
        int32_t lr = *sl/2 + *sr/2;
        *sl++ = lr;
        *sr++ = lr;
    }
}
#endif /* DSP_HAVE_ASM_SOUND_CHAN_MONO */

#ifndef DSP_HAVE_ASM_SOUND_CHAN_CUSTOM
static void channels_process_sound_chan_custom(int count, int32_t *buf[])
{
    const int32_t gain  = dsp_sw_gain;
    const int32_t cross = dsp_sw_cross;
    int32_t *sl = buf[0], *sr = buf[1];

    while (count-- > 0)
    {
        int32_t l = *sl;
        int32_t r = *sr;
        *sl++ = FRACMUL(l, gain) + FRACMUL(r, cross);
        *sr++ = FRACMUL(r, gain) + FRACMUL(l, cross);
    }
}
#endif /* DSP_HAVE_ASM_SOUND_CHAN_CUSTOM */

static void channels_process_sound_chan_mono_left(int count, int32_t *buf[])
{
    /* Just copy over the other channel */
    memcpy(buf[1], buf[0], count * sizeof (*buf));
}

static void channels_process_sound_chan_mono_right(int count, int32_t *buf[])
{
    /* Just copy over the other channel */
    memcpy(buf[0], buf[1], count * sizeof (*buf));
}

#ifndef DSP_HAVE_ASM_SOUND_CHAN_KARAOKE
static void channels_process_sound_chan_karaoke(int count, int32_t *buf[])
{
    int32_t *sl = buf[0], *sr = buf[1];

    while (count-- > 0)
    {
        int32_t ch = *sl/2 - *sr/2;
        *sl++ = ch;
        *sr++ = -ch;
    }
}
#endif /* DSP_HAVE_ASM_SOUND_CHAN_KARAOKE */

static void dsp_set_channel_config(int value)
{
    static const channels_process_fn_type channels_process_functions[] =
    {
        /* SOUND_CHAN_STEREO = All-purpose index for no channel processing */
        [SOUND_CHAN_STEREO]     = NULL,
        [SOUND_CHAN_MONO]       = channels_process_sound_chan_mono,
        [SOUND_CHAN_CUSTOM]     = channels_process_sound_chan_custom,
        [SOUND_CHAN_MONO_LEFT]  = channels_process_sound_chan_mono_left,
        [SOUND_CHAN_MONO_RIGHT] = channels_process_sound_chan_mono_right,
        [SOUND_CHAN_KARAOKE]    = channels_process_sound_chan_karaoke,
    };

    if ((unsigned)value >= ARRAYLEN(channels_process_functions) ||
        AUDIO_DSP.stereo_mode == STEREO_MONO)
    {
        value = SOUND_CHAN_STEREO;
    }

    /* This doesn't apply to voice */
    channels_mode = value;
    AUDIO_DSP.channels_process = channels_process_functions[value];
}

#if CONFIG_CODEC == SWCODEC

#ifdef HAVE_SW_TONE_CONTROLS
static void set_tone_controls(void)
{
    filter_bishelf_coefs(0xffffffff/NATIVE_FREQUENCY*200,
                         0xffffffff/NATIVE_FREQUENCY*3500,
                         bass, treble, -prescale,
                         AUDIO_DSP.tone_filter.coefs);
    /* Sync the voice dsp coefficients */
    memcpy(&VOICE_DSP.tone_filter.coefs, AUDIO_DSP.tone_filter.coefs,
           sizeof (VOICE_DSP.tone_filter.coefs));
}
#endif

/* Hook back from firmware/ part of audio, which can't/shouldn't call apps/
 * code directly.
 */
int dsp_callback(int msg, intptr_t param)
{
    switch (msg)
    {
#ifdef HAVE_SW_TONE_CONTROLS
    case DSP_CALLBACK_SET_PRESCALE:
        prescale = param;
        set_tone_controls();
        break;
    /* prescaler is always set after calling any of these, so we wait with
     * calculating coefs until the above case is hit.
     */
    case DSP_CALLBACK_SET_BASS:
        bass = param;
        break;
    case DSP_CALLBACK_SET_TREBLE:
        treble = param;
        break;
#ifdef HAVE_SW_VOLUME_CONTROL
    case DSP_CALLBACK_SET_SW_VOLUME:
        set_gain(&AUDIO_DSP);
        break;
#endif
#endif
    case DSP_CALLBACK_SET_CHANNEL_CONFIG:
        dsp_set_channel_config(param);
        break;
    case DSP_CALLBACK_SET_STEREO_WIDTH:
        dsp_set_stereo_width(param);
        break;
    default:
        break;
    }
    return 0;
}
#endif

/* Process and convert src audio to dst based on the DSP configuration,
 * reading count number of audio samples. dst is assumed to be large
 * enough; use dsp_output_count() to get the required number. src is an
 * array of pointers; for mono and interleaved stereo, it contains one
 * pointer to the start of the audio data and the other is ignored; for
 * non-interleaved stereo, it contains two pointers, one for each audio
 * channel. Returns number of bytes written to dst.
 */
int dsp_process(struct dsp_config *dsp, char *dst, const char *src[], int count)
{
    int32_t *tmp[2];
    static long last_yield;
    long tick;
    int written = 0;

#if defined(CPU_COLDFIRE)
    /* set emac unit for dsp processing, and save old macsr, we're running in
       codec thread context at this point, so can't clobber it */
    unsigned long old_macsr = coldfire_get_macsr();
    coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE);
#endif

    if (new_gain)
        dsp_set_replaygain(); /* Gain has changed */

    /* Perform at least one yield before starting */
    last_yield = current_tick;
    yield();

    /* Testing function pointers for NULL is preferred since the pointer
       will be preloaded to be used for the call if not. */
    while (count > 0)
    {
        int samples = MIN(sample_buf_count/2, count);
        count -= samples;

        dsp->input_samples(samples, src, tmp);

        if (dsp->tdspeed_active)
            samples = tdspeed_doit(tmp, samples);
        
        int chunk_offset = 0;
        while (samples > 0)
        {
            int32_t *t2[2];
            t2[0] = tmp[0]+chunk_offset;
            t2[1] = tmp[1]+chunk_offset;

            int chunk = MIN(sample_buf_count/2, samples);
            chunk_offset += chunk;
            samples -= chunk;

            if (dsp->apply_gain)
                dsp->apply_gain(chunk, &dsp->data, t2);

            if (dsp->resample && (chunk = resample(dsp, chunk, t2)) <= 0)
                break; /* I'm pretty sure we're downsampling here */

            if (dsp->apply_crossfeed)
                dsp->apply_crossfeed(chunk, t2);

            if (dsp->eq_process)
                dsp->eq_process(chunk, t2);

#ifdef HAVE_SW_TONE_CONTROLS
            if ((bass | treble) != 0)
                eq_filter(t2, &dsp->tone_filter, chunk,
                      dsp->data.num_channels, FILTER_BISHELF_SHIFT);
#endif

            if (dsp->channels_process)
                dsp->channels_process(chunk, t2);
            
            if (dsp->limiter_process)
                chunk = dsp->limiter_process(chunk, t2);

            dsp->output_samples(chunk, &dsp->data, (const int32_t **)t2, (int16_t *)dst);

            written += chunk;
            dst += chunk * sizeof (int16_t) * 2;

            /* yield at least once each tick */
            tick = current_tick;
            if (TIME_AFTER(tick, last_yield))
            {
                last_yield = tick;
                yield();
            }
        }
    }

#if defined(CPU_COLDFIRE)
    /* set old macsr again */
    coldfire_set_macsr(old_macsr);
#endif
    return written;
}

/* Given count number of input samples, calculate the maximum number of
 * samples of output data that would be generated (the calculation is not
 * entirely exact and rounds upwards to be on the safe side; during
 * resampling, the number of samples generated depends on the current state
 * of the resampler).
 */
/* dsp_input_size MUST be called afterwards */
int dsp_output_count(struct dsp_config *dsp, int count)
{
    if (dsp->tdspeed_active)
        count = tdspeed_est_output_size();
    if (dsp->resample)
    {
        count = (int)(((unsigned long)count * NATIVE_FREQUENCY
                    + (dsp->frequency - 1)) / dsp->frequency);
    }

    /* Now we have the resampled sample count which must not exceed
     * RESAMPLE_BUF_RIGHT_CHANNEL to avoid resample buffer overflow. One
     * must call dsp_input_count() to get the correct input sample
     * count.
     */
    if (count > RESAMPLE_BUF_RIGHT_CHANNEL)
        count = RESAMPLE_BUF_RIGHT_CHANNEL;
        
    /* If the limiter buffer is filling, some or all samples will
     * be captured by it, so expect fewer samples coming out. */
    if (limiter_buffer_active && !limiter_buffer_full)
    {
        int empty_space = limiter_buffer_count(false);
        count_adjust = MIN(empty_space, count);
        count -= count_adjust;
    }

    return count;
}

/* Given count output samples, calculate number of input samples
 * that would be consumed in order to fill the output buffer.
 */
int dsp_input_count(struct dsp_config *dsp, int count)
{
    /* If the limiter buffer is filling, the output count was
     * adjusted downward.  This adjusts it back so that input
     * count is not affected.
     */
    if (limiter_buffer_active && !limiter_buffer_full)
        count += count_adjust;

    /* count is now the number of resampled input samples. Convert to
       original input samples. */
    if (dsp->resample)
    {
        /* Use the real resampling delta =
         * dsp->frequency * 65536 / NATIVE_FREQUENCY, and
         * round towards zero to avoid buffer overflows. */
        count = (int)(((unsigned long)count *
                      dsp->data.resample_data.delta) >> 16);
    }

    if (dsp->tdspeed_active)
        count = tdspeed_est_input_size(count);

    return count;
}

static void dsp_set_gain_var(long *var, long value)
{
    *var = value;
    new_gain = true;
}

static void dsp_update_functions(struct dsp_config *dsp)
{
    sample_input_new_format(dsp);
    sample_output_new_format(dsp);
    if (dsp == &AUDIO_DSP)
        dsp_set_crossfeed(crossfeed_enabled);
}

intptr_t dsp_configure(struct dsp_config *dsp, int setting, intptr_t value)
{
    switch (setting)
    {
    case DSP_MYDSP:
        switch (value)
        {
        case CODEC_IDX_AUDIO:
            return (intptr_t)&AUDIO_DSP;
        case CODEC_IDX_VOICE:
            return (intptr_t)&VOICE_DSP;
        default:
            return (intptr_t)NULL;
        }

    case DSP_SET_FREQUENCY:
        memset(&dsp->data.resample_data, 0, sizeof (dsp->data.resample_data));
        /* Fall through!!! */
    case DSP_SWITCH_FREQUENCY:
        dsp->codec_frequency = (value == 0) ? NATIVE_FREQUENCY : value;
        /* Account for playback speed adjustment when setting dsp->frequency
           if we're called from the main audio thread. Voice UI thread should
           not need this feature.
         */
        if (dsp == &AUDIO_DSP)
            dsp->frequency = pitch_ratio * dsp->codec_frequency / PITCH_SPEED_100;
        else
            dsp->frequency = dsp->codec_frequency;

        resampler_new_delta(dsp);
        tdspeed_setup(dsp);
        break;

    case DSP_SET_SAMPLE_DEPTH:
        dsp->sample_depth = value;

        if (dsp->sample_depth <= NATIVE_DEPTH)
        {
            dsp->frac_bits = WORD_FRACBITS;
            dsp->sample_bytes = sizeof (int16_t); /* samples are 16 bits */
            dsp->data.clip_max =  ((1 << WORD_FRACBITS) - 1);
            dsp->data.clip_min = -((1 << WORD_FRACBITS));
        }
        else
        {
            dsp->frac_bits = value;
            dsp->sample_bytes = sizeof (int32_t); /* samples are 32 bits */
            dsp->data.clip_max = (1 << value) - 1;
            dsp->data.clip_min = -(1 << value);
        }

        dsp->data.output_scale = dsp->frac_bits + 1 - NATIVE_DEPTH;
        sample_input_new_format(dsp);
        dither_init(dsp);
        break;

    case DSP_SET_STEREO_MODE:
        dsp->stereo_mode = value;
        dsp->data.num_channels = value == STEREO_MONO ? 1 : 2;
        dsp_update_functions(dsp);
        tdspeed_setup(dsp);
        break;

    case DSP_RESET:
        dsp->stereo_mode = STEREO_NONINTERLEAVED;
        dsp->data.num_channels = 2;
        dsp->sample_depth = NATIVE_DEPTH;
        dsp->frac_bits = WORD_FRACBITS;
        dsp->sample_bytes = sizeof (int16_t);
        dsp->data.output_scale = dsp->frac_bits + 1 - NATIVE_DEPTH;
        dsp->data.clip_max =  ((1 << WORD_FRACBITS) - 1);
        dsp->data.clip_min = -((1 << WORD_FRACBITS));
        dsp->codec_frequency = dsp->frequency = NATIVE_FREQUENCY;

        if (dsp == &AUDIO_DSP)
        {
            track_gain = 0;
            album_gain = 0;
            track_peak = 0;
            album_peak = 0;
            new_gain   = true;
        }

        dsp_update_functions(dsp);
        resampler_new_delta(dsp);
        tdspeed_setup(dsp);
        reset_limiter_buffer(dsp);
        break;

    case DSP_FLUSH:
        memset(&dsp->data.resample_data, 0,
               sizeof (dsp->data.resample_data));
        resampler_new_delta(dsp);
        dither_init(dsp);
        tdspeed_setup(dsp);
        reset_limiter_buffer(dsp);
        break;

    case DSP_SET_TRACK_GAIN:
        if (dsp == &AUDIO_DSP)
            dsp_set_gain_var(&track_gain, value);
        break;

    case DSP_SET_ALBUM_GAIN:
        if (dsp == &AUDIO_DSP)
            dsp_set_gain_var(&album_gain, value);
        break;

    case DSP_SET_TRACK_PEAK:
        if (dsp == &AUDIO_DSP)
            dsp_set_gain_var(&track_peak, value);
        break;

    case DSP_SET_ALBUM_PEAK:
        if (dsp == &AUDIO_DSP)
            dsp_set_gain_var(&album_peak, value);
        break;

    default:
        return 0;
    }

    return 1;
}

void dsp_set_replaygain(void)
{
    long gain = 0;

    new_gain = false;

    if ((global_settings.replaygain_type != REPLAYGAIN_OFF) ||
            global_settings.replaygain_noclip)
    {
        bool track_mode = get_replaygain_mode(track_gain != 0,
            album_gain != 0) == REPLAYGAIN_TRACK;
        long peak = (track_mode || !album_peak) ? track_peak : album_peak;

        if (global_settings.replaygain_type != REPLAYGAIN_OFF)
        {
            gain = (track_mode || !album_gain) ? track_gain : album_gain;

            if (global_settings.replaygain_preamp)
            {
                long preamp = get_replaygain_int(
                    global_settings.replaygain_preamp * 10);

                gain = (long) (((int64_t) gain * preamp) >> 24);
            }
        }

        if (gain == 0)
        {
            /* So that noclip can work even with no gain information. */
            gain = DEFAULT_GAIN;
        }

        if (global_settings.replaygain_noclip && (peak != 0)
            && ((((int64_t) gain * peak) >> 24) >= DEFAULT_GAIN))
        {
            gain = (((int64_t) DEFAULT_GAIN << 24) / peak);
        }

        if (gain == DEFAULT_GAIN)
        {
            /* Nothing to do, disable processing. */
            gain = 0;
        }
    }

    /* Store in S7.24 format to simplify calculations. */
    replaygain = gain;
    set_gain(&AUDIO_DSP);
}

/** RESET THE LIMITER BUFFER
 *  Force the limiter buffer to its initial state and discard
 *  any samples held there. */
static void reset_limiter_buffer(struct dsp_config *dsp)
{
    if (dsp == &AUDIO_DSP)
    {
        int i;
        logf("   reset_limiter_buffer");
        for (i = 0; i < 2; i++)
            start_lim_buf[i] = end_lim_buf[i] = limiter_buffer[i];
        start_peak = end_peak = lim_buf_peak;
        limiter_buffer_full = false;
        limiter_buffer_emptying = false;
        release_peak = 0;
    }
}

/** OPERATE THE LIMITER BUFFER
 *  Handle all samples entering or exiting the limiter buffer. */
static inline int set_limiter_buffer(int count, int32_t *buf[])
{
    int32_t *in_buf[]  = {buf[0], buf[1]},
            *out_buf[] = {buf[0], buf[1]};
    int empty_space, i, out_count;
    const long clip_max = AUDIO_DSP.data.clip_max;
    const int ch = AUDIO_DSP.data.num_channels - 1;
    out_buf_peak_index = out_buf_peak;

    if (limiter_buffer_emptying)
    /** EMPTY THE BUFFER
     *  since the empty flag has been set, assume no inbound samples and
        return all samples in the limiter buffer to the outbound buffer */
    {
        count = limiter_buffer_count(true);
        out_count = count;
        logf("   Emptying limiter buffer: %d", count);
        while (count-- > 0)
        {
            for (i = 0; i <= ch; i++)
            {
                /* move samples in limiter buffer to output buffer */
                *out_buf[i]++ = *start_lim_buf[i]++;
                if (start_lim_buf[i] == &limiter_buffer[i][LIMITER_BUFFER_SIZE])
                    start_lim_buf[i] = limiter_buffer[i];
                /* move limiter buffer peak values to output peak values */
                if (i == 0)
                {
                    *out_buf_peak_index++ = *start_peak++;
                    if (start_peak == &lim_buf_peak[LIMITER_BUFFER_SIZE])
                        start_peak = lim_buf_peak;
                }
            }
        }
        limiter_buffer_full = false;
        limiter_buffer_emptying = false;
    }
    else    /* limiter buffer NOT emptying */
    {
        if (count <= 0) return 0;
        
        empty_space = limiter_buffer_count(false);
        
        if (empty_space > 0)
        /** FILL BUFFER
         *  use as many inbound samples as necessary to fill the buffer */
        {
            /* don't try to fill with more samples than available */
            if (empty_space > count)
                empty_space = count;
            logf("   Filling limiter buffer: %d", empty_space);
            while (empty_space-- > 0)
            {
                for (i = 0; i <= ch; i++)
                {
                    /* put inbound samples in the limiter buffer */
                    in_samp = *in_buf[i]++;
                    *end_lim_buf[i]++ = in_samp;
                    if (end_lim_buf[i] == &limiter_buffer[i][LIMITER_BUFFER_SIZE])
                        end_lim_buf[i] = limiter_buffer[i];
                    if (in_samp < 0)       /* make positive for comparison */
                        in_samp = -in_samp - 1;
                    if (in_samp <= clip_max)
                        in_samp = 0;       /* disregard if not clipped */
                    if (i == 0)
                        samp0 = in_samp;
                    if (i == ch)
                    {
                        /* assign peak value for each inbound sample pair */
                        *end_peak++ = ((samp0 > 0) || (in_samp > 0)) ?
                            get_peak_value(MAX(samp0, in_samp)) : 0;
                        if (end_peak == &lim_buf_peak[LIMITER_BUFFER_SIZE])
                            end_peak = lim_buf_peak;
                    }
                }
                count--;
            }
            /* after buffer fills, the remaining inbound samples are cycled */
        }
        
        limiter_buffer_full = (end_lim_buf[0] == start_lim_buf[0]);
        out_count = count;
        
        /** CYCLE BUFFER
         *  return buffered samples and backfill limiter buffer with new ones.
         *  The buffer is always full when cycling. */
        while (count-- > 0)
        {
            for (i = 0; i <= ch; i++)
            {
                /* copy incoming sample */
                in_samp = *in_buf[i]++;
                /* put limiter buffer sample into outbound buffer */
                *out_buf[i]++ = *start_lim_buf[i]++;
                /* put incoming sample on the end of the limiter buffer */
                *end_lim_buf[i]++ = in_samp;
                /* ring buffer pointer wrap */
                if (start_lim_buf[i] == &limiter_buffer[i][LIMITER_BUFFER_SIZE])
                    start_lim_buf[i] = limiter_buffer[i];
                if (end_lim_buf[i] == &limiter_buffer[i][LIMITER_BUFFER_SIZE])
                    end_lim_buf[i] = limiter_buffer[i];
                if (in_samp < 0)       /* make positive for comparison */
                    in_samp = -in_samp - 1;
                if (in_samp <= clip_max)
                    in_samp = 0;       /* disregard if not clipped */
                if (i == 0)
                {
                    samp0 = in_samp;
                    /* assign outgoing sample its associated peak value */
                    *out_buf_peak_index++ = *start_peak++;
                    if (start_peak == &lim_buf_peak[LIMITER_BUFFER_SIZE])
                        start_peak = lim_buf_peak;
                }
                if (i == ch)
                {
                    /* assign peak value for each inbound sample pair */
                    *end_peak++ = ((samp0 > 0) || (in_samp > 0)) ?
                        get_peak_value(MAX(samp0, in_samp)) : 0;
                    if (end_peak == &lim_buf_peak[LIMITER_BUFFER_SIZE])
                        end_peak = lim_buf_peak;
                }
            }
        }
    }   
    
    return out_count;
}

/** RETURN LIMITER BUFFER COUNT
 *  If argument is true, returns number of samples in the buffer,
 *  otherwise, returns empty space remaining */
static int limiter_buffer_count(bool buf_count)
{
    int count;
    if (limiter_buffer_full)
        count = LIMITER_BUFFER_SIZE;
    else if (end_lim_buf[0] >= start_lim_buf[0])
        count = (end_lim_buf[0] - start_lim_buf[0]);
    else
        count = (end_lim_buf[0] - start_lim_buf[0]) + LIMITER_BUFFER_SIZE;
    return buf_count ? count : (LIMITER_BUFFER_SIZE - count);
}

/** FLUSH THE LIMITER BUFFER
 *  Empties the limiter buffer into the buffer pointed to by the argument
 *  and returns the number of samples in that buffer */
int dsp_flush_limiter_buffer(char *dest)
{
    if ((!limiter_buffer_active) || (limiter_buffer_count(true) <= 0))
        return 0;
    
    logf("   dsp_flush_limiter_buffer");
    int32_t flush_buf[2][LIMITER_BUFFER_SIZE];
    int32_t *src[2] = {flush_buf[0], flush_buf[1]};

    limiter_buffer_emptying = true;
    int count = limiter_process(0, src);
    AUDIO_DSP.output_samples(count, &AUDIO_DSP.data,
        (const int32_t **)src, (int16_t *)dest);
    return count;
}

/** GET PEAK VALUE
 *  Return a small value representing how much the sample is clipped.  This
 *  should only be called if a sample is actually clipped.  Sample is a
 *  positive value.
 */
static uint16_t get_peak_value(int32_t sample)
{
    const int frac_bits = AUDIO_DSP.frac_bits;
    int mid,
        hi = 48,
        lo = 0;
    
    /* shift sample into 28 frac bit range for comparison */
    if (frac_bits > 28)
        sample >>= (frac_bits - 28);
    if (frac_bits < 28)
        sample <<= (28 - frac_bits);
    
    /* if clipped out of range, return maximum value */
    if (sample >= clip_steps[48])
        return 48 * 90;
    
    /* find amount of sample clipping on the table */
    do
    {
        mid = (hi + lo) / 2;
        if (sample < clip_steps[mid])
            hi = mid;
        else if (sample > clip_steps[mid])
            lo = mid;
        else
            return mid * 90;
    }
    while (hi > (lo + 1));
    
    /* interpolate linearly between steps (less accurate but faster) */
    return ((hi-1) * 90) + (((sample - clip_steps[hi-1]) * 90) /
        (clip_steps[hi] - clip_steps[hi-1]));
}

/** SET LIMITER
 *  Called by the menu system to configure the limiter process */
void dsp_set_limiter(int limiter_level)
{
    if (limiter_level > 0)
    {
        if (!limiter_buffer_active)
        {
            /* enable limiter process */
            AUDIO_DSP.limiter_process = limiter_process;
            limiter_buffer_active = true;
        }
        /* limiter preamp is a gain factor in S7.24 format */
        long old_preamp = AUDIO_DSP.limiter_preamp;
        long new_preamp = fp_factor((((long)limiter_level << 24) / 10), 24);
        if (old_preamp != new_preamp)
        {
            AUDIO_DSP.limiter_preamp = new_preamp;
            set_gain(&AUDIO_DSP);
            logf("   Limiter enable: Yes\tLimiter amp: %.8f",
                (float)AUDIO_DSP.limiter_preamp / (1 << 24));
        }
    }
    else
    {
        /* disable limiter process*/
        if (limiter_buffer_active)
        {
            AUDIO_DSP.limiter_preamp = (1 << 24);
            set_gain(&AUDIO_DSP);
            /* pcmbuf_flush_limiter_buffer(); */
            limiter_buffer_active = false;
            AUDIO_DSP.limiter_process = NULL;
            reset_limiter_buffer(&AUDIO_DSP);
            logf("   Limiter enable:  No\tLimiter amp: %.8f",
                (float)AUDIO_DSP.limiter_preamp / (1 << 24));
        }
    }
}

/** LIMITER PROCESS
 *  Checks pre-amplified signal for clipped samples and smoothly reduces gain
 *  around the clipped samples using a preset attack/release schedule.
 */
static int limiter_process(int count, int32_t *buf[])
{
    /* Limiter process passes through if limiter buffer isn't active, or the 
     * sample depth is too large for safe pre-amping */
    if ((!limiter_buffer_active) || (AUDIO_DSP.frac_bits > 29))
        return count;
    
    count = set_limiter_buffer(count, buf);
    
    if (count <= 0)
        return 0;
    
    const int attack_slope = 15;    /* 15:1 ratio between attack and release */
    const int buffer_count = limiter_buffer_count(true);
    
    int i, ch;
    uint16_t max_peak = 0,
             gain_peak,
             gain_rem;
    long gain;
    
    /* step through limiter buffer in reverse order, in order to find the
     * appropriate max_peak for modifying the output buffer */
    for (i = buffer_count - 1; i >= 0; i--)
    {
        const uint16_t peak_i = lim_buf_peak[(start_peak - lim_buf_peak + i) %
            LIMITER_BUFFER_SIZE];
        /* if no attack slope, nothing to do */
        if ((peak_i == 0) && (max_peak == 0)) continue;
        /* if new peak, start attack slope */
        if (peak_i >= max_peak)
        {
            max_peak = peak_i;
        }
        /* keep sloping */
        else
        {
            if (max_peak > attack_slope)
                max_peak -= attack_slope;
            else
                max_peak = 0;
        }
    }
    /* step through output buffer the same way, but this time modifying peak
     * values to create a smooth attack slope. */
    for (i = count - 1; i >= 0; i--)
    {
        /* if no attack slope, nothing to do */
        if ((out_buf_peak[i] == 0) && (max_peak == 0)) continue;
        /* if new peak, start attack slope */
        if (out_buf_peak[i] >= max_peak)
        {
            max_peak = out_buf_peak[i];
        }
        /* keep sloping */
        else
        {
            if (max_peak > attack_slope)
                max_peak -= attack_slope;
            else
                max_peak = 0;
            out_buf_peak[i] = max_peak;
        }
    }
    /* Now step forward through the output buffer, and modify the peak values
     * to establish a smooth, slow release slope.*/
    for (i = 0; i < count; i++)
    {
        /* if no release slope, nothing to do */
        if ((out_buf_peak[i] == 0) && (release_peak == 0)) continue;
        /* if new peak, start release slope */
        if (out_buf_peak[i] >= release_peak)
        {
            release_peak = out_buf_peak[i];
        }
        /* keep sloping */
        else
        {
            release_peak--;
            out_buf_peak[i] = release_peak;
        }
    }
    /* Implement the limiter: adjust gain of the outbound samples by the gain
     * amounts in the gain steps array corresponding to the peak values. */
    for (ch = 0; ch < AUDIO_DSP.data.num_channels; ch++)
    {
        int32_t *d = buf[ch];
        for (i = 0; i < count; i++)
        {
            if (out_buf_peak[i] > 0)
            {
                gain_peak = (out_buf_peak[i] + 1) / 90;
                gain_rem  = (out_buf_peak[i] + 1) % 90;
                gain = gain_steps[gain_peak];
                if ((gain_peak < 48) && (gain_rem > 0))
                    gain -= gain_rem * ((gain_steps[gain_peak] -
                        gain_steps[gain_peak + 1]) / 90);
                d[i] = FRACMUL_SHL(d[i], gain, 3);
            }
        }
    }
    return count;
}   
