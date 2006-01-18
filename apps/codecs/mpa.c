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

#include "codeclib.h"
#include <codecs/libmad/mad.h>
#include <inttypes.h>

CODEC_HEADER

struct mad_stream stream IBSS_ATTR;
struct mad_frame frame IBSS_ATTR;
struct mad_synth synth IBSS_ATTR;

/* The following function is used inside libmad - let's hope it's never
   called. 
*/

void abort(void) {
}

#define INPUT_CHUNK_SIZE   8192

mad_fixed_t mad_frame_overlap[2][32][18] IBSS_ATTR;
unsigned char mad_main_data[MAD_BUFFER_MDLEN] IBSS_ATTR;
/* TODO: what latency does layer 1 have? */
int mpeg_latency[3] = { 0, 481, 529 };

#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
extern char iedata[];
extern char iend[];
#endif

struct codec_api *ci;
int64_t samplecount;
int64_t samplesdone;
int stop_skip, start_skip;
int current_stereo_mode = -1;
unsigned int current_frequency = 0;

void recalc_samplecount(void)
{
    /* NOTE: currently this doesn't work, the below calculated samples_count
       seems to be right, but sometimes we just don't have all the data we
       need... */
    if (ci->id3->frame_count) {
        /* TODO: 1152 is the frame size in samples for MPEG1 layer 2 and layer 3,
           it's probably not correct at all for MPEG2 and layer 1 */
        samplecount = ((int64_t)ci->id3->frame_count) * 1152;
    } else {
        samplecount = ((int64_t)ci->id3->length) * current_frequency / 1000;
    }
    
    samplecount -= start_skip + stop_skip;
}

/* this is the codec entry point */
enum codec_status codec_start(struct codec_api *api)
{
    int status = CODEC_OK;
    long size;
    int file_end;
    int frame_skip;      /* samples to skip current frame */
    int samples_to_skip; /* samples to skip in total for this file (at start) */
    char *inputbuffer;

    ci = api;

#ifdef USE_IRAM
    ci->memcpy(iramstart, iramcopy, iramend - iramstart);
    ci->memset(iedata, 0, iend - iedata);
#endif

    if (codec_init(api))
        return CODEC_ERROR;

    /* Create a decoder instance */

    ci->configure(CODEC_DSP_ENABLE, (bool *)true);
    ci->configure(DSP_DITHER, (bool *)false);
    ci->configure(DSP_SET_SAMPLE_DEPTH, (int *)(MAD_F_FRACBITS));
    ci->configure(DSP_SET_CLIP_MIN, (int *)-MAD_F_ONE);
    ci->configure(DSP_SET_CLIP_MAX, (int *)(MAD_F_ONE - 1));
    ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (int *)(1024*16));
    
    ci->memset(&stream, 0, sizeof(struct mad_stream));
    ci->memset(&frame, 0, sizeof(struct mad_frame));
    ci->memset(&synth, 0, sizeof(struct mad_synth));
    
    mad_stream_init(&stream);
    mad_frame_init(&frame);
    mad_synth_init(&synth);

    /* We do this so libmad doesn't try to call codec_calloc() */
    ci->memset(mad_frame_overlap, 0, sizeof(mad_frame_overlap));
    frame.overlap = &mad_frame_overlap;
    stream.main_data = &mad_main_data;

    /* This label might need to be moved above all the init code, but I don't
       think reiniting the codec is necessary for MPEG. It might even be unwanted
       for gapless playback */
