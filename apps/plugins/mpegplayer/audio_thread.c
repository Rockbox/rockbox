/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * mpegplayer audio thread implementation
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
#include "../../codecs/libmad/bit.h"
#include "../../codecs/libmad/mad.h"

/** Audio stream and thread **/
struct pts_queue_slot;
struct audio_thread_data
{
    struct queue_event ev;  /* Our event queue to receive commands */
    int state;              /* Thread state */
    int status;             /* Media status (STREAM_PLAYING, etc.) */
    int mad_errors;         /* A count of the errors in each frame */
    unsigned samplerate;    /* Current stream sample rate */
    int nchannels;          /* Number of audio channels */
    struct dsp_config *dsp; /* The DSP we're using */
};

/* The audio stack is stolen from the core codec thread (but not in uisim) */
/* Used for stealing codec thread's stack */
static uint32_t* audio_stack;
static size_t audio_stack_size; /* Keep gcc happy and init */
#define AUDIO_STACKSIZE (9*1024)
#ifndef SIMULATOR
static uint32_t codec_stack_copy[AUDIO_STACKSIZE / sizeof(uint32_t)];
#endif
static struct event_queue audio_str_queue SHAREDBSS_ATTR;
static struct queue_sender_list audio_str_queue_send SHAREDBSS_ATTR;
struct stream audio_str IBSS_ATTR;

/* libmad related definitions */
static struct mad_stream stream IBSS_ATTR;
static struct mad_frame  frame IBSS_ATTR;
static struct mad_synth  synth IBSS_ATTR;

/* 2567 bytes */
static unsigned char mad_main_data[MAD_BUFFER_MDLEN];

/* There isn't enough room for this in IRAM on PortalPlayer, but there
   is for Coldfire. */

/* 4608 bytes */
#ifdef CPU_COLDFIRE
static mad_fixed_t mad_frame_overlap[2][32][18] IBSS_ATTR;
#else
static mad_fixed_t mad_frame_overlap[2][32][18];
#endif

/** A queue for saving needed information about MPEG audio packets **/
#define AUDIODESC_QUEUE_LEN  (1 << 5) /* 32 should be way more than sufficient -
                                         if not, the case is handled */
#define AUDIODESC_QUEUE_MASK (AUDIODESC_QUEUE_LEN-1)
struct audio_frame_desc
{
    uint32_t time;  /* Time stamp for packet in audio ticks       */
    ssize_t  size;  /* Number of unprocessed bytes left in packet */
};

 /* This starts out wr == rd but will never be emptied to zero during
    streaming again in order to support initializing the first packet's
    timestamp without a special case */
struct
{
    /* Compressed audio data */
    uint8_t *start;  /* Start of encoded audio buffer */
    uint8_t *ptr;    /* Pointer to next encoded audio data */
    ssize_t used;    /* Number of bytes in MPEG audio buffer */
    /* Compressed audio data descriptors */
    unsigned read, write;
    struct audio_frame_desc *curr; /* Current slot */
    struct audio_frame_desc descs[AUDIODESC_QUEUE_LEN];
} audio_queue;

static inline int audiodesc_queue_count(void)
{
    return audio_queue.write - audio_queue.read;
}

static inline bool audiodesc_queue_full(void)
{
    return audio_queue.used >= MPA_MAX_FRAME_SIZE + MAD_BUFFER_GUARD ||
            audiodesc_queue_count() >= AUDIODESC_QUEUE_LEN;
}

/* Increments the queue tail postion - should be used to preincrement */
static inline void audiodesc_queue_add_tail(void)
{
    if (audiodesc_queue_full())
    {
        DEBUGF("audiodesc_queue_add_tail: audiodesc queue full!\n");
        return;
    }

    audio_queue.write++;
}

/* Increments the queue tail position - leaves one slot as current */
static inline bool audiodesc_queue_remove_head(void)
{
    if (audio_queue.write == audio_queue.read)
        return false;

    audio_queue.read++;
    return true;
}

/* Returns the "tail" at the index just behind the write index */
static inline struct audio_frame_desc * audiodesc_queue_tail(void)
{
    return &audio_queue.descs[(audio_queue.write - 1) & AUDIODESC_QUEUE_MASK];
}

/* Returns a pointer to the current head */
static inline struct audio_frame_desc * audiodesc_queue_head(void)
{
    return &audio_queue.descs[audio_queue.read & AUDIODESC_QUEUE_MASK];
}

