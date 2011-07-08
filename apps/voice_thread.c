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

#define VOICE_FRAME_SIZE    320 /* Samples / frame */
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
static bool voice_done SHAREDDATA_ATTR = true;

/* Buffer for decoded samples */
static spx_int16_t voice_output_buf[VOICE_FRAME_SIZE] CACHEALIGN_ATTR;

#define VOICE_PCM_FRAME_COUNT   ((NATIVE_FREQUENCY*VOICE_FRAME_SIZE + \
                                 VOICE_SAMPLE_RATE) / VOICE_SAMPLE_RATE)
#define VOICE_PCM_FRAME_SIZE    (VOICE_PCM_FRAME_COUNT*4)

/* Default number of native-frequency PCM frames to queue - adjust as
   necessary per-target */
#define VOICE_FRAMES            3

/* Might have lookahead and be skipping samples, so size is needed */
static size_t voicebuf_sizes[VOICE_FRAMES];
static uint32_t (* voicebuf)[VOICE_PCM_FRAME_COUNT];
static unsigned int cur_buf_in, cur_buf_out;

/* A delay to not bring audio back to normal level too soon */
#define QUIET_COUNT 3

enum voice_thread_states
{
    TSTATE_STOPPED = 0,   /* Voice thread is stopped and awaiting commands */
    TSTATE_DECODE,        /* Voice is decoding a clip */
    TSTATE_BUFFER_INSERT, /* Voice is sending decoded audio to PCM */
};

enum voice_thread_messages
{
    Q_VOICE_NULL = 0, /* A message for thread sync - no effect on state */
    Q_VOICE_PLAY,     /* Play a clip */
    Q_VOICE_STOP,     /* Stop current clip */
};

/* Structure to store clip data callback info */
struct voice_info
{
    pcm_play_callback_type get_more; /* Callback to get more clips */
    unsigned char *start;            /* Start of clip */
    size_t size;                     /* Size of clip */
};

/* Private thread data for its current state that must be passed to its
 * internal functions */
struct voice_thread_data
{
    volatile int state;     /* Thread state (TSTATE_*) */
    struct queue_event ev;  /* Last queue event pulled from queue */
    void *st;               /* Decoder instance */
    SpeexBits bits;         /* Bit cursor */
    struct dsp_config *dsp; /* DSP used for voice output */
    struct voice_info vi;   /* Copy of clip data */
    const char *src[2];     /* Current output buffer pointers */
    int lookahead;          /* Number of samples to drop at start of clip */
    int count;              /* Count of samples remaining to send to PCM */
    int quiet_counter;      /* Countdown until audio goes back to normal */
};

/* Number of frames in queue */
static inline int voice_unplayed_frames(void)
{
    return cur_buf_in - cur_buf_out;
}

/* Mixer channel callback */
static void voice_pcm_callback(unsigned char **start, size_t *size)
{
    if (voice_unplayed_frames() == 0)
        return; /* Done! */

    unsigned int i = ++cur_buf_out % VOICE_FRAMES;

    *start = (unsigned char *)voicebuf[i];
    *size = voicebuf_sizes[i];
}

/* Start playback of voice channel if not already playing */
static void voice_start_playback(void)
{
    if (mixer_channel_status(PCM_MIXER_CHAN_VOICE) != CHANNEL_STOPPED)
        return;

    unsigned int i = cur_buf_out % VOICE_FRAMES;
    mixer_channel_play_data(PCM_MIXER_CHAN_VOICE, voice_pcm_callback,
                            (unsigned char *)voicebuf[i], voicebuf_sizes[i]);
}

/* Stop the voice channel */
static void voice_stop_playback(void)
{
    mixer_channel_stop(PCM_MIXER_CHAN_VOICE);
    cur_buf_in = cur_buf_out = 0;
}

/* Grab a free PCM frame */
static uint32_t * voice_buf_get(void)
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
static void voice_buf_commit(size_t size)
{
    voicebuf_sizes[cur_buf_in++ % VOICE_FRAMES] = size;
}

