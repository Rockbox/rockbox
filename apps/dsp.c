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
#include "debug.h"

#ifndef SIMULATOR
#include <dsp_asm.h>
#endif

/* The "dither" code to convert the 24-bit samples produced by libmad was
 * taken from the coolplayer project - coolplayer.sourceforge.net
 */

/* 16-bit samples are scaled based on these constants. The shift should be
 * no more than 15.
 */
#define WORD_SHIFT          12
#define WORD_FRACBITS       27

#define NATIVE_DEPTH        16
#define SAMPLE_BUF_SIZE     256
#define RESAMPLE_BUF_SIZE   (256 * 4)   /* Enough for 11,025 Hz -> 44,100 Hz*/
#define DEFAULT_REPLAYGAIN  0x01000000

/* These are the constants for the filters in the crossfeed */

#define ATT 0x0CCCCCCDL /* 0.1 */
#define ATT_COMP 0x73333333L /* 0.9 */
#define LOW  0x4CCCCCCDL /* 0.6 */
#define LOW_COMP 0x33333333L /* 0.4 */
#define HIGH_NEG -0x66666666L /* -0.2 (not unsigned!) */
#define HIGH_COMP 0x66666666L /* 0.8 */

#if defined(CPU_COLDFIRE) && !defined(SIMULATOR)

/* Multiply two S.31 fractional integers and return the sign bit and the
 * 31 most significant bits of the result.
 */
#define FRACMUL(x, y) \
({ \
    long t; \
    asm volatile ("mac.l    %[a], %[b], %%acc0\n\t" \
                  "movclr.l %%acc0, %[t]\n\t" \
                  : [t] "=r" (t) : [a] "r" (x), [b] "r" (y)); \
    t; \
})

/* Multiply one S.31-bit and one S8.23 fractional integer and return the
 * sign bit and the 31 most significant bits of the result.
 */
#define FRACMUL_8(x, y) \
({ \
    long t; \
    long u; \
    asm volatile ("mac.l    %[a], %[b], %%acc0\n\t" \
                  "move.l   %%accext01, %[u]\n\t" \
                  "movclr.l %%acc0, %[t]\n\t" \
                  : [t] "=r" (t), [u] "=r" (u) : [a] "r" (x), [b] "r" (y)); \
    (t << 8) | (u & 0xff); \
})

/* Multiply one S.31-bit and one S8.23 fractional integer and return the
 * sign bit and the 31 most significant bits of the result. Load next value
 * to multiply with into x from s (and increase s); x must contain the
 * initial value.
 */
#define FRACMUL_8_LOOP_PART(x, s, d, y) \
{ \
    long u; \
    asm volatile ("mac.l    %[a], %[b], (%[c])+, %[a], %%acc0\n\t" \
                  "move.l   %%accext01, %[u]\n\t" \
                  "movclr.l %%acc0, %[t]" \
                  : [a] "+r" (x), [c] "+a" (s), [t] "=r" (d), [u] "=r" (u) \
                  : [b] "r" (y)); \
    d = (d << 8) | (u & 0xff); \
}

#define FRACMUL_8_LOOP(x, y, s, d) \
{ \
    long t; \
    FRACMUL_8_LOOP_PART(x, s, t, y); \
    asm volatile ("move.l   %[t],(%[d])+" \
                  : [d] "+a" (d)\
                  : [t] "r" (t)); \
}

#define ACC(acc, x, y) \
    (void)acc; \
    asm volatile ("mac.l %[a], %[b], %%acc0" \
                  : : [a] "i,r" (x), [b] "i,r" (y));
#define GET_ACC(acc) \
({ \
    long t; \
    (void)acc; \
    asm volatile ("movclr.l %%acc0, %[t]" \
                  : [t] "=r" (t)); \
    t; \
})

#define ACC_INIT(acc, x, y) ACC(acc, x, y)

#elif defined(CPU_ARM) && !defined(SIMULATOR)

/* Multiply two S.31 fractional integers and return the sign bit and the
 * 31 most significant bits of the result.
 */
#define FRACMUL(x, y) \
({ \
    long t; \
    asm volatile ("smull    r0, r1, %[a], %[b]\n\t" \
                  "mov      %[t], r1, asl #1\n\t" \
                  "orr      %[t], %[t], r0, lsr #31\n\t" \
                  : [t] "=r" (t) : [a] "r" (x), [b] "r" (y) : "r0", "r1"); \
    t; \
})