/* Resets the pts queue - call when starting and seeking */
static void audio_queue_reset(void)
{
    audio_queue.ptr = audio_queue.start;
    audio_queue.used = 0;
    audio_queue.read = 0;
    audio_queue.write = 0;
    rb->memset(audio_queue.descs, 0, sizeof (audio_queue.descs));
    audio_queue.curr = audiodesc_queue_head();
}

static void audio_queue_advance_pos(ssize_t len)
{
    audio_queue.ptr        += len;
    audio_queue.used       -= len;
    audio_queue.curr->size -= len;
}

static int audio_buffer(struct stream *str, enum stream_parse_mode type)
{
    int ret = STREAM_OK;

    /* Carry any overshoot to the next size since we're technically
       -size bytes into it already. If size is negative an audio
       frame was split across packets. Old has to be saved before
       moving the head. */
    if (audio_queue.curr->size <= 0 && audiodesc_queue_remove_head())
    {
        struct audio_frame_desc *old = audio_queue.curr;
        audio_queue.curr = audiodesc_queue_head();
        audio_queue.curr->size += old->size;
        old->size = 0;
    }

    /* Add packets to compressed audio buffer until it's full or the
     * timestamp queue is full - whichever happens first */
    while (!audiodesc_queue_full())
    {
        ret = parser_get_next_data(str, type);
        struct audio_frame_desc *curr;
        ssize_t len;

        if (ret != STREAM_OK)
            break;

        /* Get data from next audio packet */
        len = str->curr_packet_end - str->curr_packet;

        if (str->pkt_flags & PKT_HAS_TS)
        {
            audiodesc_queue_add_tail();
            curr = audiodesc_queue_tail();
            curr->time = TS_TO_TICKS(str->pts);
            /* pts->size should have been zeroed when slot was
               freed */
        }
        else
        {
            /* Add to the one just behind the tail - this may be
             * the head or the previouly added tail - whether or
             * not we'll ever reach this is quite in question
             * since audio always seems to have every packet
             * timestamped */
            curr = audiodesc_queue_tail();
        }

        curr->size += len;

        /* Slide any remainder over to beginning */
        if (audio_queue.ptr > audio_queue.start && audio_queue.used > 0)
        {
            rb->memmove(audio_queue.start, audio_queue.ptr,
                        audio_queue.used);
        }

        /* Splice this packet onto any remainder */
        rb->memcpy(audio_queue.start + audio_queue.used,
                   str->curr_packet, len);

        audio_queue.used += len;
        audio_queue.ptr = audio_queue.start;

        rb->yield();
    }

    return ret;
}

/* Initialise libmad */
static void init_mad(void)
{
    mad_stream_init(&stream);
    mad_frame_init(&frame);
    mad_synth_init(&synth);

    /* We do this so libmad doesn't try to call codec_calloc() */
    rb->memset(mad_frame_overlap, 0, sizeof(mad_frame_overlap));
    frame.overlap = (void *)mad_frame_overlap;

    rb->memset(mad_main_data, 0, sizeof(mad_main_data));
    stream.main_data = &mad_main_data;
}

/* Sync audio stream to a particular frame - see main decoder loop for
 * detailed remarks */
static int audio_sync(struct audio_thread_data *td,
                      struct str_sync_data *sd)
{
    int retval = STREAM_MATCH;
    uint32_t sdtime = TS_TO_TICKS(clip_time(&audio_str, sd->time));
    uint32_t time;
    uint32_t duration = 0;
    struct stream *str;
    struct stream tmp_str;
    struct mad_header header;
    struct mad_stream stream;

    if (td->ev.id == STREAM_SYNC)
    {
        /* Actually syncing for playback - use real stream */
        time = 0;
        str = &audio_str;
    }
    else
    {
        /* Probing - use temp stream */
        time = INVALID_TIMESTAMP;
        str = &tmp_str;
        str->id = audio_str.id;
    }        

    str->hdr.pos = sd->sk.pos;
    str->hdr.limit = sd->sk.pos + sd->sk.len;

    mad_stream_init(&stream);
    mad_header_init(&header);

