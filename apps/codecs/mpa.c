/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "codec.h"

#include <codecs/libmad/mad.h>

#include "playback.h"
#include "mp3data.h"
#include "lib/codeclib.h"

struct mad_stream Stream IDATA_ATTR;
struct mad_frame Frame IDATA_ATTR;
struct mad_synth Synth IDATA_ATTR;
mad_timer_t Timer;
struct dither d0, d1;

/* The following function is used inside libmad - let's hope it's never
   called. 
*/

void abort(void) {
}

/* The "dither" code to convert the 24-bit samples produced by libmad was
   taken from the coolplayer project - coolplayer.sourceforge.net */

struct dither {
    mad_fixed_t error[3];
    mad_fixed_t random;
};

# define SAMPLE_DEPTH	16
# define scale(x, y)	dither((x), (y))

/*
 * NAME:		prng()
 * DESCRIPTION:	32-bit pseudo-random number generator
 */
static __inline
unsigned long prng(unsigned long state)
{
    return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}

/*
 * NAME:        dither()
 * DESCRIPTION:	dither and scale sample
 */
inline int dither(mad_fixed_t sample, struct dither *dither)
{
    unsigned int scalebits;
    mad_fixed_t output, mask, random;

    enum {
        MIN = -MAD_F_ONE,
        MAX =  MAD_F_ONE - 1
     };

    /* noise shape */
    sample += dither->error[0] - dither->error[1] + dither->error[2];

    dither->error[2] = dither->error[1];
    dither->error[1] = dither->error[0]/2;

    /* bias */
    output = sample + (1L << (MAD_F_FRACBITS + 1 - SAMPLE_DEPTH - 1));

    scalebits = MAD_F_FRACBITS + 1 - SAMPLE_DEPTH;
    mask = (1L << scalebits) - 1;

    /* dither */
    random  = prng(dither->random);
    output += (random & mask) - (dither->random & mask);

    //dither->random = random;

    /* clip */
    if (output > MAX) {
        output = MAX;
 
        if (sample > MAX)
            sample = MAX;
    } else if (output < MIN) {
        output = MIN;

        if (sample < MIN)
            sample = MIN;
    }

    /* quantize */
    output &= ~mask;

    /* error feedback */
    dither->error[0] = sample - output;

    /* scale */
    return output >> scalebits;
}

inline int detect_silence(mad_fixed_t sample)
{
    unsigned int scalebits;
    mad_fixed_t output, mask;

    enum {
        MIN = -MAD_F_ONE,
        MAX =  MAD_F_ONE - 1
    };

    /* bias */
    output = sample + (1L << (MAD_F_FRACBITS + 1 - SAMPLE_DEPTH - 1));

    scalebits = MAD_F_FRACBITS + 1 - SAMPLE_DEPTH;
    mask = (1L << scalebits) - 1;

    /* clip */
    if (output > MAX) {
        output = MAX;

        if (sample > MAX)
            sample = MAX;
    } else if (output < MIN) {
        output = MIN;

        if (sample < MIN)
            sample = MIN;
    }

    /* quantize */
    output &= ~mask;

    /* scale */
    output >>= scalebits + 4;
  
    if (output == 0x00 || output == 0xff)
        return 1;
    
    return 0;
}

#define INPUT_CHUNK_SIZE   8192
#define OUTPUT_BUFFER_SIZE 65536 /* Must be an integer multiple of 4. */

unsigned char OutputBuffer[OUTPUT_BUFFER_SIZE];
unsigned char *OutputPtr;
unsigned char *GuardPtr = NULL;
const unsigned char *OutputBufferEnd = OutputBuffer + OUTPUT_BUFFER_SIZE;
long resampled_data[2][5000]; /* enough to cope with 11khz upsampling */

mad_fixed_t mad_frame_overlap[2][32][18] IDATA_ATTR;
unsigned char mad_main_data[MAD_BUFFER_MDLEN] IDATA_ATTR;
/* TODO: what latency does layer 1 have? */
int mpeg_latency[3] = { 0, 481, 529 };
#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif

#undef DEBUG_GAPLESS

struct resampler {
    long last_sample, phase, delta;
};

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
long downsample(long *in, long *out, int num, struct resampler *s)
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

long upsample(long *in, long *out, int num, struct resampler *s)
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

long resample(long *in, long *out, int num, struct resampler *s)
{
    if (s->delta >= (1 << 16))
        return downsample(in, out, num, s);
    else
        return upsample(in, out, num, s);
}

