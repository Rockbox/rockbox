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

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "core_alloc.h"
#include "thread.h"
#include "appevents.h"
#include "voice_thread.h"
#include "talk.h"
#include "dsp_core.h"
#include "pcm.h"
#include "pcm_mixer.h"
#include "codecs/libspeex/speex/speex.h"
#include "settings.h"

/* Default number of PCM frames to queue - adjust as necessary per-target */
#define VOICE_FRAMES 4

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

/* A speex frame generally consists of 20ms of audio
 * (http://www.speex.org/docs/manual/speex-manual/node10.html)
 * for wideband mode this results in 320 samples of decoded PCM.
 */
#define VOICE_FRAME_COUNT   320 /* Samples / frame */
#define VOICE_SAMPLE_RATE 16000 /* Sample rate in HZ */
#define VOICE_SAMPLE_DEPTH   16 /* Sample depth in bits */
/* The max. wideband bitrate is 42.4 kbps
 * (http://www.speex.org/docs/manual/speex-manual/node11.html). For 20ms
 * this gives a maximum of 106 bytes for an encoded speex frame */
#define VOICE_MAX_ENCODED_FRAME_SIZE 106

/* Voice thread variables */
static unsigned int voice_thread_id = 0;
#if defined(CPU_MIPS)
/* MIPS is stack-hungry */
#define VOICE_STACK_EXTRA   0x500
#elif defined(CPU_COLDFIRE)
/* ISR uses any available stack - need a bit more room */
#define VOICE_STACK_EXTRA   0x400
#elif (CONFIG_PLATFORM & PLATFORM_HOSTED)
/* Needed at least on the Sony NWZ hosted targets, but probably a good idea on all of them */
#define VOICE_STACK_EXTRA   0x500
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
static bool voice_playing = false;

#define VOICE_PCM_FRAME_COUNT   ((PLAY_SAMPR_MAX*VOICE_FRAME_COUNT + \
                                  VOICE_SAMPLE_RATE) / VOICE_SAMPLE_RATE)
#define VOICE_PCM_FRAME_SIZE    (VOICE_PCM_FRAME_COUNT*2*sizeof (int16_t))

/* Voice processing states */
enum voice_state
{
    VOICE_STATE_MESSAGE = 0,
    VOICE_STATE_DECODE,
    VOICE_STATE_BUFFER_INSERT,
    VOICE_STATE_QUIT,
};

/* A delay to not bring audio back to normal level too soon */
#define QUIET_COUNT 3

enum voice_thread_messages
{
    Q_VOICE_PLAY = 0, /* Play a clip */
    Q_VOICE_STOP,     /* Stop current clip */
    Q_VOICE_KILL,     /* Kill voice thread till restart*/
};

/* Structure to store clip data callback info */
struct voice_info
{
    /* Callback to get more clips */
    voice_play_callback_t get_more;
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
    int lookahead;          /* Number of samples to drop at start of clip */
    struct dsp_buffer src;  /* Speex output buffer/input to DSP */
    struct dsp_buffer *dst; /* Pointer to DSP output buffer for PCM */
};

/* Functions called in their repective state that return the next state to
   state machine loop - compiler may inline them at its discretion */
static enum voice_state voice_message(struct voice_thread_data *td);
static enum voice_state voice_decode(struct voice_thread_data *td);
static enum voice_state voice_buffer_insert(struct voice_thread_data *td);

/* Might have lookahead and be skipping samples, so size is needed */
static struct voice_buf
{
    /* Buffer for decoded samples */
    spx_int16_t spx_outbuf[VOICE_FRAME_COUNT];
    /* Queue frame indexes */
    unsigned int volatile frame_in;
    unsigned int volatile frame_out;
    /* For PCM pointer adjustment */
    struct voice_thread_data *td;
    /* Buffers for mixing voice */
    struct voice_pcm_frame
    {
        size_t size;
        int16_t pcm[2*VOICE_PCM_FRAME_COUNT];
    } frames[VOICE_FRAMES];
} *voice_buf = NULL;

static int voice_buf_hid = 0;

static int move_callback(int handle, void *current, void *new)
{
    /* Have to adjust the pointers that point into things in voice_buf */
    off_t diff = new - current;
    struct voice_thread_data *td = voice_buf->td;

    if (td != NULL)
    {
        td->src.p32[0] = SKIPBYTES(td->src.p32[0], diff);
        td->src.p32[1] = SKIPBYTES(td->src.p32[1], diff);

        if (td->dst != NULL) /* Only when calling dsp_process */
            td->dst->p16out = SKIPBYTES(td->dst->p16out, diff);

        mixer_adjust_channel_address(PCM_MIXER_CHAN_VOICE, diff);
    }

    voice_buf = new;

    return BUFLIB_CB_OK;
    (void)handle;
};

