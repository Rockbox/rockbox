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
 * Copyright (C) 2011      Michael Sevakis
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
#include "codecs.h"
#include "codec_thread.h"
#include "pcmbuf.h"
#include "playback.h"
#include "buffering.h"
#include "dsp_core.h"
#include "metadata.h"
#include "settings.h"

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
 * A=audio, C=codec
 * - = reads only
 *
 * Unless otherwise noted, the extern variables are located
 * in playback.c.
 */

/* Q_LOAD_CODEC parameter data */
struct codec_load_info
{
    int hid;    /* audio handle id (specify < 0 to use afmt) */
    int afmt;   /* codec specification (AFMT_*) */
};


/** --- Main state control --- **/

static int codec_type = AFMT_UNKNOWN; /* Codec type (C,A-) */

/* Private interfaces to main playback control */
extern void audio_codec_update_elapsed(unsigned long elapsed);
extern void audio_codec_update_offset(size_t offset);
extern void audio_codec_complete(int status);
extern void audio_codec_seek_complete(void);
extern struct codec_api ci; /* from codecs.c */

/* Codec thread */
static unsigned int codec_thread_id; /* For modifying thread priority later */
static struct event_queue codec_queue SHAREDBSS_ATTR;
static struct queue_sender_list codec_queue_sender_list SHAREDBSS_ATTR;
static long codec_stack[(DEFAULT_STACK_SIZE + 0x2000)/sizeof(long)] IBSS_ATTR;
static const char codec_thread_name[] = "codec";

static void unload_codec(void);

/* Messages are only ever sent one at a time to the codec from the audio
   thread. This is important for correct operation unless playback is
   stopped. */

/* static routines */
static void codec_queue_ack(intptr_t ackme)
{
    queue_reply(&codec_queue, ackme);
}

static intptr_t codec_queue_send(long id, intptr_t data)
{
    return queue_send(&codec_queue, id, data);
}

/* Poll the state of the codec queue. Returns < 0 if the message is urgent
   and any state should exit, > 0 if it's a run message (and it was
   scrubbed), 0 if message was ignored. */
static int codec_check_queue__have_msg(void)
{
    struct queue_event ev;

    queue_peek(&codec_queue, &ev);

    /* Seek, pause or stop? Just peek and return if so. Codec
       must handle the command after returing. Inserts will not
       be allowed until it complies. */
    switch (ev.id)
    {
    case Q_CODEC_SEEK:
        LOGFQUEUE("codec - Q_CODEC_SEEK", ev.id);
        return -1;
    case Q_CODEC_PAUSE:
        LOGFQUEUE("codec - Q_CODEC_PAUSE", ev.id);
        return -1;
    case Q_CODEC_STOP:
        LOGFQUEUE("codec - Q_CODEC_STOP", ev.id);
        return -1;
    }

    /* This is in error in this context unless it's "go, go, go!" */
    queue_wait(&codec_queue, &ev);

    if (ev.id == Q_CODEC_RUN)
    {
        logf("codec < Q_CODEC_RUN: already running!");
        codec_queue_ack(Q_CODEC_RUN);
        return 1;
    }

    /* Ignore it */
    logf("codec < bad req %ld (%s)", ev.id, __func__);
    codec_queue_ack(Q_NULL);
    return 0;
}

/* Does the audio format type equal CODEC_TYPE_ENCODER? */
static inline bool type_is_encoder(int afmt)
{
#ifdef AUDIO_HAVE_RECORDING
    return (afmt & CODEC_TYPE_MASK) == CODEC_TYPE_ENCODER;
#else
    return false;
    (void)afmt;
#endif
}

/**************************************/


/** --- Miscellaneous external functions --- **/
const char * get_codec_filename(int cod_spec)
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
}

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


/** --- codec API callbacks --- **/

static void codec_pcmbuf_insert_callback(
        const void *ch1, const void *ch2, int count)
{
    struct dsp_buffer src;
    src.remcount  = count;
    src.pin[0]    = ch1;
    src.pin[1]    = ch2;
    src.proc_mask = 0;

    while (LIKELY(queue_empty(&codec_queue)) ||
           codec_check_queue__have_msg() >= 0)
    {
        struct dsp_buffer dst;
        dst.remcount = 0;
        dst.bufcount = MAX(src.remcount, 1024); /* Arbitrary min request */

        if ((dst.p16out = pcmbuf_request_buffer(&dst.bufcount)) == NULL)
        {
            cancel_cpu_boost();

            /* It may be awhile before space is available but we want
               "instant" response to any message */
            queue_wait_w_tmo(&codec_queue, NULL, HZ/20);
        }
        else
        {
            dsp_process(ci.dsp, &src, &dst);

            if (dst.remcount > 0)
            {
                pcmbuf_write_complete(dst.remcount, ci.id3->elapsed,
                                      ci.id3->offset);
            }
            else if (src.remcount <= 0)
            {
                return; /* No input remains and DSP purged */
            }
        }
    }    
}

