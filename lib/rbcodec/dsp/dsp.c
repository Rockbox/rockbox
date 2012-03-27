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
#include "system.h"
#include <sound.h>
#include "dsp.h"
#include "dsp-util.h"
#include "eq.h"
#include "compressor.h"
#include "kernel.h"
#include "settings.h"
#include "replaygain.h"
#include "tdspeed.h"
#include "core_alloc.h"
#include "fixedpoint.h"
#include "fracmul.h"

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"

/* 16-bit samples are scaled based on these constants. The shift should be
 * no more than 15.
 */
#define WORD_SHIFT              12
#define WORD_FRACBITS           27

#define NATIVE_DEPTH            16
#define SAMPLE_BUF_COUNT        128 /* Per channel, per DSP */
#define RESAMPLE_BUF_COUNT      192 /* Per channel, per DSP */
/* All told, 5120 bytes for sample/resample bufs (as before) */
#define DEFAULT_GAIN            0x01000000

/* Include header with defines which functions are implemented in assembly
   code for the target */
#include <dsp_asm.h>

/** Common function prototypes **/

/* NOTE: Typedefs keep things much neater and saner here */

/* DSP sample transform function prototype */
typedef void (*dsp_proc_fn_type)(struct dsp_data *data,
                                 struct dsp_buffer **buf);

/* DSP final buffer output function call prototype */
typedef void (*sample_output_fn_type)(int count, struct dsp_data *data,
                                      int32_t const * const src[], int16_t *dst);

/*
 ***************************************************************************/

/* These could in be put in any order
 * Exceptions:
 *  * Sample source conversion, which must be before any other processing
 *  * Anything that depends on the native sample rate must go after the
 *    resampler.
 */
static const struct dsp_stage
{
    uint32_t proc_mask;   /* Id bit flag to use in dsp->proc_mask */
    struct dsp_proc_entry
    {
        uint32_t ip_mask; /* In-place Id bit (0 or Id bit flag if IP) */
        union
        {
            dsp_proc_fn_type fn;   /* Function for stage (static) */
            void (*init_cb)(struct dsp_proc_entry *new); /* Dynamic init */
        };
    } proc_entry;
} dsp_stages[] =
{
    /* Gain stage */
    { .proc_mask  = DSP_PROC_GAIN,
      .proc_entry = { .ip_mask = DSP_PROC_GAIN,
                      { .fn = dsp_apply_gain } } },
#ifdef HAVE_PITCHSCREEN
    /* Timestretch */
    { .proc_mask  = DSP_PROC_TIMESTRETCH,
      .proc_entry = { .ip_mask = 0,
                      { .fn = tdspeed_doit } } },
#endif
    /* Resampler */
    { .proc_mask  = DSP_PROC_RESAMPLE,
      .proc_entry = { .ip_mask = 0,
                      { .fn = resample } } },
    /* Crossfeed */
    { .proc_mask  = DSP_PROC_CROSSFEED,
      .proc_entry = { .ip_mask = DSP_PROC_CROSSFEED,
                      { .fn = apply_crossfeed } } },
    /* Equalizer */
    { .proc_mask  = DSP_PROC_EQUALIZER,
      .proc_entry = { .ip_mask = DSP_PROC_EQUALIZER,
                      { .fn = eq_process } } },
#ifdef HAVE_SW_TONE_CONTROLS
    /* Tone controls */
    { .proc_mask  = DSP_PROC_TONE_CONTROLS,
      .proc_entry = { .ip_mask = DSP_PROC_TONE_CONTROLS,
                      { .fn = tone_process } } },
#endif
    /* Channel config - correct values determined later */
    { .proc_mask  = DSP_PROC_CHANNELS,
      .proc_entry = { .ip_mask = DSP_PROC_MASK_USE_INIT_CB,
                      { .init_cb = channels_process_init_cb } } },
    /* Compressor */
    { .proc_mask  = DSP_PROC_COMPRESSOR,
      .proc_entry = { .ip_mask = DSP_PROC_COMPRESSOR,
                      { .fn = compressor_process } } },
};

struct dsp_config
{
    /** General DSP-local data **/
    struct dsp_data data; /* Config members for use in external routines */
    long codec_frequency; /* Sample rate of data coming from the codec */
    long frequency;       /* Effective sample rate after pitch shift (if any) */
    int  sample_depth;    /* Codec-specified sample depth */
    int  stereo_mode;     /* Codec input format */
#ifdef CPU_COLDFIRE
    unsigned long old_macsr; /* Old macsr value to restore */
#endif
#ifdef HAVE_PITCHSCREEN
    bool processing;      /* DSP is processing (to thwart inopportune buffer
                             moves) */
#endif
#ifdef HAVE_SW_TONE_CONTROLS
    struct eqfilter tone_filter; /* Filter data for software tone controls */
#endif

    /** Sample input from src buffer **/
    dsp_proc_fn_type input_samples; /* Sample input converter (always set) */
    struct dsp_buffer sample_buf; /* Buffer descriptor for converted samples */
    int32_t sample_buf_arr[2][SAMPLE_BUF_COUNT]; /* Actual data */

    /** Resampling backend **/
    struct dsp_buffer resample_buf; /* Buffer descriptor for resampled data */
    int32_t resample_buf_arr[2][RESAMPLE_BUF_COUNT]; /* Actual data */

    /** General dependant processing functions **/
    uint32_t proc_mask;      /* Mask of enabled stages */
    int proc_count;          /* Count of enabled stages */
    struct dsp_proc_entry *proc_entries; /* Array of enabled stages */

    /** Final sample output **/
    sample_output_fn_type output_samples; /* Output function (always set) */
};

/* Allocate separately from dsp_config because not all may be applied to voice */
/* All are possible */
static struct dsp_proc_entry audio_dsp_enabled_entries[8] IBSS_ATTR;
/* resample, tone */
static struct dsp_proc_entry voice_dsp_enabled_entries[2] IBSS_ATTR;

/* General DSP config */
static struct dsp_config dsp_conf[DSP_COUNT] IBSS_ATTR;

