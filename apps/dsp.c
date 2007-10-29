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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "debug.h"

/* 16-bit samples are scaled based on these constants. The shift should be
 * no more than 15.
 */
#define WORD_SHIFT          12
#define WORD_FRACBITS       27

#define NATIVE_DEPTH        16
/* If the buffer sizes change, check the assembly code! */
#define SAMPLE_BUF_COUNT    256
#define RESAMPLE_BUF_COUNT  (256 * 4)   /* Enough for 11,025 Hz -> 44,100 Hz*/
#define DEFAULT_GAIN        0x01000000
#define SAMPLE_BUF_LEFT_CHANNEL 0
#define SAMPLE_BUF_RIGHT_CHANNEL (SAMPLE_BUF_COUNT/2)
#define RESAMPLE_BUF_LEFT_CHANNEL 0
#define RESAMPLE_BUF_RIGHT_CHANNEL (RESAMPLE_BUF_COUNT/2)

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
                                int32_t *src[], int32_t *dst[]);
typedef void (*sample_output_fn_type)(int count, struct dsp_data *data,
                                      int32_t *src[], int16_t *dst);
/* Single-DSP channel processing in place */
typedef void (*channels_process_fn_type)(int count, int32_t *buf[]);
/* DSP local channel processing in place */
typedef void (*channels_process_dsp_fn_type)(int count, struct dsp_data *data,
                                             int32_t *buf[]);


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
    int  frac_bits;
    /* Functions that change depending upon settings - NULL if stage is
       disabled */
    sample_input_fn_type         input_samples;
    resample_fn_type             resample;
    sample_output_fn_type        output_samples;
    /* These will be NULL for the voice codec and is more economical that
       way */
    channels_process_dsp_fn_type apply_gain;
    channels_process_fn_type     apply_crossfeed;
    channels_process_fn_type     channels_process;
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
static struct eq_state eq_data;                     /* A/V */
#ifdef HAVE_SW_TONE_CONTROLS
static int prescale;
static int bass;
static int treble;
/* Filter struct for software bass/treble controls */
static struct eqfilter tone_filter;
#endif

/* Settings applicable to audio codec only */
static int  pitch_ratio = 1000;
static int  channels_mode;
       long dsp_sw_gain;
       long dsp_sw_cross;
static bool dither_enabled;
static bool eq_enabled IBSS_ATTR;
static long eq_precut;
static long track_gain;
static bool new_gain;
static long album_gain;
static long track_peak;
static long album_peak;
static long replaygain;
static bool crossfeed_enabled;

#define audio_dsp (&dsp_conf[CODEC_IDX_AUDIO])
#define voice_dsp (&dsp_conf[CODEC_IDX_VOICE])
static struct dsp_config *dsp IDATA_ATTR = audio_dsp;

/* The internal format is 32-bit samples, non-interleaved, stereo. This
 * format is similar to the raw output from several codecs, so the amount
 * of copying needed is minimized for that case.
 */

int32_t sample_buf[SAMPLE_BUF_COUNT] IBSS_ATTR;
static int32_t resample_buf[RESAMPLE_BUF_COUNT] IBSS_ATTR;

/* set a new dsp and return old one */
static inline struct dsp_config * switch_dsp(struct dsp_config *_dsp)
{
    struct dsp_config * old_dsp = dsp;
    dsp = _dsp;
    return old_dsp;
}

#if 0
/* Clip sample to arbitrary limits where range > 0 and min + range = max */
static inline long clip_sample(int32_t sample, int32_t min, int32_t range)
{
    if ((uint32_t)(sample - min) > (uint32_t)range)
    {
        int32_t c = min;
        if (sample > min)
            c += range;
        sample = c
    }
    return sample;
}
#endif

/* Clip sample to signed 16 bit range */
static inline int32_t clip_sample_16(int32_t sample)
{
    if ((int16_t)sample != sample)
        sample = 0x7fff ^ (sample >> 31);
    return sample;
}

int sound_get_pitch(void)
{
    return pitch_ratio;
}

