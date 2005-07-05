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
#include <string.h>
#include "kernel.h"
#include "logf.h"

#include "dsp.h"
#include "playback.h"
#include "system.h"

/* The "dither" code to convert the 24-bit samples produced by libmad was
   taken from the coolplayer project - coolplayer.sourceforge.net */
struct s_dither {
    int error[3];
    int random;
};

static struct s_dither dither[2];
struct dsp_configuration dsp_config;
static int channel;
static int fracbits;

#define SAMPLE_DEPTH     16

/*
 * NAME:        prng()
 * DESCRIPTION: 32-bit pseudo-random number generator
 */
static __inline
unsigned long prng(unsigned long state)
{
    return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}

inline long dsp_noiseshape(long sample)
{
    sample += dither[channel].error[0] - dither[channel].error[1]
            + dither[channel].error[2];
    dither[channel].error[2] = dither[channel].error[1];
    dither[channel].error[1] = dither[channel].error[0]/2;
    
    return sample;
}

inline long dsp_bias(long sample)
{
    sample = sample + (1L << (fracbits - SAMPLE_DEPTH));
    
    return sample;
}

inline long dsp_dither(long *mask)
{
    long random, output;
    
    random  = prng(dither[channel].random);
    output = (random & *mask) - (dither[channel].random & *mask);
    dither[channel].random = random;
    
    return output;
}

inline void dsp_clip(long *sample, long *output)
{
    if (*output > dsp_config.clip_max) {
        *output = dsp_config.clip_max;
 
        if (*sample > dsp_config.clip_max)
            *sample = dsp_config.clip_max;
    } else if (*output < dsp_config.clip_min) {
        *output = dsp_config.clip_min;

        if (*sample < dsp_config.clip_min)
            *sample = dsp_config.clip_min;
    }
}

/*
 * NAME:        dither()
 * DESCRIPTION: dither and scale sample
 */
inline int scale_dither_clip(long sample)
{
    unsigned int scalebits;
    long output, mask;

    /* noise shape */
    sample = dsp_noiseshape(sample);

    /* bias */
    output = dsp_bias(sample);
    
    scalebits = fracbits + 1 - SAMPLE_DEPTH;
    mask = (1L << scalebits) - 1;
    
    /* dither */
    output += dsp_dither(&mask);
    
    /* clip */
    dsp_clip(&sample, &output);
    
    /* quantize */
    output &= ~mask;

    /* error feedback */
    dither->error[0] = sample - output;

    /* scale */
    return output >> scalebits;
}

inline int scale_clip(long sample)
{
    unsigned int scalebits;
    long output, mask;
    
    output = sample;
    scalebits = fracbits + 1 - SAMPLE_DEPTH;
    mask = (1L << scalebits) - 1;
    
    dsp_clip(&sample, &output);
    output &= ~mask;

    return output >> scalebits;
}

void dsp_scale_dither_clip(short *dest, long *src, int samplecount)
{
    dest += channel;
    while (samplecount-- > 0) {
        *dest = scale_dither_clip(*src);
        src++;
        dest += 2;
    }
}

void dsp_scale_clip(short *dest, long *src, int samplecount)
{
    dest += channel;
    while (samplecount-- > 0) {
        *dest = scale_clip(*src);
        src++;
        dest += 2;
    }
}

struct resampler {
    long last_sample, phase, delta;
};

static struct resampler resample[2];

#if CONFIG_CPU==MCF5249 && !defined(SIMULATOR)

#define INIT() asm volatile ("move.l #0xb0, %macsr") /* frac, round, clip */
#define FRACMUL(x, y) \
({ \
    long t; \
    asm volatile ("mac.l %[a], %[b], %%acc0\n\t" \
                  "movclr.l %%acc0, %[t]\n\t" \
                  : [t] "=r" (t) : [a] "r" (x), [b] "r" (y)); \
    t; \
})

#else

#define INIT()
#define FRACMUL(x, y) (long)(((long long)(x)*(long long)(y)) << 1)
#endif

/* linear resampling, introduces one sample delay, because of our inability to
   look into the future at the end of a frame */
long downsample(long *out, long *in, int num, struct resampler *s)
{
    long i = 1, pos;
    long last = s->last_sample;

    INIT();
    pos = s->phase >> 16;
    /* check if we need last sample of previous frame for interpolation */
    if (pos > 0)
        last = in[pos - 1];
    out[0] = last + FRACMUL((s->phase & 0xffff) << 15, in[pos] - last);
    s->phase += s->delta;
    while ((pos = s->phase >> 16) < num) {
        out[i++] = in[pos - 1] + FRACMUL((s->phase & 0xffff) << 15, in[pos] - in[pos - 1]);
        s->phase += s->delta;
    }
    /* wrap phase accumulator back to start of next frame */
    s->phase -= num << 16;
    s->last_sample = in[num - 1];
    return i;
}

long upsample(long *out, long *in, int num, struct resampler *s)
{
    long i = 0, pos;

    INIT();
    while ((pos = s->phase >> 16) == 0) {
        out[i++] = s->last_sample + FRACMUL((s->phase & 0xffff) << 15, in[pos] - s->last_sample);
        s->phase += s->delta;
    }
    while ((pos = s->phase >> 16) < num) {
        out[i++] = in[pos - 1] + FRACMUL((s->phase & 0xffff) << 15, in[pos] - in[pos - 1]);
        s->phase += s->delta;
    }
    /* wrap phase accumulator back to start of next frame */
    s->phase -= num << 16;
    s->last_sample = in[num - 1];
    return i;
}

