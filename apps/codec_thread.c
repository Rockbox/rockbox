/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005-2007 Miika Pekkarinen
 * Copyright (C) 2007-2008 Nicolas Pennequin
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
#include "playback.h"
#include "codec_thread.h"
#include "kernel.h"
#include "codecs.h"
#include "buffering.h"
#include "pcmbuf.h"
#include "dsp.h"
#include "abrepeat.h"
#include "metadata.h"
#include "splash.h"

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"

/* macros to enable logf for queues
   logging on SYS_TIMEOUT can be disabled */
#ifdef SIMULATOR
/* Define this for logf output of all queuing except SYS_TIMEOUT */
#define PLAYBACK_LOGQUEUES
/* Define this to logf SYS_TIMEOUT messages */
/*#define PLAYBACK_LOGQUEUES_SYS_TIMEOUT*/
#endif

#ifdef PLAYBACK_LOGQUEUES
#define LOGFQUEUE logf
#else
#define LOGFQUEUE(...)
#endif

#ifdef PLAYBACK_LOGQUEUES_SYS_TIMEOUT
#define LOGFQUEUE_SYS_TIMEOUT logf
#else
#define LOGFQUEUE_SYS_TIMEOUT(...)
#endif


/* Variables are commented with the threads that use them:
 * A=audio, C=codec, V=voice. A suffix of - indicates that
 * the variable is read but not updated on that thread.

 * Unless otherwise noted, the extern variables are located
 * in playback.c.
 */

/* Main state control */

/* Type of codec loaded? (C/A) */
static int current_codectype SHAREDBSS_ATTR = AFMT_UNKNOWN;

extern struct mp3entry *thistrack_id3,  /* the currently playing track */
                       *othertrack_id3; /* prev track during track-change-transition, or end of playlist,
                                         * next track otherwise */

/* Track change controls */
extern struct event_queue audio_queue SHAREDBSS_ATTR;


extern struct codec_api ci; /* from codecs.c */

/* Codec thread */
static unsigned int codec_thread_id; /* For modifying thread priority later */
static struct event_queue codec_queue SHAREDBSS_ATTR;
static struct queue_sender_list codec_queue_sender_list SHAREDBSS_ATTR;
static long codec_stack[(DEFAULT_STACK_SIZE + 0x2000)/sizeof(long)]
                IBSS_ATTR;
static const char codec_thread_name[] = "codec";

/* static routines */
static void codec_queue_ack(intptr_t ackme)
{
    queue_reply(&codec_queue, ackme);
}

static intptr_t codec_queue_send(long id, intptr_t data)
{
    return queue_send(&codec_queue, id, data);
}

/**************************************/

/** misc external functions */

/* Used to check whether a new codec must be loaded. See array audio_formats[]
 * in metadata.c */
int get_codec_base_type(int type)
{
    int base_type = type;
    switch (type) {
        case AFMT_MPA_L1:
        case AFMT_MPA_L2:
        case AFMT_MPA_L3:
            base_type = AFMT_MPA_L3;
            break;
        case AFMT_MPC_SV7:
        case AFMT_MPC_SV8:
            base_type = AFMT_MPC_SV7;
            break;
        case AFMT_MP4_AAC:
        case AFMT_MP4_AAC_HE:
            base_type = AFMT_MP4_AAC;
            break;
        case AFMT_SAP:
        case AFMT_CMC:
        case AFMT_CM3:
        case AFMT_CMR:
        case AFMT_CMS:
        case AFMT_DMC:
        case AFMT_DLT:
        case AFMT_MPT:
        case AFMT_MPD:
        case AFMT_RMT:
        case AFMT_TMC:
        case AFMT_TM8:
        case AFMT_TM2:
            base_type = AFMT_SAP;
            break;
        default:
            break;
    }

    return base_type;
}

const char *get_codec_filename(int cod_spec)
{
    const char *fname;

#ifdef HAVE_RECORDING
    /* Can choose decoder or encoder if one available */
    int type = cod_spec & CODEC_TYPE_MASK;
    int afmt = cod_spec & CODEC_AFMT_MASK;

    if ((unsigned)afmt >= AFMT_NUM_CODECS)
        type = AFMT_UNKNOWN | (type & CODEC_TYPE_MASK);

    fname = (type == CODEC_TYPE_ENCODER) ?
                audio_formats[afmt].codec_enc_root_fn :
                audio_formats[afmt].codec_root_fn;

    logf("%s: %d - %s",
        (type == CODEC_TYPE_ENCODER) ? "Encoder" : "Decoder",
        afmt, fname ? fname : "<unknown>");
#else /* !HAVE_RECORDING */
    /* Always decoder */
    if ((unsigned)cod_spec >= AFMT_NUM_CODECS)
        cod_spec = AFMT_UNKNOWN;
    fname = audio_formats[cod_spec].codec_root_fn;
    logf("Codec: %d - %s",  cod_spec, fname ? fname : "<unknown>");
#endif /* HAVE_RECORDING */

    return fname;
} /* get_codec_filename */

