/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Miika Pekkarinen
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
#include <stdio.h>
#include "config.h"
#include "system.h"
#include "debug.h"
#include <kernel.h>
#include "pcmbuf.h"
#include "pcm.h"
#include "playback.h"

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"
#ifndef SIMULATOR
#include "cpu.h"
#endif
#include <string.h>
#include "settings.h"
#include "audio.h"
#include "voice_thread.h"
#include "dsp.h"

#define PCMBUF_TARGET_CHUNK 32768 /* This is the target fill size of chunks
                                     on the pcm buffer */
#define PCMBUF_MINAVG_CHUNK 24576 /* This is the minimum average size of
                                     chunks on the pcm buffer (or we run out
                                     of buffer descriptors, which is
                                     non-fatal) */
#define PCMBUF_MIN_CHUNK     4096 /* We try to never feed a chunk smaller than
                                     this to the DMA */
#define PCMBUF_MIX_CHUNK     8192 /* This is the maximum size of one packet
                                     for mixing (crossfade or voice) */

#if MEMORYSIZE > 2
/* Keep watermark high for iPods at least (2s) */
#define PCMBUF_WATERMARK     (NATIVE_FREQUENCY * 4 * 2)
#else
#define PCMBUF_WATERMARK     (NATIVE_FREQUENCY * 1) /* 0.25 seconds */
#endif

/* Structure we can use to queue pcm chunks in memory to be played
 * by the driver code. */
struct chunkdesc
{
    void *addr;
    size_t size;
    struct chunkdesc* link;
    /* true if last chunk in the track */
    bool end_of_track;
};

#define NUM_CHUNK_DESCS(bufsize) \
    ((bufsize) / PCMBUF_MINAVG_CHUNK)

/* Size of the PCM buffer. */
static size_t pcmbuf_size IDATA_ATTR = 0;
static char *pcmbuf_bufend IDATA_ATTR;
static char *pcmbuffer IDATA_ATTR;
/* Current PCM buffer write index. */
static size_t pcmbuffer_pos IDATA_ATTR;
/* Amount pcmbuffer_pos will be increased.*/
static size_t pcmbuffer_fillpos IDATA_ATTR;

/* Gapless playback */
static bool end_of_track IDATA_ATTR;
bool track_transition IDATA_ATTR;

#ifdef HAVE_CROSSFADE
/* Crossfade buffer */
static char *fadebuf IDATA_ATTR;

/* Crossfade related state */
static bool crossfade_enabled;
static bool crossfade_enable_request;
static bool crossfade_mixmode;
static bool crossfade_auto_skip;
static bool crossfade_active IDATA_ATTR;
static bool crossfade_track_change_started IDATA_ATTR;

/* Track the current location for processing crossfade */
static struct chunkdesc *crossfade_chunk IDATA_ATTR;
static size_t crossfade_sample IDATA_ATTR;

/* Counters for fading in new data */
static size_t crossfade_fade_in_total IDATA_ATTR;
static size_t crossfade_fade_in_rem IDATA_ATTR;
#endif

static struct chunkdesc *read_chunk IDATA_ATTR;
static struct chunkdesc *read_end_chunk IDATA_ATTR;
static struct chunkdesc *write_chunk IDATA_ATTR;
static struct chunkdesc *write_end_chunk IDATA_ATTR;
static size_t last_chunksize IDATA_ATTR;

static size_t pcmbuf_unplayed_bytes IDATA_ATTR;
static size_t pcmbuf_watermark IDATA_ATTR;

/* Voice */
static char *voicebuf IDATA_ATTR;
static struct chunkdesc *mix_chunk IDATA_ATTR;
static size_t pcmbuf_mix_sample IDATA_ATTR;

static bool low_latency_mode = false;
static bool flush_pcmbuf = false;

#ifdef HAVE_PRIORITY_SCHEDULING
static int codec_thread_priority = PRIORITY_PLAYBACK;
#endif

extern unsigned int codec_thread_id;

/* Helpful macros for use in conditionals this assumes some of the above
 * static variable names */
#define COMMIT_IF_NEEDED if(pcmbuffer_fillpos > PCMBUF_TARGET_CHUNK || \
    (pcmbuffer_pos + pcmbuffer_fillpos) >= pcmbuf_size) commit_chunk(false)
#define LOW_DATA(quarter_secs) \
    (pcmbuf_unplayed_bytes < NATIVE_FREQUENCY * quarter_secs)

#ifdef HAVE_CROSSFADE
static void crossfade_start(void);
static void write_to_crossfade(char *buf, size_t length);
static void pcmbuf_finish_crossfade_enable(void);
#endif


/**************************************/

/* define this to show detailed chunkdesc usage information on the sim console */
/*#define DESC_DEBUG*/

