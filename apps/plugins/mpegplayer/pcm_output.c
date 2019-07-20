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

/* PCM channel we're using */
#define MPEG_PCM_CHANNEL    PCM_MIXER_CHAN_PLAYBACK

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
static int16_t silence[256*2] ALIGNED_ATTR(4) = { 0 };

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
static void get_more(const void **start, size_t *size)
{
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
                (sz & 3) != 0)
            {
                /* Just show a warning about this - will never happen
                 * without a corrupted buffer */
                DEBUGF("get_more: invalid size (%ld)\n", (long)sz);
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

                /* Audio is time master - keep clock synchronized */
                clock_time = time + (sz >> 2);

                /* Update base clock */
                clock_tick += sz >> 2;

                *start = head->data;
                *size = sz;
                return;
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
    clock_time += sizeof (silence) / 4;
    clock_tick += sizeof (silence) / 4;

    *start = silence;
    *size = sizeof (silence);

    if (sz < 0)
        pcmbuf_read = pcmbuf_written;
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
    rb->pcm_play_lock();

    enum channel_status status = rb->mixer_channel_status(MPEG_PCM_CHANNEL);

    /* Stop PCM to clear current buffer */
    if (status != CHANNEL_STOPPED)
        rb->mixer_channel_stop(MPEG_PCM_CHANNEL);

    rb->pcm_play_unlock();

    pcm_reset_buffer();

    /* Restart if playing state was current */
    if (status == CHANNEL_PLAYING)
        rb->mixer_channel_play_data(MPEG_PCM_CHANNEL,
                                    get_more, NULL, 0);
}

/* Seek the reference clock to the specified time - next audio data ready to
   go to DMA should be on the buffer with the same time index or else the PCM
   buffer should be empty */
void pcm_output_set_clock(uint32_t time)
{
    rb->pcm_play_lock();

    clock_start = time;
    clock_tick = time;
    clock_time = time;

    rb->pcm_play_unlock();
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
    do
    {
        time = clock_time;
        rem = rb->mixer_channel_get_bytes_waiting(MPEG_PCM_CHANNEL) >> 2;
    }
    while (UNLIKELY(time != clock_time ||
        (rem == 0 &&
         rb->mixer_channel_status(MPEG_PCM_CHANNEL) == CHANNEL_PLAYING))
    );

    return time - rem;

}

/* Return the raw clock as counted from the last pcm_output_set_clock
 * call */
uint32_t pcm_output_get_ticks(uint32_t *start)
{
    uint32_t tick, rem;

    /* Same procedure as pcm_output_get_clock */
    do
    {
        tick = clock_tick;
        rem = rb->mixer_channel_get_bytes_waiting(MPEG_PCM_CHANNEL) >> 2;
    }
    while (UNLIKELY(tick != clock_tick ||
        (rem == 0 &&
         rb->mixer_channel_status(MPEG_PCM_CHANNEL) == CHANNEL_PLAYING))
    );

    if (start)
        *start = clock_start;

    return tick - rem;
}

/* Pauses/Starts pcm playback - and the clock */
void pcm_output_play_pause(bool play)
{
    rb->pcm_play_lock();

    if (rb->mixer_channel_status(MPEG_PCM_CHANNEL) != CHANNEL_STOPPED)
    {
        rb->mixer_channel_play_pause(MPEG_PCM_CHANNEL, play);
        rb->pcm_play_unlock();
    }
    else
    {
        rb->pcm_play_unlock();

        if (play)
        {
            rb->mixer_channel_set_amplitude(MPEG_PCM_CHANNEL, MIX_AMP_UNITY);
            rb->mixer_channel_play_data(MPEG_PCM_CHANNEL,
                                        get_more, NULL, 0);
        }
    }
}

/* Stops all playback and resets the clock */
void pcm_output_stop(void)
{
    rb->pcm_play_lock();

    if (rb->mixer_channel_status(MPEG_PCM_CHANNEL) != CHANNEL_STOPPED)
        rb->mixer_channel_stop(MPEG_PCM_CHANNEL);

    rb->pcm_play_unlock();

    pcm_output_flush();
    pcm_output_set_clock(0);
}

/* Drains any data if the start threshold hasn't been reached */
void pcm_output_drain(void)
{
    rb->pcm_play_lock();
    pcmbuf_threshold = PCMOUT_LOW_WM;
    rb->pcm_play_unlock();
}

bool pcm_output_init(void)
{
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

#if SILENCE_TEST_TONE
    /* Make the silence clip a square wave */
    const int16_t silence_amp = INT16_MAX / 16;
    unsigned i;

    for (i = 0; i < ARRAYLEN(silence); i += 2)
    {
        if (i < ARRAYLEN(silence)/2)
        {
            silence[i] = silence_amp;
            silence[i+1] = silence_amp;
        }
        else
        {
            silence[i] = -silence_amp;
            silence[i+1] = -silence_amp;
        }           
    }
#endif

    old_sampr = rb->mixer_get_frequency();
    rb->mixer_set_frequency(CLOCK_RATE);
    rb->pcmbuf_fade(false, true);
    return true;
}

void pcm_output_exit(void)
{
    rb->pcmbuf_fade(false, false);
    if (old_sampr != 0)
        rb->mixer_set_frequency(old_sampr);
}
