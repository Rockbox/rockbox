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
#include "dsp.h"
#include "kernel.h"
#include "playback.h"
#include "system.h"
#include "settings.h"
#include "replaygain.h"
#include "debug.h"

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
#define HIGH_NEG 0x9999999AL /* -0.2 */
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
#define FRACMUL_8_LOOP(x, y, s) \
({ \
    long t; \
    long u; \
    asm volatile ("mac.l    %[a], %[b], (%[c])+, %[a], %%acc0\n\t" \
                  "move.l   %%accext01, %[u]\n\t" \
                  "movclr.l %%acc0, %[t]\n\t" \
                  : [a] "+r" (x), [c] "+a" (s), [t] "=r" (t), [u] "=r" (u) \
                  : [b] "r" (y)); \
    (t << 8) | (u & 0xff); \
})

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

#else

#define ACC_INIT(acc, x, y) acc = FRACMUL(x, y)
#define ACC(acc, x, y) acc += FRACMUL(x, y)
#define GET_ACC(acc) acc
#define FRACMUL(x, y) (long) (((((long long) (x)) * ((long long) (y))) >> 31))
#define FRACMUL_8(x, y) (long) (((((long long) (x)) * ((long long) (y))) >> 23))
#define FRACMUL_8_LOOP(x, y, s) \
({ \
    long t = x; \
    x = *(s)++; \
    (long) (((((long long) (t)) * ((long long) (y))) >> 23)); \
})

#endif

struct dsp_config
{
    long frequency;
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
};

struct resample_data
{
    long last_sample;
    long phase;
    long delta;
};

struct dither_data
{
    long error[3];
    long random;
};

struct crossfeed_data
{
    long lowpass[2];
    long highpass[2];
    long delay[2][13];
    int index;
};

static struct dsp_config dsp_conf[2] IBSS_ATTR;
static struct dither_data dither_data[2] IBSS_ATTR;
static struct resample_data resample_data[2][2] IBSS_ATTR;
static struct crossfeed_data crossfeed_data IBSS_ATTR;

extern int current_codec;
struct dsp_config *dsp;

/* The internal format is 32-bit samples, non-interleaved, stereo. This
 * format is similar to the raw output from several codecs, so the amount
 * of copying needed is minimized for that case.
 */

static long sample_buf[SAMPLE_BUF_SIZE] IBSS_ATTR;
static long resample_buf[RESAMPLE_BUF_SIZE] IBSS_ATTR;

/* Convert at most count samples to the internal format, if needed. Returns
 * number of samples ready for further processing. Updates src to point
 * past the samples "consumed" and dst is set to point to the samples to
 * consume. Note that for mono, dst[0] equals dst[1], as there is no point
 * in processing the same data twice.
 */