#ifndef SIMULATOR
#undef DESC_DEBUG
#endif
#ifdef DESC_DEBUG
static struct chunkdesc *first_desc;
static bool show_desc_in_use = false;
#define DISPLAY_DESC(caller) while(!show_desc(caller))
#define DESC_IDX(desc)  (desc ? desc - first_desc : -1)
#define DESCL_IDX(desc) (desc && desc->link ? desc->link - first_desc : -1)
#define SHOW_1ST(desc) if(DESC_IDX (desc)==-1) DEBUGF(" -- "); \
                       else DEBUGF(" %02d ",   DESC_IDX(desc))
#define SHOW_2ND(desc) if(DESCL_IDX(desc)==-1) DEBUGF("l --  "); \
                       else DEBUGF("l %02d  ", DESCL_IDX(desc))
#define DESC_SHOW(tag, desc) DEBUGF(tag);SHOW_1ST(desc); \
                             DEBUGF(tag);SHOW_2ND(desc)

static bool show_desc(char *caller)
{
    if (show_desc_in_use) return false;
    show_desc_in_use = true;
    DEBUGF("%-14s\t", caller);
    DESC_SHOW("r",  read_chunk);
    DESC_SHOW("re", read_end_chunk);
    DEBUGF(" ");
    DESC_SHOW("w",  write_chunk);
    DESC_SHOW("we", write_end_chunk);
    DEBUGF("\n");
    show_desc_in_use = false;
    return true;
}
#else
#define DISPLAY_DESC(caller) do{}while(0)
#endif


/** Accept new PCM data */

/* Commit PCM buffer samples as a new chunk for playback */
static void commit_chunk(bool flush_next_time)
{
    if (!pcmbuffer_fillpos)
        return;
    
    /* Never use the last buffer descriptor */
    while (write_chunk == write_end_chunk) {
        /* If this happens, something is being stupid */
        if (!pcm_is_playing()) {
            logf("commit_chunk error");
            pcmbuf_play_start();
        }
        /* Let approximately one chunk of data playback */
        sleep(HZ*PCMBUF_TARGET_CHUNK/(NATIVE_FREQUENCY*4));
    }
    
    /* commit the chunk */
    
    register size_t size = pcmbuffer_fillpos;
    /* Grab the next description to write, and change the write pointer */
    register struct chunkdesc *pcmbuf_current = write_chunk;
    write_chunk = pcmbuf_current->link;
    /* Fill in the values in the new buffer chunk */
    pcmbuf_current->addr = &pcmbuffer[pcmbuffer_pos];
    pcmbuf_current->size = size;
    pcmbuf_current->end_of_track = end_of_track;
    pcmbuf_current->link = NULL;
    end_of_track = false;   /* This is single use only */
    
    if (read_chunk != NULL)
    {
        if (flush_pcmbuf)
        {
            /* Flush! Discard all data after the currently playing chunk,
               and make the current chunk play next */
            write_end_chunk->link = read_chunk->link;
            read_chunk->link = pcmbuf_current;
            while (write_end_chunk->link)
            {
                write_end_chunk = write_end_chunk->link;
                pcmbuf_unplayed_bytes -= write_end_chunk->size;
            }
        }
        /* If there is already a read buffer setup, add to it */
        else
            read_end_chunk->link = pcmbuf_current;
    }
    else
    {
        /* Otherwise create the buffer */
        read_chunk = pcmbuf_current;
    }
    
    /* If flush_next_time is true, then the current chunk will be thrown out
     * and the next chunk to be committed will be the next to be played.
     * This is used to empty the PCM buffer for a track change. */
    flush_pcmbuf = flush_next_time;
    
    /* This is now the last buffer to read */
    read_end_chunk = pcmbuf_current;

    /* Update bytes counters */
    pcmbuf_unplayed_bytes += size;

    pcmbuffer_pos += size;
    if (pcmbuffer_pos >= pcmbuf_size)
        pcmbuffer_pos -= pcmbuf_size;

    pcmbuffer_fillpos = 0;
    DISPLAY_DESC("commit_chunk");
}

/* Set priority of the codec thread */
#ifdef HAVE_PRIORITY_SCHEDULING
static void boost_codec_thread(bool boost)
{
    /* Keep voice and codec threads at the same priority or else voice
     * will starve if the codec thread's priority is boosted. */
    if (boost)
    {
        int priority = (PRIORITY_PLAYBACK - PRIORITY_PLAYBACK_MAX)*pcmbuf_unplayed_bytes
                          / (2*NATIVE_FREQUENCY) + PRIORITY_PLAYBACK_MAX;

        if (priority != codec_thread_priority)
        {
            codec_thread_priority = priority;
            thread_set_priority(codec_thread_id, priority);
            voice_thread_set_priority(priority);
        }
    }
    else if (codec_thread_priority != PRIORITY_PLAYBACK)
    {
        thread_set_priority(codec_thread_id, PRIORITY_PLAYBACK);
        voice_thread_set_priority(PRIORITY_PLAYBACK);
        codec_thread_priority = PRIORITY_PLAYBACK;
    }
}
#else
#define boost_codec_thread(boost) do{}while(0)
#endif /* HAVE_PRIORITY_SCHEDULING */