#define ACC_INIT(acc, x, y) acc = FRACMUL(x, y)
#define ACC(acc, x, y) acc += FRACMUL(x, y)
#define GET_ACC(acc) acc

/* Multiply one S.31-bit and one S8.23 fractional integer and store the
 * sign bit and the 31 most significant bits of the result to d (and
 * increase d). Load next value to multiply with into x from s (and
 * increase s); x must contain the initial value.
 */
#define FRACMUL_8_LOOP(x, y, s, d) \
({ \
    asm volatile ("smull    r0, r1, %[a], %[b]\n\t" \
                  "mov      %[t], r1, asl #9\n\t" \
                  "orr      %[t], %[t], r0, lsr #23\n\t" \
                  : [t] "=r" (*(d)++) : [a] "r" (x), [b] "r" (y) : "r0", "r1"); \
    x = *(s)++; \
})

#else

#define ACC_INIT(acc, x, y) acc = FRACMUL(x, y)
#define ACC(acc, x, y) acc += FRACMUL(x, y)
#define GET_ACC(acc) acc
#define FRACMUL(x, y) (long) (((((long long) (x)) * ((long long) (y))) >> 31))
#define FRACMUL_8(x, y) (long) (((((long long) (x)) * ((long long) (y))) >> 23))
#define FRACMUL_8_LOOP(x, y, s, d) \
({ \
    long t = x; \
    x = *(s)++; \
    *(d)++ = (long) (((((long long) (t)) * ((long long) (y))) >> 23)); \
})

#endif

struct dsp_config
{
    long codec_frequency; /* Sample rate of data coming from the codec */
    long frequency;       /* Effective sample rate after pitch shift (if any) */
    long clip_min;
    long clip_max;
    long track_gain;
    long album_gain;
    long track_peak;
    long album_peak;
    long replaygain;    /* Note that this is in S8.23 format. */
    int sample_depth;
    int sample_bytes;
    int stereo_mode;
    int frac_bits;
    bool dither_enabled;
    bool new_gain;
    bool crossfeed_enabled;
    bool eq_enabled;
};

struct resample_data
{
    long phase, delta;
    int32_t last_sample[2];
};

struct dither_data
{
    long error[3];
    long random;
};

struct crossfeed_data
{
    int32_t lowpass[2];
    int32_t highpass[2];
    int32_t delay[2][13];
    int index;
};

/* Current setup is one lowshelf filters, three peaking filters and one
   highshelf filter. Varying the number of shelving filters make no sense,
   but adding peaking filters are possible. */
struct eq_state {
    char enabled[5];    /* Flags for active filters */
    struct eqfilter filters[5];
};

static struct dsp_config dsp_conf[2] IBSS_ATTR;
static struct dither_data dither_data[2] IBSS_ATTR;
static struct resample_data resample_data[2] IBSS_ATTR;
struct crossfeed_data crossfeed_data IBSS_ATTR;
static struct eq_state eq_data;

static int pitch_ratio = 1000;
static int channels_mode = 0;
static int32_t sw_gain, sw_cross;

extern int current_codec;
struct dsp_config *dsp;

/* The internal format is 32-bit samples, non-interleaved, stereo. This
 * format is similar to the raw output from several codecs, so the amount
 * of copying needed is minimized for that case.
 */

static int32_t sample_buf[SAMPLE_BUF_SIZE] IBSS_ATTR;
static int32_t resample_buf[RESAMPLE_BUF_SIZE] IBSS_ATTR;

int sound_get_pitch(void)
{
    return pitch_ratio;
}

void sound_set_pitch(int permille)
{
    pitch_ratio = permille;
    
    dsp_configure(DSP_SWITCH_FREQUENCY, (int *)dsp->codec_frequency);
}

/* Convert at most count samples to the internal format, if needed. Returns
 * number of samples ready for further processing. Updates src to point
 * past the samples "consumed" and dst is set to point to the samples to
 * consume. Note that for mono, dst[0] equals dst[1], as there is no point
 * in processing the same data twice.
 */