void sound_set_pitch(int permille)
{
    pitch_ratio = permille;
    
    dsp_configure(DSP_SWITCH_FREQUENCY, dsp->codec_frequency);
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

    do
    {
        *d++ = *s++ << scale;
    }
    while (s < send);

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

    do
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
    while (s < send);

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

    do
    {
        *dl++ = *sl++ << scale;
        *dr++ = *sr++ << scale;
    }
    while (sl < slend);

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

    do
    {
        *dl++ = *s++;
        *dr++ = *s++;
    }
    while (s < send);

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
static void sample_input_new_format(void)
{
    static const sample_input_fn_type sample_input_functions[] =
    {
        [SAMPLE_INPUT_LE_NATIVE_MONO]      = sample_input_lte_native_mono,
        [SAMPLE_INPUT_LE_NATIVE_I_STEREO]  = sample_input_lte_native_i_stereo,
        [SAMPLE_INPUT_LE_NATIVE_NI_STEREO] = sample_input_lte_native_ni_stereo,
        [SAMPLE_INPUT_GT_NATIVE_MONO]      = sample_input_gt_native_mono,
        [SAMPLE_INPUT_GT_NATIVE_I_STEREO]  = sample_input_gt_native_i_stereo,
        [SAMPLE_INPUT_GT_NATIVE_NI_STEREO] = sample_input_gt_native_ni_stereo,
    };

    int convert = dsp->stereo_mode;

    if (dsp->sample_depth > NATIVE_DEPTH)
        convert += SAMPLE_INPUT_GT_NATIVE_1ST_INDEX;

    dsp->input_samples = sample_input_functions[convert];
}

#ifndef DSP_HAVE_ASM_SAMPLE_OUTPUT_MONO
/* write mono internal format to output format */
static void sample_output_mono(int count, struct dsp_data *data,
                               int32_t *src[], int16_t *dst)
{
    const int32_t *s0 = src[0];
    const int scale = data->output_scale;
    const int dc_bias = 1 << (scale - 1);

    do
    {
        int32_t lr = clip_sample_16((*s0++ + dc_bias) >> scale);
        *dst++ = lr;
        *dst++ = lr;
    }
    while (--count > 0);
}
#endif /* DSP_HAVE_ASM_SAMPLE_OUTPUT_MONO */

/* write stereo internal format to output format */
#ifndef DSP_HAVE_ASM_SAMPLE_OUTPUT_STEREO
static void sample_output_stereo(int count, struct dsp_data *data,
                                 int32_t *src[], int16_t *dst)
{
    const int32_t *s0 = src[0];
    const int32_t *s1 = src[1];
    const int scale = data->output_scale;
    const int dc_bias = 1 << (scale - 1);

    do
    {
        *dst++ = clip_sample_16((*s0++ + dc_bias) >> scale);
        *dst++ = clip_sample_16((*s1++ + dc_bias) >> scale);
    }
    while (--count > 0);
}
#endif /* DSP_HAVE_ASM_SAMPLE_OUTPUT_STEREO */

/**
 * The "dither" code to convert the 24-bit samples produced by libmad was
 * taken from the coolplayer project - coolplayer.sourceforge.net
 *
 * This function handles mono and stereo outputs.
 */
static void sample_output_dithered(int count, struct dsp_data *data,
                                   int32_t *src[], int16_t *dst)
{
    const int32_t mask = dither_mask;
    const int32_t bias = dither_bias;
    const int scale = data->output_scale;
    const int32_t min = data->clip_min;
    const int32_t max = data->clip_max;
    const int32_t range = max - min;
    int ch;
    int16_t *d;

    for (ch = 0; ch < dsp->data.num_channels; ch++)
    {
        struct dither_data * const dither = &dither_data[ch];
        int32_t *s = src[ch];
        int i;

        for (i = 0, d = &dst[ch]; i < count; i++, s++, d += 2)
        {
            int32_t output, sample;
            int32_t random;

            /* Noise shape and bias */
            sample = *s;
            sample += dither->error[0] - dither->error[1] + dither->error[2];
            dither->error[2] = dither->error[1];
            dither->error[1] = dither->error[0]/2;

            output = sample + bias;

            /* Dither */
            random = dither->random*0x0019660dL + 0x3c6ef35fL;
            output += (random & mask) - (dither->random & mask);
            dither->random = random;

            /* Clip */
            if ((uint32_t)(output - min) > (uint32_t)range)
            {
                int32_t c = min;
                if (output > min)
                    c += range;
                output = c;
            }

            output &= ~mask;

            /* Error feedback */
            dither->error[0] = sample - output;

            /* Quantize */
            *d = output >> scale;
        }
    }

    if (dsp->data.num_channels == 2)
        return;

    /* Have to duplicate left samples into the right channel since
       pcm buffer and hardware is interleaved stereo */
    d = &dst[0];

    do
    {
        int16_t s = *d++;
        *d++ = s;
    }
    while (--count > 0);
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
static void sample_output_new_format(void)
{
    static const sample_output_fn_type sample_output_functions[] =
    {
        sample_output_mono,
        sample_output_stereo,
        sample_output_dithered,
        sample_output_dithered
    };

    int out = dsp->data.num_channels - 1;

    if (dsp == audio_dsp && dither_enabled)
        out += 2;

    dsp->output_samples = sample_output_functions[out];
}

/**
 * Linear interpolation resampling that introduces a one sample delay because
 * of our inability to look into the future at the end of a frame.
 */
#ifndef DSP_HAVE_ASM_RESAMPLING
static int dsp_downsample(int count, struct dsp_data *data,
                          int32_t *src[], int32_t *dst[])
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
        int32_t *s = src[ch];
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
                        int32_t *src[], int32_t *dst[])
{
    int  ch = data->num_channels - 1;
    uint32_t delta = data->resample_data.delta;
    uint32_t phase, pos;
    int32_t *d;

    /* Rolled channel loop actually showed slightly faster. */
    do
    {
        /* Should always be able to output a sample for a ratio up to
           RESAMPLE_BUF_COUNT / SAMPLE_BUF_COUNT. */
        int32_t *s = src[ch];
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

static void resampler_new_delta(void)
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
static inline int resample(int count, int32_t *src[])
{
    int32_t *dst[2] =
    {
        &resample_buf[RESAMPLE_BUF_LEFT_CHANNEL],
        &resample_buf[RESAMPLE_BUF_RIGHT_CHANNEL],
    };

    count = dsp->resample(count, &dsp->data, src, dst);

    src[0] = dst[0];
    src[1] = dst[dsp->data.num_channels - 1];

    return count;
}

static void dither_init(void)
{
    /* Voice codec should not reset the audio codec's dither data */
    if (dsp != audio_dsp)
        return;

    memset(dither_data, 0, sizeof (dither_data));
    dither_bias = (1L << (dsp->frac_bits - NATIVE_DEPTH));
    dither_mask = (1L << (dsp->frac_bits + 1 - NATIVE_DEPTH)) - 1;
}

void dsp_dither_enable(bool enable)
{
    /* Be sure audio dsp is current to set correct function */
    struct dsp_config *old_dsp = switch_dsp(audio_dsp);
    dither_enabled = enable;
    sample_output_new_format();
    switch_dsp(old_dsp);    
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
        ACC_INIT(acc, *di, coefs[0]);
        ACC(acc, hist_l[0], coefs[1]);
        ACC(acc, hist_l[1], coefs[2]);
        /* Save filter history for left speaker */
        hist_l[1] = GET_ACC(acc);
        hist_l[0] = *di;
        *di++ = left;
        /* Filter delayed sample from right speaker */
        ACC_INIT(acc, *di, coefs[0]);
        ACC(acc, hist_r[0], coefs[1]);
        ACC(acc, hist_r[1], coefs[2]);
        /* Save filter history for right speaker */
        hist_r[1] = GET_ACC(acc);
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
    audio_dsp->apply_crossfeed =
        (enable && audio_dsp->data.num_channels > 1)
            ? apply_crossfeed : NULL;
}

void dsp_set_crossfeed_direct_gain(int gain)
{
    crossfeed_data.gain = get_replaygain_int(gain * -10) << 7;
    /* If gain is negative, the calculation overflowed and we need to clamp */
    if (crossfeed_data.gain < 0)
        crossfeed_data.gain = 0x7fffffff;
}

/* Both gains should be below 0 dB (when inverted) */
void dsp_set_crossfeed_cross_params(long lf_gain, long hf_gain, long cutoff)
{
    int32_t *c = crossfeed_data.coefs;
    long scaler = get_replaygain_int(lf_gain * -10) << 7;

    cutoff = 0xffffffff/NATIVE_FREQUENCY*cutoff;
    hf_gain -= lf_gain;
    /* Divide cutoff by sqrt(10^(-hf_gain/20)) to place cutoff at the -3 dB
     * point instead of shelf midpoint. This is for compatibility with the old
     * crossfeed shelf filter and should be removed if crossfeed settings are
     * ever made incompatible for any other good reason.
     */
    cutoff = DIV64(cutoff, get_replaygain_int(-hf_gain*5), 24);
    filter_shelf_coefs(cutoff, -hf_gain, false, c);
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
    int ch = data->num_channels - 1;

    do
    {
        int32_t *s = buf[ch];
        int32_t *d = buf[ch];
        int32_t  samp = *s++;
        int i = 0;

        do
        {
            FRACMUL_8_LOOP(samp, gain, s, d);
        }
        while (++i < count);
    }
    while (--ch >= 0);
}
#endif /* DSP_HAVE_ASM_APPLY_GAIN */

/* Combine all gains to a global gain. */
static void set_gain(struct dsp_config *dsp)
{
    dsp->data.gain = DEFAULT_GAIN;

    /* Replay gain not relevant to voice */
    if (dsp == audio_dsp && replaygain)
    {
        dsp->data.gain = replaygain;
    }
    
    if (eq_enabled && eq_precut)
    {
        dsp->data.gain =
            (long) (((int64_t) dsp->data.gain * eq_precut) >> 24);
    }
    
    if (dsp->data.gain == DEFAULT_GAIN)
    {
        dsp->data.gain = 0;
    }
    else
    {
        dsp->data.gain >>= 1;
    }

    dsp->apply_gain = dsp->data.gain != 0 ? dsp_apply_gain : NULL;
}

/**
 * Use to enable the equalizer.
 *
 * @param enable true to enable the equalizer
 */
void dsp_set_eq(bool enable)
{
    eq_enabled = enable;
}

/**
 * Update the amount to cut the audio before applying the equalizer.
 *
 * @param precut to apply in decibels (multiplied by 10)
 */
void dsp_set_eq_precut(int precut)
{
    eq_precut = get_replaygain_int(precut * -10);
    set_gain(audio_dsp);
    set_gain(voice_dsp); /* For EQ precut */
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
    unsigned int channels = dsp->data.num_channels;
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

void dsp_set_stereo_width(int value)
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

#if CONFIG_CODEC == SWCODEC

#ifdef HAVE_SW_TONE_CONTROLS
static void set_tone_controls(void)
{
    filter_bishelf_coefs(0xffffffff/NATIVE_FREQUENCY*200,
                         0xffffffff/NATIVE_FREQUENCY*3500,
                         bass, treble, -prescale, tone_filter.coefs);
}
#endif

/* Hook back from firmware/ part of audio, which can't/shouldn't call apps/
 * code directly.
 */
int dsp_callback(int msg, intptr_t param)
{
    switch (msg) {
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

    do
    {
        int32_t lr = *sl/2 + *sr/2;
        *sl++ = lr;
        *sr++ = lr;
    }
    while (--count > 0);
}
#endif /* DSP_HAVE_ASM_SOUND_CHAN_MONO */

#ifndef DSP_HAVE_ASM_SOUND_CHAN_CUSTOM
static void channels_process_sound_chan_custom(int count, int32_t *buf[])
{
    const int32_t gain  = dsp_sw_gain;
    const int32_t cross = dsp_sw_cross;
    int32_t *sl = buf[0], *sr = buf[1];

    do
    {
        int32_t l = *sl;
        int32_t r = *sr;
        *sl++ = FRACMUL(l, gain) + FRACMUL(r, cross);
        *sr++ = FRACMUL(r, gain) + FRACMUL(l, cross);
    }
    while (--count > 0);
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

    do
    {
        int32_t ch = *sl/2 - *sr/2;
        *sl++ = ch;
        *sr++ = -ch;
    }
    while (--count > 0);
}
#endif /* DSP_HAVE_ASM_SOUND_CHAN_KARAOKE */

void dsp_set_channel_config(int value)
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
        audio_dsp->stereo_mode == STEREO_MONO)
        value = SOUND_CHAN_STEREO;

    /* This doesn't apply to voice */
    channels_mode = value;
    audio_dsp->channels_process = channels_process_functions[value];
}

/* Process and convert src audio to dst based on the DSP configuration,
 * reading count number of audio samples. dst is assumed to be large
 * enough; use dsp_output_count() to get the required number. src is an
 * array of pointers; for mono and interleaved stereo, it contains one
 * pointer to the start of the audio data and the other is ignored; for
 * non-interleaved stereo, it contains two pointers, one for each audio
 * channel. Returns number of bytes written to dst.
 */
int dsp_process(char *dst, const char *src[], int count)
{
    int32_t *tmp[2];
    int written = 0;
    int samples;

#if defined(CPU_COLDFIRE)
    /* set emac unit for dsp processing, and save old macsr, we're running in
       codec thread context at this point, so can't clobber it */
    unsigned long old_macsr = coldfire_get_macsr();
    coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE);
#endif

    if (new_gain)
        dsp_set_replaygain(); /* Gain has changed */

    /* Testing function pointers for NULL is preferred since the pointer
       will be preloaded to be used for the call if not. */
    while (count > 0)
    {
        samples = MIN(SAMPLE_BUF_COUNT/2, count);
        count -= samples;

        dsp->input_samples(samples, src, tmp);

        if (dsp->apply_gain)
            dsp->apply_gain(samples, &dsp->data, tmp);

        if (dsp->resample && (samples = resample(samples, tmp)) <= 0)
            break; /* I'm pretty sure we're downsampling here */

        if (dsp->apply_crossfeed)
            dsp->apply_crossfeed(samples, tmp);

        /* TODO: EQ and tone controls need separate structs for audio and voice
         * DSP processing thanks to filter history. isn't really audible now, but
         * might be the day we start handling voice more delicately. Planned
         * changes may well run all relevent channels through the same EQ so
         * perhaps not.
         */
        if (eq_enabled)
            eq_process(samples, tmp);

#ifdef HAVE_SW_TONE_CONTROLS
        if ((bass | treble) != 0)
            eq_filter(tmp, &tone_filter, samples, dsp->data.num_channels,
                      FILTER_BISHELF_SHIFT);
#endif

        if (dsp->channels_process)
            dsp->channels_process(samples, tmp);

        dsp->output_samples(samples, &dsp->data, tmp, (int16_t *)dst);

        written += samples;
        dst += samples * sizeof (int16_t) * 2;
        yield();
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
int dsp_output_count(int count)
{
    if (dsp->resample)
    {
        count = (int)(((unsigned long)count * NATIVE_FREQUENCY
                    + (dsp->frequency - 1)) / dsp->frequency);
    }

    /* Now we have the resampled sample count which must not exceed
     * RESAMPLE_BUF_COUNT/2 to avoid resample buffer overflow. One
     * must call dsp_input_count() to get the correct input sample
     * count.
     */
    if (count > RESAMPLE_BUF_COUNT/2)
        count = RESAMPLE_BUF_COUNT/2;

    return count;
}

/* Given count output samples, calculate number of input samples
 * that would be consumed in order to fill the output buffer.
 */
int dsp_input_count(int count)
{
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

    return count;
}

int dsp_stereo_mode(void)
{
    return dsp->stereo_mode;
}

static void dsp_set_gain_var(long *var, long value)
{
    /* Voice shouldn't mess with these */
    if (dsp == audio_dsp)
    {
        *var = value;
        new_gain = true;
    }
}

static void dsp_update_functions(void)
{
    sample_input_new_format();
    sample_output_new_format();
    if (dsp == audio_dsp)
        dsp_set_crossfeed(crossfeed_enabled);
}

bool dsp_configure(int setting, intptr_t value)
{
    switch (setting)
    {
    case DSP_SWITCH_CODEC:
        if ((uintptr_t)value <= 1)
            switch_dsp(&dsp_conf[value]);
        break;

    case DSP_SET_FREQUENCY:
        memset(&dsp->data.resample_data, 0,
               sizeof (dsp->data.resample_data));
        /* Fall through!!! */
    case DSP_SWITCH_FREQUENCY:
        dsp->codec_frequency = (value == 0) ? NATIVE_FREQUENCY : value;
        /* Account for playback speed adjustment when setting dsp->frequency
           if we're called from the main audio thread. Voice UI thread should
           not need this feature.
         */
        if (dsp == audio_dsp)
            dsp->frequency = pitch_ratio * dsp->codec_frequency / 1000;
        else
            dsp->frequency = dsp->codec_frequency;

        resampler_new_delta();
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
        sample_input_new_format();
        dither_init(); 
        break;

    case DSP_SET_STEREO_MODE:
        dsp->stereo_mode = value;
        dsp->data.num_channels = value == STEREO_MONO ? 1 : 2;
        dsp_update_functions();
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

        if (dsp == audio_dsp)
        {
            track_gain = 0;
            album_gain = 0;
            track_peak = 0;
            album_peak = 0;
            new_gain   = true;
        }

        dsp_update_functions();
        resampler_new_delta();
        break;

    case DSP_FLUSH:
        memset(&dsp->data.resample_data, 0,
               sizeof (dsp->data.resample_data));
        resampler_new_delta();
        dither_init();
        break;

    case DSP_SET_TRACK_GAIN:
        dsp_set_gain_var(&track_gain, value);
        break;

    case DSP_SET_ALBUM_GAIN:
        dsp_set_gain_var(&album_gain, value);
        break;

    case DSP_SET_TRACK_PEAK:
        dsp_set_gain_var(&track_peak, value);
        break;

    case DSP_SET_ALBUM_PEAK:
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

    if (global_settings.replaygain || global_settings.replaygain_noclip)
    {
        bool track_mode = get_replaygain_mode(track_gain != 0,
            album_gain != 0) == REPLAYGAIN_TRACK;
        long peak = (track_mode || !album_peak) ? track_peak : album_peak;

        if (global_settings.replaygain)
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

    /* Store in S8.23 format to simplify calculations. */
    replaygain = gain;
    set_gain(audio_dsp);
}