/* helper function, not a callback */
static bool codec_advance_buffer_counters(size_t amount)
{
    if (bufadvance(ci.audio_hid, amount) < 0)
    {
        ci.curpos = ci.filesize;
        return false;
    }

    ci.curpos += amount;
    return true;
}

/* copy up-to size bytes into ptr and return the actual size copied */
static size_t codec_filebuf_callback(void *ptr, size_t size)
{
    ssize_t copy_n = bufread(ci.audio_hid, size, ptr);

    /* Nothing requested OR nothing left */
    if (copy_n <= 0)
        return 0;

    /* Update read and other position pointers */
    codec_advance_buffer_counters(copy_n);

    /* Return the actual amount of data copied to the buffer */
    return copy_n;
}

static void * codec_request_buffer_callback(size_t *realsize, size_t reqsize)
{
    size_t copy_n = reqsize;
    ssize_t ret;
    void *ptr;

    ret = bufgetdata(ci.audio_hid, reqsize, &ptr);
    if (ret >= 0)
        copy_n = MIN((size_t)ret, reqsize);
    else
        copy_n = 0;

    if (copy_n == 0)
        ptr = NULL;

    *realsize = copy_n;
    return ptr;
}

static void codec_advance_buffer_callback(size_t amount)
{
    if (!codec_advance_buffer_counters(amount))
        return;

    audio_codec_update_offset(ci.curpos);
}

static bool codec_seek_buffer_callback(size_t newpos)
{
    logf("codec_seek_buffer_callback");

    int ret = bufseek(ci.audio_hid, newpos);
    if (ret == 0)
    {
        ci.curpos = newpos;
        return true;
    }

    return false;
}

static void codec_seek_complete_callback(void)
{
    logf("seek_complete");

    /* Clear DSP */
    dsp_configure(ci.dsp, DSP_FLUSH, 0);

    /* Sync position */
    audio_codec_update_offset(ci.curpos);

    /* Post notification to audio thread */
    audio_codec_seek_complete();

    /* Wait for urgent or go message */
    do
    {
        queue_wait(&codec_queue, NULL);
    }
    while (codec_check_queue__have_msg() == 0);
}

static void codec_configure_callback(int setting, intptr_t value)
{
    dsp_configure(ci.dsp, setting, value);
}

static enum codec_command_action
    codec_get_command_callback(intptr_t *param)
{
    yield();

    if (LIKELY(queue_empty(&codec_queue)))
        return CODEC_ACTION_NULL; /* As you were */

    /* Process the message - return requested action and data (if any should
       be expected) */
    while (1)
    {
        enum codec_command_action action = CODEC_ACTION_NULL;
        struct queue_event ev;

        queue_peek(&codec_queue, &ev); /* Find out what it is */

        long id = ev.id;

        switch (id)
        {
        case Q_CODEC_RUN:   /* Already running */
            LOGFQUEUE("codec < Q_CODEC_RUN");
            break;

        case Q_CODEC_PAUSE: /* Stay here and wait */
            LOGFQUEUE("codec < Q_CODEC_PAUSE");
            queue_wait(&codec_queue, &ev);  /* Remove message */
            codec_queue_ack(Q_CODEC_PAUSE);
            queue_wait(&codec_queue, NULL); /* Wait for next (no remove) */
            continue;

        case Q_CODEC_SEEK:  /* Audio wants codec to seek */
            LOGFQUEUE("codec < Q_CODEC_SEEK %ld", ev.data);
            *param = ev.data;
            action = CODEC_ACTION_SEEK_TIME;
            trigger_cpu_boost();
            break;

        case Q_CODEC_STOP:  /* Must only return 0 in main loop */
            LOGFQUEUE("codec < Q_CODEC_STOP");
            dsp_configure(ci.dsp, DSP_FLUSH, 0); /* Discontinuity */
            return CODEC_ACTION_HALT; /* Leave in queue */

        default:            /* This is in error in this context. */
            logf("codec bad req %ld (%s)", ev.id, __func__);
            id = Q_NULL;
        }

        queue_wait(&codec_queue, &ev); /* Actually remove it */
        codec_queue_ack(id);
        return action;
    }
}