static int convert_to_internal(char* src[], int count, long* dst[])
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
        dst[0] = (long*) src[0];
        dst[1] = (long*) ((dsp->stereo_mode == STEREO_MONO) ? src[0] : src[1]);
    }

    if (dsp->sample_depth <= NATIVE_DEPTH)
    {
        short* s0 = (short*) src[0];
        long* d0 = dst[0];
        long* d1 = dst[1];
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
        long* s0 = (long*) src[0];
        long* d0 = dst[0];
        long* d1 = dst[1];
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

/* Linear resampling that introduces a one sample delay, because of our
 * inability to look into the future at the end of a frame.
 */

static long downsample(long *dst, long *src, int count,
    struct resample_data *r)
{
    long phase = r->phase;
    long delta = r->delta;
    long last_sample = r->last_sample;
    int pos = phase >> 16;
    int i = 1;

    /* Do we need last sample of previous frame for interpolation? */
    if (pos > 0)
    {
        last_sample = src[pos - 1];
    }

    *dst++ = last_sample + FRACMUL((phase & 0xffff) << 15,
        src[pos] - last_sample);
    phase += delta;

    while ((pos = phase >> 16) < count)
    {
        *dst++ = src[pos - 1] + FRACMUL((phase & 0xffff) << 15,
            src[pos] - src[pos - 1]);
        phase += delta;
        i++;
    }

    /* Wrap phase accumulator back to start of next frame. */
    r->phase = phase - (count << 16);
    r->delta = delta;
    r->last_sample = src[count - 1];
    return i;
}

static long upsample(long *dst, long *src, int count, struct resample_data *r)
{
    long phase = r->phase;
    long delta = r->delta;
    long last_sample = r->last_sample;
    int i = 0;
    int pos;

    while ((pos = phase >> 16) == 0)
    {
        *dst++ = last_sample + FRACMUL((phase & 0xffff) << 15,
            src[pos] - last_sample);
        phase += delta;
        i++;
    }

    while ((pos = phase >> 16) < count)
    {
        *dst++ = src[pos - 1] + FRACMUL((phase & 0xffff) << 15,
            src[pos] - src[pos - 1]);
        phase += delta;
        i++;
    }

    /* Wrap phase accumulator back to start of next frame. */
    r->phase = phase - (count << 16);
    r->delta = delta;
    r->last_sample = src[count - 1];
    return i;
}

/* Resample count stereo samples. Updates the src array, if resampling is
 * done, to refer to the resampled data. Returns number of stereo samples
 * for further processing.
 */
static inline int resample(long* src[], int count)
{
    long new_count;

    if (dsp->frequency != NATIVE_FREQUENCY)
    {
        long* d0 = &resample_buf[0];
        /* Only process the second channel if needed. */
        long* d1 = (src[0] == src[1]) ? d0
            : &resample_buf[RESAMPLE_BUF_SIZE / 2];

        if (dsp->frequency < NATIVE_FREQUENCY)
        {
            new_count = upsample(d0, src[0], count,
                            &resample_data[current_codec][0]);

            if (d0 != d1)
            {
                upsample(d1, src[1], count,
                    &resample_data[current_codec][1]);
            }
        }
        else
        {
            new_count = downsample(d0, src[0], count,
                            &resample_data[current_codec][0]);

            if (d0 != d1)
            {
                downsample(d1, src[1], count,
                    &resample_data[current_codec][1]);
            }
        }

        src[0] = d0;
        src[1] = d1;
    }
    else
    {
        new_count = count;
    }

    return new_count;
}

static inline long clip_sample(long sample, long min, long max)
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

static long dither_sample(long sample, long bias, long mask,
    struct dither_data* dither)
{
    long output;
    long random;
    long min;
    long max;

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

/* Apply a constant gain to the samples (e.g., for ReplayGain). May update
 * the src array if gain was applied.
 * Note that this must be called before the resampler.
 */
static void apply_crossfeed(long* src[], int count)
{

    if (dsp->crossfeed_enabled && src[0] != src[1])
    {
        long a; /* accumulator */

        long low_left = crossfeed_data.lowpass[0];
        long low_right = crossfeed_data.lowpass[1];
        long high_left = crossfeed_data.highpass[0];
        long high_right = crossfeed_data.highpass[1];
        unsigned int index = crossfeed_data.index;

        long left, right;

        long * delay_l = crossfeed_data.delay[0];
        long * delay_r = crossfeed_data.delay[1];

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
}

/* Apply a constant gain to the samples (e.g., for ReplayGain). May update
 * the src array if gain was applied.
 * Note that this must be called before the resampler.
 */
static void apply_gain(long* src[], int count)
{
    if (dsp->replaygain)
    {
        long* s0 = src[0];
        long* s1 = src[1];
        long* d0 = &sample_buf[0];
        long* d1 = (s0 == s1) ? d0 : &sample_buf[SAMPLE_BUF_SIZE / 2];
        long gain = dsp->replaygain;
        long s;
        long i;

        src[0] = d0;
        src[1] = d1;
        s = *s0++;

        for (i = 0; i < count; i++)
        {
            *d0++ = FRACMUL_8_LOOP(s, gain, s0);
        }

        if (src [0] != src [1])
        {
            s = *s1++;

            for (i = 0; i < count; i++)
            {
                *d1++ = FRACMUL_8_LOOP(s, gain, s1);
            }
        }
    }
}

static void write_samples(short* dst, long* src[], int count)
{
    long* s0 = src[0];
    long* s1 = src[1];
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
long dsp_process(char* dst, char* src[], long size)
{
    long* tmp[2];
    long written = 0;
    long factor;
    int samples;

    #if defined(CPU_COLDFIRE) && !defined(SIMULATOR)
    /* set emac unit for dsp processing, and save old macsr, we're running in
       codec thread context at this point, so can't clobber it */
    unsigned long old_macsr = coldfire_get_macsr();
    coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_ROUND | EMAC_SATURATE);
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
        apply_crossfeed(tmp, samples);
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
            resample_data[current_codec][0].delta) >> 16;
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
        memset(&resample_data[current_codec][0], 0,
            sizeof(struct resample_data) * 2);
        /* Fall through!!! */
    case DSP_SWITCH_FREQUENCY:
        dsp->frequency = ((int) value == 0) ? NATIVE_FREQUENCY : (int) value;
        resample_data[current_codec][0].delta =
            resample_data[current_codec][1].delta =
            (unsigned long) dsp->frequency * 65536 / NATIVE_FREQUENCY;
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
            dsp->sample_bytes = sizeof(long);
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
        dsp->frequency = NATIVE_FREQUENCY;
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
