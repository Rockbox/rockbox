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
#include "dsp.h"
#include "mp3data.h"
#include "lib/codeclib.h"

struct mad_stream Stream IDATA_ATTR;
struct mad_frame Frame IDATA_ATTR;
struct mad_synth Synth IDATA_ATTR;
mad_timer_t Timer;

/* The following function is used inside libmad - let's hope it's never
   called. 
*/

void abort(void) {
}


#define INPUT_CHUNK_SIZE   8192
#define OUTPUT_BUFFER_SIZE 65536 /* Must be an integer multiple of 4. */

unsigned char OutputBuffer[OUTPUT_BUFFER_SIZE];
unsigned char *OutputPtr;
unsigned char *GuardPtr = NULL;
const unsigned char *OutputBufferEnd = OutputBuffer + OUTPUT_BUFFER_SIZE;

mad_fixed_t mad_frame_overlap[2][32][18] IDATA_ATTR;
unsigned char mad_main_data[MAD_BUFFER_MDLEN] IDATA_ATTR;
/* TODO: what latency does layer 1 have? */
int mpeg_latency[3] = { 0, 481, 529 };
#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif

/* this is the codec entry point */
enum codec_status codec_start(struct codec_api* api)
{
    struct codec_api *ci = api;
    struct mp3info *info;
    int Status = 0;
    size_t size;
    int file_end;
    char *InputBuffer;
    unsigned int samplecount;
    unsigned int samplesdone;
    bool first_frame;
    int stop_skip, start_skip;
    int current_stereo_mode = -1;
    int frequency_divider;
    
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
    ci->configure(DSP_SET_CLIP_MIN, (int *)-MAD_F_ONE);
    ci->configure(DSP_SET_CLIP_MAX, (int *)(MAD_F_ONE - 1));
    ci->configure(DSP_SET_SAMPLE_DEPTH, (int *)(MAD_F_FRACBITS));
    ci->configure(DSP_DITHER, (bool *)false);
    ci->configure(CODEC_DSP_ENABLE, (bool *)true);
    
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
  
    info = ci->mp3data;
    first_frame = false;
    file_end = 0;
    OutputPtr = OutputBuffer;
    
    while (!*ci->taginfo_ready)
        ci->yield();
  
    frequency_divider = ci->id3->frequency / 100;
    ci->configure(DSP_SET_FREQUENCY, (int *)ci->id3->frequency);
    
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
        samplesdone = ci->id3->elapsed * frequency_divider / 10;
    } else {
        samplecount = ci->id3->length * frequency_divider / 10;
        samplesdone = ci->id3->elapsed * frequency_divider / 10;
    }
    
    /* This is the decoding loop. */
    while (1) {
        ci->yield();
        if (ci->stop_codec || ci->reload_codec) {
            break ;
        }
    
        if (ci->seek_time) {
            unsigned int sample_loc;
            int newpos;
        
            sample_loc = ci->seek_time * frequency_divider / 10;
            newpos = ci->mp3_get_filepos(ci->seek_time-1);
            if (sample_loc >= samplecount + samplesdone)
                break ;
            
            if (ci->seek_buffer(newpos)) {
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
    
        /* Convert MAD's numbers to an array of 16-bit LE signed integers */
        /* We skip start_skip number of samples here, this should only happen for
           very first frame in the stream. */
        /* TODO: possible for start_skip to exceed one frames worth of samples? */
        
        if (MAD_NCHANNELS(&Frame.header) == 2) {
            if (current_stereo_mode != STEREO_NONINTERLEAVED) {
                ci->configure(DSP_SET_STEREO_MODE, (int *)STEREO_NONINTERLEAVED);
                current_stereo_mode = STEREO_NONINTERLEAVED;
            }
            ci->audiobuffer_insert_split(&Synth.pcm.samples[0][start_skip],
                                         &Synth.pcm.samples[1][start_skip],
                                         (Synth.pcm.length - start_skip) * 4);
        } else {
            if (current_stereo_mode != STEREO_MONO) {
                ci->configure(DSP_SET_STEREO_MODE, (int *)STEREO_MONO);
                current_stereo_mode = STEREO_MONO;
            }
            ci->audiobuffer_insert((char *)&Synth.pcm.samples[0][start_skip],
                                   (Synth.pcm.length - start_skip) * 4);
        }
        start_skip = 0; /* not very elegant, and might want to keep this value */
        
        samplesdone += Synth.pcm.length;
        samplecount -= Synth.pcm.length;
        ci->set_elapsed(samplesdone / (frequency_divider / 10));
    }
  
    Stream.error = 0;
  
    if (ci->request_next_track())
        goto next_track;
    return CODEC_OK;
}