/* Borrow the codec thread and return the ID */
void codec_thread_do_callback(void (*fn)(void), unsigned int *id)
{
    /* Set id before telling thread to call something; it may be
     * needed before this function returns. */
    if (id != NULL)
        *id = codec_thread_id;

    /* Codec thread will signal just before entering callback */
    LOGFQUEUE("codec >| Q_CODEC_DO_CALLBACK");
    codec_queue_send(Q_CODEC_DO_CALLBACK, (intptr_t)fn);
}


/** codec API callbacks */

static void* codec_get_buffer(size_t *size)
{
    ssize_t s = CODEC_SIZE - codec_size;
    void *buf = &codecbuf[codec_size];
    ALIGN_BUFFER(buf, s, CACHEALIGN_SIZE);

    if (s <= 0)
        return NULL;

    *size = s;
    return buf;
}

static void codec_pcmbuf_insert_callback(
        const void *ch1, const void *ch2, int count)
{
    const char *src[2] = { ch1, ch2 };

    while (count > 0)
    {
        int out_count = dsp_output_count(ci.dsp, count);
        int inp_count;
        char *dest;

        /* Prevent audio from a previous track from playing */
        if (ci.new_track || ci.stop_codec)
            return;

        while ((dest = pcmbuf_request_buffer(&out_count)) == NULL)
        {
            cancel_cpu_boost();
            sleep(1);
            if (ci.seek_time || ci.new_track || ci.stop_codec)
                return;
        }

        /* Get the real input_size for output_size bytes, guarding
         * against resampling buffer overflows. */
        inp_count = dsp_input_count(ci.dsp, out_count);

        if (inp_count <= 0)
            return;

        /* Input size has grown, no error, just don't write more than length */
        if (inp_count > count)
            inp_count = count;

        out_count = dsp_process(ci.dsp, dest, src, inp_count);

        if (out_count <= 0)
            return;

        pcmbuf_write_complete(out_count);

        count -= inp_count;
    }
} /* codec_pcmbuf_insert_callback */

static void codec_set_elapsed_callback(unsigned long value)
{
    if (ci.seek_time)
        return;

#ifdef AB_REPEAT_ENABLE
    ab_position_report(value);
#endif

    unsigned long latency = pcmbuf_get_latency();
    if (value < latency)
        thistrack_id3->elapsed = 0;
    else
    {
        unsigned long elapsed = value - latency;
        if (elapsed > thistrack_id3->elapsed ||
            elapsed < thistrack_id3->elapsed - 2)
            {
                thistrack_id3->elapsed = elapsed;
            }
    }
}

static void codec_set_offset_callback(size_t value)
{
    if (ci.seek_time)
        return;

    unsigned long latency = pcmbuf_get_latency() * thistrack_id3->bitrate / 8;
    if (value < latency)
        thistrack_id3->offset = 0;
    else
        thistrack_id3->offset = value - latency;
}

/* helper function, not a callback */
static void codec_advance_buffer_counters(size_t amount)
{
    bufadvance(get_audio_hid(), amount);
    ci.curpos += amount;
}

/* copy up-to size bytes into ptr and return the actual size copied */
static size_t codec_filebuf_callback(void *ptr, size_t size)
{
    ssize_t copy_n;

    if (ci.stop_codec)
        return 0;

    copy_n = bufread(get_audio_hid(), size, ptr);

    /* Nothing requested OR nothing left */
    if (copy_n == 0)
        return 0;

    /* Update read and other position pointers */
    codec_advance_buffer_counters(copy_n);

    /* Return the actual amount of data copied to the buffer */
    return copy_n;
} /* codec_filebuf_callback */

static void* codec_request_buffer_callback(size_t *realsize, size_t reqsize)
{
    size_t copy_n = reqsize;
    ssize_t ret;
    void *ptr;

    ret = bufgetdata(get_audio_hid(), reqsize, &ptr);
    if (ret >= 0)
        copy_n = MIN((size_t)ret, reqsize);
    else
        copy_n = 0;

    if (copy_n == 0)
        ptr = NULL;

    *realsize = copy_n;
    return ptr;
} /* codec_request_buffer_callback */

