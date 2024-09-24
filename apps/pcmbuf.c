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
 * Copyright (C) 2011 by Michael Sevakis
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
#include "pcm.h"
#include "pcm_mixer.h"
#include "pcmbuf.h"
#include "dsp-util.h"
#include "playback.h"
#include "codec_thread.h"

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#include "cpu.h"
#endif
#include "settings.h"
#include "audio.h"
#include "voice_thread.h"

/* 2 channels * 2 bytes/sample, interleaved */
#define PCMBUF_SAMPLE_SIZE   (2 * 2)

/* This is the target fill size of chunks on the pcm buffer
   Can be any number of samples but power of two sizes make for faster and
   smaller math - must be < 65536 bytes */
#define PCMBUF_CHUNK_SIZE    8192u

/* Small guard buf to give decent space near end */
#define PCMBUF_GUARD_SIZE    (PCMBUF_CHUNK_SIZE / 8)

/* Mnemonics for common data commit thresholds */
#define COMMIT_CHUNKS        PCMBUF_CHUNK_SIZE
#define COMMIT_ALL_DATA      1u

/* Size of the crossfade buffer where codec data is written to be faded
   on commit */
#define CROSSFADE_BUFSIZE    PCMBUF_CHUNK_SIZE

/* Maximum contiguous space that PCM buffer will allow (to avoid excessive
   draining between inserts and observe low-latency mode) */
#define PCMBUF_MAX_BUFFER    (PCMBUF_CHUNK_SIZE * 4)

/* Forced buffer insert constraint can thus be from 1KB to 32KB using 8KB
   chunks */

/* Return data level in 1/4-second increments */
#define DATA_LEVEL(quarter_secs) (pcmbuf_sampr * (quarter_secs))

/* Number of bytes played per second */
#define BYTERATE            (pcmbuf_sampr * PCMBUF_SAMPLE_SIZE)

#if MEMORYSIZE > 2
/* Keep watermark high for large memory target - at least (2s) */
#define PCMBUF_WATERMARK    (BYTERATE * 2)
#define MIN_BUFFER_SIZE     (BYTERATE * 3)
/* 1 seconds of buffer is low data */
#define LOW_DATA            DATA_LEVEL(4)
#else
#define PCMBUF_WATERMARK    (BYTERATE / 4)  /* 0.25 seconds */
#define MIN_BUFFER_SIZE     (BYTERATE * 1)
/* under watermark is low data */
#define LOW_DATA            pcmbuf_watermark
#endif

/* Describes each audio packet - keep it small since there are many of them */
struct chunkdesc
{
    uint16_t       size;    /* Actual size (0 < size <= PCMBUF_CHUNK_SIZE) */
    uint16_t       pos_key; /* Who put the position info in
                               (undefined: 0, valid: 1..POSITION_KEY_MAX) */
    unsigned long  elapsed; /* Elapsed time to use */
    off_t          offset;  /* Offset to use */
};

#define POSITION_KEY_MAX    UINT16_MAX


/* General PCM buffer data */
#define INVALID_BUF_INDEX   ((size_t)0 - (size_t)1)

static void *pcmbuf_buffer;
static void *pcmbuf_guardbuf;
static size_t pcmbuf_size;
static struct chunkdesc *pcmbuf_descriptors;
static unsigned int pcmbuf_desc_count;
static unsigned int position_key = 1;
static unsigned int pcmbuf_sampr = 0;

static size_t chunk_ridx;
static size_t chunk_widx;

static size_t pcmbuf_bytes_waiting;
static struct chunkdesc *current_desc;
static size_t chunk_transidx;

static size_t pcmbuf_watermark = 0;

static bool low_latency_mode = false;

static bool pcmbuf_sync_position = false;

/* Fade effect */
static unsigned int fade_vol = MIX_AMP_UNITY;
static enum
{
    PCM_NOT_FADING = 0,
    PCM_FADING_IN,
    PCM_FADING_OUT,
} fade_state = PCM_NOT_FADING;
static bool fade_out_complete = false;

/* Voice */
static bool soft_mode = false;

#ifdef HAVE_CROSSFADE
/* Crossfade related state */

static int  crossfade_setting;
static int  crossfade_enable_request;

static enum
{
    CROSSFADE_INACTIVE = 0, /* Crossfade is OFF */
    CROSSFADE_ACTIVE,       /* Crossfade is fading in */
    CROSSFADE_START,        /* New crossfade is starting */
    CROSSFADE_CONTINUE,     /* Next track continues fade */
} crossfade_status = CROSSFADE_INACTIVE;

static bool crossfade_mixmode;
static bool crossfade_auto_skip;
static size_t crossfade_widx;
static size_t crossfade_bufidx;

struct mixfader
{
    int32_t factor; /* Current volume factor to use */
    int32_t endfac; /* Saturating end factor */
    int32_t nsamp2; /* Twice the number of samples */
    int32_t dfact2; /* Twice the range of factors */
    int32_t ferr;   /* Current error accumulator */
    int32_t dfquo;  /* Quotient of fade range / sample range */
    int32_t dfrem;  /* Remainder of fade range / sample range */
    int32_t dfinc;  /* Base increment (-1 or +1) */
    bool    alloc;  /* Allocate blocks if needed else abort at EOB */
} crossfade_infader;

/* Defines for operations on position info when mixing/fading -
   passed in offset parameter */