#define MAX_CHUNK_SIZE 1024
static char samplebuf[MAX_CHUNK_SIZE];
/* enough to cope with 11khz upsampling */
long resampled[MAX_CHUNK_SIZE * 4];

int process(short *dest, long *src, int samplecount)
{
    long *p;
    int length = samplecount;

    p = resampled;
    
    /* Resample as necessary */
    if (dsp_config.frequency > NATIVE_FREQUENCY)
        length = upsample(resampled, src, samplecount, &resample[channel]);
    else if (dsp_config.frequency < NATIVE_FREQUENCY)
        length = downsample(resampled, src, samplecount, &resample[channel]);
    else
        p = src;
    
    /* Scale & dither */
    if (dsp_config.dither_enabled) {
        dsp_scale_dither_clip(dest, p, length);
    } else {
        dsp_scale_clip(dest, p, length);
    }
    
    return length;
}

void convert_stereo_mode(long *dest, long *src, int samplecount)
{
    int i;
    
    samplecount /= 2;
    
    for (i = 0; i < samplecount; i++) {
        dest[i] = src[i*2 + 0];
        dest[i+samplecount] = src[i*2 + 1];
    }
}

void scale_up(long *dest, short *src, int samplecount)
{
    int i;
    
    for (i = 0; i < samplecount; i++)
        dest[i] = (long)(src[i] << 8);
}

void scale_up_convert_stereo_mode(long *dest, short *src, int samplecount)
{
    int i;
    
    samplecount /= 2;
    
    for (i = 0; i < samplecount; i++) {
        dest[i] = (long)(src[i*2+0] << SAMPLE_DEPTH);
        dest[i+samplecount] = (long)(src[i*2+1] << SAMPLE_DEPTH);
    }
}

int dsp_process(char *dest, char *src, int samplecount)
{
    int copy_n, rc;
    char *p;
    int processed_bytes = 0;
    
    fracbits = dsp_config.sample_depth;
    
    while (samplecount > 0) {
        yield();
        copy_n = MIN(MAX_CHUNK_SIZE / 4, samplecount);
        
        p = src;
        /* Scale up to 32-bit samples. */
        if (dsp_config.sample_depth <= SAMPLE_DEPTH) {
            if (dsp_config.stereo_mode == STEREO_INTERLEAVED) {
                scale_up_convert_stereo_mode((long *)samplebuf, 
                                             (short *)p, copy_n);
            } else {
                scale_up((long *)samplebuf, (short *)p, copy_n);
            }
            p = samplebuf;
            fracbits = 31;
        }
        
        /* Convert to non-interleaved stereo. */
        else if (dsp_config.stereo_mode == STEREO_INTERLEAVED) {
            convert_stereo_mode((long *)samplebuf, (long *)p, copy_n / 2);
            p = samplebuf;
        } 
        
        /* Apply DSP functions. */
        if (dsp_config.stereo_mode == STEREO_INTERLEAVED) {
            channel = 0;
            rc = process((short *)dest, (long *)p, copy_n / 2) * 4;
            p += copy_n * 2;
            channel = 1;
            process((short *)dest, (long *)p, copy_n / 2);
            dest += rc;
        } else if (dsp_config.stereo_mode == STEREO_MONO) {
            channel = 0;
            rc = process((short *)dest, (long *)p, copy_n) * 4;
            channel = 1;
            process((short *)dest, (long *)p, copy_n);
            dest += rc;
        } else {
            rc = process((short *)dest, (long *)p, copy_n) * 2;
            dest += rc * 2;
        }
        
        samplecount -= copy_n;
        if (dsp_config.sample_depth <= SAMPLE_DEPTH)
            src += copy_n * 2;
        else
            src += copy_n * 4;
            
        processed_bytes += rc;
    }
    
    /* Set stereo channel */
    channel = channel ? 0 : 1;
    
    return processed_bytes;
}

bool dsp_configure(int setting, void *value)
{
    switch (setting) {
    case DSP_SET_FREQUENCY:
        memset(resample, 0, sizeof(resample));
        dsp_config.frequency = (int)value;
        resample[0].delta = resample[1].delta = 
            (unsigned long)value*65536/NATIVE_FREQUENCY;
        break ;
        
    case DSP_SET_CLIP_MIN:
        dsp_config.clip_min = (long)value;
        break ;
        
    case DSP_SET_CLIP_MAX:
        dsp_config.clip_max = (long)value;
        break ;
        
    case DSP_SET_SAMPLE_DEPTH:
        dsp_config.sample_depth = (long)value;
        break ;
    
    case DSP_SET_STEREO_MODE:
        dsp_config.stereo_mode = (long)value;
        channel = 0;
        break ;
    
    case DSP_RESET:
        dsp_config.dither_enabled = false;
        dsp_config.clip_max = 0x7fffffff;
        dsp_config.clip_min = 0x80000000;
        dsp_config.frequency = NATIVE_FREQUENCY;
        channel = 0;
        break ;
        
    case DSP_DITHER:
        dsp_config.dither_enabled = (bool)value;
        break ;
    
    default:
        return 0;
    }
    
    return 1;
}