/* Dithering */
static struct dither_data
{
    long error[3];  /* 00h */
    long random;    /* 0ch */
                    /* 10h */
} dither_data[2] IBSS_ATTR; /* 0=left, 1=right */

/* Crossfeed */
struct crossfeed_data
{
    int32_t gain;           /* 00h - Direct path gain */
    int32_t coefs[3];       /* 04h - Coefficients for the shelving filter */
    int32_t history[4];     /* 10h - Format is x[n - 1], y[n - 1] for both channels */
    int32_t delay[13][2];   /* 20h */
    int32_t *index;         /* 88h - Current pointer into the delay line */
                            /* 8ch */
} crossfeed_data IDATA_ATTR = /* A */
{
    .index = (int32_t *)crossfeed_data.delay
};

/* Equalizer
 *
 * Current setup is one lowshelf filters three peaking filters and one
 *  highshelf filter. Varying the number of shelving filters make no sense,
 *  but adding peaking filters is possible.
 */
static struct eq_state
{
    char enabled[5];            /* 00h - Flags for active filters */
    struct eqfilter filters[5]; /* 08h - packing is 4? */
                                /* 10ch */
} eq_data;                      /* A */

/* Software tone controls
 * Applied to voice and codec since hardware controls also do so */
#ifdef HAVE_SW_TONE_CONTROLS
static struct tone_control_state
{
    int prescale;                                   /* A/V */
    int bass;                                       /* A/V */
    int treble;                                     /* A/V */
} tone_data;         /* A/V */
#endif /* HAVE_SW_TONE_CONTROLS */

/* Settings applicable to audio codec only */
#ifdef HAVE_PITCHSCREEN
struct timestretch_data
{
    int32_t percent;    /* Speed% * PITCH_SPEED_PRECISION */
    bool enabled;
} timestretch_data;

static int32_t pitch_ratio = PITCH_SPEED_100;
#endif

#define DSP_NUM_CHANNELS_PROC 6
struct channels_mode_data
{
    long sw_gain;   /* 00h: for ...chan_custom */
    long sw_cross;  /* 04h: for ...chan_custom */
    const dsp_proc_fn_type fns[DSP_NUM_CHANNELS_PROC];
    int channels_mode;
} channels_mode_data =
{
    .sw_gain = 0,
    .sw_cross = 0,
    .fns =
    {
        [SOUND_CHAN_STEREO]     = NULL,
        [SOUND_CHAN_MONO]       = channels_process_sound_chan_mono,
        [SOUND_CHAN_CUSTOM]     = channels_process_sound_chan_custom,
        [SOUND_CHAN_MONO_LEFT]  = channels_process_sound_chan_mono_left,
        [SOUND_CHAN_MONO_RIGHT] = channels_process_sound_chan_mono_right,
        [SOUND_CHAN_KARAOKE]    = channels_process_sound_chan_karaoke,
    },
    .channels_mode = SOUND_CHAN_STEREO,
};

static bool dither_enabled;

struct dsp_gain
{
    int32_t eq_precut;               /* EQ precut amp setting */
    struct dsp_replay_gains rpgains; /* Cached replay gain settings */
    int32_t replaygain;              /* Final replay gain value */
} dsp_gain;

static void dsp_update_replaygain(const struct dsp_replay_gains *gains);

#define AUDIO_DSP (dsp_conf[CODEC_IDX_AUDIO])
#define VOICE_DSP (dsp_conf[CODEC_IDX_VOICE])

/* Update the processing stages array with the enabled stages */
static void dsp_update_proc_entries(struct dsp_config *dsp)
{
    struct dsp_proc_entry *e;

    if (dsp == &AUDIO_DSP)
        e = audio_dsp_enabled_entries;
    else if (dsp == &VOICE_DSP)
        e = voice_dsp_enabled_entries;
    else
        return;

    dsp->proc_count = 0; /* Assume no additional processing */
    dsp->proc_entries = e;

    if (dsp->proc_mask == 0)
        return; /* Noone enabled anything */

    for (unsigned i = 0; i < ARRAYLEN(dsp_stages); i++)
    {
        const struct dsp_stage * stage = &dsp_stages[i];
        if (dsp->proc_mask & stage->proc_mask)
        {
            if (stage->proc_entry.ip_mask == DSP_PROC_MASK_USE_INIT_CB)
                stage->proc_entry.init_cb(e);
            else
                *e = stage->proc_entry;
            e++;
        }
    }

    dsp->proc_count = e - dsp->proc_entries;
}

static void dsp_proc_enable(struct dsp_config * dsp, uint32_t mask,
                            bool enable)
{
    if (enable)
        dsp->proc_mask |= mask;
    else
        dsp->proc_mask &= ~mask;

    dsp_update_proc_entries(dsp);
}

/* The internal format is 32-bit samples, non-interleaved, stereo. This
 * format is similar to the raw output from several codecs, so the amount
 * of copying needed is minimized for that case.
 */

#ifdef HAVE_PITCHSCREEN
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

static void tdspeed_setup(void)
{
    /* Applies to audio dsp only */
    struct dsp_config * const dsp = &AUDIO_DSP;

    /* Assume timestretch will not be used */
    timestretch_data.enabled = false;

    if (dsp_timestretch_available())
    {
        if (timestretch_data.percent == 0)
            timestretch_data.percent = PITCH_SPEED_100;

        if (tdspeed_config(
            dsp->codec_frequency == 0 ?
                NATIVE_FREQUENCY : dsp->codec_frequency,
            dsp->stereo_mode != STEREO_MONO,
            timestretch_data.percent))
        {
            timestretch_data.enabled = true;
        }
    }

    dsp_proc_enable(dsp, DSP_PROC_TIMESTRETCH,
                    timestretch_data.enabled);
}

void dsp_timestretch_enable(bool enabled)
{
    /* Hook to set up timestretch buffer on first call to settings_apply() */
    if (enabled)
    {
        tdspeed_init();
        tdspeed_setup();
    }
    else
    {
        dsp_set_timestretch(PITCH_SPEED_100);
        tdspeed_finish();
    }
}

