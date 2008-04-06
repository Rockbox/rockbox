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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "system.h"
#include "thread.h"
#include "logf.h"
#include "voice_thread.h"
#include "talk.h"
#include "dsp.h"
#include "audio.h"
#include "playback.h"
#include "pcmbuf.h"
#include "codecs/libspeex/speex/speex.h"

/* Define any of these as "1" to log regular and/or timeout messages */
#define VOICE_LOGQUEUES 0
#define VOICE_LOGQUEUES_SYS_TIMEOUT 0

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

#define VOICE_FRAME_SIZE    320 /* Samples / frame */
#define VOICE_SAMPLE_RATE 16000 /* Sample rate in HZ */
#define VOICE_SAMPLE_DEPTH   16 /* Sample depth in bits */

/* Voice thread variables */
static struct thread_entry *voice_thread_p = NULL;
static long voice_stack[0x7c0/sizeof(long)] IBSS_ATTR_VOICE_STACK;
static const char voice_thread_name[] = "voice";

/* Voice thread synchronization objects */
static struct event_queue voice_queue SHAREDBSS_ATTR;
static struct mutex voice_mutex SHAREDBSS_ATTR;
static struct event voice_event SHAREDBSS_ATTR;
static struct queue_sender_list voice_queue_sender_list SHAREDBSS_ATTR;

/* Buffer for decoded samples */
static spx_int16_t voice_output_buf[VOICE_FRAME_SIZE] CACHEALIGN_ATTR;

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
    Q_VOICE_STATE,    /* Query playing state */
};

/* Structure to store clip data callback info */
struct voice_info
{
    pcm_more_callback_type get_more; /* Callback to get more clips */
    unsigned char *start;            /* Start of clip */
    size_t size;                     /* Size of clip */
};

/* Private thread data for its current state that must be passed to its
 * internal functions */
struct voice_thread_data
{
    int state;              /* Thread state (TSTATE_*) */
    struct queue_event ev;  /* Last queue event pulled from queue */
    void *st;               /* Decoder instance */
    SpeexBits bits;         /* Bit cursor */
    struct dsp_config *dsp; /* DSP used for voice output */
    struct voice_info vi;   /* Copy of clip data */
    const char *src[2];     /* Current output buffer pointers */
    int lookahead;          /* Number of samples to drop at start of clip */
    int count;              /* Count of samples remaining to send to PCM */
};

/* Audio playback is in a playing state? */
static inline bool playback_is_playing(void)
{
    return (audio_status() & AUDIO_STATUS_PLAY) != 0;
}

/* Stop any current clip and start playing a new one */
void mp3_play_data(const unsigned char* start, int size,
                   pcm_more_callback_type get_more)
{
    /* Shared struct to get data to the thread - once it replies, it has
     * safely cached it in its own private data */
    static struct voice_info voice_clip SHAREDBSS_ATTR;

    if (get_more != NULL && start != NULL && (ssize_t)size > 0)
    {
        mutex_lock(&voice_mutex);

        voice_clip.get_more = get_more;
        voice_clip.start    = (unsigned char *)start;
        voice_clip.size     = size;
        LOGFQUEUE("mp3 >| voice Q_VOICE_PLAY");
        queue_send(&voice_queue, Q_VOICE_PLAY, (intptr_t)&voice_clip);

        mutex_unlock(&voice_mutex);
    }
}

/* Stop current voice clip from playing */
void mp3_play_stop(void)
{
    mutex_lock(&voice_mutex); /* Sync against voice_stop */

    LOGFQUEUE("mp3 > voice Q_VOICE_STOP: 1");
    queue_remove_from_head(&voice_queue, Q_VOICE_STOP);
    queue_post(&voice_queue, Q_VOICE_STOP, 1);

    mutex_unlock(&voice_mutex);
}

void mp3_play_pause(bool play)
{
    /* a dummy */
    (void)play;
}

/* Tell is voice is still in a playing state */
bool mp3_is_playing(void)
{
    /* TODO: Implement a timeout or state query function for event objects */
    LOGFQUEUE("mp3 >| voice Q_VOICE_STATE");
    int state = queue_send(&voice_queue, Q_VOICE_STATE, 0);
    return state != TSTATE_STOPPED;
}

/* This function is meant to be used by the buffer request functions to
   ensure the codec is no longer active */
void voice_stop(void)
{
    mutex_lock(&voice_mutex);

    /* Stop the output and current clip */
    LOGFQUEUE("mp3 >| voice Q_VOICE_STOP: 1");
    queue_send(&voice_queue, Q_VOICE_STOP, 1);

    /* Careful if using sync objects in talk.c - make sure locking order is
     * observed with one or the other always granted first */

    /* Unqueue all future clips */
    talk_force_shutup();

    /* Wait for any final queue_post to be processed */
    LOGFQUEUE("mp3 >| voice Q_VOICE_NULL");
    queue_send(&voice_queue, Q_VOICE_NULL, 0);

    mutex_unlock(&voice_mutex);
} /* voice_stop */