static void codec_advance_buffer_callback(size_t amount)
{
    codec_advance_buffer_counters(amount);
    codec_set_offset_callback(ci.curpos);
}

static bool codec_seek_buffer_callback(size_t newpos)
{
    logf("codec_seek_buffer_callback");

    int ret = bufseek(get_audio_hid(), newpos);
    if (ret == 0) {
        ci.curpos = newpos;
        return true;
    }
    else {
        return false;
    }
}

static void codec_seek_complete_callback(void)
{
    struct queue_event ev;

    logf("seek_complete");

    /* Clear DSP */
    dsp_configure(ci.dsp, DSP_FLUSH, 0);

    /* Post notification to audio thread */
    LOGFQUEUE("audio > Q_AUDIO_SEEK_COMPLETE");
    queue_post(&audio_queue, Q_AUDIO_SEEK_COMPLETE, 0);

    /* Wait for ACK */
    queue_wait(&codec_queue, &ev);

    /* ACK back in context */
    codec_queue_ack(Q_AUDIO_SEEK_COMPLETE);
}

static bool codec_request_next_track_callback(void)
{
    struct queue_event ev;

    logf("Request new track");

    audio_set_prev_elapsed(thistrack_id3->elapsed);

#ifdef AB_REPEAT_ENABLE
    ab_end_of_track_report();
#endif

    if (ci.stop_codec)
    {
        /* Handle ACK in outer loop */
        LOGFQUEUE("codec: already stopping");
        return false;
    }

    trigger_cpu_boost();

    /* Post request to audio thread */
    LOGFQUEUE("codec > audio Q_AUDIO_CHECK_NEW_TRACK");
    queue_post(&audio_queue, Q_AUDIO_CHECK_NEW_TRACK, 0);

    /* Wait for ACK */
    queue_wait(&codec_queue, &ev);

    if (ev.data == Q_CODEC_REQUEST_COMPLETE)
    {
        /* Seek to the beginning of the new track because if the struct
           mp3entry was buffered, "elapsed" might not be zero (if the track has
           been played already but not unbuffered) */
        codec_seek_buffer_callback(thistrack_id3->first_frame_offset);
    }

    /* ACK back in context */
    codec_queue_ack(Q_AUDIO_CHECK_NEW_TRACK);

    if (ev.data != Q_CODEC_REQUEST_COMPLETE || ci.stop_codec)
    {
        LOGFQUEUE("codec <= request failed (%d)", ev.data);
        return false;
    }

    LOGFQUEUE("codec <= Q_CODEC_REQEST_COMPLETE");
    return true;
}

static void codec_configure_callback(int setting, intptr_t value)
{
    if (!dsp_configure(ci.dsp, setting, value))
        { logf("Illegal key:%d", setting); }
}

/* Initialize codec API */
void codec_init_codec_api(void)
{
    ci.dsp                 = (struct dsp_config *)dsp_configure(NULL, DSP_MYDSP,
                                                                CODEC_IDX_AUDIO);
    ci.codec_get_buffer    = codec_get_buffer;
    ci.pcmbuf_insert       = codec_pcmbuf_insert_callback;
    ci.set_elapsed         = codec_set_elapsed_callback;
    ci.read_filebuf        = codec_filebuf_callback;
    ci.request_buffer      = codec_request_buffer_callback;
    ci.advance_buffer      = codec_advance_buffer_callback;
    ci.seek_buffer         = codec_seek_buffer_callback;
    ci.seek_complete       = codec_seek_complete_callback;
    ci.request_next_track  = codec_request_next_track_callback;
    ci.set_offset          = codec_set_offset_callback;
    ci.configure           = codec_configure_callback;
}


/* track change */

