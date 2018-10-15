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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "codeclib.h"
#include <codecs/libmad/mad.h>
#include <inttypes.h>

CODEC_HEADER

#if NUM_CORES > 1 && !defined(MPEGPLAYER)
#define MPA_SYNTH_ON_COP
#endif

static struct mad_stream stream IBSS_ATTR;
static struct mad_frame frame IBSS_ATTR;
static struct mad_synth synth IBSS_ATTR;

#ifdef MPA_SYNTH_ON_COP
static volatile short die IBSS_ATTR = 0;          /*thread should die*/

#if (CONFIG_CPU == PP5024) || (CONFIG_CPU == PP5022)
static mad_fixed_t sbsample_prev[2][36][32] IBSS_ATTR;
#else
static mad_fixed_t sbsample_prev[2][36][32] SHAREDBSS_ATTR; 
#endif

static struct semaphore synth_done_sem IBSS_ATTR;
static struct semaphore synth_pending_sem IBSS_ATTR;
#endif

#define INPUT_CHUNK_SIZE   8192

static mad_fixed_t mad_frame_overlap[2][32][18] IBSS_ATTR;
static mad_fixed_t sbsample[2][36][32] IBSS_ATTR;

static unsigned char mad_main_data[MAD_BUFFER_MDLEN] IBSS_ATTR;
/* TODO: what latency does layer 1 have? */
static int mpeg_latency[3] = { 0, 481, 529 };
static int mpeg_framesize[3] = {384, 1152, 1152};

static void init_mad(void)
{
    ci->memset(&stream, 0, sizeof(struct mad_stream));
    ci->memset(&frame , 0, sizeof(struct mad_frame));
    ci->memset(&synth , 0, sizeof(struct mad_synth));

#ifdef MPA_SYNTH_ON_COP
    frame.sbsample_prev = &sbsample_prev;
    frame.sbsample      = &sbsample;
#else
    frame.sbsample_prev = &sbsample;
    frame.sbsample      = &sbsample;
#endif

    /* We do this so libmad doesn't try to call codec_calloc(). This needs to
     * be called before mad_stream_init(), mad_frame_inti() and 
     * mad_synth_init(). */
    frame.overlap    = &mad_frame_overlap;
    stream.main_data = &mad_main_data;
    
    /* Call mad initialization. Those will zero the arrays frame.overlap,
     * frame.sbsample and frame.sbsample_prev. Therefore there is no need to 
     * zero them here. */
    mad_stream_init(&stream);
    mad_frame_init(&frame);
    mad_synth_init(&synth);
}

static int get_file_pos(int newtime)
{
    int pos = -1;
    struct mp3entry *id3 = ci->id3;

    if (id3->vbr) {
        /* Convert newtime and id3->length to seconds to
         * avoid overflow */
        unsigned int newtime_s = newtime/1000;
        unsigned int length_s  = id3->length/1000;
        
        if (id3->has_toc) {
            /* Use the TOC to find the new position */
            unsigned int percent, remainder;
            int curtoc, nexttoc, plen;

            percent = (newtime_s*100) / length_s;
            if (percent > 99)
                percent = 99;

            curtoc = id3->toc[percent];

            if (percent < 99) {
                nexttoc = id3->toc[percent+1];
            } else {
                nexttoc = 256;
            }

            pos = (id3->filesize/256)*curtoc;

            /* Use the remainder to get a more accurate position */
            remainder   = (newtime_s*100) % length_s;
            remainder   = (remainder*100) / length_s;
            plen        = (nexttoc - curtoc)*(id3->filesize/256);
            pos        += (plen/100)*remainder;
        } else {
            /* No TOC exists, estimate the new position */
            pos = (id3->filesize / length_s) * newtime_s;
        }
    } else if (id3->bitrate) {
        pos = newtime * (id3->bitrate / 8);
    } else {
        return -1;
    }

    /* Don't seek right to the end of the file so that we can
       transition properly to the next song */
    if (pos >= (int)(id3->filesize - id3->id3v1len))
        pos = id3->filesize - id3->id3v1len - 1;

    /* id3->filesize excludes id3->first_frame_offset, so add it now */
    pos += id3->first_frame_offset;

    return pos;
}