static int convert_to_internal(const char* src[], int count, int32_t* dst[])
{
    count = MIN(SAMPLE_BUF_SIZE / 2, count);

    if ((dsp->sample_depth <= NATIVE_DEPTH)
        || (dsp->stereo_mode == STEREO_INTERLEAVED))
    {
        dst[0] = &sample_buf[0];
        dst[1] = (dsp->stereo_mode == STEREO_MONO)
            ? dst[0] : &sample_buf[SAMPLE_BUF_SIZE / 2];
    }
    else
    {
        dst[0] = (int32_t*) src[0];
        dst[1] = (int32_t*) ((dsp->stereo_mode == STEREO_MONO) ? src[0] : src[1]);
    }

    if (dsp->sample_depth <= NATIVE_DEPTH)
    {
        short* s0 = (short*) src[0];
        int32_t* d0 = dst[0];
        int32_t* d1 = dst[1];
        int scale = WORD_SHIFT;
        int i;

        if (dsp->stereo_mode == STEREO_INTERLEAVED)
        {
            for (i = 0; i < count; i++)
            {
                *d0++ = *s0++ << scale;
                *d1++ = *s0++ << scale;
            }
        }
        else if (dsp->stereo_mode == STEREO_NONINTERLEAVED)
        {
            short* s1 = (short*) src[1];

            for (i = 0; i < count; i++)
            {
                *d0++ = *s0++ << scale;
                *d1++ = *s1++ << scale;
            }
        }
        else
        {
            for (i = 0; i < count; i++)
            {
                *d0++ = *s0++ << scale;
            }
        }
    }
    else if (dsp->stereo_mode == STEREO_INTERLEAVED)
    {
        int32_t* s0 = (int32_t*) src[0];
        int32_t* d0 = dst[0];
        int32_t* d1 = dst[1];
        int i;

        for (i = 0; i < count; i++)
        {
            *d0++ = *s0++;
            *d1++ = *s0++;
        }
    }

    if (dsp->stereo_mode == STEREO_NONINTERLEAVED)
    {
        src[0] += count * dsp->sample_bytes;
        src[1] += count * dsp->sample_bytes;
    }
    else if (dsp->stereo_mode == STEREO_INTERLEAVED)
    {
        src[0] += count * dsp->sample_bytes * 2;
    }
    else
    {
        src[0] += count * dsp->sample_bytes;
    }

    return count;
}

static void resampler_set_delta(int frequency)
{
    resample_data[current_codec].delta = (unsigned long) 
        frequency * 65536LL / NATIVE_FREQUENCY;
}

/* Linear resampling that introduces a one sample delay, because of our
 * inability to look into the future at the end of a frame.
 */

/* TODO: we really should have a separate set of resample functions for both
   mono and stereo to avoid all this internal branching and looping. */
static long downsample(int32_t **dst, int32_t **src, int count,
    struct resample_data *r)
{
    long phase = r->phase;
    long delta = r->delta;
    int32_t last_sample;
    int32_t *d[2] = { dst[0], dst[1] };
    int pos = phase >> 16;
    int i = 1, j;
    int num_channels = dsp->stereo_mode == STEREO_MONO ? 1 : 2;
    
    for (j = 0; j < num_channels; j++) {
        last_sample = r->last_sample[j];
        /* Do we need last sample of previous frame for interpolation? */
        if (pos > 0)
        {
            last_sample = src[j][pos - 1];
        }
        *d[j]++ = last_sample + FRACMUL((phase & 0xffff) << 15,
            src[j][pos] - last_sample);
    }
    phase += delta;
 
    while ((pos = phase >> 16) < count)
    {
        for (j = 0; j < num_channels; j++)
            *d[j]++ = src[j][pos - 1] + FRACMUL((phase & 0xffff) << 15,
                src[j][pos] - src[j][pos - 1]);
         phase += delta;
         i++;
    }

    /* Wrap phase accumulator back to start of next frame. */
    r->phase = phase - (count << 16);
    r->delta = delta;
    r->last_sample[0] = src[0][count - 1];
    r->last_sample[1] = src[1][count - 1];
    return i;
}