/* Wait for voice to finish speaking. */
void voice_wait(void)
{
    /* NOTE: One problem here is that we can't tell if another thread started a
     * new clip by the time we wait. This should be resolvable if conditions
     * ever require knowing the very clip you requested has finished. */
    event_wait(&voice_event, STATE_SIGNALED);
    /* Wait for PCM buffer to be exhausted. Works only if not playing. */
    while(!playback_is_playing() && pcm_is_playing())
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
            /* Put up a block for completion signal */
            event_set_state(&voice_event, STATE_NONSIGNALED);

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
            else if (!playback_is_playing())
            {
                /* Just voice, stop any clip still playing */
                pcmbuf_play_stop();
            }

            /* Clean-start the decoder */
            td->st = speex_decoder_init(&speex_wb_mode);

            /* Make bit buffer use our own buffer */
            speex_bits_set_bit_buffer(&td->bits, td->vi.start, td->vi.size);
            speex_decoder_ctl(td->st, SPEEX_GET_LOOKAHEAD, &td->lookahead);

            td->state = TSTATE_DECODE;
            return;

        case Q_VOICE_STOP:
            LOGFQUEUE("voice < Q_VOICE_STOP: %d", ev.data);

            if (td->ev.data != 0 && !playback_is_playing())
            {
                /* If not playing, it's just voice so stop pcm playback */
                pcmbuf_play_stop();
            }

            /* Cancel boost */
            cancel_cpu_boost();

            td->state = TSTATE_STOPPED;
            event_set_state(&voice_event, STATE_SIGNALED);
            break;

        case Q_VOICE_STATE:
            LOGFQUEUE("voice < Q_VOICE_STATE");
            queue_reply(&voice_queue, td->state);

            if (td->state == TSTATE_STOPPED)
                break; /* Not in a playback state */

            return;

        default:
            /* Default messages get a reply and thread continues with no
             * state transition */
            LOGFQUEUE("voice < default");

            if (td->state == TSTATE_STOPPED)
                break;  /* Not in playback state */

            queue_reply(&voice_queue, 0);
            return;
        }

        queue_wait(&voice_queue, &td->ev);
    }
}

/* Voice thread entrypoint */
static void voice_thread(void)
{
    struct voice_thread_data td;

    voice_data_init(&td);
    audio_wait_for_init();
    
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
            if (!pcm_is_playing())
                pcmbuf_play_start();

            /* Synthesize a stop request */
            /* NOTE: We have no way to know when the pcm data placed in the
             * buffer is actually consumed and playback has reached the end
             * so until the info is available or inferred somehow, this will
             * not be accurate and the stopped signal will come too soon.
             * ie. You may not hear the "Shutting Down" splash even though
             * it waits for voice to stop. */
            td.ev.id = Q_VOICE_STOP;
            td.ev.data = 0; /* Let PCM drain by itself */
            yield();
            goto message_process;
        }

        yield();

        /* Output the decoded frame */
        td.count = VOICE_FRAME_SIZE - td.lookahead;
        td.src[0] = (const char *)&voice_output_buf[td.lookahead];
        td.src[1] = NULL;
        td.lookahead -= MIN(VOICE_FRAME_SIZE, td.lookahead);

    buffer_insert:
        /* Process the PCM samples in the DSP and send out for mixing */
        td.state = TSTATE_BUFFER_INSERT;

        while (td.count > 0)
        {
            int out_count = dsp_output_count(td.dsp, td.count);
            int inp_count;
            char *dest;

            while (1)
            {
                if (!queue_empty(&voice_queue))
                    goto message_wait;

                if ((dest = pcmbuf_request_voice_buffer(&out_count)) != NULL)
                    break;

                yield();
            }

            /* Get the real input_size for output_size bytes, guarding
             * against resampling buffer overflows. */
            inp_count = dsp_input_count(td.dsp, out_count);

            if (inp_count <= 0)
                break;

            /* Input size has grown, no error, just don't write more than
             * length */
            if (inp_count > td.count)
                inp_count = td.count;

            out_count = dsp_process(td.dsp, dest, td.src, inp_count);

            if (out_count <= 0)
                break;

            pcmbuf_write_voice_complete(out_count);
            td.count -= inp_count;
        }

        yield();
    } /* end while */
} /* voice_thread */

/* Initialize all synchronization objects create the thread */
void voice_thread_init(void)
{
    logf("Starting voice thread");
    queue_init(&voice_queue, false);
    mutex_init(&voice_mutex);
    event_init(&voice_event, STATE_SIGNALED | EVENT_MANUAL);
    voice_thread_p = create_thread(voice_thread, voice_stack,
            sizeof(voice_stack), CREATE_THREAD_FROZEN,
            voice_thread_name IF_PRIO(, PRIORITY_PLAYBACK) IF_COP(, CPU));

    queue_enable_queue_send(&voice_queue, &voice_queue_sender_list,
                            voice_thread_p);
} /* voice_thread_init */

/* Unfreeze the voice thread */
void voice_thread_resume(void)
{
    logf("Thawing voice thread");
    thread_thaw(voice_thread_p);
}

#ifdef HAVE_PRIORITY_SCHEDULING
/* Set the voice thread priority */
void voice_thread_set_priority(int priority)
{
    thread_set_priority(voice_thread_p, priority);
}
#endif
