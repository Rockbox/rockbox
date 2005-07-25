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

#if defined(CPU_COLDFIRE) && !defined(SIMULATOR)

#define INIT() asm volatile ("move.l #0xb0, %macsr") /* frac, round, clip */
/* Multiply 2 32-bit integers and return the 32 most significant bits of the
 * result.
 */
#define FRACMUL(x, y) \
({ \
    long t; \
    asm volatile ("mac.l    %[a], %[b], %%acc0\n\t" \
                  "movclr.l %%acc0, %[t]\n\t" \
                  : [t] "=r" (t) : [a] "r" (x), [b] "r" (y)); \
    t; \
})
/* Multiply 2 32-bit integers and of the 40 most significat bits of the 
 * result, return the 32 least significant bits. I.e., like FRACMUL with one
 * of the arguments shifted 8 bits to the right.
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

#else

#define INIT()
#define FRACMUL(x, y) (long) (((((long long) (x)) * ((long long) (y))) >> 32))
#define FRACMUL_8(x, y) (long) (((((long long) (x)) * ((long long) (y))) >> 24))

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
    long replaygain;
    int sample_depth;
    int sample_bytes;
    int stereo_mode;
    int frac_bits;
    bool dither_enabled;
    bool new_gain;
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

static struct dsp_config dsp IDATA_ATTR;
static struct dither_data dither_data[2] IDATA_ATTR;
static struct resample_data resample_data[2] IDATA_ATTR;

/* The internal format is 32-bit samples, non-interleaved, stereo. This
 * format is similar to the raw output from several codecs, so the amount
 * of copying needed is minimized for that case.
 */

static long sample_buf[SAMPLE_BUF_SIZE] IDATA_ATTR;
static long resample_buf[RESAMPLE_BUF_SIZE] IDATA_ATTR;


/* Convert at most count samples to the internal format, if needed. Returns
 * number of samples ready for further processing. Updates src to point
 * past the samples "consumed" and dst is set to point to the samples to
 * consume. Note that for mono, dst[0] equals dst[1], as there is no point
 * in processing the same data twice.
 */