enum
{
    MIXFADE_KEEP_POS = -1,    /* Keep position info in chunk */
    MIXFADE_NULLIFY_POS = -2, /* Ignore position info in chunk */
    /* Positive values cause stamping/restamping */
};

#define MIXFADE_UNITY_BITS  16
#define MIXFADE_UNITY       (1 << MIXFADE_UNITY_BITS)

static void crossfade_cancel(void);
static void crossfade_start(void);
static void write_to_crossfade(size_t size, unsigned long elapsed,
                               off_t offset);
static void pcmbuf_finish_crossfade_enable(void);
#else
#define crossfade_cancel() do {} while(0)
#endif /* HAVE_CROSSFADE */

/* Thread */
#ifdef HAVE_PRIORITY_SCHEDULING
static int codec_thread_priority = PRIORITY_PLAYBACK;
#endif

/* Callbacks into playback.c */
extern void audio_pcmbuf_position_callback(unsigned long elapsed,
                                           off_t offset, unsigned int key);
extern void audio_pcmbuf_track_change(bool pcmbuf);
extern bool audio_pcmbuf_may_play(void);
extern void audio_pcmbuf_sync_position(void);


/**************************************/

/* start PCM if callback says it's alright */
static void start_audio_playback(void)
{
     if (audio_pcmbuf_may_play())
        pcmbuf_play_start();
}

/* Return number of commited bytes in buffer (committed chunks count as
   a full chunk even if only partially filled) */
static size_t pcmbuf_unplayed_bytes(void)
{
    size_t ridx = chunk_ridx;
    size_t widx = chunk_widx;

    if (ridx > widx)
        widx += pcmbuf_size;

    return widx - ridx;
}

/* Returns TRUE if amount of data is under the target fill size */
static bool pcmbuf_data_critical(void)
{
    return pcmbuf_unplayed_bytes() < LOW_DATA;
}

/* Return the next PCM chunk in the PCM buffer given a byte index into it */
static size_t index_next(size_t index)
{
    index = ALIGN_DOWN(index + PCMBUF_CHUNK_SIZE, PCMBUF_CHUNK_SIZE);

    if (index >= pcmbuf_size)
        index -= pcmbuf_size;

    return index;
}

/* Convert a byte offset in the PCM buffer into a pointer in the buffer */
static FORCE_INLINE void * index_buffer(size_t index)
{
    return pcmbuf_buffer + index;
}

/* Convert a pointer in the buffer into an index offset */
static FORCE_INLINE size_t buffer_index(void *p)
{
    return (uintptr_t)p - (uintptr_t)pcmbuf_buffer;
}

/* Return a chunk descriptor for a byte index in the buffer */
static struct chunkdesc * index_chunkdesc(size_t index)
{
    return &pcmbuf_descriptors[index / PCMBUF_CHUNK_SIZE];
}

/* Return the first byte of a chunk for a byte index in the buffer, offset by 'offset'
   chunks */
static size_t index_chunk_offs(size_t index, int offset)
{
    int i = index / PCMBUF_CHUNK_SIZE;

    if (offset != 0)
    {
        i = (i + offset) % (int)pcmbuf_desc_count;

        /* remainder => modulus */
        if (i < 0)
            i += pcmbuf_desc_count;
    }

    return i * PCMBUF_CHUNK_SIZE;
}

/* Test if a buffer index lies within the committed data region */
static bool index_committed(size_t index)
{
    if (index == INVALID_BUF_INDEX)
        return false;

    size_t ridx = chunk_ridx;
    size_t widx = chunk_widx;

    if (widx < ridx)
    {
        widx += pcmbuf_size;

        if (index < ridx)
            index += pcmbuf_size;
    }

    return index >= ridx && index < widx;
}

/* Snip the tail of buffer at chunk of specified index plus chunk offset */
void snip_buffer_tail(size_t index, int offset)
{
    /* Call with PCM lockout */
    if (index == INVALID_BUF_INDEX)
        return;

    index = index_chunk_offs(index, offset);

    if (!index_committed(index) && index != chunk_widx)
        return;

    chunk_widx = index;
    pcmbuf_bytes_waiting = 0;
    index_chunkdesc(index)->pos_key = 0;

#ifdef HAVE_CROSSFADE
    /* Kill crossfade if it would now be operating in the void */
    if (crossfade_status != CROSSFADE_INACTIVE &&
        !index_committed(crossfade_widx) && crossfade_widx != chunk_widx)
    {
        crossfade_cancel();
    }
#endif /* HAVE_CROSSFADE */
}


/** Accept new PCM data */

/* Split the uncommitted data as needed into chunks, stopping when uncommitted
   data is below the threshold */
static void commit_chunks(size_t threshold)
{
    size_t index = chunk_widx;
    size_t end_index = index + pcmbuf_bytes_waiting;

    /* Copy to the beginning of the buffer all data that must wrap */
    if (end_index > pcmbuf_size)
        memcpy(pcmbuf_buffer, pcmbuf_guardbuf, end_index - pcmbuf_size);

    struct chunkdesc *desc = index_chunkdesc(index);

    do
    {
        size_t size = MIN(pcmbuf_bytes_waiting, PCMBUF_CHUNK_SIZE);
        pcmbuf_bytes_waiting -= size;

        /* Fill in the values in the new buffer chunk */
        desc->size = (uint16_t)size;

        /* Advance the current write chunk and make it available to the
           PCM callback */
        chunk_widx = index = index_next(index);
        desc = index_chunkdesc(index);

        /* Reset it before using it */
        desc->pos_key = 0;
    }
    while (pcmbuf_bytes_waiting >= threshold);
}