static long upsample(int32_t **dst, int32_t **src, int count, struct resample_data *r)
{
    long phase = r->phase;
    long delta = r->delta;
    int32_t *d[2] = { dst[0], dst[1] };
    int i = 0, j;
    int pos;
    int num_channels = dsp->stereo_mode == STEREO_MONO ? 1 : 2;
    
    while ((pos = phase >> 16) == 0)
    {
       for (j = 0; j < num_channels; j++)
           *d[j]++ = r->last_sample[j] + FRACMUL((phase & 0xffff) << 15,
                src[j][pos] - r->last_sample[j]);
        phase += delta;
        i++;
    }

    while ((pos = phase >> 16) < count)
    {
        for (j = 0; j < num_channels; j++)
            *d[j]++ = src[j][pos - 1] + FRACMUL((phase & 0xffff) << 15,
                src[j][pos] - src[j][pos - 1]);
        phase += delta;
        i++;
    }

    /* Wrap phase accumulator back to start of next frame. */
    r->phase = phase - (count << 16);
    r->delta = delta;
    r->last_sample[0] = src[0][count - 1];
    r->last_sample[1] = src[1][count - 1];
    return i;
}

/* Resample count stereo samples. Updates the src array, if resampling is
 * done, to refer to the resampled data. Returns number of stereo samples
 * for further processing.
 */
static inline int resample(int32_t* src[], int count)
{
    long new_count;

    if (dsp->frequency != NATIVE_FREQUENCY)
    {
        int32_t* dst[2] = {&resample_buf[0], &resample_buf[RESAMPLE_BUF_SIZE / 2]};

        if (dsp->frequency < NATIVE_FREQUENCY)
        {
            new_count = upsample(dst, src, count, 
                            &resample_data[current_codec]);
        }
        else
        {
            new_count = downsample(dst, src, count,
                            &resample_data[current_codec]);
        }

        src[0] = dst[0];
        if (dsp->stereo_mode != STEREO_MONO)
            src[1] = dst[1];
        else
            src[1] = dst[0];
    }
    else
    {
        new_count = count;
    }

    return new_count;
}

static inline long clip_sample(int32_t sample, int32_t min, int32_t max)
{
    if (sample > max)
    {
        sample = max;
    }
    else if (sample < min)
    {
        sample = min;
    }

    return sample;
}

/* The "dither" code to convert the 24-bit samples produced by libmad was
 * taken from the coolplayer project - coolplayer.sourceforge.net
 */

static long dither_sample(int32_t sample, int32_t bias, int32_t mask,
    struct dither_data* dither)
{
    int32_t output;
    int32_t random;
    int32_t min;
    int32_t max;

    /* Noise shape and bias */

    sample += dither->error[0] - dither->error[1] + dither->error[2];
    dither->error[2] = dither->error[1];
    dither->error[1] = dither->error[0] / 2;

    output = sample + bias;

    /* Dither */

    random = dither->random * 0x0019660dL + 0x3c6ef35fL;
    sample += (random & mask) - (dither->random & mask);
    dither->random = random;

    /* Clip and quantize */

    min = dsp->clip_min;
    max = dsp->clip_max;
    sample = clip_sample(sample, min, max);
    output = clip_sample(output, min, max) & ~mask;

    /* Error feedback */

    dither->error[0] = sample - output;

    return output;
}

/* Applies crossfeed to the stereo signal in src.
 * Crossfeed is a process where listening over speakers is simulated. This
 * is good for old hard panned stereo records, which might be quite fatiguing
 * to listen to on headphones with no crossfeed.
 */