    while (1)
    {
        if (audio_buffer(str, STREAM_PM_RANDOM_ACCESS) == STREAM_DATA_END)
        {
            DEBUGF("audio_sync:STR_DATA_END\n  aqu:%ld swl:%ld swr:%ld\n",
                    audio_queue.used, str->hdr.win_left, str->hdr.win_right);
            if (audio_queue.used <= MAD_BUFFER_GUARD)
                goto sync_data_end;
        }

        stream.error = 0;
        mad_stream_buffer(&stream, audio_queue.ptr, audio_queue.used);

        if (stream.sync && mad_stream_sync(&stream) < 0)
        {
            DEBUGF(" audio: mad_stream_sync failed\n");
            audio_queue_advance_pos(MAX(audio_queue.curr->size - 1, 1));
            continue;
        }

        stream.sync = 0;

        if (mad_header_decode(&header, &stream) < 0)
        {
            DEBUGF(" audio: mad_header_decode failed:%s\n",
                   mad_stream_errorstr(&stream));
            audio_queue_advance_pos(1);
            continue;
        }

        duration = 32*MAD_NSBSAMPLES(&header);
        time = audio_queue.curr->time;

        DEBUGF(" audio: ft:%u t:%u fe:%u nsamp:%u sampr:%u\n",
               (unsigned)TICKS_TO_TS(time), (unsigned)sd->time,
               (unsigned)TICKS_TO_TS(time + duration),
               (unsigned)duration, header.samplerate);

        audio_queue_advance_pos(stream.this_frame - audio_queue.ptr);

        if (time <= sdtime && sdtime < time + duration)
        {
            DEBUGF(" audio: ft<=t<fe\n");
            retval = STREAM_PERFECT_MATCH;
            break;
        }
        else if (time > sdtime)
        {
            DEBUGF(" audio: ft>t\n");
            break;
        }

        audio_queue_advance_pos(stream.next_frame - audio_queue.ptr);
        audio_queue.curr->time += duration;

        rb->yield();
    }

sync_data_end:
    if (td->ev.id == STREAM_FIND_END_TIME)
    {
        if (time != INVALID_TIMESTAMP)
        {
            time = TICKS_TO_TS(time);
            duration = TICKS_TO_TS(duration);
            sd->time = time + duration;
            retval = STREAM_PERFECT_MATCH;
        }
        else
        {
            retval = STREAM_NOT_FOUND;
        }
    }

    DEBUGF(" audio header: 0x%02X%02X%02X%02X\n",
           (unsigned)audio_queue.ptr[0], (unsigned)audio_queue.ptr[1],
           (unsigned)audio_queue.ptr[2], (unsigned)audio_queue.ptr[3]);

    return retval;
    (void)td;
}

static void audio_thread_msg(struct audio_thread_data *td)
{
    while (1)
    {
        intptr_t reply = 0;

        switch (td->ev.id)
        {
        case STREAM_PLAY:
            td->status = STREAM_PLAYING;

            switch (td->state)
            {
            case TSTATE_INIT:
                td->state = TSTATE_DECODE;
            case TSTATE_DECODE:
            case TSTATE_RENDER_WAIT:
            case TSTATE_RENDER_WAIT_END:
                break;

            case TSTATE_EOS:
                /* At end of stream - no playback possible so fire the
                 * completion event */
                stream_generate_event(&audio_str, STREAM_EV_COMPLETE, 0);
                break;
            }

            break;

        case STREAM_PAUSE:
            td->status = STREAM_PAUSED;
            reply = td->state != TSTATE_EOS;
            break;

        case STREAM_STOP:
            if (td->state == TSTATE_DATA)
                stream_clear_notify(&audio_str, DISK_BUF_DATA_NOTIFY);

            td->status = STREAM_STOPPED;
            td->state = TSTATE_EOS;

            reply = true;
            break;            

        case STREAM_RESET:
            if (td->state == TSTATE_DATA)
                stream_clear_notify(&audio_str, DISK_BUF_DATA_NOTIFY);

            td->status = STREAM_STOPPED;
            td->state = TSTATE_INIT;
            td->samplerate = 0;
            td->nchannels = 0;

            init_mad();
            td->mad_errors = 0;

            audio_queue_reset();

            reply = true;
            break;

        case STREAM_NEEDS_SYNC:
            reply = true; /* Audio always needs to */
            break;

        case STREAM_SYNC:
        case STREAM_FIND_END_TIME:
            if (td->state != TSTATE_INIT)
                break;

            reply = audio_sync(td, (struct str_sync_data *)td->ev.data);
            break;

        case DISK_BUF_DATA_NOTIFY:
            /* Our bun is done */
            if (td->state != TSTATE_DATA)
                break;

            td->state = TSTATE_DECODE;
            str_data_notify_received(&audio_str);
            break;

        case STREAM_QUIT:
            /* Time to go - make thread exit */
            td->state = TSTATE_EOS;
            return;
        }

        str_reply_msg(&audio_str, reply);

        if (td->status == STREAM_PLAYING)
        {
            switch (td->state)
            {
            case TSTATE_DECODE:
            case TSTATE_RENDER_WAIT:
            case TSTATE_RENDER_WAIT_END:
                /* These return when in playing state */
                return;
            }
        }

        str_get_msg(&audio_str, &td->ev);
    }
}

