/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * PCM output buffer definitions
 *
 * Copyright (c) 2007 Michael Sevakis
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
#include "plugin.h"
#include "mpegplayer.h"

static pcm_handle_t pcm_handle;

/* Pointers */

/* Start of buffer */
static struct pcm_frame_header * ALIGNED_ATTR(4) pcm_buffer;
/* End of buffer (not guard) */
static struct pcm_frame_header * ALIGNED_ATTR(4) pcmbuf_end;
/* Read pointer */
static struct pcm_frame_header * ALIGNED_ATTR(4) pcmbuf_head IBSS_ATTR;
/* Write pointer */
static struct pcm_frame_header * ALIGNED_ATTR(4) pcmbuf_tail IBSS_ATTR;

/* Bytes */
static ssize_t  pcmbuf_curr_size IBSS_ATTR; /* Size of currently playing frame */
static ssize_t  pcmbuf_read      IBSS_ATTR; /* Number of bytes read by DMA */
static ssize_t  pcmbuf_written   IBSS_ATTR; /* Number of bytes written by source */
static ssize_t  pcmbuf_threshold IBSS_ATTR; /* Non-silence threshold */

/* Clock */
static uint32_t clock_start  IBSS_ATTR; /* Clock at playback start */
static uint32_t volatile clock_tick IBSS_ATTR; /* Our base clock */
static uint32_t volatile clock_time IBSS_ATTR; /* Timestamp adjusted */

static int pcm_skipped = 0;
static int pcm_underruns = 0;

static unsigned int old_sampr = 0;

/* Small silence clip. ~5.80ms @ 44.1kHz */
#define SILENCE_FRAMES 256
static pcm_dma_t silence[SILENCE_FRAMES*PCM_DMA_T_CHANNELS];

/* Delete all buffer contents */
static void pcm_reset_buffer(void)
{
    pcmbuf_threshold = PCMOUT_PLAY_WM;
    pcmbuf_curr_size = pcmbuf_read = pcmbuf_written = 0;
    pcmbuf_head = pcmbuf_tail = pcm_buffer;
    pcm_skipped = pcm_underruns = 0;
}

/* Advance a PCM buffer pointer by size bytes circularly */
static inline void pcm_advance_buffer(struct pcm_frame_header **p,
                                      size_t size)
{
    *p = SKIPBYTES(*p, size);
    if (*p >= pcmbuf_end)
        *p = SKIPBYTES(*p, -PCMOUT_BUFSIZE);
}

/* Return physical space used */
static inline ssize_t pcm_output_bytes_used(void)
{
    return pcmbuf_written - pcmbuf_read; /* wrap-safe */
}

/* Return physical space free */
static inline ssize_t pcm_output_bytes_free(void)
{
    return PCMOUT_BUFSIZE - pcm_output_bytes_used();
}

/* Audio DMA handler */
static int pcm_callback(int status, const void **start, unsigned long *frames)
{
    if (status < 0)
        return status;

    ssize_t sz;

    /* Free-up the last frame played frame if any */
    pcmbuf_read += pcmbuf_curr_size;
    pcmbuf_curr_size = 0;

    sz = pcm_output_bytes_used();

    if (sz > pcmbuf_threshold)
    {
        pcmbuf_threshold = PCMOUT_LOW_WM;

        while (1)
        {
            uint32_t time = pcmbuf_head->time;
            int32_t offset = time - clock_time;

            sz = pcmbuf_head->size;

            if (sz < (ssize_t)(PCM_HDR_SIZE + 4) ||
                (sz % PCM_DMA_T_SIZE) != 0)
            {
                /* Just show a warning about this - will never happen
                 * without a corrupted buffer */
                DEBUGF("get_more: invalid size (%zd)\n", sz);
            }

            if (offset < -100*CLOCK_RATE/1000)
            {
                /* Frame more than 100ms late - drop it */
                pcm_advance_buffer(&pcmbuf_head, sz);
                pcmbuf_read += sz;
                pcm_skipped++;
                if (pcm_output_bytes_used() > 0)
                    continue;

                /* Ran out so revert to default watermark */
                pcmbuf_threshold = PCMOUT_PLAY_WM;
                pcm_underruns++;
            }
            else if (offset < 100*CLOCK_RATE/1000)
            {
                /* Frame less than 100ms early - play it */
                struct pcm_frame_header *head = pcmbuf_head;

                pcm_advance_buffer(&pcmbuf_head, sz);
                pcmbuf_curr_size = sz;

                sz -= PCM_HDR_SIZE;

                unsigned long frcount = pcm_dma_t_size_frames(sz);

                /* Audio is time master - keep clock synchronized */
                clock_time = time + frcount;

                /* Update base clock */
                clock_tick += frcount;

                *start = head->data;
                *frames = frcount;
                return 0;
            }
            /* Frame will be dropped - play silence clip */
            break;
        }
    }
    else
    {
        /* Ran out so revert to default watermark */
        if (pcmbuf_threshold == PCMOUT_LOW_WM)
            pcm_underruns++;

        pcmbuf_threshold = PCMOUT_PLAY_WM;
    }

    /* Keep clock going at all times */
    clock_time += SILENCE_FRAMES;
    clock_tick += SILENCE_FRAMES;

    *start = silence;
    *frames = SILENCE_FRAMES;

    if (sz < 0)
        pcmbuf_read = pcmbuf_written;

    return 0;
}

static inline bool pcm_output_is_running(void)
{
    return rb->pcm_get_state(pcm_handle) == PCM_STATE_RUNNING;
}