#ifndef DSP_HAVE_ASM_CROSSFEED
static void apply_crossfeed(int32_t* src[], int count)
{
    int32_t a; /* accumulator */

    int32_t low_left = crossfeed_data.lowpass[0];
    int32_t low_right = crossfeed_data.lowpass[1];
    int32_t high_left = crossfeed_data.highpass[0];
    int32_t high_right = crossfeed_data.highpass[1];
    unsigned int index = crossfeed_data.index;

    int32_t left, right;

    int32_t* delay_l = crossfeed_data.delay[0];
    int32_t* delay_r = crossfeed_data.delay[1];

    int i;

    for (i = 0; i < count; i++)
    {
        /* use a low-pass filter on the signal */
        left = src[0][i];
        right = src[1][i];

        ACC_INIT(a, LOW, low_left); ACC(a, LOW_COMP, left);
        low_left = GET_ACC(a);

        ACC_INIT(a, LOW, low_right); ACC(a, LOW_COMP, right);
        low_right = GET_ACC(a);

        /* use a high-pass filter on the signal */

        ACC_INIT(a, HIGH_NEG, high_left); ACC(a, HIGH_COMP, left);
        high_left = GET_ACC(a);

        ACC_INIT(a, HIGH_NEG, high_right); ACC(a, HIGH_COMP, right);
        high_right = GET_ACC(a);

        /* New data is the high-passed signal + delayed and attenuated 
         * low-passed signal from the other channel */

        ACC_INIT(a, ATT, delay_r[index]); ACC(a, ATT_COMP, high_left);
        src[0][i] = GET_ACC(a);

        ACC_INIT(a, ATT, delay_l[index]); ACC(a, ATT_COMP, high_right);
        src[1][i] = GET_ACC(a);

        /* Store the low-passed signal in the ringbuffer */

        delay_l[index] = low_left;
        delay_r[index] = low_right;

        index = (index + 1) % 13;
    }

    crossfeed_data.index = index;
    crossfeed_data.lowpass[0] = low_left;
    crossfeed_data.lowpass[1] = low_right;
    crossfeed_data.highpass[0] = high_left;
    crossfeed_data.highpass[1] = high_right;
}
#endif

/* Synchronize the EQ filters with the global settings */
void dsp_eq_update_data(bool enabled, int band)
{
    const int *setting;
    long gain;
    unsigned long cutoff, q;

    dsp->eq_enabled = enabled;
    
    /* Adjust setting pointer to the band we actually want to change */
    setting = &global_settings.eq_band0_cutoff + (band * 3);

    /* Convert user settings to format required by coef generator functions */
    cutoff = 0xffffffff / NATIVE_FREQUENCY * (*setting++);
    q = ((*setting++) << 16) / 10;        /* 16.16 */
    gain = ((*setting++) << 16) / 10;     /* s15.16 */

    /* The coef functions assume the EMAC unit is in fractional mode */
    #if defined(CPU_COLDFIRE) && !defined(SIMULATOR)
    /* set emac unit for dsp processing, and save old macsr, we're running in
       codec thread context at this point, so can't clobber it */
    unsigned long old_macsr = coldfire_get_macsr();
    coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE | EMAC_ROUND);
    #endif

    /* Assume a band is disabled if the gain is zero */
    if (gain == 0) {
        eq_data.enabled[band] = 0;
    } else {
        if (band == 0)
            eq_ls_coefs(cutoff, q, gain, eq_data.filters[band].coefs);
        else if (band == 4)
            eq_hs_coefs(cutoff, q, gain, eq_data.filters[band].coefs);
        else
            eq_pk_coefs(cutoff, q, gain, eq_data.filters[band].coefs);

        eq_data.enabled[band] = 1;
    }

    #if defined(CPU_COLDFIRE) && !defined(SIMULATOR)
    /* set old macsr again */
    coldfire_set_macsr(old_macsr);
    #endif
}

/* Apply EQ filters to those bands that have got it switched on. */
static void eq_process(int32_t **x, unsigned num)
{
    int i;
    unsigned int channels = dsp->stereo_mode != STEREO_MONO ? 2 : 1;
    unsigned shift;
    
    /* filter configuration currently is 1 low shelf filter, 3 band peaking
       filters and 1 high shelf filter, in that order. we need to know this
       so we can choose the correct shift factor.
     */
    for (i = 0; i < 5; i++) {
        if (eq_data.enabled[i]) {
            if (i == 0 || i == 4) /* shelving filters */
                shift = EQ_SHELF_SHIFT;
            else
                shift = EQ_PEAK_SHIFT;
            eq_filter(x, &eq_data.filters[i], num, channels, shift);
        }
    }
}

/* Apply a constant gain to the samples (e.g., for ReplayGain). May update
 * the src array if gain was applied.
 * Note that this must be called before the resampler.
 */