void dsp_set_timestretch(int32_t percent)
{
    timestretch_data.percent = percent;
    tdspeed_setup();
}

int32_t dsp_get_timestretch(void)
{
    return timestretch_data.percent;
}

bool dsp_timestretch_available(void)
{
    return global_settings.timestretch_enabled;
}
#endif /* HAVE_PITCHSCREEN */

/* Convert count samples to the internal format, if needed.  Updates src
 * to point past the samples "consumed" and dst is set to point to the
 * samples to consume. Note that for mono, dst[0] equals dst[1], as there
 * is no point in processing the same data twice.
 */

/* convert count 16-bit mono to 32-bit mono */
static void sample_input_lte_native_mono(struct dsp_data *data,
                                         struct dsp_buffer **buf_p)
{
    struct dsp_config *dsp = CONFIG_FROM_DATA(data);
    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &dsp->sample_buf;

    *buf_p = dst;

    if (dst->remcount > 0)
        return; /* data still remains */

    int count = MIN(src->remcount, SAMPLE_BUF_COUNT);

    dst->remcount  = count;
    dst->p32[0]    = dsp->sample_buf_arr[0];
    dst->p32[1]    = dst->p32[0];
    dst->proc_mask = src->proc_mask;

    if (count <= 0)
        return; /* purged sample_buf */

    const int16_t *s = src->pin[0];
    int32_t *d = dst->p32[0];
    const int scale = WORD_SHIFT;

    dsp_advance_buffer_input(src, count, sizeof (int16_t));

    do
    {
        *d++ = *s++ << scale;
    }
    while (--count > 0);
}

/* convert count 16-bit interleaved stereo to 32-bit noninterleaved */
static void sample_input_lte_native_i_stereo(struct dsp_data *data,
                                             struct dsp_buffer **buf_p)
{
    struct dsp_config *dsp = CONFIG_FROM_DATA(data);
    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &dsp->sample_buf;

    *buf_p = dst;

    if (dst->remcount > 0)
        return; /* data still remains */

    int count = MIN(src->remcount, SAMPLE_BUF_COUNT);
    dst->remcount  = count;
    dst->p32[0]    = dsp->sample_buf_arr[0];
    dst->p32[1]    = dsp->sample_buf_arr[1];
    dst->proc_mask = src->proc_mask;

    if (count <= 0)
        return; /* purged sample_buf */

    const int16_t *s = src->pin[0];
    int32_t *dl = dst->p32[0];
    int32_t *dr = dst->p32[1];
    const int scale = WORD_SHIFT;

    dsp_advance_buffer_input(src, count, 2*sizeof (int16_t));

    do
    {
        *dl++ = *s++ << scale;
        *dr++ = *s++ << scale;
    }
    while (--count > 0);
}

/* convert count 16-bit noninterleaved stereo to 32-bit noninterleaved */
static void sample_input_lte_native_ni_stereo(struct dsp_data *data,
                                              struct dsp_buffer **buf_p)
{
    struct dsp_config *dsp = CONFIG_FROM_DATA(data);
    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &dsp->sample_buf;

    *buf_p = dst;

    if (dst->remcount > 0)
        return; /* data still remains */

    int count = MIN(src->remcount, SAMPLE_BUF_COUNT);

    dst->remcount  = count;
    dst->p32[0]    = dsp->sample_buf_arr[0];
    dst->p32[1]    = dsp->sample_buf_arr[1];
    dst->proc_mask = src->proc_mask;

    if (count <= 0)
        return; /* purged sample_buf */

    const int16_t *sl = src->pin[0];
    const int16_t *sr = src->pin[1];
    int32_t *dl = dst->p32[0];
    int32_t *dr = dst->p32[1];
    const int scale = WORD_SHIFT;

    dsp_advance_buffer_input(src, count, sizeof (int16_t));

    do
    {
        *dl++ = *sl++ << scale;
        *dr++ = *sr++ << scale;
    }
    while (--count > 0);
}

/* convert count 32-bit mono to 32-bit mono */
static void sample_input_gt_native_mono(struct dsp_data *data,
                                        struct dsp_buffer **buf_p)
{
    struct dsp_config *dsp = CONFIG_FROM_DATA(data);
    struct dsp_buffer *dst = &dsp->sample_buf;

    if (dst->remcount > 0)
    {
        *buf_p = dst;
        return; /* data still remains */
    }

    struct dsp_buffer *src = *buf_p;
    src->p32[1] = src->p32[0];
}

/* convert count 32-bit interleaved stereo to 32-bit noninterleaved stereo */
static void sample_input_gt_native_i_stereo(struct dsp_data *data,
                                            struct dsp_buffer **buf_p)
{
    struct dsp_config *dsp = CONFIG_FROM_DATA(data);
    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &dsp->sample_buf;

    *buf_p = dst;

    if (dst->remcount > 0)
        return; /* data still remains */

    int count = MIN(src->remcount, SAMPLE_BUF_COUNT);
    dst->remcount  = count;
    dst->p32[0]    = dsp->sample_buf_arr[0];
    dst->p32[1]    = dsp->sample_buf_arr[1];
    dst->proc_mask = src->proc_mask;

    if (count <= 0)
        return; /* purged sample_buf */

    const int32_t *s = src->pin[0];
    int32_t *dl = dst->p32[0];
    int32_t *dr = dst->p32[1];

    dsp_advance_buffer_input(src, count, 2*sizeof (int32_t));

    do
    {
        *dl++ = *s++;
        *dr++ = *s++;
    }
    while (--count > 0);
}

/* convert 32 bit-noninterleaved stereo to 32-bit noninterleaved stereo */
static void sample_input_gt_native_ni_stereo(struct dsp_data *data,
                                             struct dsp_buffer **buf_p)
{
    struct dsp_config *dsp = CONFIG_FROM_DATA(data);
    struct dsp_buffer *dst = &dsp->sample_buf;