/* this is the codec entry point */
enum codec_status codec_start(struct codec_api* api)
{
    struct codec_api *ci = api;
    struct mp3info *info;
    int Status = 0;
    size_t size;
    int file_end;
    unsigned short Sample;
    char *InputBuffer;
    unsigned int samplecount;
    unsigned int samplesdone;
    bool first_frame;
#ifdef DEBUG_GAPLESS
    bool first = true;
    int fd;
#endif
    int i;
    int yieldcounter = 0;
    int stop_skip, start_skip;
    struct resampler lr = { 0, 0, 0 }, rr = { 0, 0, 0 };
    long length;  
    /* Generic codec inititialisation */

    TEST_CODEC_API(api);

#ifdef USE_IRAM
    ci->memcpy(iramstart, iramcopy, iramend - iramstart);
#endif

    /* This function sets up the buffers and reads the file into RAM */
  
    if (codec_init(api)) {
        return CODEC_ERROR;
    }

    /* Create a decoder instance */

    ci->configure(CODEC_SET_FILEBUF_LIMIT, (int *)(1024*1024*2));
    ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (int *)(1024*16));
  
    ci->memset(&Stream, 0, sizeof(struct mad_stream));
    ci->memset(&Frame, 0, sizeof(struct mad_frame));
    ci->memset(&Synth, 0, sizeof(struct mad_synth));
    ci->memset(&Timer, 0, sizeof(mad_timer_t));
  
    mad_stream_init(&Stream);
    mad_frame_init(&Frame);
    mad_synth_init(&Synth);
    mad_timer_reset(&Timer);

    /* We do this so libmad doesn't try to call codec_calloc() */
    memset(mad_frame_overlap, 0, sizeof(mad_frame_overlap));
    Frame.overlap = &mad_frame_overlap;
    Stream.main_data = &mad_main_data;
    /* This label might need to be moved above all the init code, but I don't
       think reiniting the codec is necessary for MPEG. It might even be unwanted
       for gapless playback */
  next_track:
  
#ifdef DEBUG_GAPLESS
    if (first)
        fd = ci->open("/first.pcm", O_WRONLY | O_CREAT);
    else
        fd = ci->open("/second.pcm", O_WRONLY | O_CREAT);
    first = false;