static void set_elapsed(struct mp3entry* id3)
{
    unsigned long offset = id3->offset > id3->first_frame_offset ?
        id3->offset - id3->first_frame_offset : 0;
    unsigned long elapsed = id3->elapsed;

    if ( id3->vbr ) {
        if ( id3->has_toc ) {
            /* calculate elapsed time using TOC */
            int i;
            unsigned int remainder, plen, relpos, nextpos;

            /* find wich percent we're at */
            for (i=0; i<100; i++ )
                if ( offset < id3->toc[i] * (id3->filesize / 256) )
                    break;

            i--;
            if (i < 0)
                i = 0;

            relpos = id3->toc[i];

            if (i < 99)
                nextpos = id3->toc[i+1];
            else
                nextpos = 256;

            remainder = offset - (relpos * (id3->filesize / 256));

            /* set time for this percent (divide before multiply to prevent
               overflow on long files. loss of precision is negligible on
               short files) */
            elapsed = i * (id3->length / 100);

            /* calculate remainder time */
            plen = (nextpos - relpos) * (id3->filesize / 256);
            elapsed += (((remainder * 100) / plen) * (id3->length / 10000));
        }
        else {
            /* no TOC exists. set a rough estimate using average bitrate */
            int tpk = id3->length /
                ((id3->filesize - id3->first_frame_offset - id3->id3v1len) /
                1024);
            elapsed = offset / 1024 * tpk;
        }
    }
    else
    {
        /* constant bitrate, use exact calculation */
        if (id3->bitrate != 0)
            elapsed = offset / (id3->bitrate / 8);
    }

    ci->set_elapsed(elapsed);
}

#ifdef MPA_SYNTH_ON_COP

/*
 * Run the synthesis filter on the COProcessor 
 */

static int mad_synth_thread_stack[DEFAULT_STACK_SIZE/sizeof(int)] IBSS_ATTR;

static const unsigned char * const mad_synth_thread_name = "mp3dec";
static unsigned int mad_synth_thread_id = 0;


static void mad_synth_thread(void)
{
    while(1) {
        ci->semaphore_release(&synth_done_sem);
        ci->semaphore_wait(&synth_pending_sem, TIMEOUT_BLOCK);
        
        if(die)
            break;

        mad_synth_frame(&synth, &frame);
    }    
}

/* wait for the synth thread to go idle which indicates a PCM frame has been
 * synthesized */
static inline void mad_synth_thread_wait_pcm(void)
{
    ci->semaphore_wait(&synth_done_sem, TIMEOUT_BLOCK);
}

/* increment the done semaphore - used after a wait for idle to preserve the
 * semaphore count */
static inline void mad_synth_thread_unwait_pcm(void)
{
    ci->semaphore_release(&synth_done_sem);
}

/* after synth thread has gone idle - switch decoded frames and commence
 * synthesis on it */
static void mad_synth_thread_ready(void)
{
    mad_fixed_t (*temp)[2][36][32];

    /*circular buffer that holds 2 frames' samples*/
    temp=frame.sbsample;
    frame.sbsample = frame.sbsample_prev;
    frame.sbsample_prev=temp;

    ci->semaphore_release(&synth_pending_sem);
}

static bool mad_synth_thread_create(void)
{
    ci->semaphore_init(&synth_done_sem, 1, 0);
    ci->semaphore_init(&synth_pending_sem, 1, 0);
       
    mad_synth_thread_id = ci->create_thread(mad_synth_thread, 
                            mad_synth_thread_stack,
                            sizeof(mad_synth_thread_stack), 0,
                            mad_synth_thread_name 
                            IF_PRIO(, PRIORITY_PLAYBACK)
                            IF_COP(, COP));
    
    if (mad_synth_thread_id == 0)
        return false;

    return true;
}