static void sync_callback(int handle, bool sync_on)
{
    /* A move must not allow PCM to access the channel */
    if (sync_on)
        pcm_play_lock();
    else
        pcm_play_unlock();

    (void)handle;
}

static struct buflib_callbacks ops =
{
    .move_callback = move_callback,
    .sync_callback = sync_callback,
};

/* Number of frames in queue */
static unsigned int voice_unplayed_frames(void)
{
    return voice_buf->frame_in - voice_buf->frame_out;
}

/* Mixer channel callback */
static void voice_pcm_callback(const void **start, size_t *size)
{
    unsigned int frame_out = ++voice_buf->frame_out;

    if (voice_unplayed_frames() == 0)
        return; /* Done! */

    struct voice_pcm_frame *frame =
        &voice_buf->frames[frame_out % VOICE_FRAMES];

    *start = frame->pcm;
    *size = frame->size;
}

/* Start playback of voice channel if not already playing */
static void voice_start_playback(void)
{
    if (mixer_channel_status(PCM_MIXER_CHAN_VOICE) != CHANNEL_STOPPED ||
        voice_unplayed_frames() == 0)
        return;

    struct voice_pcm_frame *frame =
        &voice_buf->frames[voice_buf->frame_out % VOICE_FRAMES];

    mixer_channel_play_data(PCM_MIXER_CHAN_VOICE, voice_pcm_callback,
                            frame->pcm, frame->size);
}

/* Stop the voice channel */
static void voice_stop_playback(void)
{
    mixer_channel_stop(PCM_MIXER_CHAN_VOICE);
    voice_buf->frame_in = voice_buf->frame_out = 0;
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

    return voice_buf->frames[voice_buf->frame_in % VOICE_FRAMES].pcm;
}

/* Commit a frame returned by voice_buf_get and set the actual size */
static void voice_buf_commit(int count)
{
    if (count > 0)
    {
        unsigned int frame_in = voice_buf->frame_in;
        voice_buf->frames[frame_in % VOICE_FRAMES].size =
            count * 2 * sizeof (int16_t);
        voice_buf->frame_in = frame_in + 1;
    }
}

/* Stop any current clip and start playing a new one */
void voice_play_data(const void *start, size_t size,
                     voice_play_callback_t get_more)
{
    if (voice_thread_id && start && size && get_more)
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
void voice_play_stop(void)
{
    if (voice_thread_id != 0)
    {
        LOGFQUEUE("mp3 >| voice Q_VOICE_STOP");
        queue_send(&voice_queue, Q_VOICE_STOP, 0);
    }
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

    while (voice_playing)
        sleep(1);
}

void voice_set_mixer_level(int percent)
{
    percent *= MIX_AMP_UNITY;
    percent /= 100;
    mixer_channel_set_amplitude(PCM_MIXER_CHAN_VOICE, percent);
}

/* Initialize voice thread data that must be valid upon starting and the
 * setup the DSP parameters */
static void voice_data_init(struct voice_thread_data *td)
{
    td->dsp = dsp_get_config(CODEC_IDX_VOICE);
    dsp_configure(td->dsp, DSP_RESET, 0);
    dsp_configure(td->dsp, DSP_SET_FREQUENCY, VOICE_SAMPLE_RATE);
    dsp_configure(td->dsp, DSP_SET_SAMPLE_DEPTH, VOICE_SAMPLE_DEPTH);
    dsp_configure(td->dsp, DSP_SET_STEREO_MODE, STEREO_MONO);

    voice_set_mixer_level(global_settings.talk_mixer_amp);

    voice_buf->td = td;
    td->dst = NULL;
}

/* Voice thread message processing */
static enum voice_state voice_message(struct voice_thread_data *td)
{
    queue_wait_w_tmo(&voice_queue, &td->ev,
                     quiet_counter > 0 ? HZ/10 : TIMEOUT_BLOCK);

    switch (td->ev.id)
    {
    case Q_VOICE_PLAY:
        LOGFQUEUE("voice < Q_VOICE_PLAY");

        /* Boost CPU now */
        trigger_cpu_boost();

        if (quiet_counter != 0)
        {
            /* Stop any clip still playing */
            voice_stop_playback();
            dsp_configure(td->dsp, DSP_FLUSH, 0);
        }

        if (quiet_counter <= 0)
        {
            voice_playing = true;
            dsp_configure(td->dsp, DSP_SET_OUT_FREQUENCY, mixer_get_frequency());
            send_event(VOICE_EVENT_IS_PLAYING, &voice_playing);
        }

        quiet_counter = QUIET_COUNT;

        /* Copy the clip info */
        td->vi = *(struct voice_info *)td->ev.data;

        /* We need nothing more from the sending thread - let it run */
        queue_reply(&voice_queue, 1);

        /* Clean-start the decoder */
        td->st = speex_decoder_init(&speex_wb_mode);

        /* Make bit buffer use our own buffer */
        speex_bits_set_bit_buffer(&td->bits, (void *)td->vi.start,
                                  td->vi.size);
        speex_decoder_ctl(td->st, SPEEX_GET_LOOKAHEAD, &td->lookahead);

        return VOICE_STATE_DECODE;
    case Q_VOICE_KILL:
        queue_delete(&voice_queue);
        return VOICE_STATE_QUIT;
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
            {
                voice_playing = false;
                send_event(VOICE_EVENT_IS_PLAYING, &voice_playing);
            }
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
    if (speex_decode_int(td->st, &td->bits, voice_buf->spx_outbuf) < 0)
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
            if (voice_unplayed_frames() > 0)
                voice_start_playback();
            return VOICE_STATE_MESSAGE;
        }
    }
    else
    {
        if (td->vi.size > VOICE_MAX_ENCODED_FRAME_SIZE
            && td->bits.charPtr > (int)(td->vi.size - VOICE_MAX_ENCODED_FRAME_SIZE)
            && td->vi.get_more != NULL)
        {
            /* request more data _before_ running out of data (requesting
             * more after the fact prevents speex from successful decoding)
             * place a hint telling the callback how much of the
             * previous buffer we have consumed such that it can rewind
             * as necessary */
            int bitPtr = td->bits.bitPtr;
            td->vi.size = td->bits.charPtr;
            td->vi.get_more(&td->vi.start, &td->vi.size);
            speex_bits_set_bit_buffer(&td->bits, (void *)td->vi.start,
                                      td->vi.size);
            td->bits.bitPtr = bitPtr;
        }

        yield();

        /* Output the decoded frame */
        td->src.remcount  = VOICE_FRAME_COUNT - td->lookahead;
        td->src.pin[0]    = &voice_buf->spx_outbuf[td->lookahead];
        td->src.pin[1]    = NULL;
        td->src.proc_mask = 0;

        td->lookahead -= MIN(VOICE_FRAME_COUNT, td->lookahead);

        if (td->src.remcount > 0)
            return VOICE_STATE_BUFFER_INSERT;
    }

    return VOICE_STATE_DECODE;
}