static void audio_thread(void)
{
    struct audio_thread_data td;

    rb->memset(&td, 0, sizeof (td));
    td.status = STREAM_STOPPED;
    td.state = TSTATE_EOS;

    /* We need this here to init the EMAC for Coldfire targets */
    init_mad();

    td.dsp = (struct dsp_config *)rb->dsp_configure(NULL, DSP_MYDSP,
                                                    CODEC_IDX_AUDIO);
    rb->sound_set_pitch(1000);
    rb->dsp_configure(td.dsp, DSP_RESET, 0);
    rb->dsp_configure(td.dsp, DSP_SET_SAMPLE_DEPTH, MAD_F_FRACBITS);

    goto message_wait;

    /* This is the decoding loop. */
    while (1)
    {
        td.state = TSTATE_DECODE;

        /* Check for any pending messages and process them */
        if (str_have_msg(&audio_str))
        {
        message_wait:
            /* Wait for a message to be queued */
            str_get_msg(&audio_str, &td.ev);

        message_process:
            /* Process a message already dequeued */
            audio_thread_msg(&td);

            switch (td.state)
            {
            /* These states are the only ones that should return */
            case TSTATE_DECODE:          goto audio_decode;
            case TSTATE_RENDER_WAIT:     goto render_wait;
            case TSTATE_RENDER_WAIT_END: goto render_wait_end;
            /* Anything else is interpreted as an exit */
            default:                     return;
            }
        }

    audio_decode:

        /** Buffering **/
        switch (audio_buffer(&audio_str, STREAM_PM_STREAMING))
        {
        case STREAM_DATA_NOT_READY:
        {
            td.state = TSTATE_DATA;
            goto message_wait;
            } /* STREAM_DATA_NOT_READY: */

        case STREAM_DATA_END:
        {
            if (audio_queue.used > MAD_BUFFER_GUARD)
                break;

            /* Used up remainder of compressed audio buffer.
             * Force any residue to play if audio ended before
             * reaching the threshold */
            td.state = TSTATE_RENDER_WAIT_END;
            audio_queue_reset();

        render_wait_end:
            pcm_output_drain();

            while (pcm_output_used() > (ssize_t)PCMOUT_LOW_WM)
            {
                str_get_msg_w_tmo(&audio_str, &td.ev, 1);
                if (td.ev.id != SYS_TIMEOUT)
                    goto message_process;
            }

            td.state = TSTATE_EOS;
            if (td.status == STREAM_PLAYING)
                stream_generate_event(&audio_str, STREAM_EV_COMPLETE, 0);

            rb->yield();
            goto message_wait;
            } /* STREAM_DATA_END: */
        }

        /** Decoding **/
        mad_stream_buffer(&stream, audio_queue.ptr, audio_queue.used);

        int mad_stat = mad_frame_decode(&frame, &stream);

        ssize_t len = stream.next_frame - audio_queue.ptr;

        if (mad_stat != 0)
        {
            DEBUGF("audio: Stream error: %s\n",
                   mad_stream_errorstr(&stream));

            /* If something's goofed - try to perform resync by moving
             * at least one byte at a time */
            audio_queue_advance_pos(MAX(len, 1));

            if (stream.error == MAD_FLAG_INCOMPLETE
                || stream.error == MAD_ERROR_BUFLEN)
            {
                /* This makes the codec support partially corrupted files */
                if (++td.mad_errors <= MPA_MAX_FRAME_SIZE)
                {
                    stream.error = 0;
                    rb->yield();
                    continue;
                }
                DEBUGF("audio: Too many errors\n");
            }
            else if (MAD_RECOVERABLE(stream.error))
            {
                /* libmad says it can recover - just keep on decoding */
                rb->yield();
                continue;
            }
            else
            {
                /* Some other unrecoverable error */
                DEBUGF("audio: Unrecoverable error\n");
            }

            /* This is too hard - bail out */
            td.state = TSTATE_EOS;

            if (td.status == STREAM_PLAYING)
                stream_generate_event(&audio_str, STREAM_EV_COMPLETE, 0);

            td.status = STREAM_ERROR;
            goto message_wait;
        }

        /* Adjust sizes by the frame size */
        audio_queue_advance_pos(len);
        td.mad_errors = 0; /* Clear errors */

        /* Generate the pcm samples */
        mad_synth_frame(&synth, &frame);

        /** Output **/
        if (frame.header.samplerate != td.samplerate)
        {
            td.samplerate = frame.header.samplerate;
            rb->dsp_configure(td.dsp, DSP_SWITCH_FREQUENCY,
                              td.samplerate);
        }

        if (MAD_NCHANNELS(&frame.header) != td.nchannels)
        {
            td.nchannels = MAD_NCHANNELS(&frame.header);
            rb->dsp_configure(td.dsp, DSP_SET_STEREO_MODE,
                              td.nchannels == 1 ?
                                STEREO_MONO : STEREO_NONINTERLEAVED);
        }

        td.state  = TSTATE_RENDER_WAIT;

        /* Add a frame of audio to the pcm buffer. Maximum is 1152 samples. */
    render_wait:
        if (synth.pcm.length > 0)
        {
            struct pcm_frame_header *dst_hdr = pcm_output_get_buffer();
            const char *src[2] =
                { (char *)synth.pcm.samples[0], (char *)synth.pcm.samples[1] };
            int out_count = (synth.pcm.length * CLOCK_RATE
                                + (td.samplerate - 1)) / td.samplerate;
            ssize_t size = sizeof(*dst_hdr) + out_count*4;

            /* Wait for required amount of free buffer space */
            while (pcm_output_free() < size)
            {
                /* Wait one frame */
                int timeout = out_count*HZ / td.samplerate;
                str_get_msg_w_tmo(&audio_str, &td.ev, MAX(timeout, 1));
                if (td.ev.id != SYS_TIMEOUT)
                    goto message_process;
            }

            out_count = rb->dsp_process(td.dsp, dst_hdr->data, src,
                                        synth.pcm.length);

            if (out_count <= 0)
                break;

            dst_hdr->size = sizeof(*dst_hdr) + out_count*4;
            dst_hdr->time = audio_queue.curr->time;

            /* As long as we're on this timestamp, the time is just
               incremented by the number of samples */
            audio_queue.curr->time += out_count;

            /* Make this data available to DMA */
            pcm_output_add_data();
        }

        rb->yield();
    } /* end decoding loop */
}