/* Return true if the PCM buffer is able to receive new data.
 * Also maintain buffer level above the watermark. */
static bool prepare_insert(size_t length)
{
    if (low_latency_mode)
    {
        /* 1/4s latency. */
        if (!LOW_DATA(1) && pcm_is_playing())
            return false;
    }

    /* Need to save PCMBUF_MIN_CHUNK to prevent wrapping overwriting */
    if (pcmbuf_free() < length + PCMBUF_MIN_CHUNK)
        return false;

    /* Maintain the buffer level above the watermark */
    if (pcm_is_playing())
    {
        /* Only codec thread initiates boost - voice boosts the cpu when playing
           a clip */
#ifndef SIMULATOR
        if (thread_get_current() == codec_thread_id)
#endif /* SIMULATOR */
        {
            if (pcmbuf_unplayed_bytes <= pcmbuf_watermark)
            {
                /* Fill PCM buffer by boosting cpu */
                trigger_cpu_boost();
                /* If buffer is critically low, override UI priority, else
                   set back to the original priority. */
                boost_codec_thread(LOW_DATA(2));
            }
            else
            {
                boost_codec_thread(false);
            }
        }

#ifdef HAVE_CROSSFADE
        /* Disable crossfade if < .5s of audio */
        if (LOW_DATA(2))
        {
            crossfade_active = false;
        }
#endif
    }
    else    /* pcm_is_playing */
    {
        /* Boost CPU for pre-buffer */
        trigger_cpu_boost();

        /* If pre-buffered to the watermark, start playback */
#if MEMORYSIZE > 2
        if (!LOW_DATA(4))
#else
        if (pcmbuf_unplayed_bytes > pcmbuf_watermark)
#endif
        {
            logf("pcm starting");
            if (!(audio_status() & AUDIO_STATUS_PAUSE))
                pcmbuf_play_start();
        }
    }

    return true;
}

/* Request space in the buffer for writing output samples */
void *pcmbuf_request_buffer(int *count)
{
#ifdef HAVE_CROSSFADE
    /* we're going to crossfade to a new track, which is now on its way */
    if (crossfade_track_change_started)
        crossfade_start();

    /* crossfade has begun, put the new track samples in fadebuf */
    if (crossfade_active)
    {
        *count = MIN(*count, PCMBUF_MIX_CHUNK/4);
        return fadebuf;
    }
    else
#endif
    /* if possible, reserve room in the PCM buffer for new samples */
    {
        if(prepare_insert(*count << 2))
        {
            size_t pcmbuffer_index = pcmbuffer_pos + pcmbuffer_fillpos;
            if (pcmbuf_size - pcmbuffer_index >= PCMBUF_MIN_CHUNK)
            {
                /* Usual case, there's space here */
                return &pcmbuffer[pcmbuffer_index];
            }
            else
            {
                /* Wrap the buffer, the new samples go at the beginning */
                commit_chunk(false);
                pcmbuffer_pos = 0;
                return &pcmbuffer[0];
            }
        }
    }
    /* PCM buffer not ready to receive new data yet */
    return NULL;
}

/* Handle new samples to the buffer */
void pcmbuf_write_complete(int count)
{
    size_t length = (size_t)(unsigned int)count << 2;
#ifdef HAVE_CROSSFADE
    if (crossfade_active)
        write_to_crossfade(fadebuf, length);
    else
#endif    
    {
        pcmbuffer_fillpos += length;
        COMMIT_IF_NEEDED;
    }
}


/** Init */

static inline void init_pcmbuffers(void)
{
#ifdef DESC_DEBUG
    first_desc = write_chunk;
#endif
    struct chunkdesc *next = write_chunk;
    next++;
    write_end_chunk = write_chunk;
    while ((void *)next < (void *)pcmbuf_bufend) {
        write_end_chunk->link=next;
        write_end_chunk=next;
        next++;
    }
    DISPLAY_DESC("init");
}

static size_t get_next_required_pcmbuf_size(void)
{
    size_t seconds = 1;

#ifdef HAVE_CROSSFADE
    if (crossfade_enable_request)
        seconds += global_settings.crossfade_fade_out_delay +
                   global_settings.crossfade_fade_out_duration;
#endif

#if MEMORYSIZE > 2
    /* Buffer has to be at least 2s long. */
    seconds += 2;
#endif
    logf("pcmbuf len: %ld", (long)seconds);
    return seconds * (NATIVE_FREQUENCY*4); /* 2 channels + 2 bytes/sample */
}

/* Initialize the pcmbuffer the structure looks like this:
 * ...|---------PCMBUF---------|FADEBUF|VOICEBUF|DESCS|... */
