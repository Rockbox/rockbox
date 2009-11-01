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

#include "playback.h"
#include "codec_thread.h"
#include "system.h"
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
volatile bool audio_codec_loaded SHAREDBSS_ATTR = false; /* Codec loaded? (C/A-) */

extern struct mp3entry *thistrack_id3,  /* the currently playing track */
                       *othertrack_id3; /* prev track during track-change-transition, or end of playlist,
                                         * next track otherwise */

/* Track change controls */
extern bool automatic_skip; /* Who initiated in-progress skip? (C/A-) */

/* Set to true if the codec thread should send an audio stop request
 * (typically because the end of the playlist has been reached).
 */
static bool codec_requested_stop = false;

extern struct event_queue audio_queue;
extern struct event_queue codec_queue;

extern struct codec_api ci; /* from codecs.c */

/* Codec thread */
unsigned int codec_thread_id;   /* For modifying thread priority later.
                                   Used by playback.c and pcmbuf.c */
static struct queue_sender_list codec_queue_sender_list;
static long codec_stack[(DEFAULT_STACK_SIZE + 0x2000)/sizeof(long)]
IBSS_ATTR;
static const char codec_thread_name[] = "codec";

/* function prototypes */
static bool codec_load_next_track(void);


/**************************************/

/** misc external functions */

int get_codec_base_type(int type)
{
    switch (type) {
        case AFMT_MPA_L1:
        case AFMT_MPA_L2:
        case AFMT_MPA_L3:
            return AFMT_MPA_L3;
    }

    return type;
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
    queue_send(&codec_queue, Q_CODEC_DO_CALLBACK, (intptr_t)fn);
}


/** codec API callbacks */

static void* codec_get_buffer(size_t *size)
{
    if (codec_size >= CODEC_SIZE)
        return NULL;
    *size = CODEC_SIZE - codec_size;
    return &codecbuf[codec_size];
}

static bool codec_pcmbuf_insert_callback(
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
            return true;

        while ((dest = pcmbuf_request_buffer(&out_count)) == NULL)
        {
            cancel_cpu_boost();
            sleep(1);
            if (ci.seek_time || ci.new_track || ci.stop_codec)
                return true;
        }

        /* Get the real input_size for output_size bytes, guarding
         * against resampling buffer overflows. */
        inp_count = dsp_input_count(ci.dsp, out_count);

        if (inp_count <= 0)
            return true;

        /* Input size has grown, no error, just don't write more than length */
        if (inp_count > count)
            inp_count = count;

        out_count = dsp_process(ci.dsp, dest, src, inp_count);

        if (out_count <= 0)
            return true;

        pcmbuf_write_complete(out_count);

        count -= inp_count;
    }

    return true;
} /* codec_pcmbuf_insert_callback */