/* Initializes the audio thread resources and starts the thread */
bool audio_thread_init(void)
{
    int i;
#ifdef SIMULATOR
    /* The simulator thread implementation doesn't have stack buffers, and
       these parameters are ignored. */
    (void)i;  /* Keep gcc happy */
    audio_stack = NULL;
    audio_stack_size = 0;
#else
    /* Borrow the codec thread's stack (in IRAM on most targets) */
    audio_stack = NULL;
    for (i = 0; i < MAXTHREADS; i++)
    {
        if (rb->strcmp(rb->threads[i].name, "codec") == 0)
        {
            /* Wait to ensure the codec thread has blocked */
            while (rb->threads[i].state != STATE_BLOCKED)
                rb->yield();

            /* Now we can steal the stack */
            audio_stack = rb->threads[i].stack;
            audio_stack_size = rb->threads[i].stack_size;

            /* Backup the codec thread's stack */
            rb->memcpy(codec_stack_copy, audio_stack, audio_stack_size);
            break;
        }
    }

    if (audio_stack == NULL)
    {
        /* This shouldn't happen, but deal with it anyway by using
           the copy instead */
        audio_stack = codec_stack_copy;
        audio_stack_size = AUDIO_STACKSIZE;
    }
#endif

    /* Initialise the encoded audio buffer and its descriptors */
    audio_queue.start = mpeg_malloc(AUDIOBUF_ALLOC_SIZE,
                                    MPEG_ALLOC_AUDIOBUF);
    if (audio_queue.start == NULL)
        return false;

    /* Start the audio thread */
    audio_str.hdr.q = &audio_str_queue;
    rb->queue_init(audio_str.hdr.q, false);

    /* One-up on the priority since the core DSP over-yields internally */
    audio_str.thread = rb->create_thread(
        audio_thread, audio_stack, audio_stack_size, 0,
        "mpgaudio" IF_PRIO(,PRIORITY_PLAYBACK-4) IF_COP(, CPU));

    rb->queue_enable_queue_send(audio_str.hdr.q, &audio_str_queue_send,
                                audio_str.thread);

    if (audio_str.thread == NULL)
        return false;

    /* Wait for thread to initialize */
    str_send_msg(&audio_str, STREAM_NULL, 0);

    return true;
}

/* Stops the audio thread */
void audio_thread_exit(void)
{
    if (audio_str.thread != NULL)
    {
        str_post_msg(&audio_str, STREAM_QUIT, 0);
        rb->thread_wait(audio_str.thread);
        audio_str.thread = NULL;
    }

#ifndef SIMULATOR
    /* Restore the codec thread's stack */
    rb->memcpy(audio_stack, codec_stack_copy, audio_stack_size);    
#endif
}