static void apply_gain(int32_t* _src[], int _count)
{
    struct dsp_config *my_dsp = dsp;
    if (my_dsp->replaygain)
    {
        int32_t** src = _src;
        int count = _count;
        int32_t* s0 = src[0];
        int32_t* s1 = src[1];
        long gain = my_dsp->replaygain;
        int32_t s;
        int i;
        int32_t *d;

        if (s0 != s1)
        {
            d = &sample_buf[SAMPLE_BUF_SIZE / 2];
            src[1] = d;
            s = *s1++;

            for (i = 0; i < count; i++)
                FRACMUL_8_LOOP(s, gain, s1, d);
        }
        else
        {
            src[1] = &sample_buf[0];
        }

        d = &sample_buf[0];
        src[0] = d;
        s = *s0++;

        for (i = 0; i < count; i++)
            FRACMUL_8_LOOP(s, gain, s0, d);
    }
}

void channels_set(int value)
{
    channels_mode =  value;    
}

void stereo_width_set(int value)
{
    long width, straight, cross;
    
    width = value*0x7fffff/100;
    if (value <= 100) {
        straight = (0x7fffff + width)/2;
        cross = straight - width;
    } else {
        straight = 0x7fffff; 
        cross = 0x7fffff - ((int64_t)(2*width) << 23)/(0x7fffff + width);
    }
    sw_gain = straight << 8;
    sw_cross = cross << 8;
}

/* Implements the different channel configurations and stereo width.
 * We might want to combine this with the write_samples stage for efficiency,
 * but for now we'll just let it stay as a stage of its own. 
 */
static void channels_process(int32_t **src, int num)
{
    int i;
    int32_t *sl = src[0], *sr = src[1];

    if (channels_mode == SOUND_CHAN_STEREO)
        return;
    switch (channels_mode) {
    case SOUND_CHAN_MONO:
        for (i = 0; i < num; i++)
            sl[i] = sr[i] = sl[i]/2 + sr[i]/2;
        break;
    case SOUND_CHAN_CUSTOM:
        for (i = 0; i < num; i++) {
            int32_t left_sample = sl[i];

            sl[i] = FRACMUL(sl[i], sw_gain) + FRACMUL(sr[i], sw_cross);
            sr[i] = FRACMUL(sr[i], sw_gain) + FRACMUL(left_sample, sw_cross);
        }
        break;
    case SOUND_CHAN_MONO_LEFT:
        for (i = 0; i < num; i++)
            sr[i] = sl[i];
        break;
    case SOUND_CHAN_MONO_RIGHT:
        for (i = 0; i < num; i++)
            sl[i] = sr[i];
        break;
    case SOUND_CHAN_KARAOKE:
        for (i = 0; i < num; i++) {
            int32_t left_sample = sl[i];
            
            sl[i] -= sr[i];
            sr[i] -= left_sample;
        }
        break;
    }
}

static void write_samples(short* dst, int32_t* src[], int count)
{
    int32_t* s0 = src[0];
    int32_t* s1 = src[1];
    int scale = dsp->frac_bits + 1 - NATIVE_DEPTH;

    if (dsp->dither_enabled)
    {
        long bias = (1L << (dsp->frac_bits - NATIVE_DEPTH));
        long mask = (1L << scale) - 1;

        while (count-- > 0)
        {
            *dst++ = (short) (dither_sample(*s0++, bias, mask, &dither_data[0])
                >> scale);
            *dst++ = (short) (dither_sample(*s1++, bias, mask, &dither_data[1])
                >> scale);
        }
    }
    else
    {
        long min = dsp->clip_min;
        long max = dsp->clip_max;

        while (count-- > 0)
        {
            *dst++ = (short) (clip_sample(*s0++, min, max) >> scale);
            *dst++ = (short) (clip_sample(*s1++, min, max) >> scale);
        }
    }
}

/* Process and convert src audio to dst based on the DSP configuration,
 * reading size bytes of audio data. dst is assumed to be large enough; use
 * dst_get_dest_size() to get the required size. src is an array of
 * pointers; for mono and interleaved stereo, it contains one pointer to the
 * start of the audio data; for non-interleaved stereo, it contains two
 * pointers, one for each audio channel. Returns number of bytes written to
 * dest.
 */