#endif
  
    info = ci->mp3data;
    first_frame = false;
    file_end = 0;
    OutputPtr = OutputBuffer;
    
    while (!*ci->taginfo_ready)
        ci->yield();
  
    ci->request_buffer(&size, ci->id3->first_frame_offset);
    ci->advance_buffer(size);
 
    if (info->enc_delay >= 0 && info->enc_padding >= 0) {
        stop_skip = info->enc_padding - mpeg_latency[info->layer];
        if (stop_skip < 0) stop_skip = 0;
        start_skip = info->enc_delay + mpeg_latency[info->layer];
    } else {
        stop_skip = 0;
        /* We want to skip this amount anyway */
        start_skip = mpeg_latency[info->layer];
    }
  
    /* NOTE: currently this doesn't work, the below calculated samples_count
       seems to be right, but sometimes libmad just can't supply us with
       all the data we need... */
    if (info->frame_count) {
        /* TODO: 1152 is the frame size in samples for MPEG1 layer 2 and layer 3,
           it's probably not correct at all for MPEG2 and layer 1 */
        samplecount = info->frame_count*1152 - (start_skip + stop_skip);
        samplesdone = ci->id3->elapsed * (ci->id3->frequency / 100) / 10;
    } else {
        samplecount = ci->id3->length * (ci->id3->frequency / 100) / 10;
        samplesdone = ci->id3->elapsed * (ci->id3->frequency / 100) / 10;
    }
    /* rb->snprintf(buf2, sizeof(buf2), "sc: %d", samplecount);
       rb->splash(0, true, buf2);
       rb->snprintf(buf2, sizeof(buf2), "length: %d", ci->id3->length);
       rb->splash(HZ*5, true, buf2);
       rb->snprintf(buf2, sizeof(buf2), "frequency: %d", ci->id3->frequency);
       rb->splash(HZ*5, true, buf2); */
    lr.delta = rr.delta = ci->id3->frequency*65536/44100; 
    /* This is the decoding loop. */
    while (1) {
        ci->yield();
        if (ci->stop_codec || ci->reload_codec) { 
            break ;
        }
    
        if (ci->seek_time) {
            unsigned int sample_loc;
            int newpos;
        
            sample_loc = ci->seek_time/1000 * ci->id3->frequency;
            newpos = ci->mp3_get_filepos(ci->seek_time-1);
            if (ci->seek_buffer(newpos)) {
                if (sample_loc >= samplecount + samplesdone)
                    break ;
                samplecount += samplesdone - sample_loc;
                samplesdone = sample_loc;
            }
            ci->seek_time = 0;
        }

        /* Lock buffers */
        if (Stream.error == 0) {
            InputBuffer = ci->request_buffer(&size, INPUT_CHUNK_SIZE);
            if (size == 0 || InputBuffer == NULL)
                break ;
            mad_stream_buffer(&Stream, InputBuffer, size);
        }
    
        //if ((int)ci->curpos >= ci->id3->first_frame_offset)
        //first_frame = true;
        
        if(mad_frame_decode(&Frame,&Stream))
        {
            if (Stream.error == MAD_FLAG_INCOMPLETE || Stream.error == MAD_ERROR_BUFLEN) {
                // ci->splash(HZ*1, true, "Incomplete");
                /* This makes the codec to support partially corrupted files too. */
                if (file_end == 30)
                    break ;
        
                /* Fill the buffer */
                Stream.error = 0;
                file_end++;
                continue ;
            }
            else if(MAD_RECOVERABLE(Stream.error))
            {
                if(Stream.error!=MAD_ERROR_LOSTSYNC || Stream.this_frame!=GuardPtr)
                {
                    // rb->splash(HZ*1, true, "Recoverable...!");
                }
                continue;
            }
            else if(Stream.error==MAD_ERROR_BUFLEN) {
                //rb->splash(HZ*1, true, "Buflen error");
                break ;
            } else {
                //rb->splash(HZ*1, true, "Unrecoverable error");
                Status=1;
                break;
            }
        }
        if (Stream.next_frame)
            ci->advance_buffer_loc((void *)Stream.next_frame);
        file_end = false;
        /* ?? Do we need the timer module? */
        // mad_timer_add(&Timer,Frame.header.duration);

        mad_synth_frame(&Synth,&Frame);
    
        //if (!first_frame) {
        //samplecount -= Synth.pcm.length;
        //continue ;
        //}
    
        /* Convert MAD's numbers to an array of 16-bit LE signed integers */
        /* We skip start_skip number of samples here, this should only happen for
           very first frame in the stream. */
        /* TODO: possible for start_skip to exceed one frames worth of samples? */
        length = resample((long *)&Synth.pcm.samples[0][start_skip], resampled_data[0], Synth.pcm.length, &lr);
        if (MAD_NCHANNELS(&Frame.header) == 2)
            resample((long *)&Synth.pcm.samples[1][start_skip], resampled_data[1], Synth.pcm.length, &rr);
        for (i = 0; i < length; i++)
        {
            start_skip = 0; /* not very elegant, and might want to keep this value */
            samplesdone++;
            //if (ci->mp3data->padding > 0) {
            //  ci->mp3data->padding--;
            //  continue ;
            //}
            /*if (!first_frame) {
              if (detect_silence(Synth.pcm.samples[0][i]))
              continue ;
              first_frame = true;
              }*/
      
            /* Left channel */
            Sample = scale(resampled_data[0][i], &d0);
            *(OutputPtr++) = Sample >> 8;
            *(OutputPtr++) = Sample & 0xff;

            /* Right channel. If the decoded stream is monophonic then
             * the right output channel is the same as the left one.
             */
            if (MAD_NCHANNELS(&Frame.header) == 2)
                Sample = scale(resampled_data[1][i], &d1);
            *(OutputPtr++) = Sample >> 8;
            *(OutputPtr++) = Sample & 0xff;
      
            samplecount--;
            if (samplecount == 0) {
#ifdef DEBUG_GAPLESS
                ci->write(fd, OutputBuffer, (int)OutputPtr - (int)OutputBuffer);
#endif
                while (!ci->audiobuffer_insert(OutputBuffer, (int)OutputPtr - (int)OutputBuffer))
                    ci->yield();
                goto song_end;
            }
  
            if (yieldcounter++ == 200) {
                ci->yield();
                yieldcounter = 0;
            }
      
            /* Flush the buffer if it is full. */
            if (OutputPtr == OutputBufferEnd)
            {
#ifdef DEBUG_GAPLESS
                ci->write(fd, OutputBuffer, OUTPUT_BUFFER_SIZE);
#endif
                while (!ci->audiobuffer_insert(OutputBuffer, OUTPUT_BUFFER_SIZE))
                    ci->yield();
                OutputPtr = OutputBuffer;
            }
        }
        ci->set_elapsed(samplesdone / (ci->id3->frequency/1000));
    }
  
  song_end:
#ifdef DEBUG_GAPLESS
    ci->close(fd);
#endif
    Stream.error = 0;
  
    if (ci->request_next_track())
        goto next_track;
    return CODEC_OK;
}