/* If uncommitted data count is above or equal to the threshold, commit it */
static FORCE_INLINE void commit_if_needed(size_t threshold)
{
    if (pcmbuf_bytes_waiting >= threshold)
        commit_chunks(threshold);
}

/* Place positioning information in the chunk */
static void stamp_chunk(struct chunkdesc *desc, unsigned long elapsed,
                        off_t offset)
{
    /* One-time stamping of a given chunk by the same track - new track may
       overwrite */
    unsigned int key = position_key;

    if (desc->pos_key != key)
    {
        desc->pos_key = key;
        desc->elapsed = elapsed;
        desc->offset = offset;
    }
}

/* Set priority of the codec thread */
#ifdef HAVE_PRIORITY_SCHEDULING
/*
 * expects pcm_fill_state in tenth-% units (e.g. full pcm buffer is 10) */
static void boost_codec_thread(int pcm_fill_state)
{
    static const int8_t prios[11] =
    {
        PRIORITY_PLAYBACK_MAX,      /*   0 - 10% */
        PRIORITY_PLAYBACK_MAX+1,    /*  10 - 20% */
        PRIORITY_PLAYBACK_MAX+3,    /*  20 - 30% */
        PRIORITY_PLAYBACK_MAX+5,    /*  30 - 40% */
        PRIORITY_PLAYBACK_MAX+7,    /*  40 - 50% */
        PRIORITY_PLAYBACK_MAX+8,    /*  50 - 60% */
        PRIORITY_PLAYBACK_MAX+9,    /*  60 - 70% */
        /* raising priority above 70% shouldn't be needed */
        PRIORITY_PLAYBACK,          /*  70 - 80% */
        PRIORITY_PLAYBACK,          /*  80 - 90% */
        PRIORITY_PLAYBACK,          /*  90 -100% */
        PRIORITY_PLAYBACK,          /*      100% */
    };

    int new_prio = prios[pcm_fill_state];

    /* Keep voice and codec threads at the same priority or else voice
     * will starve if the codec thread's priority is boosted. */
    if (new_prio != codec_thread_priority)
    {
        codec_thread_set_priority(new_prio);
        voice_thread_set_priority(new_prio);
        codec_thread_priority = new_prio;
    }
}
#else
#define boost_codec_thread(pcm_fill_state) do{}while(0)
#endif /* HAVE_PRIORITY_SCHEDULING */

/* Get the next available buffer and size - assumes adequate space exists */
static void * get_write_buffer(size_t *size)
{
    /* Obtain current chunk fill address */
    size_t index = chunk_widx + pcmbuf_bytes_waiting;
    size_t index_end = pcmbuf_size + PCMBUF_GUARD_SIZE;

    /* Get count to the end of the buffer where a wrap will happen +
       the guard */
    size_t endsize = index_end - index;

    /* Return available unwrapped space */
    *size = MIN(*size, endsize);

    return index_buffer(index);
}

/* Commit outstanding data leaving less than a chunk size remaining */
static void commit_write_buffer(size_t size)
{
    /* Add this data and commit if one or more chunks are ready */
    pcmbuf_bytes_waiting += size;
    commit_if_needed(COMMIT_CHUNKS);
}

/* Request space in the buffer for writing output samples */
void * pcmbuf_request_buffer(int *count)
{
    size_t size = *count * PCMBUF_SAMPLE_SIZE;

#ifdef HAVE_CROSSFADE
    /* We're going to crossfade to a new track, which is now on its way */
    if (crossfade_status > CROSSFADE_ACTIVE)
        crossfade_start();

    /* If crossfade has begun, put the new track samples in the crossfade
       buffer area */
    if (crossfade_status != CROSSFADE_INACTIVE && size > CROSSFADE_BUFSIZE)
        size = CROSSFADE_BUFSIZE;
    else
#endif /* HAVE_CROSSFADE */
    if (size > PCMBUF_MAX_BUFFER)
        size = PCMBUF_MAX_BUFFER; /* constrain request */

    enum channel_status status = mixer_channel_status(PCM_MIXER_CHAN_PLAYBACK);
    size_t remaining = pcmbuf_unplayed_bytes();

    /* Need to have length bytes to prevent wrapping overwriting - leave one
       descriptor free to guard so that 0 != full in ring buffer */
    size_t freespace = pcmbuf_free();

    if (pcmbuf_sync_position)
        audio_pcmbuf_sync_position();

    if (freespace < size + PCMBUF_CHUNK_SIZE)
        return NULL;

    /* Maintain the buffer level above the watermark */
    if (status != CHANNEL_STOPPED)
    {
        if (low_latency_mode)
        {
            /* 1/4s latency. */
            if (remaining > DATA_LEVEL(1))
                return NULL;
        }

        /* Boost CPU if necessary */
        size_t realrem = pcmbuf_size - freespace;

        if (realrem < pcmbuf_watermark)
            trigger_cpu_boost();

        boost_codec_thread(realrem*10 / pcmbuf_size);
    }
    else    /* !playing */
    {
        /* Boost CPU for pre-buffer */
        trigger_cpu_boost();

        /* If pre-buffered to the watermark, start playback */
        if (!pcmbuf_data_critical())
            start_audio_playback();
    }

    void *buf;

#ifdef HAVE_CROSSFADE
    if (crossfade_status != CROSSFADE_INACTIVE)
    {
        crossfade_bufidx = index_chunk_offs(chunk_ridx, -1);
        buf = index_buffer(crossfade_bufidx); /* always CROSSFADE_BUFSIZE */
    }
    else
#endif
    {
        /* Give the maximum amount available if there's more */
        if (size + PCMBUF_CHUNK_SIZE < freespace)
            size = freespace - PCMBUF_CHUNK_SIZE;

        buf = get_write_buffer(&size);
    }

    *count = size / PCMBUF_SAMPLE_SIZE;
    return buf;
}