/* Stop any current clip and start playing a new one */
void mp3_play_data(const unsigned char* start, int size,
                   pcm_play_callback_type get_more)
{
    if (get_more != NULL && start != NULL && (ssize_t)size > 0)
    {
        struct voice_info voice_clip =
        {
            .get_more = get_more,
            .start    = (unsigned char *)start,
            .size     = size,
        };

        LOGFQUEUE("mp3 >| voice Q_VOICE_PLAY");
        queue_send(&voice_queue, Q_VOICE_PLAY, (intptr_t)&voice_clip);
    }
}

/* Stop current voice clip from playing */
void mp3_play_stop(void)
{
    if(!audio_is_thread_ready())
       return;

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
    return !voice_done;
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

    while (!voice_done)
        sleep(1);
}

/* Initialize voice thread data that must be valid upon starting and the
 * setup the DSP parameters */
static void voice_data_init(struct voice_thread_data *td)
{
    td->state = TSTATE_STOPPED;
    td->dsp = (struct dsp_config *)dsp_configure(NULL, DSP_MYDSP,
                                                 CODEC_IDX_VOICE);

    dsp_configure(td->dsp, DSP_RESET, 0);
    dsp_configure(td->dsp, DSP_SET_FREQUENCY, VOICE_SAMPLE_RATE);
    dsp_configure(td->dsp, DSP_SET_SAMPLE_DEPTH, VOICE_SAMPLE_DEPTH);
    dsp_configure(td->dsp, DSP_SET_STEREO_MODE, STEREO_MONO);

    mixer_channel_set_amplitude(PCM_MIXER_CHAN_VOICE, MIX_AMP_UNITY);
    td->quiet_counter = 0;
}

/* Voice thread message processing */
static void voice_message(struct voice_thread_data *td)
{
    while (1)
    {
        switch (td->ev.id)
        {
        case Q_VOICE_PLAY:
            LOGFQUEUE("voice < Q_VOICE_PLAY");
            voice_done = false;

            /* Copy the clip info */
            td->vi = *(struct voice_info *)td->ev.data;

            /* Be sure audio buffer is initialized */
            audio_restore_playback(AUDIO_WANT_VOICE);

            /* We need nothing more from the sending thread - let it run */
            queue_reply(&voice_queue, 1);

            if (td->state == TSTATE_STOPPED)
            {
                /* Boost CPU now */
                trigger_cpu_boost();
            }
            else
            {
                /* Stop any clip still playing */
                voice_stop_playback();
            }

            /* Make audio play more softly and set delay to return to normal
               playback level */
            pcmbuf_soft_mode(true);
            td->quiet_counter = QUIET_COUNT;

            /* Clean-start the decoder */
            td->st = speex_decoder_init(&speex_wb_mode);

            /* Make bit buffer use our own buffer */
            speex_bits_set_bit_buffer(&td->bits, td->vi.start, td->vi.size);
            speex_decoder_ctl(td->st, SPEEX_GET_LOOKAHEAD, &td->lookahead);

            td->state = TSTATE_DECODE;
            return;

        case SYS_TIMEOUT:
            if (voice_unplayed_frames())
            {
                /* Waiting for PCM to finish */
                break;
            }

            /* Drop through and stop the first time after clip runs out */
            if (td->quiet_counter-- != QUIET_COUNT)
            {
                if (td->quiet_counter <= 0)
                    pcmbuf_soft_mode(false);

                break;
            }

            /* Fall-through */
        case Q_VOICE_STOP:
            LOGFQUEUE("voice < Q_VOICE_STOP");

            td->state = TSTATE_STOPPED;
            voice_done = true;

            cancel_cpu_boost();
            voice_stop_playback();
            break;

        default:
            /* Default messages get a reply and thread continues with no
             * state transition */
            LOGFQUEUE("voice < default");

            if (td->state == TSTATE_STOPPED)
                break;  /* Not in (active) playback state */

            queue_reply(&voice_queue, 0);
            return;
        }

        if (td->quiet_counter > 0)
            queue_wait_w_tmo(&voice_queue, &td->ev, HZ/10);
        else
            queue_wait(&voice_queue, &td->ev);
    }
}