next_track:
    file_end = 0;
    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);
  
    ci->configure(DSP_SET_FREQUENCY, (int *)ci->id3->frequency);
    current_frequency = ci->id3->frequency;
    codec_set_replaygain(ci->id3);
    
    ci->request_buffer(&size, ci->id3->first_frame_offset);
    ci->advance_buffer(size);

    if (ci->id3->lead_trim >= 0 && ci->id3->tail_trim >= 0) {
        stop_skip = ci->id3->tail_trim - mpeg_latency[ci->id3->layer];
        if (stop_skip < 0) stop_skip = 0;
        start_skip = ci->id3->lead_trim + mpeg_latency[ci->id3->layer];
    } else {
        stop_skip = 0;
        /* We want to skip this amount anyway */
        start_skip = mpeg_latency[ci->id3->layer];
    }

    samplesdone = ((int64_t)ci->id3->elapsed) * current_frequency / 1000;
    samples_to_skip = start_skip;
    recalc_samplecount();
    
    /* This is the decoding loop. */
    while (1) {
        int framelength;

        ci->yield();
        if (ci->stop_codec || ci->reload_codec)
            break;
    
        if (ci->seek_time) {
            int newpos;
        
            samplesdone = ((int64_t) (ci->seek_time - 1)) 
                * current_frequency / 1000;
            newpos = ci->mp3_get_filepos(ci->seek_time-1) +
                ci->id3->first_frame_offset;

            if (!ci->seek_buffer(newpos))
                goto next_track;
            if (newpos == 0)
                samples_to_skip = start_skip;
            ci->seek_complete();
        }

        /* Lock buffers */
        if (stream.error == 0) {
            inputbuffer = ci->request_buffer(&size, INPUT_CHUNK_SIZE);
            if (size == 0 || inputbuffer == NULL)
                break;
            mad_stream_buffer(&stream, (unsigned char *)inputbuffer, size);
        }
    
        if (mad_frame_decode(&frame, &stream)) {
            if (stream.error == MAD_FLAG_INCOMPLETE 
                || stream.error == MAD_ERROR_BUFLEN) {
                /* This makes the codec support partially corrupted files */
                if (file_end == 30)
                    break;
        
                /* Fill the buffer */
                if (stream.next_frame)
                    ci->advance_buffer_loc((void *)stream.next_frame);
                else
                    ci->advance_buffer(size);
                stream.error = 0;
                file_end++;
                continue;
            } else if (MAD_RECOVERABLE(stream.error)) {
                continue;
            } else {
                /* Some other unrecoverable error */
                status = CODEC_ERROR;
                break;
            }
            break;
        }
        
        file_end = 0;

        mad_synth_frame(&synth, &frame);
        
        /* We need to skip samples_to_skip samples from the start of every file
           to properly support LAME style gapless MP3 files. samples_to_skip
           might be larger than one frame. */
        if (samples_to_skip < synth.pcm.length) {
            /* skip just part of the frame */
            frame_skip = samples_to_skip;
            samples_to_skip = 0;
        } else {
            /* we need to skip an entire frame */
            frame_skip = synth.pcm.length;
            samples_to_skip -= synth.pcm.length;
        }
       
        framelength = synth.pcm.length - frame_skip;
        
        if (stop_skip > 0) {
            int64_t max = samplecount - samplesdone;
            
            if (max < 0) max = 0;
            if (max < framelength) framelength = (int)max;
            if (framelength == 0 && frame_skip == 0) break;
        }
        
        /* Check if sample rate and stereo settings changed in this frame. */
        if (frame.header.samplerate != current_frequency) {
            current_frequency = frame.header.samplerate;
            ci->configure(DSP_SWITCH_FREQUENCY, (int *)current_frequency);
            recalc_samplecount();
        }
        if (MAD_NCHANNELS(&frame.header) == 2) {
            if (current_stereo_mode != STEREO_NONINTERLEAVED) {
                ci->configure(DSP_SET_STEREO_MODE, (int *)STEREO_NONINTERLEAVED);
                current_stereo_mode = STEREO_NONINTERLEAVED;
            }
        } else {
            if (current_stereo_mode != STEREO_MONO) {
                ci->configure(DSP_SET_STEREO_MODE, (int *)STEREO_MONO);
                current_stereo_mode = STEREO_MONO;
            }
        }
      
        /* Check if we can just skip the entire frame. */
        if (frame_skip < synth.pcm.length) {
            /* In case of a mono file, the second array will be ignored. */
            ci->pcmbuf_insert_split(&synth.pcm.samples[0][frame_skip],
                                    &synth.pcm.samples[1][frame_skip],
                                    framelength * 4);
        }
        
        if (stream.next_frame)
            ci->advance_buffer_loc((void *)stream.next_frame);
        else
            ci->advance_buffer(size);

        samplesdone += framelength;
        ci->set_elapsed(samplesdone / (current_frequency / 1000));
    }
    stream.error = 0;
  
    if (ci->request_next_track())
        goto next_track;
    
    return status;
}