static bool codec_loop_track_callback(void)
{
    return global_settings.repeat_mode == REPEAT_ONE;
}


/** --- CODEC THREAD --- **/

/* Handle Q_CODEC_LOAD */
static void load_codec(const struct codec_load_info *ev_data)
{
    int status = CODEC_ERROR;
    /* Save a local copy so we can let the audio thread go ASAP */
    struct codec_load_info data = *ev_data;
    bool const encoder = type_is_encoder(data.afmt);

    if (codec_type != AFMT_UNKNOWN)
    {
        /* Must have unloaded it first */
        logf("a codec is already loaded");
        if (data.hid >= 0)
            bufclose(data.hid);
        return;
    }

    trigger_cpu_boost();

    if (!encoder)
    {
        /* Do this now because codec may set some things up at load time */
        dsp_configure(ci.dsp, DSP_RESET, 0);
    }

    if (data.hid >= 0)
    {
        /* First try buffer load */
        status = codec_load_buf(data.hid, &ci);
        bufclose(data.hid);
    }

    if (status < 0)
    {
        /* Either not a valid handle or the buffer method failed */
        const char *codec_fn = get_codec_filename(data.afmt);
        if (codec_fn)
        {
#ifdef HAVE_IO_PRIORITY
            buf_back_off_storage(true);
#endif
            status = codec_load_file(codec_fn, &ci);
#ifdef HAVE_IO_PRIORITY
            buf_back_off_storage(false);
#endif
        }
    }

    if (status >= 0)
    {
        codec_type = data.afmt;
        codec_queue_ack(Q_CODEC_LOAD);
        return;
    }

    /* Failed - get rid of it */
    unload_codec();
}

/* Handle Q_CODEC_RUN */
static void run_codec(void)
{
    bool const encoder = type_is_encoder(codec_type);
    int status;

    if (codec_type == AFMT_UNKNOWN)
    {
        logf("no codec to run");
        return;
    }

    codec_queue_ack(Q_CODEC_RUN);

    trigger_cpu_boost();

    if (!encoder)
    {
        /* This will be either the initial buffered offset or where it left off
           if it remained buffered and we're skipping back to it and it is best
           to have ci.curpos in sync with the handle's read position - it's the
           codec's responsibility to ensure it has the correct positions -
           playback is sorta dumb and only has a vague idea about what to
           buffer based upon what metadata has to say */
        ci.curpos = bufftell(ci.audio_hid);

        /* Pin the codec's audio data in place */
        buf_pin_handle(ci.audio_hid, true);
    }

    status = codec_run_proc();

    if (!encoder)
    {
        /* Codec is done with it - let it move */
        buf_pin_handle(ci.audio_hid, false);

        /* Notify audio that we're done for better or worse - advise of the
           status */
        audio_codec_complete(status);
    }
}

/* Handle Q_CODEC_SEEK */
static void seek_codec(unsigned long time)
{
    if (codec_type == AFMT_UNKNOWN)
    {
        logf("no codec to seek");
        codec_queue_ack(Q_CODEC_SEEK);
        codec_seek_complete_callback();
        return;
    }

    /* Post it up one level */
    queue_post(&codec_queue, Q_CODEC_SEEK, time);
    codec_queue_ack(Q_CODEC_SEEK);

    /* Have to run it again */
    run_codec();
}

/* Handle Q_CODEC_UNLOAD */
static void unload_codec(void)
{
    /* Tell codec to clean up */
    codec_type = AFMT_UNKNOWN;
    codec_close();
}

/* Handle Q_CODEC_DO_CALLBACK */
static void do_callback(void (* callback)(void))
{
    codec_queue_ack(Q_CODEC_DO_CALLBACK);

    if (callback)
    {
        commit_discard_idcache();
        callback();
        commit_dcache();
    }
}