    if (dst->remcount > 0)
        *buf_p = dst; /* data still remains */
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
    static const dsp_proc_fn_type fns[STEREO_NUM_MODES][2] =
    {
        [STEREO_INTERLEAVED] =
            { sample_input_lte_native_i_stereo,
              sample_input_gt_native_i_stereo },
        [STEREO_NONINTERLEAVED] =
            { sample_input_lte_native_ni_stereo,
              sample_input_gt_native_ni_stereo },
        [STEREO_MONO] =
            { sample_input_lte_native_mono,
              sample_input_gt_native_mono },
    };

    dsp->input_samples = fns[dsp->stereo_mode]
                            [dsp->sample_depth > NATIVE_DEPTH ? 1 : 0];
}


#ifndef DSP_HAVE_ASM_SAMPLE_OUTPUT_MONO
/* write mono internal format to output format */
static void sample_output_mono(int count, struct dsp_data *data,
                               int32_t const * const src[], int16_t *dst)
{
    const int32_t *s0 = src[0];
    const int scale = data->output_scale;
    const int32_t dc_bias = 1L << (scale - 1);

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
                                 int32_t const * const src[], int16_t *dst)
{
    const int32_t *s0 = src[0];
    const int32_t *s1 = src[1];
    const int scale = data->output_scale;
    const int32_t dc_bias = 1L << (scale - 1);

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
                                   int32_t const * const src[], int16_t *dst)
{
    const int scale = data->output_scale;
    const int32_t dc_bias = 1L << (scale - 1); /* 1/2 bit of significance */
    const int32_t mask = (1L << scale) - 1; /* Mask of bits quantized away */

    for (int ch = 0; ch < data->num_channels; ch++)
    {
        struct dither_data * const dither = &dither_data[ch];
        const int32_t *s = src[ch];
        int16_t *d = &dst[ch];

        for (int i = 0; i < count; i++, s++, d += 2)
        {
            /* Noise shape and bias (for correct rounding later) */
            int32_t sample = *s;

            sample += dither->error[0] - dither->error[1] + dither->error[2];
            dither->error[2] = dither->error[1];
            dither->error[1] = dither->error[0] / 2;

            int32_t output = sample + dc_bias;

            /* Dither, highpass triangle PDF */
            int32_t random = dither->random*0x0019660dL + 0x3c6ef35fL;
            output += (random & mask) - (dither->random & mask);
            dither->random = random;

            /* Quantize sample to output range */
            output >>= scale;

            /* Error feedback of quantization */
            dither->error[0] = sample - (output << scale);

            /* Clip and store */
            *d = clip_sample_16(output);
        }
    }

    if (data->num_channels > 1)
        return;

    /* Have to duplicate left samples into the right channel since
       pcm buffer and hardware is interleaved stereo */
    int16_t *d = &dst[0];

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
static void sample_output_new_format(struct dsp_config *dsp)
{
    static const sample_output_fn_type fns[2][2] =
    {
        { sample_output_mono,        /* DC-biased quantizing */
          sample_output_stereo },
        { sample_output_dithered,    /* Tri-PDF dithering */
          sample_output_dithered },
    };

    dsp->output_samples = fns[dsp == &AUDIO_DSP && dither_enabled ? 1 : 0]
                             [dsp->data.num_channels - 1];
}

#ifndef DSP_HAVE_ASM_RESAMPLING
/**
 * Linear interpolation resampling that introduces a one sample delay because
 * of our inability to look into the future at the end of a frame.
 */
static int dsp_lin_resample(struct dsp_data *data,
                            struct dsp_buffer *src, struct dsp_buffer *dst)
{
    int ch = data->num_channels - 1;
    uint32_t count = src->remcount;
    uint32_t delta = data->resample_data.delta;
    uint32_t phase, pos;
    int32_t *d;

    do
    {
        const int32_t *s = src->p32[ch];

        d = dst->p32[ch];
        int32_t *dmax = d + dst->bufcount;

        phase = data->resample_data.phase;
        pos = phase >> 16;
        pos = MIN(pos, count);

        int32_t last = pos > 0 ? s[pos - 1] : data->resample_data.last_sample[ch];

        if (pos < count)
        {
            while (1)
            {
                *d++ = last + FRACMUL((phase & 0xffff) << 15, s[pos] - last);
                phase += delta;
                pos = phase >> 16;

                if (pos >= count || d >= dmax)
                    break;

                if (pos > 0)
                    last = s[pos - 1];
            }

            if (pos > 0)
            {
                pos = MIN(pos, count);
                last = s[pos - 1];
            }
        }

        data->resample_data.last_sample[ch] = last;
    }
    while (--ch >= 0);

    /* Wrap phase accumulator back to start of next frame. */
    data->resample_data.phase = phase - (pos << 16);

    dst->remcount = d - dst->p32[0];

    return pos;
}
#endif /* DSP_HAVE_ASM_RESAMPLING */

/* Resample count stereo samples or stop when the destination is full.
 * Updates the src buffer and changes to its own output buffer to refer to
 * the resampled data.
 */
static void resample(struct dsp_data *data, struct dsp_buffer **buf_p)
{
    struct dsp_config *dsp = CONFIG_FROM_DATA(data);
    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &dsp->resample_buf;

    *buf_p = dst;

    if (dst->remcount > 0)
        return; /* data still remains */

    dst->remcount = 0;
    dst->p32[0] = dsp->resample_buf_arr[0];
    dst->p32[1] = dsp->resample_buf_arr[dsp->data.num_channels - 1];

    if (src->remcount > 0)
    {
        dst->bufcount = RESAMPLE_BUF_COUNT;

        int consumed = dsp_lin_resample(&dsp->data, src, dst);

        /* Advance src by consumed amount */
        if (consumed > 0)
            dsp_advance_buffer32(src, consumed);
    }
    /* else purged resample_buf */

    /* Inherit in-place processed mask from source buffer */
    dst->proc_mask = src->proc_mask;
}

static void resampler_new_delta(struct dsp_config *dsp)
{
    dsp->data.resample_data.delta = (unsigned long)
        dsp->frequency * 65536LL / NATIVE_FREQUENCY;

    if (dsp->frequency == NATIVE_FREQUENCY)
    {
        /* NOTE: If fully glitch-free transistions from no resampling to
           resampling are desired, last_sample history should be maintained
           even when not resampling. */
        dsp->data.resample_data.phase = 0;
        dsp->data.resample_data.last_sample[0] = 0;
        dsp->data.resample_data.last_sample[1] = 0;
    }

    dsp_proc_enable(dsp, DSP_PROC_RESAMPLE,
                    dsp->frequency != NATIVE_FREQUENCY);
}

static int resample_est_output_count(struct dsp_config *dsp, int incount)
{
    if (incount <= 0)
    {
        if (dsp->resample_buf.remcount > 0)
            incount = dsp->resample_buf.remcount;
        else
            incount = 0;
    }
    else
    {
        /* We can spit out up to one extra sample over the expected ratio */
        incount = (incount * NATIVE_FREQUENCY + dsp->frequency) / dsp->frequency;
    }

    return incount;
}

static void dither_init(struct dsp_config *dsp)
{
    memset(dither_data, 0, sizeof (dither_data));
    (void)dsp;
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
static void apply_crossfeed(struct dsp_data *data,
                            struct dsp_buffer **buf_p)
{
    int32_t *hist_l = &crossfeed_data.history[0];
    int32_t *hist_r = &crossfeed_data.history[2];
    int32_t *delay = &crossfeed_data.delay[0][0];
    int32_t *coefs = &crossfeed_data.coefs[0];
    int32_t gain = crossfeed_data.gain;
    int32_t *di = crossfeed_data.index;

    struct dsp_buffer *buf = *buf_p;
    int count = buf->remcount;

    for (int i = 0; i < count; i++)
    {
        int32_t left = buf->p32[0][i];
        int32_t right = buf->p32[1][i];

        /* Filter delayed sample from left speaker */
        int32_t acc = FRACMUL(*di, coefs[0]);
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
        buf->p32[0][i] = FRACMUL(left, gain) + hist_r[1];
        buf->p32[1][i] = FRACMUL(right, gain) + hist_l[1];

        /* Wrap delay line index if bigger than delay line size */
        if (di >= delay + 13*2)
            di = delay;
    }
    /* Write back local copies of data we've modified */
    crossfeed_data.index = di;

    (void)data;
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
    struct dsp_config * const dsp = &AUDIO_DSP;
    dsp_proc_enable(dsp, DSP_PROC_CROSSFEED,
                    enable && dsp->data.num_channels > 1);
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

    cutoff = 0xffffffff / NATIVE_FREQUENCY*cutoff;
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
static void dsp_apply_gain(struct dsp_data *data,
                           struct dsp_buffer **buf_p)
{
    const int32_t gain = data->gain;
    struct dsp_buffer *buf = *buf_p;

    for (int ch = 0; ch < data->num_channels; ch++)
    {
        int32_t *d = buf->p32[ch];
        int count = buf->remcount;

        for (int i = 0; i < count; i++)
            d[i] = FRACMUL_SHL(d[i], gain, 8);
    }
}
#endif /* DSP_HAVE_ASM_APPLY_GAIN */

/* Combine all gains to a global gain. */
static void update_gain(struct dsp_config *dsp)
{
    /* gains are in S7.24 format */
    int32_t gain = DEFAULT_GAIN;

    /* Replay gain not relevant to voice */
    if (dsp == &AUDIO_DSP && dsp_gain.replaygain != 0)
    {
        gain = dsp_gain.replaygain;
    }

    if ((dsp->proc_mask & DSP_PROC_EQUALIZER) && dsp_gain.eq_precut != 0)
    {
        gain = fp_mul(gain, dsp_gain.eq_precut, 24);
    }

#ifdef HAVE_SW_VOLUME_CONTROL
    /* FIXME: This sould be done by bottom-level PCM driver so it works with
              all PCM, not here and not in mixer -- jethead71 */
    if (global_settings.volume < SW_VOLUME_MAX ||
        global_settings.volume > SW_VOLUME_MIN)
    {
        int vol_gain = get_replaygain_int(global_settings.volume * 100);
        gain = (long) (((int64_t)gain * vol_gain) >> 24);
    }
#endif

    if (gain == DEFAULT_GAIN)
        gain = 0;

    gain >>= 1;   /* convert gain to S8.23 format */

    dsp->data.gain = gain;

    dsp_proc_enable(dsp, DSP_PROC_GAIN, gain != 0);
}

/**
 * Update the amount to cut the audio before applying the equalizer.
 *
 * @param precut to apply in decibels (multiplied by 10)
 */
void dsp_set_eq_precut(int precut)
{
    dsp_gain.eq_precut = get_replaygain_int(precut * -10);
    update_gain(&AUDIO_DSP);
}

/**
 * Synchronize the equalizer filter coefficients with the global settings.
 *
 * @param band the equalizer band to synchronize
 */
void dsp_set_eq_coefs(int band)
{
    /* Adjust setting pointer to the band we actually want to change */
    struct eq_band_setting *setting = &global_settings.eq_band_settings[band];

    /* Convert user settings to format required by coef generator functions */
    unsigned long cutoff = 0xffffffff / NATIVE_FREQUENCY * setting->cutoff;
    unsigned long q = setting->q;
    int gain = setting->gain;

    /* NOTE: The coef functions assume the EMAC unit is in fractional mode,
       which it should be, since we're executed from the main thread. */

    /* Assume a band is disabled if the gain is zero */
    if (gain == 0)
    {
        eq_data.enabled[band] = 0;
    }
    else
    {
        static const struct
        {
            void (*coef_gen)(unsigned long cutoff, unsigned long Q, long db,
                             int32_t *c);
            unsigned shift;
        } band_params[5] =
        {
            [0]       = { .coef_gen = eq_ls_coefs, .shift = EQ_SHELF_SHIFT },
            [1 ... 3] = { .coef_gen = eq_pk_coefs, .shift = EQ_PEAK_SHIFT  }, 
            [4]       = { .coef_gen = eq_hs_coefs, .shift = EQ_SHELF_SHIFT },
        };

        if (q == 0)
            q = 1;

        eq_data.filters[band].shift = band_params[band].shift;
        band_params[band].coef_gen(cutoff, q, gain, eq_data.filters[band].coefs);
        eq_data.enabled[band] = 1;
    }
}

/* Apply EQ filters to those bands that have got it switched on. */
static void eq_process(struct dsp_data *data,
                       struct dsp_buffer **buf_p)
{
    int channels = data->num_channels;
    struct dsp_buffer *buf = *buf_p;
    int count = buf->remcount;

    /* filter configuration currently is 1 low shelf filter, 3 band peaking
       filters and 1 high shelf filter, in that order. we need to know this
       so we can choose the correct shift factor.
     */
    for (int i = 0; i < 5; i++)
    {
        if (!eq_data.enabled[i])
            continue;

        eq_filter(buf->p32, &eq_data.filters[i], count, channels);
    }
}

/**
 * Use to enable the equalizer.
 *
 * @param enable true to enable the equalizer
 */
void dsp_set_eq(bool enable)
{
    struct dsp_config * const dsp = &AUDIO_DSP;

    if (enable)
        dsp->proc_mask |= DSP_PROC_EQUALIZER;
    else
        dsp->proc_mask &= ~DSP_PROC_EQUALIZER;

    update_gain(dsp); /* Updates processing entries too */
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

    channels_mode_data.sw_gain  = straight << 8;
    channels_mode_data.sw_cross = cross << 8;
}

/**
 * Implements the different channel configurations and stereo width.
 *
 * !DSPPARAMSYNC
 * needs syncing with changes to the following dsp parameters:
 *  * dsp->stereo_mode (A)
 *
 * Note: Will never be called with count <= 0
 */

#if 0
/* SOUND_CHAN_STEREO mode is a noop so has no function - just outline one for
 * completeness. */
static void channels_process_sound_chan_stereo(struct dsp_data *data,
                                               struct dsp_buffer **buf_p)
{
    /* The channels are each just themselves */
    (void)data; (void)buf_p;
}
#endif

#ifndef DSP_HAVE_ASM_SOUND_CHAN_MONO
static void channels_process_sound_chan_mono(struct dsp_data *data,
                                             struct dsp_buffer **buf_p)
{
    struct dsp_buffer *buf = *buf_p;
    int32_t *sl = buf->p32[0];
    int32_t *sr = buf->p32[1];
    int count = buf->remcount;

    do
    {
        int32_t lr = *sl / 2 + *sr / 2;
        *sl++ = lr;
        *sr++ = lr;
    }
    while (--count > 0);

    (void)data;
}
#endif /* DSP_HAVE_ASM_SOUND_CHAN_MONO */

#ifndef DSP_HAVE_ASM_SOUND_CHAN_CUSTOM
static void channels_process_sound_chan_custom(struct dsp_data *data,
                                               struct dsp_buffer **buf_p)
{
    struct dsp_buffer *buf = *buf_p;
    int32_t *sl = buf->p32[0];
    int32_t *sr = buf->p32[1];
    int count = buf->remcount;

    const int32_t gain  = channels_mode_data.sw_gain;
    const int32_t cross = channels_mode_data.sw_cross;

    do
    {
        int32_t l = *sl;
        int32_t r = *sr;
        *sl++ = FRACMUL(l, gain) + FRACMUL(r, cross);
        *sr++ = FRACMUL(r, gain) + FRACMUL(l, cross);
    }
    while (--count > 0);

    (void)data;
}
#endif /* DSP_HAVE_ASM_SOUND_CHAN_CUSTOM */

static void channels_process_sound_chan_mono_left(struct dsp_data *data,
                                                  struct dsp_buffer **buf_p)
{
    /* Just copy over the other channel */
    struct dsp_buffer *buf = *buf_p;
    memcpy(buf->p32[1], buf->p32[0], buf->remcount * sizeof (int32_t));

    (void)data;
}

static void channels_process_sound_chan_mono_right(struct dsp_data *data,
                                                   struct dsp_buffer **buf_p)
{
    /* Just copy over the other channel */
    struct dsp_buffer *buf = *buf_p;
    memcpy(buf->p32[0], buf->p32[1], buf->remcount * sizeof (int32_t));

    (void)data;
}

#ifndef DSP_HAVE_ASM_SOUND_CHAN_KARAOKE
static void channels_process_sound_chan_karaoke(struct dsp_data *data,
                                                struct dsp_buffer **buf_p)
{
    struct dsp_buffer *buf = *buf_p;
    int32_t *sl = buf->p32[0];
    int32_t *sr = buf->p32[1];
    int count = buf->remcount;

    do
    {
        int32_t ch = *sl / 2 - *sr / 2;
        *sl++ = ch;
        *sr++ = -ch;
    }
    while (--count > 0);

    (void)data;
}
#endif /* DSP_HAVE_ASM_SOUND_CHAN_KARAOKE */

static void channels_process_init_cb(struct dsp_proc_entry *new)
{
    new->ip_mask = DSP_PROC_CHANNELS;
    new->fn = channels_mode_data.fns[channels_mode_data.channels_mode];
}

static void dsp_set_channel_config(int value)
{
    struct dsp_config * const dsp = &AUDIO_DSP;

    if (value < 0 || value >= DSP_NUM_CHANNELS_PROC)
        value = SOUND_CHAN_STEREO; /* Out of range */

    channels_mode_data.channels_mode = value;

    if (dsp->data.num_channels < 2)
        value = SOUND_CHAN_STEREO; /* No processing possible */

    dsp_proc_enable(dsp, DSP_PROC_CHANNELS, value != SOUND_CHAN_STEREO);
}

#ifdef HAVE_SW_TONE_CONTROLS
static void tone_process(struct dsp_data *data,
                         struct dsp_buffer **buf_p)
{
    struct dsp_buffer *buf = *buf_p;
    eq_filter(buf->p32, &CONFIG_FROM_DATA(data)->tone_filter,
              buf->remcount, data->num_channels);
}

static void set_tone_controls(void)
{
    struct eqfilter tone_filter;

    filter_bishelf_coefs(0xffffffff / NATIVE_FREQUENCY * 200,
                         0xffffffff / NATIVE_FREQUENCY * 3500,
                         tone_data.bass, tone_data.treble,
                         -tone_data.prescale,
                         tone_filter.coefs);

    /* Applies to all dsp to be consistent with hardware controls */
    for (unsigned i = 0; i < ARRAYLEN(dsp_conf); i++)
    {
        struct dsp_config * const dsp = &dsp_conf[i];

        dsp->tone_filter.shift = FILTER_BISHELF_SHIFT;

        memcpy(dsp->tone_filter.coefs, tone_filter.coefs,
               sizeof (dsp->tone_filter.coefs));
    
        dsp_proc_enable(dsp, DSP_PROC_TONE_CONTROLS,
                        tone_data.bass != 0 || tone_data.treble != 0);
    }
}
#endif /* HAVE_SW_TONE_CONTROLS */

/* Hook back from firmware/ part of audio, which can't/shouldn't call apps/
 * code directly.
 */
int dsp_callback(int msg, intptr_t param)
{
    switch (msg)
    {
#ifdef HAVE_SW_TONE_CONTROLS
    case DSP_CALLBACK_SET_PRESCALE:
        tone_data.prescale = param;
        set_tone_controls();
        break;
    /* prescaler is always set after calling any of these, so we wait with
     * calculating coefs until the above case is hit.
     */
    case DSP_CALLBACK_SET_BASS:
        tone_data.bass = param;
        break;
    case DSP_CALLBACK_SET_TREBLE:
        tone_data.treble = param;
        break;
#ifdef HAVE_SW_VOLUME_CONTROL
    /* FIXME: This sould be done by bottom-level PCM driver so it works with
              all PCM, not here and not in mixer -- jethead71 */
    case DSP_CALLBACK_SET_SW_VOLUME:
        update_gain(&AUDIO_DSP);
        break;
#endif /* HAVE_SW_VOLUME_CONTROL */
#endif /* HAVE_SW_TONE_CONTROLS */
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

/* Given count number of input samples, calculate the maximum number of
 * samples of output data that would be generated (the calculation is not
 * entirely exact and rounds upwards to be on the safe side; during
 * resampling, the number of samples generated depends on the current state
 * of the resampler).
 */
int dsp_output_count(struct dsp_config *dsp, int count)
{
    uint32_t proc_mask = dsp->proc_mask;

#ifdef HAVE_PITCHSCREEN
    if (proc_mask & DSP_PROC_TIMESTRETCH)
        count = tdspeed_est_output_count();
#endif /* HAVE_PITCHSCREEN */

    if (proc_mask & DSP_PROC_RESAMPLE)
        count = resample_est_output_count(dsp, count);
       
    return count;
}

/* Process and convert src audio to dst based on the DSP configuration,
 * reading count number of audio samples. dst is assumed to be large
 * enough; use dsp_output_count() to get the required number. src is an
 * array of pointers; for mono and interleaved stereo, it contains one
 * pointer to the start of the audio data and the other is ignored; for
 * non-interleaved stereo, it contains two pointers, one for each audio
 * channel. Adjusts dst_count and src_count to reflect actual consumed
 * and generated counts.
 */
static inline void dsp_process_start(struct dsp_config *dsp)
{
#if defined(CPU_COLDFIRE)
    /* set emac unit for dsp processing, and save old macsr, we're running in
       codec thread context at this point, so can't clobber it */
    dsp->old_macsr = coldfire_get_macsr();
    coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE);
#endif
#ifdef HAVE_PITCHSCREEN
    dsp->processing = true;
#endif
    (void)dsp;
}

static inline void dsp_process_end(struct dsp_config *dsp)
{
#ifdef HAVE_PITCHSCREEN
    dsp->processing = false;
#endif
#if defined(CPU_COLDFIRE)
    /* set old macsr again */
    coldfire_set_macsr(dsp->old_macsr);
#endif
    (void)dsp;
}

void dsp_process(struct dsp_config *dsp, struct dsp_buffer *src,
                 struct dsp_buffer *dst)
{
    if (dst->bufcount <= 0)
        return; /* No place to put anything, thus nothing can be consumed */

    /* At least perform one yield before starting */
    long last_yield = current_tick;
    yield();

    dsp_process_start(dsp);

    while (1)
    {
        /* Out-of-place-processing stages take the current buf as input
         * and switch the buffer to their own output buffer */
        struct dsp_buffer *buf = src;

        /* Convert input samples to internal format */
        dsp->input_samples(&dsp->data, &buf);

        /* Check if additional processing is required */
        int count = dsp->proc_count;

        if (count != 0)
        {
            /* Have additional work to do */
            struct dsp_proc_entry *e = dsp->proc_entries;

            do
            {
                uint32_t ip_mask = e->ip_mask;

                if (ip_mask == 0 || /* Not in-place */
                    ((ip_mask & buf->proc_mask) == 0 &&
                     (buf->proc_mask |= ip_mask, buf->remcount > 0)))
                {
                    e->fn(&dsp->data, &buf);
                }
            }
            while (e++, --count > 0);
        }

        /* Don't overread/write src/destination */
        int outcount = MIN(dst->bufcount, buf->remcount);

        if (outcount <= 0)
            break; /* Output full or purged internal buffers */

        /* Convert internal format to output format */
        dsp->output_samples(outcount, &dsp->data, (void *)buf->p32,
                            dst->p16out);

        /* Advance buffers by what output consumed and produced */
        dsp_advance_buffer32(buf, outcount);
        dsp_advance_buffer_output(dst, outcount);

        /* Yield at least once each tick */
        long tick = current_tick;
        if (TIME_AFTER(tick, last_yield))
        {
            last_yield = tick;
            yield();
        }
    }

    dsp_process_end(dsp);
}

static void dsp_update_functions(struct dsp_config *dsp)
{
    sample_input_new_format(dsp);
    sample_output_new_format(dsp);
    if (dsp == &AUDIO_DSP)
    {
        dsp_set_crossfeed(dsp->proc_mask & DSP_PROC_CROSSFEED);
        dsp_set_channel_config(channels_mode_data.channels_mode);
    }
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

#ifdef HAVE_PITCHSCREEN
    case DSP_IS_BUSY:
        return dsp->processing;
#endif

    case DSP_SET_FREQUENCY:
        memset(&dsp->data.resample_data, 0, sizeof (dsp->data.resample_data));
        /* Fall through!!! */
    case DSP_SWITCH_FREQUENCY:
        dsp->codec_frequency = (value == 0) ? NATIVE_FREQUENCY : value;
        /* Account for playback speed adjustment when setting dsp->frequency
           if we're called from the main audio thread. Voice UI thread should
           not need this feature.
         */
#ifdef HAVE_PITCHSCREEN
        if (dsp == &AUDIO_DSP)
            dsp->frequency = pitch_ratio * dsp->codec_frequency / PITCH_SPEED_100;
        else
#endif
            dsp->frequency = dsp->codec_frequency;

        resampler_new_delta(dsp);

#ifdef HAVE_PITCHSCREEN
        if (dsp == &AUDIO_DSP)
            tdspeed_setup();
#endif
        break;

    case DSP_SET_SAMPLE_DEPTH:
        dsp->sample_depth = value;
        dsp->data.frac_bits = (dsp->sample_depth <= NATIVE_DEPTH) ?
                                WORD_FRACBITS : value;
        dsp->data.output_scale = dsp->data.frac_bits + 1 - NATIVE_DEPTH;
        sample_input_new_format(dsp);
        dither_init(dsp);
        break;

    case DSP_SET_STEREO_MODE:
        dsp->stereo_mode = value;
        dsp->data.num_channels = value == STEREO_MONO ? 1 : 2;
        dsp_update_functions(dsp);

#ifdef HAVE_PITCHSCREEN
        if (dsp == &AUDIO_DSP)
            tdspeed_setup();
#endif
        break;

    case DSP_RESET:
        dsp->stereo_mode = STEREO_NONINTERLEAVED;
        dsp->data.num_channels = 2;
        dsp->sample_depth = NATIVE_DEPTH;
        dsp->data.frac_bits = WORD_FRACBITS;
        dsp->data.output_scale = WORD_FRACBITS + 1 - NATIVE_DEPTH;
        dsp->codec_frequency = dsp->frequency = NATIVE_FREQUENCY;

        if (dsp == &AUDIO_DSP)
        {
            struct dsp_replay_gains gains = { 0, 0, 0, 0 };
            dsp_update_replaygain(&gains);
        }

        dsp_update_functions(dsp);
        resampler_new_delta(dsp);

        if (dsp == &AUDIO_DSP)
        {
#ifdef HAVE_PITCHSCREEN
            tdspeed_setup();
#endif
            compressor_reset();
        }
        break;

    case DSP_FLUSH:
        memset(&dsp->data.resample_data, 0,
               sizeof (dsp->data.resample_data));
        resampler_new_delta(dsp);
        dither_init(dsp);

        if (dsp == &AUDIO_DSP)
        {
#ifdef HAVE_PITCHSCREEN
            tdspeed_setup();
#endif
            compressor_reset();
        }
        break;

    case DSP_SET_REPLAY_GAIN:
        if (dsp == &AUDIO_DSP && (void *)value)
            dsp_update_replaygain((struct dsp_replay_gains *)value);
        break;

    default:
        return 0;
    }

    return 1;
}

int get_replaygain_mode(bool have_track_gain, bool have_album_gain)
{
    bool track = ((global_settings.replaygain_type == REPLAYGAIN_TRACK)
        || ((global_settings.replaygain_type == REPLAYGAIN_SHUFFLE)
            && global_settings.playlist_shuffle));

    int type = (!track && have_album_gain) ? REPLAYGAIN_ALBUM 
                : have_track_gain ? REPLAYGAIN_TRACK : -1;
    
    return type;
}

static void dsp_update_replaygain(const struct dsp_replay_gains *gains)
{
    dsp_gain.rpgains = *gains;

    int32_t gain = 0;

    if ((global_settings.replaygain_type != REPLAYGAIN_OFF) ||
            global_settings.replaygain_noclip)
    {
        bool track_mode = get_replaygain_mode(gains->track_gain != 0,
            gains->album_gain != 0) == REPLAYGAIN_TRACK;
        int32_t peak = (track_mode || gains->album_peak == 0) ?
            gains->track_peak : gains->album_peak;

        if (global_settings.replaygain_type != REPLAYGAIN_OFF)
        {
            gain = (track_mode || gains->album_gain == 0) ?
                gains->track_gain : gains->album_gain;

            if (global_settings.replaygain_preamp)
            {
                int32_t preamp = get_replaygain_int(
                    global_settings.replaygain_preamp * 10);

                gain = (int32_t) (((int64_t) gain * preamp) >> 24);
            }
        }

        if (gain == 0)
        {
            /* So that noclip can work even with no gain information. */
            gain = DEFAULT_GAIN;
        }

        if (global_settings.replaygain_noclip && (peak != 0)
            && (((int64_t) gain * peak) >> 24) >= DEFAULT_GAIN)
        {
            gain = (int32_t)(((int64_t) DEFAULT_GAIN << 24) / peak);
        }

        if (gain == DEFAULT_GAIN)
        {
            /* Nothing to do, disable processing. */
            gain = 0;
        }
    }

    /* Store in S7.24 format to simplify calculations. */
    dsp_gain.replaygain = gain;

    /* Sync audio DSP */
    update_gain(&AUDIO_DSP);
}

void dsp_set_replaygain(void)
{
    /* Resync with current gains */
    dsp_update_replaygain(&dsp_gain.rpgains);
}

/** SET COMPRESSOR
 *  Called by the menu system to configure the compressor process */
void dsp_set_compressor(void)
{
    /* enable/disable the compressor */
    dsp_proc_enable(&AUDIO_DSP, DSP_PROC_COMPRESSOR,
                    compressor_update(&global_settings.compressor_settings));
}