static void mad_synth_thread_quit(void)
{
    /* mop up COP thread */
    die = 1;
    ci->semaphore_release(&synth_pending_sem);
    ci->thread_wait(mad_synth_thread_id);
    ci->commit_discard_dcache();
}
#else
static inline void mad_synth_thread_ready(void)
{
     mad_synth_frame(&synth, &frame);
}

static inline bool mad_synth_thread_create(void)
{
    return true;
}

static inline void mad_synth_thread_quit(void)
{
}

static inline void mad_synth_thread_wait_pcm(void)
{
}

static inline void mad_synth_thread_unwait_pcm(void)
{
}
#endif /* MPA_SYNTH_ON_COP */

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* Create a decoder instance */
        if (codec_init())
            return CODEC_ERROR;

        ci->configure(DSP_SET_SAMPLE_DEPTH, MAD_F_FRACBITS);

        /* does nothing on 1 processor systems except return true */
        if(!mad_synth_thread_create())
            return CODEC_ERROR;
    }
    else if (reason == CODEC_UNLOAD) {
        /* mop up COP thread - MT only */
        mad_synth_thread_quit();
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    size_t size;
    int file_end;
    int samples_to_skip; /* samples to skip in total for this file (at start) */
    char *inputbuffer;
    int64_t samplesdone;
    int stop_skip, start_skip;
    int current_stereo_mode = -1;
    unsigned long current_frequency = 0;
    int framelength;
    int padding = MAD_BUFFER_GUARD; /* to help mad decode the last frame */
    intptr_t param;

    /* Reinitializing seems to be necessary to avoid playback quircks when seeking. */
    init_mad();

    file_end = 0;

    ci->configure(DSP_SET_FREQUENCY, ci->id3->frequency);
    current_frequency = ci->id3->frequency;
    codec_set_replaygain(ci->id3);
    
    if (!ci->id3->offset && ci->id3->elapsed) {
        /* Have elapsed time but not offset */
        ci->id3->offset = get_file_pos(ci->id3->elapsed);
    }

    if (ci->id3->offset) {
        ci->seek_buffer(ci->id3->offset);
        set_elapsed(ci->id3);
    }
    else
        ci->seek_buffer(ci->id3->first_frame_offset);

    if (ci->id3->lead_trim >= 0 && ci->id3->tail_trim >= 0) {
        stop_skip = ci->id3->tail_trim - mpeg_latency[ci->id3->layer];
        if (stop_skip < 0) stop_skip = 0;
        start_skip = ci->id3->lead_trim + mpeg_latency[ci->id3->layer];
    } else {
        stop_skip = 0;
        /* We want to skip this amount anyway */
        start_skip = mpeg_latency[ci->id3->layer];
    }

    /* Libmad will not decode the last frame without 8 bytes of extra padding
       in the buffer. So, we can trick libmad into not decoding the last frame
       if we are to skip it entirely and then cut the appropriate samples from
       final frame that we did decode. Note, if all tags (ID3, APE) are not
       properly stripped from the end of the file, this trick will not work. */
    if (stop_skip >= mpeg_framesize[ci->id3->layer]) {
        padding = 0;
        stop_skip -= mpeg_framesize[ci->id3->layer];
    } else {
        padding = MAD_BUFFER_GUARD;
    }

    samplesdone = ((int64_t)ci->id3->elapsed) * current_frequency / 1000;

    /* Don't skip any samples unless we start at the beginning. */
    if (samplesdone > 0)
        samples_to_skip = 0;
    else
        samples_to_skip = start_skip;

    framelength = 0;

    /* This is the decoding loop. */
    while (1) {
        long action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME) {
            int newpos;

            /*make sure the synth thread is idle before seeking - MT only*/
            mad_synth_thread_wait_pcm();
            mad_synth_thread_unwait_pcm();

            samplesdone = ((int64_t)param)*current_frequency/1000;

            if (param == 0) {
                newpos = ci->id3->first_frame_offset;
                samples_to_skip = start_skip;
            } else {
                newpos = get_file_pos(param);
                samples_to_skip = 0;
            }

            if (!ci->seek_buffer(newpos))
            {
                ci->seek_complete();
                break;
            }

            ci->set_elapsed((samplesdone * 1000LL) / current_frequency);
            ci->seek_complete();
            init_mad();
            framelength = 0;
        }

        /* Lock buffers */
        if (stream.error == 0) {
            inputbuffer = ci->request_buffer(&size, INPUT_CHUNK_SIZE);
            if (size == 0 || inputbuffer == NULL)
                break;
            mad_stream_buffer(&stream, (unsigned char *)inputbuffer,
                              size + padding);
        }

        if (mad_frame_decode(&frame, &stream)) {
            if (stream.error == MAD_ERROR_BUFLEN) {
                /* This makes the codec support partially corrupted files */
                if (file_end == 30)
                    break;

                /* Fill the buffer */
                if (stream.next_frame)
                    ci->advance_buffer(stream.next_frame - stream.buffer);
                else
                    ci->advance_buffer(size);
                stream.error = 0; /* Must get new inputbuffer next time */
                file_end++;
                continue;
            } else if (MAD_RECOVERABLE(stream.error)) {
                /* Probably syncing after a seek */
                continue;
            } else {
                /* Some other unrecoverable error */
               return CODEC_ERROR;
            }
        }

        /* Do the pcmbuf insert here. Note, this is the PREVIOUS frame's pcm
           data (not the one just decoded above). When we exit the decoding
           loop we will need to process the final frame that was decoded. */
        mad_synth_thread_wait_pcm();

        if (framelength > 0) {
            
            /* In case of a mono file, the second array will be ignored. */
            ci->pcmbuf_insert(&synth.pcm.samples[0][samples_to_skip],
                              &synth.pcm.samples[1][samples_to_skip],
                              framelength);

            /* Only skip samples for the first frame added. */
            samples_to_skip = 0;
        }

        /* Initiate PCM synthesis on the COP (MT) or perform it here (ST) */
        mad_synth_thread_ready();

        /* Check if sample rate and stereo settings changed in this frame. */
        if (frame.header.samplerate != current_frequency) {
            current_frequency = frame.header.samplerate;
            ci->configure(DSP_SET_FREQUENCY, current_frequency);
        }
        if (MAD_NCHANNELS(&frame.header) == 2) {
            if (current_stereo_mode != STEREO_NONINTERLEAVED) {
                ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);
                current_stereo_mode = STEREO_NONINTERLEAVED;
            }
        } else {
            if (current_stereo_mode != STEREO_MONO) {
                ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
                current_stereo_mode = STEREO_MONO;
            }
        }

        if (stream.next_frame)
            ci->advance_buffer(stream.next_frame - stream.buffer);
        else
            ci->advance_buffer(size);
        stream.error = 0; /* Must get new inputbuffer next time */
        file_end = 0;

        framelength = synth.pcm.length - samples_to_skip;
        if (framelength < 0) {
            framelength = 0;
            samples_to_skip -= synth.pcm.length;
        }

        samplesdone += framelength;
        ci->set_elapsed((samplesdone * 1000LL) / current_frequency);
    }

    /* wait for synth idle - MT only*/
    mad_synth_thread_wait_pcm();
    mad_synth_thread_unwait_pcm();

    /* Finish the remaining decoded frame.
       Cut the required samples from the end. */
    if (framelength > stop_skip){
        ci->pcmbuf_insert(synth.pcm.samples[0], synth.pcm.samples[1],
                          framelength - stop_skip);
    }

    return CODEC_OK;
}