size_t pcmbuf_init(unsigned char *bufend)
{
    pcmbuf_bufend = bufend;
    pcmbuf_size = get_next_required_pcmbuf_size();
    write_chunk = (struct chunkdesc *)pcmbuf_bufend -
        NUM_CHUNK_DESCS(pcmbuf_size);
    voicebuf = (char *)write_chunk - PCMBUF_MIX_CHUNK;
#ifdef HAVE_CROSSFADE
    fadebuf = voicebuf - PCMBUF_MIX_CHUNK;
    pcmbuffer = fadebuf - pcmbuf_size;
#else
    pcmbuffer = voicebuf - pcmbuf_size;
#endif

    init_pcmbuffers();

#ifdef HAVE_CROSSFADE
    pcmbuf_finish_crossfade_enable();
#else
    pcmbuf_watermark = PCMBUF_WATERMARK;
#endif

    pcmbuf_play_stop();

    return pcmbuf_bufend - pcmbuffer;
}


/** Track change */

void pcmbuf_start_track_change(bool auto_skip)
{
    bool crossfade = false;
#ifdef HAVE_CROSSFADE
    /* Determine whether this track change needs to crossfade */
    if(crossfade_enabled && !pcmbuf_is_crossfade_active())
    {
        switch(global_settings.crossfade)
        {
            case CROSSFADE_ENABLE_AUTOSKIP:
                crossfade = auto_skip;
                break;
            case CROSSFADE_ENABLE_MANSKIP:
                crossfade = !auto_skip;
                break;
            case CROSSFADE_ENABLE_SHUFFLE:
                crossfade = global_settings.playlist_shuffle;
                break;
            case CROSSFADE_ENABLE_SHUFFLE_OR_MANSKIP:
                crossfade = global_settings.playlist_shuffle || !auto_skip;
                break;
            case CROSSFADE_ENABLE_ALWAYS:
                crossfade = true;
                break;
        }
    }
#endif
    
    if (!auto_skip || crossfade)
    /* manual skip or crossfade */
    {
        if (crossfade)
            { logf("  crossfade track change"); }
        else
            { logf("  manual track change"); }
        
        /* Notify the wps that the track change starts now */
        audio_post_track_change(false);

        /* Can't do two crossfades at once and, no fade if pcm is off now */
        if (
#ifdef HAVE_CROSSFADE
            pcmbuf_is_crossfade_active() ||
#endif
            !pcm_is_playing())
        {
            pcmbuf_play_stop();
            return;
        }

        trigger_cpu_boost();

        /* Not enough data, or not crossfading, flush the old data instead */
        if (LOW_DATA(2) || !crossfade || low_latency_mode)
        {
            commit_chunk(true);
            return;
        }

#ifdef HAVE_CROSSFADE
        /* Don't enable mix mode when skipping tracks manually. */
        crossfade_mixmode = auto_skip && global_settings.crossfade_fade_out_mixmode;
        
        crossfade_auto_skip = auto_skip;

        crossfade_track_change_started = crossfade;
#endif
    }
    else    /* automatic and not crossfading, so do gapless track change */
    {
        /* The codec is moving on to the next track, but the current track will
         * continue to play.  Set a flag to make sure the elapsed time of the
         * current track will be updated properly, and mark the current chunk
         * as the last one in the track. */
        logf("  gapless track change");
        track_transition = true;
        end_of_track = true;
    }
}


/** Playback */

/* PCM driver callback
 * This function has 3 major logical parts (separated by brackets both for
 * readability and variable scoping).  The first part performs the
 * operations related to finishing off the last chunk we fed to the DMA.
 * The second part detects the end of playlist condition when the PCM
 * buffer is empty except for uncommitted samples.  Then they are committed
 * and sent to the PCM driver for playback.  The third part performs the
 * operations involved in sending a new chunk to the DMA. */
static void pcmbuf_pcm_callback(unsigned char** start, size_t* size) ICODE_ATTR;
static void pcmbuf_pcm_callback(unsigned char** start, size_t* size)
{
    {
        struct chunkdesc *pcmbuf_current = read_chunk;
        /* Take the finished chunk out of circulation */
        read_chunk = pcmbuf_current->link;

        /* if during a track transition, update the elapsed time */
        if (track_transition)
            audio_pcmbuf_position_callback(last_chunksize);
        
        /* if last chunk in the track, stop updates and notify audio thread */
        if (pcmbuf_current->end_of_track)
        {
            track_transition = false;
            audio_post_track_change(true);
        }

        /* Put the finished chunk back into circulation */
        write_end_chunk->link = pcmbuf_current;
        write_end_chunk = pcmbuf_current;

        /* If we've read over the mix chunk while it's still mixing there */
        if (pcmbuf_current == mix_chunk)
            mix_chunk = NULL;

#ifdef HAVE_CROSSFADE
        /* If we've read over the crossfade chunk while it's still fading */
        if (pcmbuf_current == crossfade_chunk)
            crossfade_chunk = read_chunk;
#endif
    }
    
    {
        /* Commit last samples at end of playlist */
        if (pcmbuffer_fillpos && !read_chunk)
        {
            logf("pcmbuf_pcm_callback: commit last samples");
            commit_chunk(false);
        }
    }

    {
        /* Send the new chunk to the PCM */
        if(read_chunk)
        {
            size_t current_size = read_chunk->size;

            pcmbuf_unplayed_bytes -= current_size;
            last_chunksize = current_size;
            *size = current_size;
            *start = read_chunk->addr;
        }
        else
        {
            /* No more chunks */
            logf("pcmbuf_pcm_callback: no more chunks");
            last_chunksize = 0;
            *size = 0;
            *start = NULL;
        }
    }
    DISPLAY_DESC("callback");
}

