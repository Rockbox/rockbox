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
#include "pcm_output.h"

/* PCM channel we're using */
#define PCM_CHANNEL    PCM_MIXER_CHAN_PLAYBACK

#define SILENCE_TEST_TONE 1
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

static int pcm_skipped = 0;
static int pcm_underruns = 0;

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
ssize_t pcm_output_bytes_used(void)
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
        pcmbuf_threshold = PCMOUT_EMPTY_WM;

        while (1)
        {
            sz = pcmbuf_head->size;

            if (sz < (ssize_t)(PCM_HDR_SIZE + 4) ||
                (sz & 3) != 0)
            {
                /* Just show a warning about this - will never happen
                 * without a corrupted buffer */
                DEBUGF("get_more: invalid size (%ld)\n", (long)sz);
            }

            /* Frame less than 100ms early - play it */
            struct pcm_frame_header *head = pcmbuf_head;
            pcm_advance_buffer(&pcmbuf_head, sz);
            pcmbuf_curr_size = sz;

DEBUGF("get_more() pcm_curr_size %d\n", sz);
            sz -= PCM_HDR_SIZE;

            *start = head->data;
            *size = sz;
            return;
        }
    }
    else
    {
        /* Ran out so revert to default watermark */
        if (pcmbuf_threshold == PCMOUT_EMPTY_WM)
            pcm_underruns++;

        pcmbuf_threshold = PCMOUT_PLAY_WM;
    }

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
bool pcm_output_commit_data(ssize_t size)
{
    if (size <= 0 || (size & 3))
        return false; /* invalid */

    size += PCM_HDR_SIZE;

    if (size > pcm_output_bytes_free())
        return false; /* too big */

    pcmbuf_tail->size = size;

    pcm_advance_buffer(&pcmbuf_tail, size);
    pcmbuf_written += size;
DEBUGF("pcm_output_commit_data(%d)\n", pcmbuf_written);
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

    enum channel_status status = rb->mixer_channel_status(PCM_CHANNEL);

    /* Stop PCM to clear current buffer */
    if (status != CHANNEL_STOPPED)
        rb->mixer_channel_stop(PCM_CHANNEL);

    rb->pcm_play_unlock();

    pcm_reset_buffer();

    /* Restart if playing state was current */
    if (status == CHANNEL_PLAYING)
        rb->mixer_channel_play_data(PCM_CHANNEL,
                                    get_more, NULL, 0);
}

/* Pauses/Starts pcm playback - and the clock */
void pcm_output_play_pause(bool play)
{
    rb->pcm_play_lock();

    if (rb->mixer_channel_status(PCM_CHANNEL) != CHANNEL_STOPPED)
    {
        rb->mixer_channel_play_pause(PCM_CHANNEL, play);
        rb->pcm_play_unlock();
    }
    else
    {
        rb->pcm_play_unlock();

        if (play)
        {
            rb->mixer_channel_set_amplitude(PCM_CHANNEL, MIX_AMP_UNITY);
            rb->mixer_channel_play_data(PCM_CHANNEL,
                                        get_more, NULL, 0);
        }
    }
}

/* Stops all playback and resets the clock */
void pcm_output_stop(void)
{
    rb->pcm_play_lock();

    if (rb->mixer_channel_status(PCM_CHANNEL) != CHANNEL_STOPPED)
        rb->mixer_channel_stop(PCM_CHANNEL);

    rb->pcm_play_unlock();

    pcm_output_flush();
}

/* Drains any data if the start threshold hasn't been reached */
void pcm_output_drain(void)
{
    rb->pcm_play_lock();
    pcmbuf_threshold = PCMOUT_EMPTY_WM;
    rb->pcm_play_unlock();
}

bool pcm_output_init(void *mem_ptr, size_t *size)
{
    /* 32bit align */
    pcm_buffer = mem_ptr;
    if (pcm_buffer == NULL)
        return false;

    ALIGN_BUFFER(pcm_buffer, *size, CACHEALIGN_UP(4));
    pcmbuf_end  = SKIPBYTES(pcm_buffer, *size);

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

    //old_sampr = rb->mixer_get_frequency();
    //rb->mixer_set_frequency(CLOCK_RATE);
    return true;
}

void pcm_output_exit(void)
{
    //if (old_sampr != 0)
    //    rb->mixer_set_frequency(old_sampr);
}
