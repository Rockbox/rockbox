/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Michael Sevakis
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
#include <sys/types.h>
#include "system.h"
#include "thread.h"
#include "voice_thread.h"
#include "talk.h"
#include "dsp.h"
#include "audio.h"
#include "playback.h"
#include "pcmbuf.h"
#include "pcm.h"
#include "pcm_mixer.h"
#include "codecs/libspeex/speex/speex.h"

/* Define any of these as "1" and uncomment the LOGF_ENABLE line to log
   regular and/or timeout messages */
#define VOICE_LOGQUEUES 0
#define VOICE_LOGQUEUES_SYS_TIMEOUT 0

/*#define LOGF_ENABLE*/
#include "logf.h"

#if VOICE_LOGQUEUES
#define LOGFQUEUE logf
#else
#define LOGFQUEUE(...)
#endif

#if VOICE_LOGQUEUES_SYS_TIMEOUT
#define LOGFQUEUE_SYS_TIMEOUT logf
#else
#define LOGFQUEUE_SYS_TIMEOUT(...)
#endif

#ifndef IBSS_ATTR_VOICE_STACK
#define IBSS_ATTR_VOICE_STACK IBSS_ATTR
#endif

/* Minimum priority needs to be a bit elevated since voice has fairly low
   latency */
#define PRIORITY_VOICE (PRIORITY_PLAYBACK-4)

#define VOICE_FRAME_COUNT   320 /* Samples / frame */
#define VOICE_SAMPLE_RATE 16000 /* Sample rate in HZ */
#define VOICE_SAMPLE_DEPTH   16 /* Sample depth in bits */

/* Voice thread variables */
static unsigned int voice_thread_id = 0;
#ifdef CPU_COLDFIRE
/* ISR uses any available stack - need a bit more room */
#define VOICE_STACK_EXTRA   0x400
#else
#define VOICE_STACK_EXTRA   0x3c0
#endif
static long voice_stack[(DEFAULT_STACK_SIZE + VOICE_STACK_EXTRA)/sizeof(long)]
    IBSS_ATTR_VOICE_STACK;
static const char voice_thread_name[] = "voice";

/* Voice thread synchronization objects */
static struct event_queue voice_queue SHAREDBSS_ATTR;
static struct queue_sender_list voice_queue_sender_list SHAREDBSS_ATTR;
static int quiet_counter SHAREDDATA_ATTR = 0;

/* Buffer for decoded samples */
static spx_int16_t voice_output_buf[VOICE_FRAME_COUNT] MEM_ALIGN_ATTR;

#define VOICE_PCM_FRAME_COUNT   ((NATIVE_FREQUENCY*VOICE_FRAME_COUNT + \
                                 VOICE_SAMPLE_RATE) / VOICE_SAMPLE_RATE)
#define VOICE_PCM_FRAME_SIZE    (VOICE_PCM_FRAME_COUNT*2*sizeof (int16_t))

/* Default number of native-frequency PCM frames to queue - adjust as
   necessary per-target */
#define VOICE_FRAMES            3

/* Might have lookahead and be skipping samples, so size is needed */
static size_t voicebuf_sizes[VOICE_FRAMES];
static int16_t (* voicebuf)[2*VOICE_PCM_FRAME_COUNT];
static unsigned int cur_buf_in, cur_buf_out;

/* Voice processing states */
enum voice_state
{
    VOICE_STATE_MESSAGE = 0,
    VOICE_STATE_DECODE,
    VOICE_STATE_BUFFER_INSERT,
};

/* A delay to not bring audio back to normal level too soon */
#define QUIET_COUNT 3

enum voice_thread_messages
{
    Q_VOICE_PLAY = 0, /* Play a clip */
    Q_VOICE_STOP,     /* Stop current clip */
};

/* Structure to store clip data callback info */
struct voice_info
{
    /* Callback to get more clips */
    mp3_play_callback_t get_more;
    /* Start of clip */
    const void *start;
    /* Size of clip */
    size_t size;
};

/* Private thread data for its current state that must be passed to its
 * internal functions */
struct voice_thread_data
{
    struct queue_event ev;  /* Last queue event pulled from queue */
    void *st;               /* Decoder instance */
    SpeexBits bits;         /* Bit cursor */
    struct dsp_config *dsp; /* DSP used for voice output */
    struct voice_info vi;   /* Copy of clip data */
    const char *src[2];     /* Current output buffer pointers */
    int lookahead;          /* Number of samples to drop at start of clip */
    int count;              /* Count of samples remaining to send to PCM */
};

/* Functions called in their repective state that return the next state to
   state machine loop - compiler may inline them at its discretion */
static enum voice_state voice_message(struct voice_thread_data *td);
static enum voice_state voice_decode(struct voice_thread_data *td);
static enum voice_state voice_buffer_insert(struct voice_thread_data *td);