static int convert_to_internal(char* src[], int count, long* dst[])
{
    count = MIN(SAMPLE_BUF_SIZE / 2, count);

    if ((dsp.sample_depth <= NATIVE_DEPTH)
        || (dsp.stereo_mode == STEREO_INTERLEAVED))
    {
        dst[0] = &sample_buf[0];
        dst[1] = (dsp.stereo_mode == STEREO_MONO)
            ? dst[0] : &sample_buf[SAMPLE_BUF_SIZE / 2];
    }
    else
    {
        dst[0] = (long*) src[0];
        dst[1] = (long*) ((dsp.stereo_mode == STEREO_MONO) ? src[0] : src[1]);
    }

    if (dsp.sample_depth <= NATIVE_DEPTH)
    {
        short* s0 = (short*) src[0];
        long* d0 = dst[0];
        long* d1 = dst[1];
        int scale = WORD_SHIFT;
        int i;

        if (dsp.stereo_mode == STEREO_INTERLEAVED)
        {
            for (i = 0; i < count; i++)
            {
                *d0++ = *s0++ << scale;
                *d1++ = *s0++ << scale;
            }
        }
        else if (dsp.stereo_mode == STEREO_NONINTERLEAVED)
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
    else if (dsp.stereo_mode == STEREO_INTERLEAVED)
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

    if (dsp.stereo_mode == STEREO_NONINTERLEAVED)
    {
        src[0] += count * dsp.sample_bytes;
        src[1] += count * dsp.sample_bytes;
    }
    else if (dsp.stereo_mode == STEREO_INTERLEAVED)
    {
        src[0] += count * dsp.sample_bytes * 2;
    }
    else
    {
        src[0] += count * dsp.sample_bytes;
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

    if (dsp.frequency != NATIVE_FREQUENCY)
    {
        long* d0 = &resample_buf[0];
        /* Only process the second channel if needed. */
        long* d1 = (src[0] == src[1]) ? d0
            : &resample_buf[RESAMPLE_BUF_SIZE / 2];

        if (dsp.frequency < NATIVE_FREQUENCY)
        {
            new_count = upsample(d0, src[0], count, &resample_data[0]);

            if (d0 != d1)
            {
                upsample(d1, src[1], count, &resample_data[1]);
            }
        }
        else
        {
            new_count = downsample(d0, src[0], count, &resample_data[0]);

            if (d0 != d1)
            {
                downsample(d1, src[1], count, &resample_data[1]);
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

static inline long clip_sample(long sample)
{
    if (sample > dsp.clip_max)
    {
        sample = dsp.clip_max;
    }
    else if (sample < dsp.clip_min)
    {
        sample = dsp.clip_min;
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

    sample = clip_sample(sample);
    output = clip_sample(output) & ~mask;

    /* Error feedback */

    dither->error[0] = sample - output;

    return output;
}

/* Apply a constant gain to the samples (e.g., for ReplayGain). May update
 * the src array if gain was applied.
 * Note that this must be called before the resampler.
 */
static void apply_gain(long* src[], int count)
{
    if (dsp.replaygain)
    {
        long* s0 = src[0];
        long* s1 = src[1];
        long* d0 = &sample_buf[0];
        long* d1 = (s0 == s1) ? d0 : &sample_buf[SAMPLE_BUF_SIZE / 2];
        long gain = dsp.replaygain;
        long i;
        
        src[0] = d0;
        src[1] = d1;
        
        for (i = 0; i < count; i++)
        {
            *d0++ = FRACMUL_8(*s0++, gain);
        }
        
        if (src [0] != src [1])
        {
            for (i = 0; i < count; i++)
            {
                *d1++ = FRACMUL_8(*s1++, gain);
            }
        }
    }
}

static void write_samples(short* dst, long* src[], int count)
{
    long* s0 = src[0];
    long* s1 = src[1];
    int scale = dsp.frac_bits + 1 - NATIVE_DEPTH;

    if (dsp.dither_enabled)
    {
        long bias = (1L << (dsp.frac_bits - NATIVE_DEPTH));
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
        while (count-- > 0)
        {
            *dst++ = (short) (clip_sample(*s0++) >> scale);
            *dst++ = (short) (clip_sample(*s1++) >> scale);
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
    long factor = (dsp.stereo_mode != STEREO_MONO) ? 2 : 1;
    int samples;

    size /= dsp.sample_bytes * factor;
    INIT();
    dsp_set_replaygain(false);

    while (size > 0)
    {
        samples = convert_to_internal(src, size, tmp);
        size -= samples;
        apply_gain(tmp, samples);
        samples = resample(tmp, samples);
        write_samples((short*) dst, tmp, samples);
        written += samples;
        dst += samples * sizeof(short) * 2;
        yield();
    }

    return written * sizeof(short) * 2;
}

/* Given size bytes of input data, calculate the maximum number of bytes of
 * output data that would be generated (the calculation is not entirely
 * exact and rounds upwards to be on the safe side; during resampling,
 * the number of samples generated depends on the current state of the
 * resampler).
 */
long dsp_output_size(long size)
{
    if (dsp.stereo_mode == STEREO_MONO)
    {
        size *= 2;
    }

    if (dsp.sample_depth > NATIVE_DEPTH)
    {
        size /= 2;
    }

    if (dsp.frequency != NATIVE_FREQUENCY)
    {
        size = (long) ((((unsigned long) size * NATIVE_FREQUENCY)
            + (dsp.frequency - 1)) / dsp.frequency);
    }

    return (size + 3) & ~3;
}

/* Given size bytes of output buffer, calculate number of bytes of input
 * data that would be consumed in order to fill the output buffer.
 */
long dsp_input_size(long size)
{
    if (dsp.stereo_mode == STEREO_MONO)
    {
        size /= 2;
    }

    if (dsp.sample_depth > NATIVE_DEPTH)
    {
        size *= 2;
    }

    if (dsp.frequency != NATIVE_FREQUENCY)
    {
        size = (long) ((((unsigned long) size * dsp.frequency)
            + (NATIVE_FREQUENCY - 1)) / NATIVE_FREQUENCY);
    }

    return size;
}

int dsp_stereo_mode(void)
{
    return dsp.stereo_mode;
}

bool dsp_configure(int setting, void *value)
{
    switch (setting)
    {
    case DSP_SET_FREQUENCY:
        dsp.frequency = ((int) value == 0) ? NATIVE_FREQUENCY : (int) value;
        memset(resample_data, 0, sizeof(resample_data));
        resample_data[0].delta = resample_data[1].delta =
            (unsigned long) dsp.frequency * 65536 / NATIVE_FREQUENCY;
        break;

    case DSP_SET_CLIP_MIN:
        dsp.clip_min = (long) value;
        break;

    case DSP_SET_CLIP_MAX:
        dsp.clip_max = (long) value;
        break;

    case DSP_SET_SAMPLE_DEPTH:
        dsp.sample_depth = (long) value;

        if (dsp.sample_depth <= NATIVE_DEPTH)
        {
            dsp.frac_bits = WORD_FRACBITS;
            dsp.sample_bytes = sizeof(short);
            dsp.clip_max =  ((1 << WORD_FRACBITS) - 1);
            dsp.clip_min = -((1 << WORD_FRACBITS));
        }
        else
        {
            dsp.frac_bits = (long) value;
            dsp.sample_bytes = sizeof(long);
        }

        break;

    case DSP_SET_STEREO_MODE:
        dsp.stereo_mode = (long) value;
        break;

    case DSP_RESET:
        dsp.dither_enabled = false;
        dsp.stereo_mode = STEREO_NONINTERLEAVED;
        dsp.clip_max =  ((1 << WORD_FRACBITS) - 1);
        dsp.clip_min = -((1 << WORD_FRACBITS));
        dsp.track_gain = 0;
        dsp.album_gain = 0;
        dsp.track_peak = 0;
        dsp.album_peak = 0;
        dsp.frequency = NATIVE_FREQUENCY;
        dsp.sample_depth = NATIVE_DEPTH;
        dsp.frac_bits = WORD_FRACBITS;
        dsp.new_gain = true;
        break;

    case DSP_DITHER:
        memset(dither_data, 0, sizeof(dither_data));
        dsp.dither_enabled = (bool) value;
        break;

    case DSP_SET_TRACK_GAIN:
        dsp.track_gain = (long) value;
        dsp.new_gain = true;
        break;

    case DSP_SET_ALBUM_GAIN:
        dsp.album_gain = (long) value;
        dsp.new_gain = true;
        break;

    case DSP_SET_TRACK_PEAK:
        dsp.track_peak = (long) value;
        dsp.new_gain = true;
        break;

    case DSP_SET_ALBUM_PEAK:
        dsp.album_peak = (long) value;
        dsp.new_gain = true;
        break;

    default:
        return 0;
    }

    return 1;
}

void dsp_set_replaygain(bool always)
{
    if (always || dsp.new_gain)
    {
        long gain = 0;

        dsp.new_gain = false;
    
        if (global_settings.replaygain || global_settings.replaygain_noclip)
        {
            long peak;

            if (global_settings.replaygain)
            {
                gain = (global_settings.replaygain_track || !dsp.album_gain)
                    ? dsp.track_gain : dsp.album_gain;
            }
            
            peak = (global_settings.replaygain_track || !dsp.album_peak)
                ? dsp.track_peak : dsp.album_peak;
            
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

        dsp.replaygain = gain;
    }
}