/* Force playback */
void pcmbuf_play_start(void)
{
    if (!pcm_is_playing() && pcmbuf_unplayed_bytes && read_chunk != NULL)
    {
        logf("pcmbuf_play_start");
        last_chunksize = read_chunk->size;
        pcmbuf_unplayed_bytes -= last_chunksize;
        pcm_play_data(pcmbuf_pcm_callback,
            (unsigned char *)read_chunk->addr, last_chunksize);
    }
}

void pcmbuf_play_stop(void)
{
    logf("pcmbuf_play_stop");
    pcm_play_stop();

    pcmbuf_unplayed_bytes = 0;
    mix_chunk = NULL;
    if (read_chunk) {
        write_end_chunk->link = read_chunk;
        write_end_chunk = read_end_chunk;
        read_chunk = read_end_chunk = NULL;
    }
    pcmbuffer_pos = 0;
    pcmbuffer_fillpos = 0;
#ifdef HAVE_CROSSFADE
    crossfade_track_change_started = false;
    crossfade_active = false;
#endif
    end_of_track = false;
    track_transition = false;
    DISPLAY_DESC("play_stop");

    /* Can unboost the codec thread here no matter who's calling */
    boost_codec_thread(false);
}

void pcmbuf_pause(bool pause)
{
    logf("pcmbuf_pause: %s", pause?"pause":"play");
    if (pcm_is_playing())
        pcm_play_pause(!pause);
    else if (!pause)
        pcmbuf_play_start();
}


/** Crossfade */

/* Clip sample to signed 16 bit range */
static inline int32_t clip_sample_16(int32_t sample)
{
    if ((int16_t)sample != sample)
        sample = 0x7fff ^ (sample >> 31);
    return sample;
}

#ifdef HAVE_CROSSFADE
/* Find the chunk that's (length) deep in the list. Return the position within
 * the chunk, and leave the chunkdesc pointer pointing to the chunk. */
static size_t find_chunk(size_t length, struct chunkdesc **chunk)
{
    while (*chunk && length >= (*chunk)->size)
    {
        length -= (*chunk)->size;
        *chunk = (*chunk)->link;
    }
    return length / 2;
}

/* Fade samples in the PCM buffer */
static void fade_block(int factor, size_t length, size_t *fade_sample,
                       struct chunkdesc **fade_chunk)
{
    while (length > 0 && *fade_chunk != NULL)
    {
        /* Fade one sample */
        int16_t *buf = (int16_t *)((*fade_chunk)->addr);
        int32_t sample = buf[*fade_sample];
        buf[(*fade_sample)++] = (sample * factor) >> 8;
        length -= 2;

        /* Move to the next chunk as needed */
        if (*fade_sample * 2 >= (*fade_chunk)->size)
        {
            *fade_chunk = (*fade_chunk)->link;
            *fade_sample = 0;
        }
    }
}

/* Initializes crossfader, calculates all necessary parameters and performs
 * fade-out with the PCM buffer.  */