/* Codec thread function */
static void NORETURN_ATTR codec_thread(void)
{
    struct queue_event ev;

    while (1)   
    {
        cancel_cpu_boost();

        queue_wait(&codec_queue, &ev);

        switch (ev.id)
        {
        case Q_CODEC_LOAD:
            LOGFQUEUE("codec < Q_CODEC_LOAD");
            load_codec((const struct codec_load_info *)ev.data);
            break;

        case Q_CODEC_RUN:
            LOGFQUEUE("codec < Q_CODEC_RUN");
            run_codec();
            break;

        case Q_CODEC_PAUSE:
            LOGFQUEUE("codec < Q_CODEC_PAUSE");
            break;

        case Q_CODEC_SEEK:
            LOGFQUEUE("codec < Q_CODEC_SEEK: %lu", (unsigned long)ev.data);
            seek_codec(ev.data);
            break;

        case Q_CODEC_UNLOAD:
            LOGFQUEUE("codec < Q_CODEC_UNLOAD");
            unload_codec();
            break;

        case Q_CODEC_DO_CALLBACK:
            LOGFQUEUE("codec < Q_CODEC_DO_CALLBACK");
            do_callback((void (*)(void))ev.data);
            break;

        default:
            LOGFQUEUE("codec < default : %ld", ev.id);
        }
    }
}


/** --- Miscellaneous external interfaces -- **/

/* Initialize playback's codec interface */
void codec_thread_init(void)
{
    /* Init API */
    ci.dsp              = dsp_get_config(CODEC_IDX_AUDIO);
    ci.codec_get_buffer = codec_get_buffer_callback;
    ci.pcmbuf_insert    = codec_pcmbuf_insert_callback;
    ci.set_elapsed      = audio_codec_update_elapsed;
    ci.read_filebuf     = codec_filebuf_callback;
    ci.request_buffer   = codec_request_buffer_callback;
    ci.advance_buffer   = codec_advance_buffer_callback;
    ci.seek_buffer      = codec_seek_buffer_callback;
    ci.seek_complete    = codec_seek_complete_callback;
    ci.set_offset       = audio_codec_update_offset;
    ci.configure        = codec_configure_callback;
    ci.get_command      = codec_get_command_callback;
    ci.loop_track       = codec_loop_track_callback;

    /* Init threading */
    queue_init(&codec_queue, false);
    codec_thread_id = create_thread(
            codec_thread, codec_stack, sizeof(codec_stack), 0,
            codec_thread_name IF_PRIO(, PRIORITY_PLAYBACK)
            IF_COP(, CPU));
    queue_enable_queue_send(&codec_queue, &codec_queue_sender_list,
                            codec_thread_id);
}

#ifdef HAVE_PRIORITY_SCHEDULING
/* Obtain codec thread's current priority */
int codec_thread_get_priority(void)
{
    return thread_get_priority(codec_thread_id);
}

/* Set the codec thread's priority and return the old value */
int codec_thread_set_priority(int priority)
{
    return thread_set_priority(codec_thread_id, priority);
}
#endif /* HAVE_PRIORITY_SCHEDULING */


/** --- Functions for audio thread use --- **/

/* Load a decoder or encoder and set the format type */
bool codec_load(int hid, int cod_spec)
{
    struct codec_load_info parm = { hid, cod_spec };

    LOGFQUEUE("audio >| codec Q_CODEC_LOAD: %d, %d", hid, cod_spec);
    return codec_queue_send(Q_CODEC_LOAD, (intptr_t)&parm) != 0;
}

/* Begin decoding the current file */
void codec_go(void)
{
    LOGFQUEUE("audio >| codec Q_CODEC_RUN");
    codec_queue_send(Q_CODEC_RUN, 0);
}

/* Instruct the codec to seek to the specified time (should be properly
   paused or stopped first to avoid possible buffering deadlock) */
void codec_seek(long time)
{
    LOGFQUEUE("audio > codec Q_CODEC_SEEK: %ld", time);
    codec_queue_send(Q_CODEC_SEEK, time);
}

/* Pause the codec and make it wait for further instructions inside the
   command callback */
bool codec_pause(void)
{
    LOGFQUEUE("audio >| codec Q_CODEC_PAUSE");
    return codec_queue_send(Q_CODEC_PAUSE, 0) != Q_NULL;
}

/* Stop codec if running - codec stays resident if loaded */
void codec_stop(void)
{
    /* Wait until it's in the main loop */
    LOGFQUEUE("audio >| codec Q_CODEC_STOP");
    while (codec_queue_send(Q_CODEC_STOP, 0) != Q_NULL);
}

/* Call the codec's exit routine and close all references */
void codec_unload(void)
{
    codec_stop();
    LOGFQUEUE("audio >| codec Q_CODEC_UNLOAD");
    codec_queue_send(Q_CODEC_UNLOAD, 0);
}

/* Return the afmt type of the loaded codec - sticks until calling
   codec_unload unless initial load failed */
int codec_loaded(void)
{
    return codec_type;
}
