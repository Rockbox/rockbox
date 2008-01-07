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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "mpegplayer.h"

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
static uint64_t pcmbuf_read      IBSS_ATTR; /* Number of bytes read by DMA */
static uint64_t pcmbuf_written   IBSS_ATTR; /* Number of bytes written by source */
static ssize_t  pcmbuf_threshold IBSS_ATTR; /* Non-silence threshold */

/* Clock */
static uint32_t clock_base   IBSS_ATTR; /* Our base clock */
static uint32_t clock_start  IBSS_ATTR; /* Clock at playback start */
static int32_t  clock_adjust IBSS_ATTR; /* Clock drift adjustment */

/* Small silence clip. ~5.80ms @ 44.1kHz */
static int16_t silence[256*2] ALIGNED_ATTR(4) = { 0 };

/* Advance a PCM buffer pointer by size bytes circularly */
static inline void pcm_advance_buffer(struct pcm_frame_header **p,
                                      size_t size)
{
    *p = SKIPBYTES(*p, size);
    if (*p >= pcmbuf_end)
        *p = pcm_buffer;
}

/* Inline internally but not externally */
inline ssize_t pcm_output_used(void)
{
    return (ssize_t)(pcmbuf_written - pcmbuf_read);
}

inline ssize_t pcm_output_free(void)
{
    return (ssize_t)(PCMOUT_BUFSIZE - pcmbuf_written + pcmbuf_read);
}

/* Audio DMA handler */
static void get_more(unsigned char **start, size_t *size)
{
    ssize_t sz = pcm_output_used();

    if (sz > pcmbuf_threshold)
    {
        pcmbuf_threshold = PCMOUT_LOW_WM;

        while (1)
        {
            uint32_t time = pcmbuf_head->time;
            int32_t offset = time - (clock_base + clock_adjust);

            sz = pcmbuf_head->size;

            if (sz < (ssize_t)(sizeof(pcmbuf_head) + 4) ||
                (sz & 3) != 0)
            {
                /* Just show a warning about this - will never happen
                 * without a bug in the audio thread code or a clobbered
                 * buffer */
                DEBUGF("get_more: invalid size (%ld)\n", sz);
            }

            if (offset < -100*CLOCK_RATE/1000)
            {
                /* Frame more than 100ms late - drop it */
                pcm_advance_buffer(&pcmbuf_head, sz);
                pcmbuf_read += sz;
                if (pcmbuf_read < pcmbuf_written)
                    continue;
            }
            else if (offset < 100*CLOCK_RATE/1000)
            {
                /* Frame less than 100ms early - play it */
                *start = (unsigned char *)pcmbuf_head->data;

                pcm_advance_buffer(&pcmbuf_head, sz);
                pcmbuf_read += sz;

                sz -= sizeof (struct pcm_frame_header);

                *size = sz;

                /* Audio is time master - keep clock synchronized */
                clock_adjust = time - clock_base;

                /* Update base clock */
                clock_base += sz >> 2;
                return;
            }
            /* Frame will be dropped - play silence clip */
            break;
        }
    }
    else
    {
        /* Ran out so revert to default watermark */
        pcmbuf_threshold = PCMOUT_PLAY_WM;
    }

    /* Keep clock going at all times */
    *start = (unsigned char *)silence;
    *size = sizeof (silence);

    clock_base += sizeof (silence) / 4;

    if (pcmbuf_read > pcmbuf_written)
        pcmbuf_read = pcmbuf_written;
}

struct pcm_frame_header * pcm_output_get_buffer(void)
{
    return pcmbuf_tail;
}

void pcm_output_add_data(void)
{
    size_t size = pcmbuf_tail->size;
    pcm_advance_buffer(&pcmbuf_tail, size);
    pcmbuf_written += size;
}

/* Flushes the buffer - clock keeps counting */
void pcm_output_flush(void)
{
    bool playing, paused;

    rb->pcm_play_lock();

    playing = rb->pcm_is_playing();
    paused = rb->pcm_is_paused();

    /* Stop PCM to clear current buffer */
    if (playing)
        rb->pcm_play_stop();

    pcmbuf_threshold = PCMOUT_PLAY_WM;
    pcmbuf_read = pcmbuf_written = 0;
    pcmbuf_head = pcmbuf_tail = pcm_buffer;

    /* Restart if playing state was current */
    if (playing && !paused)
        rb->pcm_play_data(get_more, NULL, 0);

    rb->pcm_play_unlock();
}

/* Seek the reference clock to the specified time - next audio data ready to
   go to DMA should be on the buffer with the same time index or else the PCM
   buffer should be empty */
void pcm_output_set_clock(uint32_t time)
{
    rb->pcm_play_lock();

    clock_base = time;
    clock_start = time;
    clock_adjust = 0;

    rb->pcm_play_unlock();
}

uint32_t pcm_output_get_clock(void)
{
    return clock_base + clock_adjust
        - (rb->pcm_get_bytes_waiting() >> 2);
}

uint32_t pcm_output_get_ticks(uint32_t *start)
{
    if (start)
        *start = clock_start;

    return clock_base - (rb->pcm_get_bytes_waiting() >> 2);
}

/* Pauses/Starts pcm playback - and the clock */
void pcm_output_play_pause(bool play)
{
    rb->pcm_play_lock();

    if (rb->pcm_is_playing())
    {
        rb->pcm_play_pause(play);
    }
    else if (play)
    {
        rb->pcm_play_data(get_more, NULL, 0);
    }

    rb->pcm_play_unlock();
}

/* Stops all playback and resets the clock */
void pcm_output_stop(void)
{
    rb->pcm_play_lock();

    if (rb->pcm_is_playing())
        rb->pcm_play_stop();

    pcm_output_flush();
    pcm_output_set_clock(0);

    rb->pcm_play_unlock();
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

    pcmbuf_threshold = PCMOUT_PLAY_WM;
    pcmbuf_head = pcm_buffer;
    pcmbuf_tail = pcm_buffer;
    pcmbuf_end  = SKIPBYTES(pcm_buffer, PCMOUT_BUFSIZE);
    pcmbuf_read = 0;
    pcmbuf_written = 0;

    rb->pcm_set_frequency(SAMPR_44);

#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif

#if SILENCE_TEST_TONE
    /* Make the silence clip a square wave */
    const int16_t silence_amp = 32767 / 16;
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

    return true;
}

void pcm_output_exit(void)
{
    rb->pcm_set_frequency(HW_SAMPR_DEFAULT);
}