static void codec_set_elapsed_callback(unsigned int value)
{
    if (ci.seek_time)
        return;

#ifdef AB_REPEAT_ENABLE
    ab_position_report(value);
#endif

    unsigned int latency = pcmbuf_get_latency();
    if (value < latency)
        thistrack_id3->elapsed = 0;
    else
    {
        unsigned int elapsed = value - latency;
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

    unsigned int latency = pcmbuf_get_latency() * thistrack_id3->bitrate / 8;
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

    if (ci.stop_codec || !audio_is_playing())
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

    if (!audio_is_playing())
    {
        *realsize = 0;
        return NULL;
    }

    ret = bufgetdata(get_audio_hid(), reqsize, &ptr);
    if (ret >= 0)
        copy_n = MIN((size_t)ret, reqsize);

    if (copy_n == 0)
    {
        *realsize = 0;
        return NULL;
    }

    *realsize = copy_n;

    return ptr;
} /* codec_request_buffer_callback */

static void codec_advance_buffer_callback(size_t amount)
{
    codec_advance_buffer_counters(amount);
    codec_set_offset_callback(ci.curpos);
}

static void codec_advance_buffer_loc_callback(void *ptr)
{
    size_t amount = buf_get_offset(get_audio_hid(), ptr);
    codec_advance_buffer_callback(amount);
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
    logf("seek_complete");
    /* If seeking-while-playing, pcm_is_paused() is true.
     * If seeking-while-paused, audio_is_paused() is true.
     * A seamless seek skips this section. */
    if (pcm_is_paused() || audio_is_paused())
    {
        /* Clear the buffer */
        pcmbuf_play_stop();
        dsp_configure(ci.dsp, DSP_FLUSH, 0);

        /* If seeking-while-playing, resume pcm playback */
        if (!audio_is_paused())
            pcmbuf_pause(false);
    }
    ci.seek_time = 0;
}

static void codec_discard_codec_callback(void)
{
    int *codec_hid = get_codec_hid();
    if (*codec_hid >= 0)
    {
        bufclose(*codec_hid);
        *codec_hid = -1;
    }
}

static bool codec_request_next_track_callback(void)
{
    int prev_codectype;

    if (ci.stop_codec || !audio_is_playing())
        return false;

    prev_codectype = get_codec_base_type(thistrack_id3->codectype);
    if (!codec_load_next_track())
        return false;

    /* Seek to the beginning of the new track because if the struct
       mp3entry was buffered, "elapsed" might not be zero (if the track has
       been played already but not unbuffered) */
    codec_seek_buffer_callback(thistrack_id3->first_frame_offset);
    /* Check if the next codec is the same file. */
    if (prev_codectype == get_codec_base_type(thistrack_id3->codectype))
    {
        logf("New track loaded");
        codec_discard_codec_callback();
        return true;
    }
    else
    {
        logf("New codec:%d/%d", thistrack_id3->codectype, prev_codectype);
        return false;
    }
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
    ci.advance_buffer_loc  = codec_advance_buffer_loc_callback;
    ci.seek_buffer         = codec_seek_buffer_callback;
    ci.seek_complete       = codec_seek_complete_callback;
    ci.request_next_track  = codec_request_next_track_callback;
    ci.discard_codec       = codec_discard_codec_callback;
    ci.set_offset          = codec_set_offset_callback;
    ci.configure           = codec_configure_callback;
}


/** pcmbuf track change callbacks */

/* Between the codec and PCM track change, we need to keep updating the
   "elapsed" value of the previous (to the codec, but current to the
   user/PCM/WPS) track, so that the progressbar reaches the end.
   During that transition, the WPS will display prevtrack_id3. */
static void codec_pcmbuf_position_callback(size_t size) ICODE_ATTR;
static void codec_pcmbuf_position_callback(size_t size)
{
    /* This is called from an ISR, so be quick */
    unsigned int time = size * 1000 / 4 / NATIVE_FREQUENCY +
        othertrack_id3->elapsed;

    if (time >= othertrack_id3->length)
    {
        pcmbuf_set_position_callback(NULL);
        othertrack_id3->elapsed = othertrack_id3->length;
    }
    else
        othertrack_id3->elapsed = time;
}

static void codec_pcmbuf_track_changed_callback(void)
{
    LOGFQUEUE("codec > pcmbuf/audio Q_AUDIO_TRACK_CHANGED");
    pcmbuf_set_position_callback(NULL);
    audio_post_track_change();
}


/** track change functions */

static inline void codec_gapless_track_change(void)
{
    /* callback keeps the progress bar moving while the pcmbuf empties */
    pcmbuf_set_position_callback(codec_pcmbuf_position_callback);
    /* set the pcmbuf callback for when the track really changes */
    pcmbuf_set_event_handler(codec_pcmbuf_track_changed_callback);
}

static inline void codec_crossfade_track_change(void)
{
    /* Initiate automatic crossfade mode */
    pcmbuf_crossfade_init(false);
    /* Notify the wps that the track change starts now */
    LOGFQUEUE("codec > audio Q_AUDIO_TRACK_CHANGED");
    queue_post(&audio_queue, Q_AUDIO_TRACK_CHANGED, 0);
}

static void codec_track_skip_done(bool was_manual)
{
    /* Manual track change (always crossfade or flush audio). */
    if (was_manual)
    {
        pcmbuf_crossfade_init(true);
        LOGFQUEUE("codec > audio Q_AUDIO_TRACK_CHANGED");
        queue_post(&audio_queue, Q_AUDIO_TRACK_CHANGED, 0);
    }
    /* Automatic track change w/crossfade, if not in "Track Skip Only" mode. */
    else if (pcmbuf_is_crossfade_enabled() && !pcmbuf_is_crossfade_active()
             && global_settings.crossfade != CROSSFADE_ENABLE_TRACKSKIP)
    {
        if (global_settings.crossfade == CROSSFADE_ENABLE_SHUFFLE_AND_TRACKSKIP)
        {
            if (global_settings.playlist_shuffle)
                /* shuffle mode is on, so crossfade: */
                codec_crossfade_track_change();
            else
                /* shuffle mode is off, so do a gapless track change */
                codec_gapless_track_change();
        }
        else
            /* normal crossfade:  */
            codec_crossfade_track_change();
    }
    else
        /* normal gapless playback. */
        codec_gapless_track_change();
}

static bool codec_load_next_track(void)
{
    intptr_t result = Q_CODEC_REQUEST_FAILED;

    audio_set_prev_elapsed(thistrack_id3->elapsed);

#ifdef AB_REPEAT_ENABLE
    ab_end_of_track_report();
#endif

    logf("Request new track");

    if (ci.new_track == 0)
    {
        ci.new_track++;
        automatic_skip = true;
    }

    if (!ci.stop_codec)
    {
        trigger_cpu_boost();
        LOGFQUEUE("codec >| audio Q_AUDIO_CHECK_NEW_TRACK");
        result = queue_send(&audio_queue, Q_AUDIO_CHECK_NEW_TRACK, 0);
    }

    switch (result)
    {
        case Q_CODEC_REQUEST_COMPLETE:
            LOGFQUEUE("codec |< Q_CODEC_REQUEST_COMPLETE");
            codec_track_skip_done(!automatic_skip);
            return true;

        case Q_CODEC_REQUEST_FAILED:
            LOGFQUEUE("codec |< Q_CODEC_REQUEST_FAILED");
            ci.new_track = 0;
            ci.stop_codec = true;
            codec_requested_stop = true;
            return false;

        default:
            LOGFQUEUE("codec |< default");
            ci.stop_codec = true;
            codec_requested_stop = true;
            return false;
    }
}

/** CODEC THREAD */
static void codec_thread(void)
{
    struct queue_event ev;
    int status;

    while (1) {
        status = 0;
        
        if (!pcmbuf_is_crossfade_active()) {
            cancel_cpu_boost();
        }
            
        queue_wait(&codec_queue, &ev);
        codec_requested_stop = false;

        switch (ev.id) {
            case Q_CODEC_LOAD_DISK:
                LOGFQUEUE("codec < Q_CODEC_LOAD_DISK");
                queue_reply(&codec_queue, 1);
                audio_codec_loaded = true;
                ci.stop_codec = false;
                status = codec_load_file((const char *)ev.data, &ci);
                LOGFQUEUE("codec_load_file %s %d\n", (const char *)ev.data, status);
                break;

            case Q_CODEC_LOAD:
                LOGFQUEUE("codec < Q_CODEC_LOAD");
                if (*get_codec_hid() < 0) {
                    logf("Codec slot is empty!");
                    /* Wait for the pcm buffer to go empty */
                    while (pcm_is_playing())
                        yield();
                    /* This must be set to prevent an infinite loop */
                    ci.stop_codec = true;
                    LOGFQUEUE("codec > codec Q_AUDIO_PLAY");
                    queue_post(&codec_queue, Q_AUDIO_PLAY, 0);
                    break;
                }

                audio_codec_loaded = true;
                ci.stop_codec = false;
                status = codec_load_buf(*get_codec_hid(), &ci);
                LOGFQUEUE("codec_load_buf %d\n", status);
                break;

            case Q_CODEC_DO_CALLBACK:
                LOGFQUEUE("codec < Q_CODEC_DO_CALLBACK");
                queue_reply(&codec_queue, 1);
                if ((void*)ev.data != NULL)
                {
                    cpucache_invalidate();
                    ((void (*)(void))ev.data)();
                    cpucache_flush();
                }
                break;

#ifdef AUDIO_HAVE_RECORDING
            case Q_ENCODER_LOAD_DISK:
                LOGFQUEUE("codec < Q_ENCODER_LOAD_DISK");
                audio_codec_loaded = false; /* Not audio codec! */
                logf("loading encoder");
                ci.stop_encoder = false;
                status = codec_load_file((const char *)ev.data, &ci);
                logf("encoder stopped");
                break;
#endif /* AUDIO_HAVE_RECORDING */

            default:
                LOGFQUEUE("codec < default");
        }

        if (audio_codec_loaded)
        {
            if (ci.stop_codec)
            {
                status = CODEC_OK;
                if (!audio_is_playing())
                    pcmbuf_play_stop();

            }
            audio_codec_loaded = false;
        }

        switch (ev.id) {
            case Q_CODEC_LOAD_DISK:
            case Q_CODEC_LOAD:
                LOGFQUEUE("codec < Q_CODEC_LOAD");
                if (audio_is_playing())
                {
                    if (ci.new_track || status != CODEC_OK)
                    {
                        if (!ci.new_track)
                        {
                            logf("Codec failure, %d %d", ci.new_track, status);
                            splash(HZ*2, "Codec failure");
                        }

                        if (!codec_load_next_track())
                        {
                            LOGFQUEUE("codec > audio Q_AUDIO_STOP");
                            /* End of playlist */
                            queue_post(&audio_queue, Q_AUDIO_STOP, 0);
                            break;
                        }
                    }
                    else
                    {
                        logf("Codec finished");
                        if (ci.stop_codec)
                        {
                            /* Wait for the audio to stop playing before
                             * triggering the WPS exit */
                            while(pcm_is_playing())
                            {
                                /* There has been one too many struct pointer swaps by now
                                 * so even though it says othertrack_id3, its the correct one! */
                                othertrack_id3->elapsed =
                                    othertrack_id3->length - pcmbuf_get_latency();
                                sleep(1);
                            }

                            if (codec_requested_stop)
                            {
                                LOGFQUEUE("codec > audio Q_AUDIO_STOP");
                                queue_post(&audio_queue, Q_AUDIO_STOP, 0);
                            }
                            break;
                        }
                    }

                    if (*get_codec_hid() >= 0)
                    {
                        LOGFQUEUE("codec > codec Q_CODEC_LOAD");
                        queue_post(&codec_queue, Q_CODEC_LOAD, 0);
                    }
                    else
                    {
                        const char *codec_fn =
                            get_codec_filename(thistrack_id3->codectype);
                        if (codec_fn)
                        {
                            LOGFQUEUE("codec > codec Q_CODEC_LOAD_DISK");
                            queue_post(&codec_queue, Q_CODEC_LOAD_DISK,
                                       (intptr_t)codec_fn);
                        }
                    }
                }
                break;

#ifdef AUDIO_HAVE_RECORDING
            case Q_ENCODER_LOAD_DISK:
                LOGFQUEUE("codec < Q_ENCODER_LOAD_DISK");

                if (status == CODEC_OK)
                    break;

                logf("Encoder failure");
                splash(HZ*2, "Encoder failure");

                if (ci.enc_codec_loaded < 0)
                    break;

                logf("Encoder failed to load");
                ci.enc_codec_loaded = -1;
                break;
#endif /* AUDIO_HAVE_RECORDING */

            default:
                LOGFQUEUE("codec < default");

        } /* end switch */
    }
}

void make_codec_thread(void)
{
    codec_thread_id = create_thread(
            codec_thread, codec_stack, sizeof(codec_stack),
            CREATE_THREAD_FROZEN,
            codec_thread_name IF_PRIO(, PRIORITY_PLAYBACK)
            IF_COP(, CPU));
    queue_enable_queue_send(&codec_queue, &codec_queue_sender_list,
                            codec_thread_id);
}