long dsp_process(char* dst, const char* src[], long size)
{
    int32_t* tmp[2];
    long written = 0;
    long factor;
    int samples;

    #if defined(CPU_COLDFIRE) && !defined(SIMULATOR)
    /* set emac unit for dsp processing, and save old macsr, we're running in
       codec thread context at this point, so can't clobber it */
    unsigned long old_macsr = coldfire_get_macsr();
    coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE);
    #endif
    
    dsp = &dsp_conf[current_codec];

    factor = (dsp->stereo_mode != STEREO_MONO) ? 2 : 1;
    size /= dsp->sample_bytes * factor;
    dsp_set_replaygain(false);

    while (size > 0)
    {
        samples = convert_to_internal(src, size, tmp);
        size -= samples;
        apply_gain(tmp, samples);
        samples = resample(tmp, samples);
        if (dsp->crossfeed_enabled && dsp->stereo_mode != STEREO_MONO)
            apply_crossfeed(tmp, samples);
        if (dsp->eq_enabled)
            eq_process(tmp, samples);
        if (dsp->stereo_mode != STEREO_MONO)
            channels_process(tmp, samples);
        write_samples((short*) dst, tmp, samples);
        written += samples;
        dst += samples * sizeof(short) * 2;
        yield();
    }
    #if defined(CPU_COLDFIRE) && !defined(SIMULATOR)
    /* set old macsr again */
    coldfire_set_macsr(old_macsr);
    #endif
    return written * sizeof(short) * 2;
}

/* Given size bytes of input data, calculate the maximum number of bytes of
 * output data that would be generated (the calculation is not entirely
 * exact and rounds upwards to be on the safe side; during resampling,
 * the number of samples generated depends on the current state of the
 * resampler).
 */
/* dsp_input_size MUST be called afterwards */
long dsp_output_size(long size)
{
    dsp = &dsp_conf[current_codec];
    
    if (dsp->sample_depth > NATIVE_DEPTH)
    {
        size /= 2;
    }

    if (dsp->frequency != NATIVE_FREQUENCY)
    {
        size = (long) ((((unsigned long) size * NATIVE_FREQUENCY)
            + (dsp->frequency - 1)) / dsp->frequency);
    }

    /* round to the next multiple of 2 (these are shorts) */
    size = (size + 1) & ~1;

    if (dsp->stereo_mode == STEREO_MONO)
    {
        size *= 2;
    }

    /* now we have the size in bytes for two resampled channels,
     * and the size in (short) must not exceed RESAMPLE_BUF_SIZE to
     * avoid resample buffer overflow. One must call dsp_input_size()
     * to get the correct input buffer size. */
    if (size > RESAMPLE_BUF_SIZE*2)
        size = RESAMPLE_BUF_SIZE*2;

    return size;
}

/* Given size bytes of output buffer, calculate number of bytes of input
 * data that would be consumed in order to fill the output buffer.
 */
long dsp_input_size(long size)
{
    dsp = &dsp_conf[current_codec];
    
    /* convert to number of output stereo samples. */
    size /= 2;

    /* Mono means we need half input samples to fill the output buffer */
    if (dsp->stereo_mode == STEREO_MONO)
        size /= 2;

    /* size is now the number of resampled input samples. Convert to
       original input samples. */
    if (dsp->frequency != NATIVE_FREQUENCY)
    {
        /* Use the real resampling delta =
         *  (unsigned long) dsp->frequency * 65536 / NATIVE_FREQUENCY, and
         * round towards zero to avoid buffer overflows. */
        size = ((unsigned long)size *
            resample_data[current_codec].delta) >> 16;
    }

    /* Convert back to bytes. */
    if (dsp->sample_depth > NATIVE_DEPTH)
        size *= 4;
    else
        size *= 2;

    return size;
}

int dsp_stereo_mode(void)
{
    dsp = &dsp_conf[current_codec];
    
    return dsp->stereo_mode;
}