/* Number of frames in queue */
static inline int voice_unplayed_frames(void)
{
    return cur_buf_in - cur_buf_out;
}

/* Mixer channel callback */
static void voice_pcm_callback(const void **start, size_t *size)
{
    if (voice_unplayed_frames() == 0)
        return; /* Done! */

    unsigned int i = ++cur_buf_out % VOICE_FRAMES;

    *start = voicebuf[i];
    *size = voicebuf_sizes[i];
}

/* Start playback of voice channel if not already playing */
static void voice_start_playback(void)
{
    if (mixer_channel_status(PCM_MIXER_CHAN_VOICE) != CHANNEL_STOPPED ||
        voice_unplayed_frames() <= 0)
        return;

    unsigned int i = cur_buf_out % VOICE_FRAMES;
    mixer_channel_play_data(PCM_MIXER_CHAN_VOICE, voice_pcm_callback,
                            voicebuf[i], voicebuf_sizes[i]);
}

/* Stop the voice channel */
static void voice_stop_playback(void)
{
    mixer_channel_stop(PCM_MIXER_CHAN_VOICE);
    cur_buf_in = cur_buf_out = 0;
}

/* Grab a free PCM frame */
static int16_t * voice_buf_get(void)
{
    if (voice_unplayed_frames() >= VOICE_FRAMES)
    {
        /* Full */
        voice_start_playback();
        return NULL;
    }

    return voicebuf[cur_buf_in % VOICE_FRAMES];
}

/* Commit a frame returned by voice_buf_get and set the actual size */
static void voice_buf_commit(int count)
{
    if (count > 0)
    {
        voicebuf_sizes[cur_buf_in++ % VOICE_FRAMES] =
            count * 2 * sizeof (int16_t);
    }
}

/* Stop any current clip and start playing a new one */
void mp3_play_data(const void *start, size_t size,
                   mp3_play_callback_t get_more)
{
    if (get_more != NULL && start != NULL && size > 0)
    {
        struct voice_info voice_clip =
        {
            .get_more = get_more,
            .start    = start,
            .size     = size,
        };

        LOGFQUEUE("mp3 >| voice Q_VOICE_PLAY");
        queue_send(&voice_queue, Q_VOICE_PLAY, (intptr_t)&voice_clip);
    }
}

/* Stop current voice clip from playing */
void mp3_play_stop(void)
{
    LOGFQUEUE("mp3 >| voice Q_VOICE_STOP");
    queue_send(&voice_queue, Q_VOICE_STOP, 0);
}

void mp3_play_pause(bool play)
{
    /* a dummy */
    (void)play;
}

/* Tell if voice is still in a playing state */
bool mp3_is_playing(void)
{
    return quiet_counter != 0;
}

/* This function is meant to be used by the buffer request functions to
   ensure the codec is no longer active */
void voice_stop(void)
{
    /* Unqueue all future clips */
    talk_force_shutup();
}

/* Wait for voice to finish speaking. */
void voice_wait(void)
{
    /* NOTE: One problem here is that we can't tell if another thread started a
     * new clip by the time we wait. This should be resolvable if conditions
     * ever require knowing the very clip you requested has finished. */

    while (quiet_counter != 0)
        sleep(1);
}

/* Initialize voice thread data that must be valid upon starting and the
 * setup the DSP parameters */
static void voice_data_init(struct voice_thread_data *td)
{
    td->dsp = (struct dsp_config *)dsp_configure(NULL, DSP_MYDSP,
                                                 CODEC_IDX_VOICE);

    dsp_configure(td->dsp, DSP_RESET, 0);
    dsp_configure(td->dsp, DSP_SET_FREQUENCY, VOICE_SAMPLE_RATE);
    dsp_configure(td->dsp, DSP_SET_SAMPLE_DEPTH, VOICE_SAMPLE_DEPTH);
    dsp_configure(td->dsp, DSP_SET_STEREO_MODE, STEREO_MONO);

    mixer_channel_set_amplitude(PCM_MIXER_CHAN_VOICE, MIX_AMP_UNITY);
}

/* Voice thread message processing */
static enum voice_state voice_message(struct voice_thread_data *td)
{
    if (quiet_counter > 0)
        queue_wait_w_tmo(&voice_queue, &td->ev, HZ/10);
    else
        queue_wait(&voice_queue, &td->ev);