static inline bool pcm_output_is_stopped(void)
{
    return rb->pcm_get_state(pcm_handle) == PCM_STATE_STOPPED;
}

/* Start the pcm playback, native output format */
void pcm_output_play(void)
{
    rb->pcm_play_data(pcm_handle, pcm_callback, NULL, 0, NULL);
}

/** Public interface **/

/* Return a buffer pointer if at least size bytes are available and if so,
 * give the actual free space */
void * pcm_output_get_buffer(ssize_t *size)
{
    ssize_t sz = *size;
    ssize_t free = pcm_output_bytes_free() - PCM_HDR_SIZE;

    if (sz >= 0 && free >= sz)
    {
        *size = free; /* return actual free space (- header) */
        return pcmbuf_tail->data;
    }

    /* Leave *size alone so caller doesn't have to reinit */
    return NULL;
}

/* Commit the buffer returned by pcm_ouput_get_buffer; timestamp is PCM
 * clock time units, not video format time units */
bool pcm_output_commit_data(ssize_t size, uint32_t timestamp)
{
    if (size <= 0 || (size & 3))
        return false; /* invalid */

    size += PCM_HDR_SIZE;

    if (size > pcm_output_bytes_free())
        return false; /* too big */

    pcmbuf_tail->size = size;
    pcmbuf_tail->time = timestamp;

    pcm_advance_buffer(&pcmbuf_tail, size);
    pcmbuf_written += size;

    return true;
}

/* Returns 'true' if the buffer is completely empty */
bool pcm_output_empty(void)
{
    return pcm_output_bytes_used() <= 0;
}

/* Flushes the buffer - clock keeps counting */
void pcm_output_flush(void)
{
    rb->pcm_lock_callback(pcm_handle);

    unsigned int state = rb->pcm_get_state(pcm_handle);

    /* Stop PCM to clear current buffer */
    if (state > PCM_STATE_STOPPED)
        rb->pcm_stop(pcm_handle);

    rb->pcm_unlock_callback(pcm_handle);

    pcm_reset_buffer();

    /* Restart if playing state was current */
    if (state == PCM_STATE_RUNNING)
        pcm_output_play();
}

/* Seek the reference clock to the specified time - next audio data ready to
   go to DMA should be on the buffer with the same time index or else the PCM
   buffer should be empty */
void pcm_output_set_clock(uint32_t time)
{
    rb->pcm_lock_callback(pcm_handle);

    clock_start = time;
    clock_tick = time;
    clock_time = time;

    rb->pcm_unlock_callback(pcm_handle);
}

/* Return the clock as synchronized by audio frame timestamps */
uint32_t pcm_output_get_clock(void)
{
    uint32_t time, rem;

    /* Reread if data race detected - rem will be 0 if driver hasn't yet
     * updated to the new buffer size. Also be sure pcm state doesn't
     * cause indefinite loop.
     *
     * FYI: NOT scrutinized for rd/wr reordering on different cores. */
    while (1)
    {
        time = clock_time;
        rem = rb->pcm_get_frames_waiting(pcm_handle);

        if (time != clock_time)
            continue;

        if (rem == 0 && pcm_output_is_running())
            continue;

        break;
    }

    return time - rem;

}

/* Return the raw clock as counted from the last pcm_output_set_clock
 * call */
uint32_t pcm_output_get_ticks(uint32_t *start)
{
    uint32_t tick, rem;

    /* Same procedure as pcm_output_get_clock */
    while (1)
    {
        tick = clock_tick;
        rem = rb->pcm_get_frames_waiting(pcm_handle);

        if (tick != clock_tick)
            continue;

        if (rem == 0 && pcm_output_is_running())
            continue;

        break;
    }

    if (start)
        *start = clock_start;

    return tick - rem;
}

/* Pauses/Starts pcm playback - and the clock */
void pcm_output_play_pause(bool play)
{
    rb->pcm_lock_callback(pcm_handle);

    if (!pcm_output_is_stopped())
    {
        rb->pcm_pause(pcm_handle, !play);
        rb->pcm_unlock_callback(pcm_handle);
    }
    else
    {
        rb->pcm_unlock_callback(pcm_handle);

        if (play)
            pcm_output_play();
    }
}

/* Stops all playback and resets the clock */
void pcm_output_stop(void)
{
    rb->pcm_stop(pcm_handle);
    pcm_output_flush();
    pcm_output_set_clock(0);
}

/* Drains any data if the start threshold hasn't been reached */
void pcm_output_drain(void)
{
    rb->pcm_lock_callback(pcm_handle);
    pcmbuf_threshold = PCMOUT_LOW_WM;
    rb->pcm_unlock_callback(pcm_handle);
}

bool pcm_output_init(void)
{
    if (rb->pcm_open(&pcm_handle, PCM_STREAM_PLAYBACK))
        return false;

    pcm_buffer = mpeg_malloc(PCMOUT_ALLOC_SIZE, MPEG_ALLOC_PCMOUT);
    if (pcm_buffer == NULL)
        return false;

    pcmbuf_end  = SKIPBYTES(pcm_buffer, PCMOUT_BUFSIZE);

    pcm_reset_buffer();

#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif

    old_sampr = rb->pcm_get_frequency(pcm_handle);
    rb->pcm_set_frequency(pcm_handle, CLOCK_RATE);
    return true;
}

void pcm_output_exit(void)
{
    if (old_sampr != 0)
        rb->pcm_set_frequency(pcm_handle, old_sampr);

    rb->pcm_close(pcm_handle);
    pcm_handle = 0;
}