/* Process the PCM samples in the DSP and send out for mixing */
static enum voice_state voice_buffer_insert(struct voice_thread_data *td)
{
    if (!queue_empty(&voice_queue))
        return VOICE_STATE_MESSAGE;

    struct dsp_buffer dst;

    if ((dst.p16out = voice_buf_get()) != NULL)
    {
        dst.remcount = 0;
        dst.bufcount = VOICE_PCM_FRAME_COUNT;

        td->dst = &dst;
        dsp_process(td->dsp, &td->src, &dst);
        td->dst = NULL;

        voice_buf_commit(dst.remcount);

        /* Unless other effects are introduced to voice that have delays,
           all output should have been purged to dst in one call */
        return td->src.remcount > 0 ?
            VOICE_STATE_BUFFER_INSERT : VOICE_STATE_DECODE;
    }

    sleep(0);
    return VOICE_STATE_BUFFER_INSERT;
}

/* Voice thread entrypoint */
static void voice_thread(void)
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
        case VOICE_STATE_QUIT:
            logf("Exiting voice thread");
            core_free(voice_buf_hid);
            voice_buf_hid = 0;
            return;
        }
    }
    return;
}

/* kill voice thread and dont allow re-init*/
void voice_thread_kill(void)
{
    queue_send(&voice_queue, Q_VOICE_KILL, 0);
}

/* Initialize buffers, all synchronization objects and create the thread */
void voice_thread_init(void)
{
    if (voice_thread_id != 0)
        return; /* Already did an init and succeeded at it */

    voice_buf_hid = core_alloc_ex("voice buf", sizeof (*voice_buf), &ops);

    if (voice_buf_hid <= 0)
    {
        logf("voice: core_alloc_ex failed");
        return;
    }

    voice_buf = core_get_data(voice_buf_hid);

    if (voice_buf == NULL)
    {
        logf("voice: core_get_data failed");
        core_free(voice_buf_hid);
        voice_buf_hid = 0;
        return;
    }

    memset(voice_buf, 0, sizeof (*voice_buf));

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
    if (voice_thread_id == 0)
        return;

    if (priority > PRIORITY_VOICE)
        priority = PRIORITY_VOICE;

    thread_set_priority(voice_thread_id, priority);
}
#endif