bool dsp_configure(int setting, void *value)
{
    dsp = &dsp_conf[current_codec];

    switch (setting)
    {
    case DSP_SET_FREQUENCY:
        memset(&resample_data[current_codec], 0,
            sizeof(struct resample_data));
        /* Fall through!!! */
    case DSP_SWITCH_FREQUENCY:
        dsp->codec_frequency = ((long) value == 0) ? NATIVE_FREQUENCY : (long) value;
        /* Account for playback speed adjustment when settingg dsp->frequency
           if we're called from the main audio thread. Voice UI thread should
           not need this feature.
         */
        if (current_codec == CODEC_IDX_AUDIO)
            dsp->frequency = pitch_ratio * dsp->codec_frequency / 1000;
        else
            dsp->frequency = dsp->codec_frequency;
        resampler_set_delta(dsp->frequency); 
        break;

    case DSP_SET_CLIP_MIN:
        dsp->clip_min = (long) value;
        break;

    case DSP_SET_CLIP_MAX:
        dsp->clip_max = (long) value;
        break;

    case DSP_SET_SAMPLE_DEPTH:
        dsp->sample_depth = (long) value;

        if (dsp->sample_depth <= NATIVE_DEPTH)
        {
            dsp->frac_bits = WORD_FRACBITS;
            dsp->sample_bytes = sizeof(short);
            dsp->clip_max =  ((1 << WORD_FRACBITS) - 1);
            dsp->clip_min = -((1 << WORD_FRACBITS));
        }
        else
        {
            dsp->frac_bits = (long) value;
            dsp->sample_bytes = 4; /* samples are 32 bits */
            dsp->clip_max = (1 << (long)value) - 1;
            dsp->clip_min = -(1 << (long)value);
        }

        break;

    case DSP_SET_STEREO_MODE:
        dsp->stereo_mode = (long) value;
        break;

    case DSP_RESET:
        dsp->dither_enabled = false;
        dsp->stereo_mode = STEREO_NONINTERLEAVED;
        dsp->clip_max =  ((1 << WORD_FRACBITS) - 1);
        dsp->clip_min = -((1 << WORD_FRACBITS));
        dsp->track_gain = 0;
        dsp->album_gain = 0;
        dsp->track_peak = 0;
        dsp->album_peak = 0;
        dsp->codec_frequency = dsp->frequency = NATIVE_FREQUENCY;
        dsp->sample_depth = NATIVE_DEPTH;
        dsp->frac_bits = WORD_FRACBITS;
        dsp->new_gain = true;
        break;

    case DSP_DITHER:
        memset(dither_data, 0, sizeof(dither_data));
        dsp->dither_enabled = (bool) value;
        break;

    case DSP_SET_TRACK_GAIN:
        dsp->track_gain = (long) value;
        dsp->new_gain = true;
        break;

    case DSP_SET_ALBUM_GAIN:
        dsp->album_gain = (long) value;
        dsp->new_gain = true;
        break;

    case DSP_SET_TRACK_PEAK:
        dsp->track_peak = (long) value;
        dsp->new_gain = true;
        break;

    case DSP_SET_ALBUM_PEAK:
        dsp->album_peak = (long) value;
        dsp->new_gain = true;
        break;

    default:
        return 0;
    }

    return 1;
}

void dsp_set_crossfeed(bool enable)
{
    if (enable)
        memset(&crossfeed_data, 0, sizeof(crossfeed_data));
    dsp->crossfeed_enabled = enable;
}

void dsp_set_replaygain(bool always)
{
    dsp = &dsp_conf[current_codec];
    
    if (always || dsp->new_gain)
    {
        long gain = 0;

        dsp->new_gain = false;

        if (global_settings.replaygain || global_settings.replaygain_noclip)
        {
            bool track_mode
                = ((global_settings.replaygain_type == REPLAYGAIN_TRACK)
                    || ((global_settings.replaygain_type == REPLAYGAIN_SHUFFLE)
                        && global_settings.playlist_shuffle));
            long peak = (track_mode || !dsp->album_peak)
                ? dsp->track_peak : dsp->album_peak;

            if (global_settings.replaygain)
            {
                gain = (track_mode || !dsp->album_gain)
                    ? dsp->track_gain : dsp->album_gain;

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
                gain = DEFAULT_REPLAYGAIN;
            }

            if (global_settings.replaygain_noclip && (peak != 0)
                && ((((int64_t) gain * peak) >> 24) >= DEFAULT_REPLAYGAIN))
            {
                gain = (((int64_t) DEFAULT_REPLAYGAIN << 24) / peak);
            }

            if (gain == DEFAULT_REPLAYGAIN)
            {
                /* Nothing to do, disable processing. */
                gain = 0;

            }
        }

        /* Store in S8.23 format to simplify calculations. */
        dsp->replaygain = gain >> 1;
    }
}