/* Voice thread entrypoint */
static void NORETURN_ATTR voice_thread(void)
{
    struct voice_thread_data td;
    char *dest;

    voice_data_init(&td);

    /* audio thread will only set this once after it finished the final
     * audio hardware init so this little construct is safe - even
     * cross-core. */
    while (!audio_is_thread_ready())
        sleep(0);

    goto message_wait;

    while (1)
    {
        td.state = TSTATE_DECODE;

        if (!queue_empty(&voice_queue))
        {
        message_wait:
            queue_wait(&voice_queue, &td.ev);

        message_process:
            voice_message(&td);

            /* Branch to initial start point or branch back to previous
             * operation if interrupted by a message */
            switch (td.state)
            {
            case TSTATE_DECODE:        goto voice_decode;
            case TSTATE_BUFFER_INSERT: goto buffer_insert;
            default:                   goto message_wait;
            }
        }

    voice_decode:
        /* Decode the data */
        if (speex_decode_int(td.st, &td.bits, voice_output_buf) < 0)
        {
            /* End of stream or error - get next clip */
            td.vi.size = 0;

            if (td.vi.get_more != NULL)
                td.vi.get_more(&td.vi.start, &td.vi.size);

            if (td.vi.start != NULL && (ssize_t)td.vi.size > 0)
            {
                /* Make bit buffer use our own buffer */
                speex_bits_set_bit_buffer(&td.bits, td.vi.start, td.vi.size);
                /* Don't skip any samples when we're stringing clips together */
                td.lookahead = 0;

                /* Paranoid check - be sure never to somehow get stuck in a
                 * loop without listening to the queue */
                yield();

                if (!queue_empty(&voice_queue))
                    goto message_wait;
                else
                    goto voice_decode;
            }

            /* If all clips are done and not playing, force pcm playback. */
            voice_start_playback();

            td.state = TSTATE_STOPPED;
            td.ev.id = SYS_TIMEOUT;
            goto message_process;
        }

        yield();

        /* Output the decoded frame */
        td.count = VOICE_FRAME_SIZE - td.lookahead;
        td.src[0] = (const char *)&voice_output_buf[td.lookahead];
        td.src[1] = NULL;
        td.lookahead -= MIN(VOICE_FRAME_SIZE, td.lookahead);

        if (td.count <= 0)
            continue;

        td.state = TSTATE_BUFFER_INSERT;

    buffer_insert:
        /* Process the PCM samples in the DSP and send out for mixing */

        while (1)
        {
            if (!queue_empty(&voice_queue))
                goto message_wait;

            if ((dest = (char *)voice_buf_get()) != NULL)
                break;

            sleep(0);
        }

        voice_buf_commit(dsp_process(td.dsp, dest, td.src, td.count)
                         * sizeof (int32_t));
    } /* end while */
}

/* Initialize all synchronization objects create the thread */
void voice_thread_init(void)
{
    logf("Starting voice thread");
    queue_init(&voice_queue, false);

    voice_thread_id = create_thread(voice_thread, voice_stack,
            sizeof(voice_stack), CREATE_THREAD_FROZEN,
            voice_thread_name IF_PRIO(, PRIORITY_VOICE) IF_COP(, CPU));

    queue_enable_queue_send(&voice_queue, &voice_queue_sender_list,
                            voice_thread_id);
} /* voice_thread_init */

/* Unfreeze the voice thread */
void voice_thread_resume(void)
{
    logf("Thawing voice thread");
    thread_thaw(voice_thread_id);
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
size_t voicebuf_init(unsigned char *bufend)
{
    size_t size = VOICE_FRAMES * VOICE_PCM_FRAME_SIZE;
    cur_buf_out = cur_buf_in = 0;
    voicebuf = (uint32_t (*)[VOICE_PCM_FRAME_COUNT])(bufend - size);
    return size;
}