/* Handle new samples to the buffer */
void pcmbuf_write_complete(int count, unsigned long elapsed, off_t offset)
{
    size_t size = count * PCMBUF_SAMPLE_SIZE;

#ifdef HAVE_CROSSFADE
    if (crossfade_status != CROSSFADE_INACTIVE)
    {
        write_to_crossfade(size, elapsed, offset);
    }
    else
#endif
    {
        stamp_chunk(index_chunkdesc(chunk_widx), elapsed, offset);
        commit_write_buffer(size);
    }

    /* Revert to position updates by PCM */
    pcmbuf_sync_position = false;
}


/** Init */
static unsigned int get_next_required_pcmbuf_chunks(void)
{
    size_t size = MIN_BUFFER_SIZE;

#ifdef HAVE_CROSSFADE
    if (crossfade_enable_request != CROSSFADE_ENABLE_OFF)
    {
        size_t seconds = global_settings.crossfade_fade_out_delay +
                         global_settings.crossfade_fade_out_duration;
        size += seconds * BYTERATE;
    }
#endif

    logf("pcmbuf len: %lu", (unsigned long)(size / BYTERATE));
    return size / PCMBUF_CHUNK_SIZE;
}

/* Initialize the ringbuffer state */
static void init_buffer_state(void)
{
    /* Reset counters */
    chunk_ridx = chunk_widx = 0;
    pcmbuf_bytes_waiting = 0;

    /* Reset first descriptor */
    if (pcmbuf_descriptors)
        pcmbuf_descriptors->pos_key = 0;

    /* Clear change notification */
    chunk_transidx = INVALID_BUF_INDEX;
}

/* call prior to init to get bytes required */
size_t pcmbuf_size_reqd(void)
{
    return get_next_required_pcmbuf_chunks() * PCMBUF_CHUNK_SIZE;
}

/* Initialize the PCM buffer. The structure looks like this:
 * ...|---------PCMBUF---------|GUARDBUF|DESCS| */
size_t pcmbuf_init(void *bufend)
{
    void *bufstart;

    /* Set up the buffers */
    pcmbuf_desc_count = get_next_required_pcmbuf_chunks();
    pcmbuf_size = pcmbuf_desc_count * PCMBUF_CHUNK_SIZE;
    pcmbuf_descriptors = (struct chunkdesc *)bufend - pcmbuf_desc_count;

    pcmbuf_buffer = (void *)pcmbuf_descriptors -
                    pcmbuf_size - PCMBUF_GUARD_SIZE;

    /* Mem-align buffer chunks for more efficient handling in lower layers */
    pcmbuf_buffer = ALIGN_DOWN(pcmbuf_buffer, (uintptr_t)MEM_ALIGN_SIZE);

    pcmbuf_guardbuf = pcmbuf_buffer + pcmbuf_size;
    bufstart = pcmbuf_buffer;

#ifdef HAVE_CROSSFADE
    pcmbuf_finish_crossfade_enable();
#else 
    pcmbuf_watermark = PCMBUF_WATERMARK;
#endif /* HAVE_CROSSFADE */

    init_buffer_state();

    pcmbuf_soft_mode(false);

    return bufend - bufstart;
}


/** Track change */

/* Place a track change notification in a specific descriptor or post it
   immediately if the buffer is empty or the index is invalid */
static void pcmbuf_monitor_track_change_ex(size_t index)
{
    /* Call with PCM lockout */
    if (chunk_ridx != chunk_widx && index != INVALID_BUF_INDEX)
    {
        /* If monitoring, set flag for one previous to specified chunk */
        index = index_chunk_offs(index, -1);

        /* Ensure PCM playback hasn't already played this out */
        if (index_committed(index))
        {
            chunk_transidx = index;
            return;
        }
    }

    /* Post now if buffer is no longer coming up */
    chunk_transidx = INVALID_BUF_INDEX;
    audio_pcmbuf_track_change(false);
}

/* Clear end of track and optionally the positioning info for all data */
static void pcmbuf_cancel_track_change(bool position)
{
    /* Call with PCM lockout */
    snip_buffer_tail(chunk_transidx, 1);

    chunk_transidx = INVALID_BUF_INDEX;

    if (!position)
        return;

    size_t index = chunk_ridx;

    while (1)
    {
        index_chunkdesc(index)->pos_key = 0;

        if (index == chunk_widx)
            break;

        index = index_next(index);
    }
}

/* Place a track change notification at the end of the buffer or post it
   immediately if the buffer is empty */
void pcmbuf_monitor_track_change(bool monitor)
{
    pcm_play_lock();

    if (monitor)
        pcmbuf_monitor_track_change_ex(chunk_widx);
    else
        pcmbuf_cancel_track_change(false);

    pcm_play_unlock();
}