    switch (td->ev.id)
    {
    case Q_VOICE_PLAY:
        LOGFQUEUE("voice < Q_VOICE_PLAY");
        if (quiet_counter == 0)
        {
            /* Boost CPU now */
            trigger_cpu_boost();
        }
        else
        {
            /* Stop any clip still playing */
            voice_stop_playback();
        }

        quiet_counter = QUIET_COUNT;

        /* Copy the clip info */
        td->vi = *(struct voice_info *)td->ev.data;

        /* Be sure audio buffer is initialized */
        audio_restore_playback(AUDIO_WANT_VOICE);

        /* We need nothing more from the sending thread - let it run */
        queue_reply(&voice_queue, 1);

        /* Make audio play more softly and set delay to return to normal
           playback level */
        pcmbuf_soft_mode(true);

        /* Clean-start the decoder */
        td->st = speex_decoder_init(&speex_wb_mode);

        /* Make bit buffer use our own buffer */
        speex_bits_set_bit_buffer(&td->bits, (void *)td->vi.start,
                                  td->vi.size);
        speex_decoder_ctl(td->st, SPEEX_GET_LOOKAHEAD, &td->lookahead);

        return VOICE_STATE_DECODE;

    case SYS_TIMEOUT:
        if (voice_unplayed_frames())
        {
            /* Waiting for PCM to finish */
            break;
        }

        /* Drop through and stop the first time after clip runs out */
        if (quiet_counter-- != QUIET_COUNT)
        {
            if (quiet_counter <= 0)
                pcmbuf_soft_mode(false);

            break;
        }

        /* Fall-through */
    case Q_VOICE_STOP:
        LOGFQUEUE("voice < Q_VOICE_STOP");
        cancel_cpu_boost();
        voice_stop_playback();
        break;

    /* No default: no other message ids are sent */
    }

    return VOICE_STATE_MESSAGE;
}

/* Decode frames or stop if all have completed */
static enum voice_state voice_decode(struct voice_thread_data *td)
{
    if (!queue_empty(&voice_queue))
        return VOICE_STATE_MESSAGE;

    /* Decode the data */
    if (speex_decode_int(td->st, &td->bits, voice_output_buf) < 0)
    {
        /* End of stream or error - get next clip */
        td->vi.size = 0;

        if (td->vi.get_more != NULL)
            td->vi.get_more(&td->vi.start, &td->vi.size);

        if (td->vi.start != NULL && td->vi.size > 0)
        {
            /* Make bit buffer use our own buffer */
            speex_bits_set_bit_buffer(&td->bits, (void *)td->vi.start,
                                      td->vi.size);
            /* Don't skip any samples when we're stringing clips together */
            td->lookahead = 0;
        }
        else
        {
            /* If all clips are done and not playing, force pcm playback. */
            voice_start_playback();
            return VOICE_STATE_MESSAGE;
        }
    }
    else
    {
        yield();

        /* Output the decoded frame */
        td->count = VOICE_FRAME_COUNT - td->lookahead;
        td->src[0] = (const char *)&voice_output_buf[td->lookahead];
        td->src[1] = NULL;
        td->lookahead -= MIN(VOICE_FRAME_COUNT, td->lookahead);

        if (td->count > 0)
            return VOICE_STATE_BUFFER_INSERT;
    }

    return VOICE_STATE_DECODE;
}

/* Process the PCM samples in the DSP and send out for mixing */
static enum voice_state voice_buffer_insert(struct voice_thread_data *td)
{
    if (!queue_empty(&voice_queue))
        return VOICE_STATE_MESSAGE;

    char *dest = (char *)voice_buf_get();

    if (dest != NULL)
    {
        voice_buf_commit(dsp_process(td->dsp, dest, td->src, td->count));
        return VOICE_STATE_DECODE;
    }

    sleep(0);
    return VOICE_STATE_BUFFER_INSERT;
}

/* Voice thread entrypoint */
static void NORETURN_ATTR voice_thread(void)
{
    struct voice_thread_data td;
    enum voice_state state = VOICE_STATE_MESSAGE;

    voice_data_init(&td);

    while (1)
    {
        switch (state)
        {
        case VOICE_STATE_MESSAGE:
            state = voice_message(&td);
            break;
        case VOICE_STATE_DECODE:
            state = voice_decode(&td);
            break;
        case VOICE_STATE_BUFFER_INSERT:
            state = voice_buffer_insert(&td);
            break;
        }
    }
}

/* Initialize all synchronization objects create the thread */
void voice_thread_init(void)
{
    logf("Starting voice thread");
    queue_init(&voice_queue, false);

    voice_thread_id = create_thread(voice_thread, voice_stack,
            sizeof(voice_stack), 0, voice_thread_name
            IF_PRIO(, PRIORITY_VOICE) IF_COP(, CPU));

    queue_enable_queue_send(&voice_queue, &voice_queue_sender_list,
                            voice_thread_id);
}

#ifdef HAVE_PRIORITY_SCHEDULING
/* Set the voice thread priority */
void voice_thread_set_priority(int priority)
{
    if (priority > PRIORITY_VOICE)
        priority = PRIORITY_VOICE;

    thread_set_priority(voice_thread_id, priority);
}
#endif

/* Initialize voice PCM buffer and return size, allocated from the end */
size_t voicebuf_init(void *bufend)
{
    size_t size = VOICE_FRAMES * sizeof (voicebuf[0]);
    cur_buf_out = cur_buf_in = 0;
    voicebuf = bufend - size;
    return size;
}