static void crossfade_start(void)
{
    size_t crossfade_rem;
    size_t crossfade_need;
    size_t fade_out_rem;
    size_t fade_out_delay;
    size_t fade_in_delay;

    crossfade_track_change_started = false;
    /* Reject crossfade if less than .5s of data */
    if (LOW_DATA(2)) {
        logf("crossfade rejected");
        pcmbuf_play_stop();
        return ;
    }

    logf("crossfade_start");
    commit_chunk(false);
    crossfade_active = true;

    /* Initialize the crossfade buffer size to all of the buffered data that
     * has not yet been sent to the DMA */
    crossfade_rem = pcmbuf_unplayed_bytes;
    crossfade_chunk = read_chunk->link;
    crossfade_sample = 0;

    /* Get fade out info from settings. */
    fade_out_delay =
        NATIVE_FREQUENCY * global_settings.crossfade_fade_out_delay * 4;
    fade_out_rem =
        NATIVE_FREQUENCY * global_settings.crossfade_fade_out_duration * 4;

    crossfade_need = fade_out_delay + fade_out_rem;
    if (crossfade_rem > crossfade_need)
    {
        if (crossfade_auto_skip)
        /* Automatic track changes only modify the last part of the buffer,
         * so find the right chunk and sample to start the crossfade */
        {
            crossfade_sample = find_chunk(crossfade_rem - crossfade_need,
                &crossfade_chunk);
            crossfade_rem = crossfade_need;
        }
        else
        /* Manual skips occur immediately, but give time to process */
        {
            crossfade_rem -= crossfade_chunk->size;
            crossfade_chunk = crossfade_chunk->link;
        }
    }
    /* Truncate fade out duration if necessary. */
    if (crossfade_rem < crossfade_need)
    {
        size_t crossfade_short = crossfade_need - crossfade_rem;
        if (fade_out_rem >= crossfade_short)
            fade_out_rem -= crossfade_short;
        else
        {
            fade_out_delay -= crossfade_short - fade_out_rem;
            fade_out_rem = 0;
        }
    }
    crossfade_rem -= fade_out_delay + fade_out_rem;

    /* Completely process the crossfade fade-out effect with current PCM buffer */
    if (!crossfade_mixmode)
    {
        /* Fade out the specified amount of the already processed audio */
        size_t total_fade_out = fade_out_rem;
        size_t fade_out_sample;
        struct chunkdesc *fade_out_chunk = crossfade_chunk;

        /* Find the right chunk and sample to start fading out */
        fade_out_delay += crossfade_sample * 2;
        fade_out_sample = find_chunk(fade_out_delay, &fade_out_chunk);

        while (fade_out_rem > 0)
        {
            /* Each 1/10 second of audio will have the same fade applied */
            size_t block_rem = MIN(NATIVE_FREQUENCY * 4 / 10, fade_out_rem);
            int factor = (fade_out_rem << 8) / total_fade_out;

            fade_out_rem -= block_rem;

            fade_block(factor, block_rem, &fade_out_sample, &fade_out_chunk);
        }
        
        /* zero out the rest of the buffer */
        fade_block(0, crossfade_rem, &fade_out_sample, &fade_out_chunk);
    }

    /* Initialize fade-in counters */
    crossfade_fade_in_total =
        NATIVE_FREQUENCY * global_settings.crossfade_fade_in_duration * 4;
    crossfade_fade_in_rem = crossfade_fade_in_total;

    fade_in_delay =
        NATIVE_FREQUENCY * global_settings.crossfade_fade_in_delay * 4;

    /* Find the right chunk and sample to start fading in */
    fade_in_delay += crossfade_sample * 2;
    crossfade_sample = find_chunk(fade_in_delay, &crossfade_chunk);
    logf("crossfade_start done!");
}

/* Returns the number of bytes _NOT_ mixed */
static size_t crossfade_mix(int factor, const char *buf, size_t length)
{
    const int16_t *input_buf = (const int16_t *)buf;
    int16_t *output_buf = (int16_t *)(crossfade_chunk->addr);
    int16_t *chunk_end = SKIPBYTES(output_buf, crossfade_chunk->size);
    output_buf = &output_buf[crossfade_sample];
    int32_t sample;

    while (length)
    {
        /* fade left and right channel at once to keep buffer alignment */
        int i;
        for (i = 0; i < 2; i++)
        {
            sample = *input_buf++;
            sample = ((sample * factor) >> 8) + *output_buf;
            *output_buf++ = clip_sample_16(sample);
        }

        length -= 4; /* 2 samples, each 16 bit -> 4 bytes */

        if (output_buf >= chunk_end)
        {
            crossfade_chunk = crossfade_chunk->link;
            if (!crossfade_chunk)
                return length;
            output_buf = (int16_t *)crossfade_chunk->addr;
            chunk_end = SKIPBYTES(output_buf, crossfade_chunk->size);
        }
    }
    crossfade_sample = output_buf - (int16_t *)crossfade_chunk->addr;
    return 0;
}

/* Perform fade-in of new track */
static void write_to_crossfade(char *buf, size_t length)
{
    if (length)
    {
        if (crossfade_fade_in_rem)
        {
            size_t samples;
            int16_t *input_buf;

            /* Fade factor for this packet */
            int factor =
                ((crossfade_fade_in_total - crossfade_fade_in_rem) << 8) /
                crossfade_fade_in_total;
            /* Bytes to fade */
            size_t fade_rem = MIN(length, crossfade_fade_in_rem);

            /* We _will_ fade this many bytes */
            crossfade_fade_in_rem -= fade_rem;

            if (crossfade_chunk)
            {
                /* Mix the data */
                size_t fade_total = fade_rem;
                fade_rem = crossfade_mix(factor, buf, fade_rem);
                length -= fade_total - fade_rem;
                buf += fade_total - fade_rem;
                if (!length)
                    return;
            }

            samples = fade_rem / 2;
            input_buf = (int16_t *)buf;
            /* Fade remaining samples in place */
            while (samples--)
            {
                int32_t sample = *input_buf;
                *input_buf++ = (sample * factor) >> 8;
            }
        }

        if (crossfade_chunk)
        {
            /* Mix the data */
            size_t mix_total = length;
            /* A factor of 256 means no fading */
            length = crossfade_mix(256, buf, length);
            buf += mix_total - length;
            if (!length)
                return;
        }

        /* Commit samples to the buffer */
        while (!prepare_insert(length))
            sleep(1);
        while (length > 0)
        {
            COMMIT_IF_NEEDED;
            size_t pcmbuffer_index = pcmbuffer_pos + pcmbuffer_fillpos;
            size_t copy_n = MIN(length, pcmbuf_size - pcmbuffer_index);
            memcpy(&pcmbuffer[pcmbuffer_index], buf, copy_n);
            buf += copy_n;
            pcmbuffer_fillpos += copy_n;
            length -= copy_n;
        }
    }
    /* if no more fading-in to do, stop the crossfade */
    if (!(crossfade_fade_in_rem || crossfade_chunk))
        crossfade_active = false;
}