/** CODEC THREAD */
static void codec_thread(void)
{
    struct queue_event ev;


    while (1)   
    {
        int status = CODEC_OK;
        void *handle = NULL;
        int hid;
        const char *codec_fn;
        
#ifdef HAVE_CROSSFADE
        if (!pcmbuf_is_crossfade_active())
#endif
        {
            cancel_cpu_boost();
        }

        queue_wait(&codec_queue, &ev);

        switch (ev.id)
        {
            case Q_CODEC_LOAD_DISK:
                LOGFQUEUE("codec < Q_CODEC_LOAD_DISK");
                codec_fn = get_codec_filename(ev.data);
                if (!codec_fn)
                    break;
#ifdef AUDIO_HAVE_RECORDING
                if (ev.data & CODEC_TYPE_ENCODER)
                {
                    ev.id = Q_ENCODER_LOAD_DISK;
                    handle = codec_load_file(codec_fn, &ci);
                    if (handle)
                        codec_queue_ack(Q_ENCODER_LOAD_DISK);
                }
                else
#endif
                {
                    codec_queue_ack(Q_CODEC_LOAD_DISK);
                    handle = codec_load_file(codec_fn, &ci);
                }
                break;

            case Q_CODEC_LOAD:
                LOGFQUEUE("codec < Q_CODEC_LOAD");
                codec_queue_ack(Q_CODEC_LOAD);
                hid = (int)ev.data;
                handle = codec_load_buf(hid, &ci);
                bufclose(hid);
                break;

            case Q_CODEC_DO_CALLBACK:
                LOGFQUEUE("codec < Q_CODEC_DO_CALLBACK");
                codec_queue_ack(Q_CODEC_DO_CALLBACK);
                if ((void*)ev.data != NULL)
                {
                    cpucache_commit_discard();
                    ((void (*)(void))ev.data)();
                    cpucache_commit();
                }
                break;

            default:
                LOGFQUEUE("codec < default : %ld", ev.id);
        }

        if (handle)
        {
            /* Codec loaded - call the entrypoint */
            yield();
            logf("codec running");
            status = codec_begin(handle);
            logf("codec stopped");
            codec_close(handle);
            current_codectype = AFMT_UNKNOWN;

            if (ci.stop_codec)
                status = CODEC_OK;
        }

        switch (ev.id)
        {
#ifdef AUDIO_HAVE_RECORDING
            case Q_ENCODER_LOAD_DISK:
#endif
            case Q_CODEC_LOAD_DISK:
            case Q_CODEC_LOAD:
                /* Notify about the status */
                if (!handle)
                    status = CODEC_ERROR;
                LOGFQUEUE("codec > audio notify status: %d", status);
                queue_post(&audio_queue, ev.id, status);
                break;
        }
    }
}

void make_codec_thread(void)
{
    queue_init(&codec_queue, false);
    codec_thread_id = create_thread(
            codec_thread, codec_stack, sizeof(codec_stack),
            CREATE_THREAD_FROZEN,
            codec_thread_name IF_PRIO(, PRIORITY_PLAYBACK)
            IF_COP(, CPU));
    queue_enable_queue_send(&codec_queue, &codec_queue_sender_list,
                            codec_thread_id);
}

void codec_thread_resume(void)
{
    thread_thaw(codec_thread_id);
}

bool is_codec_thread(void)
{
    return thread_self() == codec_thread_id;
}

#ifdef HAVE_PRIORITY_SCHEDULING
int codec_thread_get_priority(void)
{
    return thread_get_priority(codec_thread_id);
}

int codec_thread_set_priority(int priority)
{
    return thread_set_priority(codec_thread_id, priority);
}
#endif /* HAVE_PRIORITY_SCHEDULING */

/* functions for audio thread use */
intptr_t codec_ack_msg(intptr_t data, bool stop_codec)
{
    intptr_t resp;
    LOGFQUEUE("codec >| Q_CODEC_ACK: %d", data);
    if (stop_codec)
        ci.stop_codec = true;
    resp = codec_queue_send(Q_CODEC_ACK, data);
    if (stop_codec)
        codec_stop();
    LOGFQUEUE("  ack: %ld", resp);
    return resp;
}

bool codec_load(int hid, int cod_spec)
{
    bool retval = false;

    ci.stop_codec = false;
    current_codectype = cod_spec;

    if (hid >= 0)
    {
        LOGFQUEUE("audio >| codec Q_CODEC_LOAD: %d", hid);
        retval = codec_queue_send(Q_CODEC_LOAD, hid) != Q_NULL;
    }
    else
    {
        LOGFQUEUE("audio >| codec Q_CODEC_LOAD_DISK: %d", cod_spec);
        retval = codec_queue_send(Q_CODEC_LOAD_DISK, cod_spec) != Q_NULL;
    }

    if (!retval)
    {
        ci.stop_codec = true;
        current_codectype = AFMT_UNKNOWN;
    }

    return retval;
}

void codec_stop(void)
{
    ci.stop_codec = true;
    /* Wait until it's in the main loop */
    while (codec_ack_msg(0, false) != Q_NULL);
    current_codectype = AFMT_UNKNOWN;
}

int codec_loaded(void)
{
    return current_codectype;
}