void pcmbuf_start_track_change(enum pcm_track_change_type type)
{
    /* Commit all outstanding data before starting next track - tracks don't
       comingle inside a single buffer chunk */
    commit_if_needed(COMMIT_ALL_DATA);

    if (type == TRACK_CHANGE_AUTO_PILEUP)
    {
        /* Fill might not have been above watermark */
        start_audio_playback();
        return;
    }

#ifdef HAVE_CROSSFADE
    bool crossfade = false;
#endif
    bool auto_skip = type != TRACK_CHANGE_MANUAL;

    /* Update position key so that:
       1) Positions are keyed to the track to which they belong for sync
          purposes

       2) Buffers stamped with the outgoing track's positions are restamped
          to the incoming track's positions when crossfading
    */
    if (++position_key > POSITION_KEY_MAX)
        position_key = 1;

    if (type == TRACK_CHANGE_END_OF_DATA)
    {
        crossfade_cancel();

        /* Fill might not have been above watermark */
        start_audio_playback();
    }
#ifdef HAVE_CROSSFADE
    /* Determine whether this track change needs to crossfaded and how */
    else if (crossfade_setting != CROSSFADE_ENABLE_OFF)
    {
        if (crossfade_status == CROSSFADE_INACTIVE &&
            pcmbuf_unplayed_bytes() >= DATA_LEVEL(2) &&
            !low_latency_mode)
        {
            switch (crossfade_setting)
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
    }

    if (crossfade)
    {
        logf("crossfade track change");

        /* Don't enable mix mode when skipping tracks manually */
        crossfade_mixmode = auto_skip &&
            global_settings.crossfade_fade_out_mixmode;

        crossfade_auto_skip = auto_skip;

        crossfade_status = CROSSFADE_START;

        pcmbuf_monitor_track_change(auto_skip);

        trigger_cpu_boost();
    }
    else
#endif /* HAVE_CROSSFADE */
    if (auto_skip)
    {        
        /* The codec is moving on to the next track, but the current track will
         * continue to play, so mark the last write chunk as the last one in
         * the track */
        logf("gapless track change");

#ifdef HAVE_CROSSFADE
        if (crossfade_status == CROSSFADE_ACTIVE)
            crossfade_status = CROSSFADE_CONTINUE;
#endif

        pcmbuf_monitor_track_change(true);
    }
    else
    {
        /* Discard old data; caller needs no transition notification */
        logf("manual track change");
        pcmbuf_play_stop();
    }
}


/** Playback */

/* PCM driver callback */
static void pcmbuf_pcm_callback(const void **start, size_t *size)
{
    /*- Process the chunk that just finished -*/
    size_t index = chunk_ridx;
    struct chunkdesc *desc = current_desc;

    if (desc)
    {
        /* If last chunk in the track, notify of track change */
        if (index == chunk_transidx)
        {
            chunk_transidx = INVALID_BUF_INDEX;
            audio_pcmbuf_track_change(true);
        }

        /* Free it for reuse */
        chunk_ridx = index = index_next(index);
    }

    /*- Process the new one -*/
    if (index != chunk_widx && !fade_out_complete)
    {
        current_desc = desc = index_chunkdesc(index);

        *start = index_buffer(index);
        *size = desc->size;

        if (desc->pos_key != 0)
        {
            /* Positioning chunk - notify playback */
            audio_pcmbuf_position_callback(desc->elapsed, desc->offset,
                                           desc->pos_key);
        }
    }
}

/* Force playback */
void pcmbuf_play_start(void)
{
    logf("pcmbuf_play_start");

    if (mixer_channel_status(PCM_MIXER_CHAN_PLAYBACK) == CHANNEL_STOPPED &&
        chunk_widx != chunk_ridx)
    {
        current_desc = NULL;
        mixer_channel_play_data(PCM_MIXER_CHAN_PLAYBACK, pcmbuf_pcm_callback,
                                NULL, 0);
    }
}

/* Stop channel, empty and reset buffer */
void pcmbuf_play_stop(void)
{
    logf("pcmbuf_play_stop");

    /* Reset channel */
    mixer_channel_stop(PCM_MIXER_CHAN_PLAYBACK);

    /* Reset buffer */
    init_buffer_state();

    /* Revert to position updates by PCM */
    pcmbuf_sync_position = false;

    /* Fader OFF */
    crossfade_cancel();

    /* Can unboost the codec thread here no matter who's calling,
     * pretend full pcm buffer to unboost */
    boost_codec_thread(10);
}

void pcmbuf_pause(bool pause)
{
    logf("pcmbuf_pause: %s", pause?"pause":"play");

    if (mixer_channel_status(PCM_MIXER_CHAN_PLAYBACK) != CHANNEL_STOPPED)
        mixer_channel_play_pause(PCM_MIXER_CHAN_PLAYBACK, !pause);
    else if (!pause)
        pcmbuf_play_start();
}


/** Crossfade */

#ifdef HAVE_CROSSFADE

/* Initialize a fader */
static void mixfader_init(struct mixfader *faderp, int32_t start_factor,
                          int32_t end_factor, size_t size, bool alloc)
{
    /* Linear fade */
    faderp->endfac = end_factor;
    faderp->nsamp2 = size / PCMBUF_SAMPLE_SIZE * 2;
    faderp->alloc  = alloc;

    if (faderp->nsamp2 == 0)
    {
        /* No data; set up as if fader finished the fade */
        faderp->factor = end_factor;
        return;
    }

    int32_t dfact2 = 2*abs(end_factor - start_factor);
    faderp->factor = start_factor;
    faderp->ferr   = dfact2 / 2;
    faderp->dfquo  = dfact2 / faderp->nsamp2;
    faderp->dfrem  = dfact2 - faderp->dfquo*faderp->nsamp2;
    faderp->dfinc  = end_factor < start_factor ? -1 : +1;
    faderp->dfquo *= faderp->dfinc;
}

/* Query if the fader has finished its envelope */
static inline bool mixfader_finished(const struct mixfader *faderp)
{
    return faderp->factor == faderp->endfac;
}

/* Step fader by one sample */
static inline void mixfader_step(struct mixfader *faderp)
{
    if (mixfader_finished(faderp))
        return;

    faderp->factor += faderp->dfquo;
    faderp->ferr += faderp->dfrem;

    if (faderp->ferr >= faderp->nsamp2)
    {
        faderp->factor += faderp->dfinc;
        faderp->ferr -= faderp->nsamp2;
    }
}

static FORCE_INLINE int32_t mixfade_sample(const struct mixfader *faderp, int32_t s)
{
    return (faderp->factor * s + MIXFADE_UNITY/2) >> MIXFADE_UNITY_BITS;
}

/* Cancel crossfade operation */
static void crossfade_cancel(void)
{
    crossfade_status = CROSSFADE_INACTIVE;
    crossfade_widx = INVALID_BUF_INDEX;
}

/* Find the buffer index that's 'size' bytes away from 'index' */
static size_t crossfade_find_index(size_t index, size_t size)
{
    if (index != INVALID_BUF_INDEX)
    {
        size_t i = index_chunk_offs(index, 0);
        size += index - i;

        while (i != chunk_widx)
        {
            size_t desc_size = index_chunkdesc(i)->size;

            if (size < desc_size)
            {
                index = i + size;
                break;
            }

            size -= desc_size;
            i = index_next(i);
        }
    }

    return index;
}

/* Align the needed buffer area up to the end of existing data */
static size_t crossfade_find_buftail(bool auto_skip, size_t buffer_rem,
                                     size_t buffer_need, size_t *buffer_rem_outp)
{
    size_t index = chunk_ridx;

    if (buffer_rem > buffer_need)
    {
        size_t distance;

        if (auto_skip)
        {
            /* Automatic track changes only modify the last part of the buffer,
             * so find the right chunk and sample to start the crossfade */
            distance = buffer_rem - buffer_need;
            buffer_rem = buffer_need;
        }
        else
        {
            /* Manual skips occur immediately, but give 1/5s to process */
            distance = MIN(BYTERATE / 5, buffer_rem);
            buffer_rem -= distance;
        }

        index = crossfade_find_index(index, distance);
    }

    if (buffer_rem_outp)
        *buffer_rem_outp = buffer_rem;

    return index;
}

/* Run a fader on some buffers */
static void crossfade_mix_fade(struct mixfader *faderp, size_t size,
                               void *input_buf, size_t *out_index,
                               unsigned long elapsed, off_t offset)
{
    if (size == 0)
        return;

    size_t index = *out_index;

    if (index == INVALID_BUF_INDEX)
        return;

    int16_t *inbuf = input_buf;

    bool alloced = inbuf && faderp->alloc &&
                   index_chunk_offs(index, 0) == chunk_widx;

    while (size)
    {
        struct chunkdesc *desc = index_chunkdesc(index);
        int16_t *outbuf = index_buffer(index);

        switch (offset)
        {
        case MIXFADE_NULLIFY_POS:
            /* Stop position updates for the chunk */
            desc->pos_key = 0;
            break;
        case MIXFADE_KEEP_POS:
            /* Keep position info as it is */
            break;
        default:
            /* Replace position info */
            stamp_chunk(desc, elapsed, offset);
        }

        size_t amount = (alloced ? PCMBUF_CHUNK_SIZE : desc->size)
                            - (index % PCMBUF_CHUNK_SIZE);
        int16_t *chunkend = SKIPBYTES(outbuf, amount);

        if (size < amount)
            amount = size;

        size -= amount;

        if (alloced)
        {
            /* Fade the input buffer into the new destination chunk */
            for (size_t s = amount; s != 0; s -= PCMBUF_SAMPLE_SIZE)
            {
                *outbuf++ = mixfade_sample(faderp, *inbuf++);
                *outbuf++ = mixfade_sample(faderp, *inbuf++);
                mixfader_step(faderp);
            }

            commit_write_buffer(amount);
        }
        else if (inbuf)
        {
            /* Fade the input buffer and mix into the destination chunk */
            for (size_t s = amount; s != 0; s -= PCMBUF_SAMPLE_SIZE)
            {
                int32_t left  = outbuf[0];
                int32_t right = outbuf[1];
                left  += mixfade_sample(faderp, *inbuf++);
                right += mixfade_sample(faderp, *inbuf++);
                *outbuf++ = clip_sample_16(left);
                *outbuf++ = clip_sample_16(right);
                mixfader_step(faderp);
            }
        }
        else
        {
            /* Fade the chunk in place */
            for (size_t s = amount; s != 0; s -= PCMBUF_SAMPLE_SIZE)
            {
                int32_t left  = outbuf[0];
                int32_t right = outbuf[1];
                *outbuf++ = mixfade_sample(faderp, left);
                *outbuf++ = mixfade_sample(faderp, right);
                mixfader_step(faderp);
            }
        }

        if (outbuf < chunkend)
        {
            index += amount;
            continue;
        }

        /* Move destination to next chunk as needed */
        index = index_next(index);

        if (index == chunk_widx)
        {
            /* End of existing data */
            if (!inbuf || !faderp->alloc)
            {
                index = INVALID_BUF_INDEX;
                break;
            }

            alloced = true;
        }
    }

    *out_index = index;
}

/* Initializes crossfader, calculates all necessary parameters and performs
 * fade-out with the PCM buffer.  */
static void crossfade_start(void)
{
    logf("crossfade_start");

    pcm_play_lock();

    if (crossfade_status == CROSSFADE_CONTINUE)
    {
        logf("fade-in continuing");

        crossfade_status = CROSSFADE_ACTIVE;

        if (crossfade_auto_skip)
            pcmbuf_monitor_track_change_ex(crossfade_widx);

        pcm_play_unlock();
        return;
    }

    /* Initialize the crossfade buffer size to all of the buffered data that
     * has not yet been sent to the DMA */
    size_t unplayed = pcmbuf_unplayed_bytes();

    /* Reject crossfade if less than .5s of data */
    if (unplayed < DATA_LEVEL(2))
    {
        logf("crossfade rejected");
        crossfade_cancel();
        pcm_play_unlock();
        return;
    }

    /* Fading will happen */
    crossfade_status = CROSSFADE_ACTIVE;

    /* Get fade info from settings. */
    size_t fade_out_delay = global_settings.crossfade_fade_out_delay * BYTERATE;
    size_t fade_out_rem = global_settings.crossfade_fade_out_duration * BYTERATE;
    size_t fade_in_delay = global_settings.crossfade_fade_in_delay * BYTERATE;
    size_t fade_in_duration = global_settings.crossfade_fade_in_duration * BYTERATE;

    if (!crossfade_auto_skip)
    {
        /* Forego fade-in delay on manual skip - do the best to preserve auto skip
           relationship */
        fade_out_delay -= MIN(fade_out_delay, fade_in_delay);
        fade_in_delay = 0;
    }

    size_t fade_out_need = fade_out_delay + fade_out_rem;

    if (!crossfade_mixmode)
    {
        /* Completely process the crossfade fade-out effect with current PCM buffer */
        size_t buffer_rem;
        size_t index = crossfade_find_buftail(crossfade_auto_skip, unplayed,
                                              fade_out_need, &buffer_rem);

        pcm_play_unlock();

        if (buffer_rem < fade_out_need)
        {
            /* Existing buffers are short */
            size_t fade_out_short = fade_out_need - buffer_rem;

            if (fade_out_delay >= fade_out_short)
            {
                /* Truncate fade-out delay */
                fade_out_delay -= fade_out_short;
            }
            else
            {
                /* Truncate fade-out and eliminate fade-out delay */
                fade_out_rem = buffer_rem;
                fade_out_delay = 0;
            }

            fade_out_need = fade_out_delay + fade_out_rem;
        }

        /* Find the right chunk and sample to start fading out */
        index = crossfade_find_index(index, fade_out_delay);

        /* Fade out the specified amount of the already processed audio */
        struct mixfader outfader;

        mixfader_init(&outfader, MIXFADE_UNITY, 0, fade_out_rem, false);
        crossfade_mix_fade(&outfader, fade_out_rem, NULL, &index, 0,
                           MIXFADE_KEEP_POS);

        /* Zero-out the rest of the buffer */
        crossfade_mix_fade(&outfader, pcmbuf_size, NULL, &index, 0,
                           MIXFADE_NULLIFY_POS);

        pcm_play_lock();
    }

    /* Initialize fade-in counters */
    mixfader_init(&crossfade_infader, 0, MIXFADE_UNITY, fade_in_duration, true);

    /* Find the right chunk and sample to start fading in - redo from read
       chunk in case original position were/was overrun in callback - the
       track change event _must not_ ever fail to happen */
    unplayed = pcmbuf_unplayed_bytes() + fade_in_delay;

    crossfade_widx = crossfade_find_buftail(crossfade_auto_skip, unplayed,
                                            fade_out_need, NULL);

    /* Move track transistion to chunk before the first one of incoming track */
    if (crossfade_auto_skip)
        pcmbuf_monitor_track_change_ex(crossfade_widx);

    pcm_play_unlock();

    logf("crossfade_start done!");
}

/* Perform fade-in of new track */
static void write_to_crossfade(size_t size, unsigned long elapsed, off_t offset)
{
    /* Mix the data */
    crossfade_mix_fade(&crossfade_infader, size, index_buffer(crossfade_bufidx),
                       &crossfade_widx, elapsed, offset);

    /* If no more fading-in to do, stop the crossfade */
    if (mixfader_finished(&crossfade_infader) &&
        index_chunk_offs(crossfade_widx, 0) == chunk_widx)
    {
        crossfade_cancel();
    }
}

static void pcmbuf_finish_crossfade_enable(void)
{
    /* Copy the pending setting over now */
    crossfade_setting = crossfade_enable_request;

    pcmbuf_watermark = (crossfade_setting != CROSSFADE_ENABLE_OFF && pcmbuf_size) ?
        /* If crossfading, try to keep the buffer full other than 1 second */
        (pcmbuf_size - BYTERATE) :
        /* Otherwise, just use the default */
        PCMBUF_WATERMARK;
}

void pcmbuf_request_crossfade_enable(int setting)
{
    /* Next setting to be used, not applied now */
    crossfade_enable_request = setting;
}

bool pcmbuf_is_same_size(void)
{
    /* if pcmbuf_buffer is NULL, then not set up yet even once so always */
    bool same_size = pcmbuf_buffer ?
        (get_next_required_pcmbuf_chunks() == pcmbuf_desc_count) : true;

    /* no buffer change needed, so finish crossfade setup now */
    if (same_size)
        pcmbuf_finish_crossfade_enable();

    return same_size;
}
#endif /* HAVE_CROSSFADE */


/** Debug menu, other metrics */

/* Amount of bytes left in the buffer, accounting for uncommitted bytes */
size_t pcmbuf_free(void)
{
    return pcmbuf_size - pcmbuf_unplayed_bytes() - pcmbuf_bytes_waiting;
}

/* Data bytes allocated for buffer */
size_t pcmbuf_get_bufsize(void)
{
    return pcmbuf_size;
}

/* Number of committed descriptors */
int pcmbuf_used_descs(void)
{
    return pcmbuf_unplayed_bytes() / PCMBUF_CHUNK_SIZE;
}

/* Total number of descriptors allocated */
int pcmbuf_descs(void)
{
    return pcmbuf_desc_count;
}


/** Fading and channel volume control */

/* Sync the channel amplitude to all states */
static void pcmbuf_update_volume(void)
{
    unsigned int vol = fade_vol;

    if (soft_mode)
        vol >>= 2;

    mixer_channel_set_amplitude(PCM_MIXER_CHAN_PLAYBACK, vol);
}

/* Tick that does the fade for the playback channel */
static void pcmbuf_fade_tick(void)
{
    /* ~1/3 second for full range fade */
    const unsigned int fade_step = MIX_AMP_UNITY / (HZ / 3);

    if (fade_state == PCM_FADING_IN)
        fade_vol += MIN(fade_step, MIX_AMP_UNITY - fade_vol);
    else if (fade_state == PCM_FADING_OUT)
        fade_vol -= MIN(fade_step, fade_vol - MIX_AMP_MUTE);

    pcmbuf_update_volume();

    if (fade_vol == MIX_AMP_MUTE || fade_vol == MIX_AMP_UNITY)
    {
        /* Fade is complete */
        tick_remove_task(pcmbuf_fade_tick);
        if (fade_state == PCM_FADING_OUT)
        {
            /* Tell PCM to stop at its earliest convenience */
            fade_out_complete = true;
        }

        fade_state = PCM_NOT_FADING;
    }
}

/* Fade channel in or out in the background */
void pcmbuf_fade(bool fade, bool in)
{
    /* Must pause any active fade */
    pcm_play_lock();

    if (fade_state != PCM_NOT_FADING)
        tick_remove_task(pcmbuf_fade_tick);

    fade_out_complete = false;

    pcm_play_unlock();

    if (!fade)
    {
        /* Simply set the level */
        fade_state = PCM_NOT_FADING;
        fade_vol = in ? MIX_AMP_UNITY : MIX_AMP_MUTE;
        pcmbuf_update_volume();
    }
    else
    {
        /* Set direction and resume fade from current point */
        fade_state = in ? PCM_FADING_IN : PCM_FADING_OUT;
        tick_add_task(pcmbuf_fade_tick);
    }
}

/* Return 'true' if fade is in progress */
bool pcmbuf_fading(void)
{
    return fade_state != PCM_NOT_FADING;
}

/* Quiet-down the channel if 'shhh' is true or else play at normal level */
void pcmbuf_soft_mode(bool shhh)
{
    /* Have to block the tick or improper order could leave volume in soft
       mode if fading reads the old value first but updates after us. */
    int res = fade_state != PCM_NOT_FADING ?
                tick_remove_task(pcmbuf_fade_tick) : -1;

    soft_mode = shhh;
    pcmbuf_update_volume();

    if (res == 0)
        tick_add_task(pcmbuf_fade_tick);
}


/** Time and position */

/* Return the current position key value */
unsigned int pcmbuf_get_position_key(void)
{
    return position_key;
}

/* Set position updates to be synchronous and immediate in addition to during
   PCM frames - cancelled upon first codec insert or upon stopping */
void pcmbuf_sync_position_update(void)
{
    pcmbuf_sync_position = true;
}



/** Misc */

bool pcmbuf_is_lowdata(void)
{
    enum channel_status status = mixer_channel_status(PCM_MIXER_CHAN_PLAYBACK);

    if (status != CHANNEL_PLAYING)
        return false;

#ifdef HAVE_CROSSFADE
    if (crossfade_status != CROSSFADE_INACTIVE)
        return false;
#endif

    return pcmbuf_data_critical();
}

void pcmbuf_set_low_latency(bool state)
{
    low_latency_mode = state;
}

void pcmbuf_update_frequency(void)
{
    pcmbuf_sampr = mixer_get_frequency();
}

unsigned int pcmbuf_get_frequency(void)
{
    return pcmbuf_sampr;
}