static void pcmbuf_finish_crossfade_enable(void)
{
    /* Copy the pending setting over now */
    crossfade_enabled = crossfade_enable_request;
    
    pcmbuf_watermark = (crossfade_enabled && pcmbuf_size) ?
        /* If crossfading, try to keep the buffer full other than 1 second */
        (pcmbuf_size - (NATIVE_FREQUENCY * 4 * 1)) :
        /* Otherwise, just use the default */
        PCMBUF_WATERMARK;
}

bool pcmbuf_is_crossfade_active(void)
{
    return crossfade_active || crossfade_track_change_started;
}

void pcmbuf_request_crossfade_enable(bool on_off)
{
    /* Next setting to be used, not applied now */
    crossfade_enable_request = on_off;
}

bool pcmbuf_is_same_size(void)
{
    /* if pcmbuffer is NULL, then not set up yet even once so always */
    bool same_size = pcmbuffer ?
        (get_next_required_pcmbuf_size() == pcmbuf_size) : true;
    
    /* no buffer change needed, so finish crossfade setup now */
    if (same_size)
        pcmbuf_finish_crossfade_enable();
    
    return same_size;
}
#endif /* HAVE_CROSSFADE */


/** Voice */

/* Returns pcm buffer usage in percents (0 to 100). */
static int pcmbuf_usage(void)
{
    return pcmbuf_unplayed_bytes * 100 / pcmbuf_size;
}

static int pcmbuf_mix_free(void)
{
    if (mix_chunk)
    {
        size_t my_mix_end =
            (size_t)&((int16_t *)mix_chunk->addr)[pcmbuf_mix_sample];
        size_t my_write_pos = (size_t)&pcmbuffer[pcmbuffer_pos];
        if (my_write_pos < my_mix_end)
            my_write_pos += pcmbuf_size;
        return (my_write_pos - my_mix_end) * 100 / pcmbuf_unplayed_bytes;
    }
    return 100;
}

void *pcmbuf_request_voice_buffer(int *count)
{
    /* A get-it-to-work-for-now hack (audio status could change by
       completion) */
    if (audio_status() & AUDIO_STATUS_PLAY)
    {
        if (read_chunk == NULL)
        {
            return NULL;
        }
        else if (pcmbuf_usage() >= 10 && pcmbuf_mix_free() >= 30 &&
                 (mix_chunk || read_chunk->link))
        {
            *count = MIN(*count, PCMBUF_MIX_CHUNK/4);
            return voicebuf;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return pcmbuf_request_buffer(count);
    }
}

void pcmbuf_write_voice_complete(int count)
{
    /* A get-it-to-work-for-now hack (audio status could have changed) */
    if (!(audio_status() & AUDIO_STATUS_PLAY))
    {
        pcmbuf_write_complete(count);
        return;
    }

    int16_t *ibuf = (int16_t *)voicebuf;
    int16_t *obuf;
    size_t chunk_samples;

    if (mix_chunk == NULL && read_chunk != NULL)
    {
        mix_chunk = read_chunk->link;
        /* Start 1/8s into the next chunk */
        pcmbuf_mix_sample = NATIVE_FREQUENCY * 4 / 16;
    }

    if (!mix_chunk)
        return;

    obuf = (int16_t *)mix_chunk->addr;
    chunk_samples = mix_chunk->size / sizeof (int16_t);

    count <<= 1;

    while (count-- > 0)
    {
        int32_t sample = *ibuf++;

        if (pcmbuf_mix_sample >= chunk_samples)
        {
            mix_chunk = mix_chunk->link;
            if (!mix_chunk)
                return;
            pcmbuf_mix_sample = 0;
            obuf = mix_chunk->addr;
            chunk_samples = mix_chunk->size / 2;
        }
        sample += obuf[pcmbuf_mix_sample] >> 2;
        obuf[pcmbuf_mix_sample++] = clip_sample_16(sample);
    }
}


/** Debug menu, other metrics */

/* Amount of bytes left in the buffer. */
size_t pcmbuf_free(void)
{
    if (read_chunk != NULL)
    {
        void *read = read_chunk->addr;
        void *write = &pcmbuffer[pcmbuffer_pos + pcmbuffer_fillpos];
        if (read < write)
            return (size_t)(read - write) + pcmbuf_size;
        else
            return (size_t) (read - write);
    }
    return pcmbuf_size - pcmbuffer_fillpos;
}

size_t pcmbuf_get_bufsize(void)
{
    return pcmbuf_size;
}

int pcmbuf_used_descs(void)
{
    struct chunkdesc *temp = read_chunk;
    unsigned int i = 0;
    while (temp) {
        temp = temp->link;
        i++;
    }
    return i;
}

int pcmbuf_descs(void)
{
    return NUM_CHUNK_DESCS(pcmbuf_size);
}

#ifdef ROCKBOX_HAS_LOGF
unsigned char *pcmbuf_get_meminfo(size_t *length)
{
    *length = pcmbuf_bufend - pcmbuffer;
    return pcmbuffer;
}
#endif


/** Misc */

bool pcmbuf_is_lowdata(void)
{
    if (!pcm_is_playing() || pcm_is_paused()
#ifdef HAVE_CROSSFADE
        || pcmbuf_is_crossfade_active()
#endif
        )
        return false;

#if MEMORYSIZE > 2
    /* 1 seconds of buffer is low data */
    return LOW_DATA(4);
#else
    /* under watermark is low data */
    return (pcmbuf_unplayed_bytes < pcmbuf_watermark);
#endif
}

void pcmbuf_set_low_latency(bool state)
{
    low_latency_mode = state;
}

unsigned long pcmbuf_get_latency(void)
{
    /* Be careful how this calculation is rearranged, it's easy to overflow */
    size_t bytes = pcmbuf_unplayed_bytes + pcm_get_bytes_waiting();
    return bytes / 4 * 1000 / NATIVE_FREQUENCY;
}

#ifndef HAVE_HARDWARE_BEEP
#define MINIBUF_SAMPLES (NATIVE_FREQUENCY / 1000 * KEYCLICK_DURATION)
#define MINIBUF_SIZE (MINIBUF_SAMPLES*4)

/* Generates a constant square wave sound with a given frequency
   in Hertz for a duration in milliseconds. */
void pcmbuf_beep(unsigned int frequency, size_t duration, int amplitude)
{
    unsigned int step = 0xffffffffu / NATIVE_FREQUENCY * frequency;
    int32_t phase = 0;
    int16_t *bufptr, *bufstart, *bufend;
    int32_t sample;
    int nsamples = NATIVE_FREQUENCY / 1000 * duration;
    bool mix = read_chunk != NULL && read_chunk->link != NULL;
    int i;

    bufend = SKIPBYTES((int16_t *)pcmbuffer, pcmbuf_size);

    /* Find the insertion point and set bufstart to the start of it */
    if (mix)
    {
        /* Get the currently playing chunk at the current position. */
        bufstart = (int16_t *)pcm_play_dma_get_peak_buffer(&i);

        /* If above isn't implemented or pcm is stopped, no beepeth. */
        if (!bufstart || !pcm_is_playing())
            return;

        /* Give 5ms clearance. */    
        bufstart += NATIVE_FREQUENCY * 4 / 200;

#ifdef HAVE_PCM_DMA_ADDRESS
        /* Returned peak addresses are DMA addresses */
        bufend = pcm_dma_addr(bufend);
#endif

        /* Wrapped above? */
        if (bufstart >= bufend)
            bufstart -= pcmbuf_size;

        /* NOTE: On some targets using hardware DMA, cache range flushing may
         * be required or the writes may not be picked up by the controller.
         * An incremental flush should be done periodically during the mixdown. */
    }
    else if (nsamples <= MINIBUF_SAMPLES)
    {
        static int16_t minibuf[MINIBUF_SAMPLES*2] __attribute__((aligned(4)));
        /* Use mini buffer */
        bufstart = minibuf;
        bufend = SKIPBYTES(bufstart, MINIBUF_SIZE);
    }
    else if (audio_buffer_state() != AUDIOBUF_STATE_TRASHED)
    {
        /* Use pcmbuffer */
        bufstart = (int16_t *)pcmbuffer;
    }
    else
    {
        /* No place */
        return;
    }

    bufptr = bufstart;

    /* Mix square wave into buffer */
    for (i = 0; i < nsamples; ++i)
    {
        int32_t amp = (phase >> 31) ^ (int32_t)amplitude;
        sample = mix ? *bufptr : 0;
        *bufptr++ = clip_sample_16(sample + amp);
        if (bufptr >= bufend)
            bufptr = (int16_t *)pcmbuffer;
        sample = mix ? *bufptr : 0;
        *bufptr++ = clip_sample_16(sample + amp);
        if (bufptr >= bufend)
            bufptr = (int16_t *)pcmbuffer;

        phase += step;
    }

    pcm_play_lock();
#ifdef HAVE_RECORDING
    pcm_rec_lock();
#endif

    /* Kick off playback if required and it won't interfere */
    if (!pcm_is_playing()
#ifdef HAVE_RECORDING
        && !pcm_is_recording()
#endif
        )
    {
        pcm_play_data(NULL, (unsigned char *)bufstart, nsamples * 4);
    }

    pcm_play_unlock();
#ifdef HAVE_RECORDING
    pcm_rec_unlock();
#endif
}
#endif /* HAVE_HARDWARE_BEEP */
