/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Miika Pekkarinen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* TODO: Can use the track changed callback to detect end of track and seek
 * in the previous track until this happens */
/* Design: we have prev_ti already, have a conditional for what type of seek
 * to do on a seek request, if it is a previous track seek, skip previous,
 * and in the request_next_track callback set the offset up the same way that
 * starting from an offset works. */
/* TODO: Pause should be handled in here, rather than PCMBUF so that voice can
 * play whilst audio is paused */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "system.h"
#include "thread.h"
#include "file.h"
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "kernel.h"
#include "tree.h"
#include "debug.h"
#include "sprintf.h"
#include "settings.h"
#include "codecs.h"
#include "audio.h"
#include "logf.h"
#include "mp3_playback.h"
#include "usb.h"
#include "status.h"
#include "main_menu.h"
#include "ata.h"
#include "screens.h"
#include "playlist.h"
#include "playback.h"
#include "pcmbuf.h"
#include "buffer.h"
#include "dsp.h"
#include "abrepeat.h"
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#endif
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "peakmeter.h"
#include "action.h"
#endif
#include "lang.h"
#include "bookmark.h"
#include "misc.h"
#include "sound.h"
#include "metadata.h"
#include "splash.h"
#include "talk.h"
#include "ata_idle_notify.h"

#ifdef HAVE_RECORDING
#include "recording.h"
#include "talk.h"
#endif

#define PLAYBACK_VOICE


/* default point to start buffer refill */
#define AUDIO_DEFAULT_WATERMARK      (1024*512)
/* amount of data to read in one read() call */
#define AUDIO_DEFAULT_FILECHUNK      (1024*32)
/* point at which the file buffer will fight for CPU time */
#define AUDIO_FILEBUF_CRITICAL       (1024*128)
/* amount of guess-space to allow for codecs that must hunt and peck
 * for their correct seeek target, 32k seems a good size */
#define AUDIO_REBUFFER_GUESS_SIZE    (1024*32)

/* macros to enable logf for queues
   logging on SYS_TIMEOUT can be disabled */
#ifdef SIMULATOR
/* Define this for logf output of all queuing except SYS_TIMEOUT */
#define PLAYBACK_LOGQUEUES
/* Define this to logf SYS_TIMEOUT messages */
#define PLAYBACK_LOGQUEUES_SYS_TIMEOUT
#endif

#ifdef PLAYBACK_LOGQUEUES
#define LOGFQUEUE(s) logf("%s", s)
#else
#define LOGFQUEUE(s)
#endif

#ifdef PLAYBACK_LOGQUEUES_SYS_TIMEOUT
#define LOGFQUEUE_SYS_TIMEOUT(s) logf("%s", s)
#else
#define LOGFQUEUE_SYS_TIMEOUT(s)
#endif


/* Define one constant that includes recording related functionality */
#if defined(HAVE_RECORDING) && !defined(SIMULATOR)
#define AUDIO_HAVE_RECORDING
#endif

enum {
    Q_AUDIO_PLAY = 1,
    Q_AUDIO_STOP,
    Q_AUDIO_PAUSE,
    Q_AUDIO_SKIP,
    Q_AUDIO_PRE_FF_REWIND,
    Q_AUDIO_FF_REWIND,
    Q_AUDIO_REBUFFER_SEEK,
    Q_AUDIO_CHECK_NEW_TRACK,
    Q_AUDIO_FLUSH,
    Q_AUDIO_TRACK_CHANGED,
    Q_AUDIO_DIR_SKIP,
    Q_AUDIO_NEW_PLAYLIST,
    Q_AUDIO_POSTINIT,
    Q_AUDIO_FILL_BUFFER,
#if MEM > 8
    Q_AUDIO_FILL_BUFFER_IF_ACTIVE_ATA,
#endif
#ifdef AUDIO_HAVE_RECORDING
    Q_AUDIO_LOAD_ENCODER,
#endif

    Q_CODEC_REQUEST_PENDING,
    Q_CODEC_REQUEST_COMPLETE,
    Q_CODEC_REQUEST_FAILED,

    Q_VOICE_PLAY,
    Q_VOICE_STOP,

    Q_CODEC_LOAD,
    Q_CODEC_LOAD_DISK,

#ifdef AUDIO_HAVE_RECORDING
    Q_ENCODER_LOAD_DISK,
    Q_ENCODER_RECORD,
#endif
};

/* As defined in plugins/lib/xxx2wav.h */
#if MEM > 1
#define MALLOC_BUFSIZE (512*1024)
#define GUARD_BUFSIZE  (32*1024)
#else
#define MALLOC_BUFSIZE (100*1024)
#define GUARD_BUFSIZE  (8*1024)
#endif

/* As defined in plugin.lds */
#if defined(CPU_PP)
#define CODEC_IRAM_ORIGIN   0x4000c000
#define CODEC_IRAM_SIZE     0xc000
#elif defined(IAUDIO_X5)
#define CODEC_IRAM_ORIGIN   0x10010000
#define CODEC_IRAM_SIZE     0x10000
#else
#define CODEC_IRAM_ORIGIN   0x1000c000
#define CODEC_IRAM_SIZE     0xc000
#endif

#ifndef IBSS_ATTR_VOICE_STACK
#define IBSS_ATTR_VOICE_STACK IBSS_ATTR
#endif

#ifndef SIMULATOR
extern bool audio_is_initialized;
#else
static bool audio_is_initialized = false;
#endif


/* Variables are commented with the threads that use them: *
 * A=audio, C=codec, V=voice. A suffix of - indicates that *
 * the variable is read but not updated on that thread.    */
/* TBD: Split out "audio" and "playback" (ie. calling) threads */

/* Main state control */
static struct event_queue codec_callback_queue; /* Queue for codec callback responses */
static volatile bool audio_codec_loaded;        /* Is codec loaded? (C/A-) */
static volatile bool playing;                   /* Is audio playing? (A) */
static volatile bool paused;                    /* Is audio paused? (A/C-) */
static volatile bool filling IDATA_ATTR;        /* Is file buffer currently being refilled? (A/C-) */

/* Ring buffer where tracks and codecs are loaded */
static unsigned char *filebuf;                  /* Pointer to start of ring buffer (A/C-) */
size_t filebuflen;                              /* Total size of the ring buffer FIXME: make static (A/C-)*/
static volatile size_t buf_ridx IDATA_ATTR;     /* Ring buffer read position (A/C) FIXME? should be (C/A-) */
static volatile size_t buf_widx IDATA_ATTR;     /* Ring buffer read position (A/C-) */

#define BUFFER_STATE_TRASHED        -1          /* Buffer is in a trashed state and must be reset */
#define BUFFER_STATE_NORMAL          0          /* Buffer is arranged for voice and audio         */
#define BUFFER_STATE_VOICED_ONLY     1          /* Buffer is arranged for voice-only use          */
static int buffer_state = BUFFER_STATE_TRASHED; /* Buffer state */

#define RINGBUF_ADD(p,v) ((p+v)<filebuflen ? p+v : p+v-filebuflen)
#define RINGBUF_SUB(p,v) ((p>=v) ? p-v : p+filebuflen-v)
#define RINGBUF_ADD_CROSS(p1,v,p2) ((p1<p2)?(int)(p1+v)-(int)p2:(int)(p1+v-p2)-(int)filebuflen)
#define FILEBUFUSED RINGBUF_SUB(buf_widx, buf_ridx) /* Bytes available in the buffer */

/* Track info buffer */
static struct track_info tracks[MAX_TRACK];     /* Track info structure about songs in the file buffer (A/C-) */
static volatile int track_ridx;                 /* Track being decoded (A/C-) */
static int track_widx;                          /* Track being buffered (A) */
static bool track_changed;                      /* Set to indicate track has changed (A) */
static struct track_info *prev_ti;              /* Pointer to previous track played info (A/C-) */

#define CUR_TI (&tracks[track_ridx])            /* Pointer to current track playing info (A/C-) */

/* Audio buffering controls */
static int last_peek_offset;                    /* Step count to the next unbuffered track (A) */
static int current_fd;                          /* Partially loaded track file handle to continue buffering (A) */

/* Scrobbler support */
static unsigned long prev_track_elapsed;        /* Previous track elapsed time (C/A-) */

/* Track change controls */
static bool automatic_skip = false;             /* Was the skip being executed manual or automatic? (C/A-) */
static bool playlist_end = false;               /* Have we reached end of the current playlist? (A) */
static bool dir_skip = false;                   /* Is a directory skip pending? (A) */
static bool new_playlist = false;               /* Are we starting a new playlist? (A) */
static int wps_offset = 0;                      /* Pending track change offset, to keep WPS responsive (A) */

/* Callbacks..  */
void (*track_changed_callback)(struct mp3entry *id3); /* ...when current track has really changed */
void (*track_buffer_callback)(struct mp3entry *id3, bool last_track); /* ...when track has been buffered */
void (*track_unbuffer_callback)(struct mp3entry *id3, bool last_track); /* ...when track is being unbuffered */

/* Configuration */
static size_t conf_watermark;                   /* Low water mark (A/C) FIXME */
static size_t conf_filechunk;                   /* Largest chunk the codec accepts (A/C) FIXME */
static size_t conf_preseek;                     /* Codec pre-seek margin (A/C) FIXME */
static size_t buffer_margin;                    /* Buffer margin aka anti-skip buffer (A/C-) */
static bool v1first = false;                    /* ID3 data control, true if V1 then V2 (A) */

/* Multiple threads */
static const char *get_codec_filename(int enc_spec); /* Returns codec filename (A-/C-/V-) */
static void set_filebuf_watermark(int seconds); /* Set low watermark (A/C) FIXME */

/* Audio thread */
static struct event_queue audio_queue;
static long audio_stack[(DEFAULT_STACK_SIZE + 0x1000)/sizeof(long)];
static const char audio_thread_name[] = "audio";

static void audio_thread(void);
static void audio_initiate_track_change(long direction);
static bool audio_have_tracks(void);
static void audio_reset_buffer(size_t pcmbufsize);

/* Codec thread */
extern struct codec_api ci;
static struct event_queue codec_queue;
static long codec_stack[(DEFAULT_STACK_SIZE + 0x2000)/sizeof(long)]
IBSS_ATTR;
static const char codec_thread_name[] = "codec";
struct thread_entry *codec_thread_p;            /* For modifying thread priority later. */

volatile int current_codec IDATA_ATTR;          /* Current codec (normal/voice) */

/* Voice thread */
#ifdef PLAYBACK_VOICE

extern struct codec_api ci_voice;

static struct thread_entry *voice_thread_p = NULL;
static struct event_queue voice_queue;
static long voice_stack[(DEFAULT_STACK_SIZE + 0x2000)/sizeof(long)]
IBSS_ATTR_VOICE_STACK;
static const char voice_thread_name[] = "voice codec";

/* Voice codec swapping control */
extern unsigned char codecbuf[];                /* DRAM codec swap buffer */

#ifdef SIMULATOR
static unsigned char sim_iram[CODEC_IRAM_SIZE]; /* IRAM codec swap buffer for sim*/
#undef CODEC_IRAM_ORIGIN
#define CODEC_IRAM_ORIGIN sim_iram
#endif

static unsigned char *iram_buf[2] = { NULL, NULL }; /* Ptr to IRAM buffers for normal/voice codecs */
static unsigned char *dram_buf[2] = { NULL, NULL }; /* Ptr to DRAM buffers for normal/voice codecs */
static struct mutex mutex_codecthread;          /* Mutex to control which codec (normal/voice) is running */

/* Voice state */
static volatile bool voice_thread_start;    /* Set to trigger voice playback (A/V) */
static volatile bool voice_is_playing;      /* Is voice currently playing? (V) */
static volatile bool voice_codec_loaded;    /* Is voice codec loaded (V/A-) */
static char *voicebuf;
static size_t voice_remaining;

#ifdef IRAM_STEAL
static bool voice_iram_stolen = false;      /* Voice IRAM has been stolen for other use */
#endif

static void (*voice_getmore)(unsigned char** start, int* size);

struct voice_info {
    void (*callback)(unsigned char **start, int *size);
    int size;
    char *buf;
};
static void voice_thread(void);

#endif /* PLAYBACK_VOICE */

/* --- Shared semi-private interfaces --- */

/* imported */
extern void talk_buffer_steal(void);
#ifdef HAVE_RECORDING
extern void pcm_rec_error_clear(void);
extern unsigned long pcm_rec_status(void);
#endif


/* --- External interfaces --- */

void mp3_play_data(const unsigned char* start, int size,
                   void (*get_more)(unsigned char** start, int* size))
{
#ifdef PLAYBACK_VOICE
    static struct voice_info voice_clip;
    voice_clip.callback = get_more;
    voice_clip.buf = (char *)start;
    voice_clip.size = size;
    LOGFQUEUE("mp3 > voice Q_VOICE_STOP");
    queue_post(&voice_queue, Q_VOICE_STOP, 0);
    LOGFQUEUE("mp3 > voice Q_VOICE_PLAY");
    queue_post(&voice_queue, Q_VOICE_PLAY, &voice_clip);
    voice_thread_start = true;
    trigger_cpu_boost();
#else
    (void) start;
    (void) size;
    (void) get_more;
#endif
}

void mp3_play_stop(void)
{
#ifdef PLAYBACK_VOICE
    queue_remove_from_head(&voice_queue, Q_VOICE_STOP);
    LOGFQUEUE("mp3 > voice Q_VOICE_STOP");
    queue_post(&voice_queue, Q_VOICE_STOP, (void *)1);
#endif
}

bool mp3_pause_done(void)
{
    return pcm_is_paused();
}

void mpeg_id3_options(bool _v1first)
{
    v1first = _v1first;
}

unsigned char *audio_get_buffer(bool talk_buf, size_t *buffer_size)
{
    unsigned char *buf, *end;

    if (audio_is_initialized)
        audio_stop();

    if (buffer_size == NULL)
    {
        /* Special case for talk_init to use */
        buffer_state = BUFFER_STATE_TRASHED;
        return NULL;
    }

    buf = audiobuf;
    end = audiobufend;

    if (talk_buf || buffer_state == BUFFER_STATE_TRASHED
           || !talk_voice_required())
    {
        logf("get buffer: talk_buf");
        /* ok to use everything from audiobuf to audiobufend */
        if (buffer_state != BUFFER_STATE_TRASHED)
        {
            talk_buffer_steal();
#ifdef PLAYBACK_VOICE
            if (NULL != iram_buf[CODEC_IDX_VOICE])
            {
                /* Voice could be swapped out - wait for it to return */
                while (current_codec != CODEC_IDX_VOICE)
                    yield();
            }
#endif /* PLAYBACK_VOICE */
            buffer_state = BUFFER_STATE_TRASHED;
        }
    }
    else
    {
        /* skip talk buffer and move pcm buffer to end */
        logf("get buffer: voice");
        mp3_play_stop();
        buf += talk_get_bufsize();
        end -= pcmbuf_init(pcmbuf_get_bufsize(), audiobufend);
        buffer_state = BUFFER_STATE_VOICED_ONLY;
    }

    *buffer_size = end - buf;

    return buf;
}

#ifdef IRAM_STEAL
void audio_iram_steal(void)
{
    /* We need to stop audio playback in order to use codec IRAM */
    audio_stop();

#ifdef PLAYBACK_VOICE
    if (NULL != iram_buf[CODEC_IDX_VOICE])
    {
        /* Can't already be stolen */
        if (voice_iram_stolen)
            return;

        /* Wait for voice to swap back in if current codec was audio */
        while (current_codec != CODEC_IDX_VOICE)
            yield();

        voice_stop();

        /* Save voice IRAM - safe to do here since state is known */
        memcpy(iram_buf[CODEC_IDX_VOICE], (void *)CODEC_IRAM_ORIGIN,
               CODEC_IRAM_SIZE);
        voice_iram_stolen = true;
    }
    else
    {
        /* Nothing much to do if no voice */
        voice_iram_stolen = false;
    }
#endif
}
#endif /* IRAM_STEAL */

#ifdef HAVE_RECORDING
unsigned char *audio_get_recording_buffer(size_t *buffer_size)
{
    /* don't allow overwrite of voice swap area or we'll trash the
       swapped-out voice codec but can use whole thing if none */
    unsigned char *end;

    audio_stop();
    talk_buffer_steal();

#ifdef PLAYBACK_VOICE
#ifdef IRAM_STEAL
    end = dram_buf[CODEC_IDX_VOICE];
#else
    end = iram_buf[CODEC_IDX_VOICE];
#endif /* IRAM_STEAL */
    if (NULL == end)
#endif /* PLAYBACK_VOICE */
        end = audiobufend;


    buffer_state = BUFFER_STATE_TRASHED;

    *buffer_size = end - audiobuf;

    return (unsigned char *)audiobuf;
}

bool audio_load_encoder(int afmt)
{
#ifndef SIMULATOR
    const char *enc_fn = get_codec_filename(afmt | CODEC_TYPE_ENCODER);
    if (!enc_fn)
        return false;

    audio_remove_encoder();
    ci.enc_codec_loaded = 0; /* clear any previous error condition */

    LOGFQUEUE("audio > Q_AUDIO_LOAD_ENCODER");
    queue_post(&audio_queue, Q_AUDIO_LOAD_ENCODER, (void *)enc_fn);

    while (ci.enc_codec_loaded == 0)
        yield();

    logf("codec loaded: %d", ci.enc_codec_loaded);

    return ci.enc_codec_loaded > 0;
#else
    (void)afmt;
    return true;
#endif
} /* audio_load_encoder */

void audio_remove_encoder(void)
{
#ifndef SIMULATOR
    /* force encoder codec unload (if currently loaded) */
    if (ci.enc_codec_loaded <= 0)
        return;

    ci.stop_codec = true;
    while (ci.enc_codec_loaded > 0)
        yield();
#endif
} /* audio_remove_encoder */

#endif /* HAVE_RECORDING */

struct mp3entry* audio_current_track(void)
{
    const char *filename;
    const char *p;
    static struct mp3entry temp_id3;
    int cur_idx;
    int offset = ci.new_track + wps_offset;
    
    cur_idx = track_ridx + offset;
    cur_idx &= MAX_TRACK_MASK;

    if (tracks[cur_idx].taginfo_ready)
        return &tracks[cur_idx].id3;

    memset(&temp_id3, 0, sizeof(struct mp3entry));
    
    filename = playlist_peek(offset);
    if (!filename)
        filename = "No file!";

#ifdef HAVE_TC_RAMCACHE
    if (tagcache_fill_tags(&temp_id3, filename))
        return &temp_id3;
#endif

    p = strrchr(filename, '/');
    if (!p)
        p = filename;
    else
        p++;

    strncpy(temp_id3.path, p, sizeof(temp_id3.path)-1);
    temp_id3.title = &temp_id3.path[0];

    return &temp_id3;
}

struct mp3entry* audio_next_track(void)
{
    int next_idx = track_ridx;

    if (!audio_have_tracks())
        return NULL;

    next_idx++;
    next_idx &= MAX_TRACK_MASK;

    if (!tracks[next_idx].taginfo_ready)
        return NULL;

    return &tracks[next_idx].id3;
}

bool audio_has_changed_track(void)
{
    if (track_changed) 
    {
        track_changed = false;
        return true;
    }

    return false;
}

void audio_play(long offset)
{
    logf("audio_play");

#ifdef PLAYBACK_VOICE
    /* Truncate any existing voice output so we don't have spelling
     * etc. over the first part of the played track */
    LOGFQUEUE("mp3 > voice Q_VOICE_STOP");
    queue_post(&voice_queue, Q_VOICE_STOP, (void *)1);
#endif

    /* Start playback */
    if (playing && offset <= 0)
    {
        LOGFQUEUE("audio > audio Q_AUDIO_NEW_PLAYLIST");
        queue_post(&audio_queue, Q_AUDIO_NEW_PLAYLIST, 0);
    }
    else
    {
        LOGFQUEUE("audio > audio Q_AUDIO_STOP");
        queue_post(&audio_queue, Q_AUDIO_STOP, 0);
        LOGFQUEUE("audio > audio Q_AUDIO_PLAY");
        queue_post(&audio_queue, Q_AUDIO_PLAY, (void *)offset);
    }

    /* Don't return until playback has actually started */
    while (!playing)
        yield();
}

void audio_stop(void)
{
    /* Stop playback */
    LOGFQUEUE("audio > audio Q_AUDIO_STOP");
    queue_post(&audio_queue, Q_AUDIO_STOP, 0);

    /* Don't return until playback has actually stopped */
    while(playing || !queue_empty(&audio_queue))
        yield();
}

void audio_pause(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_PAUSE");
    queue_post(&audio_queue, Q_AUDIO_PAUSE, (void *)true);
}

void audio_resume(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_PAUSE resume");
    queue_post(&audio_queue, Q_AUDIO_PAUSE, (void *)false);
}

void audio_next(void)
{
    if (playlist_check(ci.new_track + wps_offset + 1))
    {
        if (global_settings.beep)
            pcmbuf_beep(5000, 100, 2500*global_settings.beep);

        LOGFQUEUE("audio > audio Q_AUDIO_SKIP 1");
        queue_post(&audio_queue, Q_AUDIO_SKIP, (void *)1);
        /* Keep wps fast while our message travels inside deep playback queues. */
        wps_offset++;
        track_changed = true;
    }
    else
    {
        /* No more tracks. */
        if (global_settings.beep)
            pcmbuf_beep(1000, 100, 1000*global_settings.beep);
    }
}

void audio_prev(void)
{
    if (playlist_check(ci.new_track + wps_offset - 1))
    {
        if (global_settings.beep)
            pcmbuf_beep(5000, 100, 2500*global_settings.beep);

        LOGFQUEUE("audio > audio Q_AUDIO_SKIP -1");
        queue_post(&audio_queue, Q_AUDIO_SKIP, (void *)-1);
        /* Keep wps fast while our message travels inside deep playback queues. */
        wps_offset--;
        track_changed = true;
    }
    else
    {
        /* No more tracks. */
        if (global_settings.beep)
            pcmbuf_beep(1000, 100, 1000*global_settings.beep);
    }
}

void audio_next_dir(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_DIR_SKIP 1");
    queue_post(&audio_queue, Q_AUDIO_DIR_SKIP, (void *)1);
}

void audio_prev_dir(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_DIR_SKIP -1");
    queue_post(&audio_queue, Q_AUDIO_DIR_SKIP, (void *)-1);
}

void audio_pre_ff_rewind(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_PRE_FF_REWIND");
    queue_post(&audio_queue, Q_AUDIO_PRE_FF_REWIND, 0);
}

void audio_ff_rewind(long newpos)
{
    LOGFQUEUE("audio > audio Q_AUDIO_FF_REWIND");
    queue_post(&audio_queue, Q_AUDIO_FF_REWIND, (int *)newpos);
}

void audio_flush_and_reload_tracks(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_FLUSH");
    queue_post(&audio_queue, Q_AUDIO_FLUSH, 0);
}

void audio_error_clear(void)
{
#ifdef AUDIO_HAVE_RECORDING
    pcm_rec_error_clear();
#endif
}

int audio_status(void)
{
    int ret = 0;

    if (playing)
        ret |= AUDIO_STATUS_PLAY;

    if (paused)
        ret |= AUDIO_STATUS_PAUSE;

#ifdef HAVE_RECORDING
    /* Do this here for constitency with mpeg.c version */
    ret |= pcm_rec_status();
#endif

    return ret;
}

int audio_get_file_pos(void)
{
    return 0;
}

void audio_set_buffer_margin(int setting)
{
    static const int lookup[] = {5, 15, 30, 60, 120, 180, 300, 600};
    buffer_margin = lookup[setting];
    logf("buffer margin: %ds", buffer_margin);
    set_filebuf_watermark(buffer_margin);
}

/* Set crossfade & PCM buffer length. */
void audio_set_crossfade(int enable)
{
    size_t size;
    bool was_playing = (playing && audio_is_initialized);
    size_t offset = 0;
#if MEM > 1
    int seconds = 1;
#endif

    if (!filebuf)
        return;     /* Audio buffers not yet set up */

#if MEM > 1
    if (enable)
        seconds = global_settings.crossfade_fade_out_delay
                + global_settings.crossfade_fade_out_duration;

    /* Buffer has to be at least 2s long. */
    seconds += 2;
    logf("buf len: %d", seconds);
    size = seconds * (NATIVE_FREQUENCY*4);
#else
    enable = 0;
    size = NATIVE_FREQUENCY*2;
#endif
    if (buffer_state == BUFFER_STATE_NORMAL && pcmbuf_is_same_size(size))
        return ;

    if (was_playing)
    {
        /* Store the track resume position */
        offset = CUR_TI->id3.offset;

        /* Playback has to be stopped before changing the buffer size. */
        gui_syncsplash(0, true, (char *)str(LANG_RESTARTING_PLAYBACK));
        audio_stop();
    }

    voice_stop();

    /* Re-initialize audio system. */
    audio_reset_buffer(size);
    pcmbuf_crossfade_enable(enable);
    logf("abuf:%dB", pcmbuf_get_bufsize());
    logf("fbuf:%dB", filebuflen);

    voice_init();

    /* Restart playback. */
    if (was_playing) 
        audio_play(offset);
}

void audio_preinit(void)
{
    logf("playback system pre-init");

    filling = false;
    current_codec = CODEC_IDX_AUDIO;
    playing = false;
    paused = false;
    audio_codec_loaded = false;
#ifdef PLAYBACK_VOICE
    voice_is_playing = false;
    voice_thread_start = false;
    voice_codec_loaded = false;
#endif
    track_changed = false;
    current_fd = -1;
    track_buffer_callback = NULL;
    track_unbuffer_callback = NULL;
    track_changed_callback = NULL;
    track_ridx = 0; /* Just to prevent CUR_TI from being anything random. */
    prev_ti = &tracks[MAX_TRACK-1]; /* And prevent prev_ti being random too */

#ifdef PLAYBACK_VOICE
    mutex_init(&mutex_codecthread);
#endif

    queue_init(&audio_queue, true);
    queue_init(&codec_queue, true);
    /* create a private queue */
    queue_init(&codec_callback_queue, false);

    create_thread(audio_thread, audio_stack, sizeof(audio_stack),
                  audio_thread_name IF_PRIO(, PRIORITY_BUFFERING));
}

void audio_init(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_POSTINIT");
    queue_post(&audio_queue, Q_AUDIO_POSTINIT, 0);
}

void voice_init(void)
{
#ifdef PLAYBACK_VOICE
    if (!filebuf)
        return;     /* Audio buffers not yet set up */

    if (voice_thread_p)
        return;

    if (!talk_voice_required())
        return;

    logf("Starting voice codec");
    queue_init(&voice_queue, true);
    voice_thread_p = create_thread(voice_thread, voice_stack,
            sizeof(voice_stack), voice_thread_name 
            IF_PRIO(, PRIORITY_PLAYBACK));
    
    while (!voice_codec_loaded)
        yield();
#endif
} /* voice_init */

void voice_stop(void)
{
#ifdef PLAYBACK_VOICE
    /* Messages should not be posted to voice codec queue unless it is the
       current codec or deadlocks happen. */
    if (current_codec != CODEC_IDX_VOICE)
        return;

    LOGFQUEUE("mp3 > voice Q_VOICE_STOP");
    queue_post(&voice_queue, Q_VOICE_STOP, 0);
    while (voice_is_playing || !queue_empty(&voice_queue))
        yield();
    if (!playing)   
        pcmbuf_play_stop();
#endif
} /* voice_stop */



/* --- Routines called from multiple threads --- */
#ifdef PLAYBACK_VOICE
static void swap_codec(void)
{
    int my_codec = current_codec;

    logf("swapping out codec:%d", my_codec);

    /* Save our current IRAM and DRAM */
#ifdef IRAM_STEAL
    if (voice_iram_stolen)
    {
        logf("swap: iram restore");
        voice_iram_stolen = false;
        /* Don't swap trashed data into buffer - _should_ always be the case
           if voice_iram_stolen is true since the voice has been swapped in
           before hand */
        if (my_codec == CODEC_IDX_VOICE)
            goto skip_iram_swap;
    }
#endif

    memcpy(iram_buf[my_codec], (unsigned char *)CODEC_IRAM_ORIGIN,
            CODEC_IRAM_SIZE);

#ifdef IRAM_STEAL
skip_iram_swap:
#endif

    memcpy(dram_buf[my_codec], codecbuf, CODEC_SIZE);

    /* Release my semaphore */
    mutex_unlock(&mutex_codecthread);

    /* Loop until the other codec has locked and run */
    do {
        /* Release my semaphore and force a task switch. */
        yield();
    } while (my_codec == current_codec);

    /* Wait for other codec to unlock */
    mutex_lock(&mutex_codecthread);

    /* Take control */
    current_codec = my_codec;

    /* Reload our IRAM and DRAM */
    memcpy((unsigned char *)CODEC_IRAM_ORIGIN, iram_buf[my_codec],
            CODEC_IRAM_SIZE);
    invalidate_icache();
    memcpy(codecbuf, dram_buf[my_codec], CODEC_SIZE);

    logf("resuming codec:%d", my_codec);
}
#endif

static void set_filebuf_watermark(int seconds)
{
    size_t bytes;

    if (current_codec == CODEC_IDX_VOICE)
        return;

    if (!filebuf)
        return;     /* Audio buffers not yet set up */

    bytes = MAX(CUR_TI->id3.bitrate * seconds * (1000/8), conf_watermark);
    bytes = MIN(bytes, filebuflen / 2);
    conf_watermark = bytes;
}

static const char * get_codec_filename(int cod_spec)
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


/* --- Voice thread --- */

#ifdef PLAYBACK_VOICE

static bool voice_pcmbuf_insert_split_callback(
        const void *ch1, const void *ch2, size_t length)
{
    const char* src[2];
    char *dest;
    long input_size;
    size_t output_size;

    src[0] = ch1;
    src[1] = ch2;

    if (dsp_stereo_mode() == STEREO_NONINTERLEAVED)
        length *= 2;    /* Length is per channel */

    while (length)
    {
        long est_output_size = dsp_output_size(length);
        
        while ((dest = pcmbuf_request_voice_buffer(est_output_size,
                        &output_size, playing)) == NULL)
        {
            if (playing && audio_codec_loaded)
                swap_codec();
            else
                yield();
        }
        
        /* Get the real input_size for output_size bytes, guarding
         * against resampling buffer overflows. */
        input_size = dsp_input_size(output_size);

        if (input_size <= 0) 
        {
            DEBUGF("Error: dsp_input_size(%ld=dsp_output_size(%ld))=%ld<=0\n",
                    output_size, length, input_size);
            /* If this happens, there are samples of codec data that don't
             * become a number of pcm samples, and something is broken */
            return false;
        }

        /* Input size has grown, no error, just don't write more than length */
        if ((size_t)input_size > length)
            input_size = length;

        output_size = dsp_process(dest, src, input_size);

        if (playing)
        {
            pcmbuf_mix_voice(output_size);
            if ((pcmbuf_usage() < 10 || pcmbuf_mix_free() < 30) && audio_codec_loaded)
                swap_codec();
        }
        else
            pcmbuf_write_complete(output_size);

        length -= input_size;
    }

    return true;
} /* voice_pcmbuf_insert_split_callback */

static bool voice_pcmbuf_insert_callback(const char *buf, size_t length)
{
    /* TODO: The audiobuffer API should probably be updated, and be based on
     *       pcmbuf_insert_split().  */
    long real_length = length;

    if (dsp_stereo_mode() == STEREO_NONINTERLEAVED)
        length /= 2;    /* Length is per channel */

    /* Second channel is only used for non-interleaved stereo. */
    return voice_pcmbuf_insert_split_callback(buf, buf + (real_length / 2),
        length);
}

static void* voice_get_memory_callback(size_t *size)
{
    *size = 0;
    return NULL;
}

static void voice_set_elapsed_callback(unsigned int value)
{
    (void)value;
}

static void voice_set_offset_callback(size_t value)
{
    (void)value;
}

static size_t voice_filebuf_callback(void *ptr, size_t size)
{
    (void)ptr;
    (void)size;

    return 0;
}

static void* voice_request_buffer_callback(size_t *realsize, size_t reqsize)
{
    struct event ev;

    if (ci_voice.new_track)
    {
        *realsize = 0;
        return NULL;
    }

    while (1)
    {
        if (voice_is_playing || playing)
            queue_wait_w_tmo(&voice_queue, &ev, 0);
        else
            queue_wait(&voice_queue, &ev);
        if (!voice_is_playing)
        {
            if (ev.id == SYS_TIMEOUT)
                ev.id = Q_AUDIO_PLAY;
        }

        switch (ev.id) {
            case Q_AUDIO_PLAY:
                LOGFQUEUE("voice < Q_AUDIO_PLAY");
                if (playing)
                {
                    if (audio_codec_loaded)
                        swap_codec();
                    yield();
                }
                break;

#ifdef AUDIO_HAVE_RECORDING
            case Q_ENCODER_RECORD:
                LOGFQUEUE("voice < Q_ENCODER_RECORD");
                swap_codec();
                break;
#endif

            case Q_VOICE_STOP:
                LOGFQUEUE("voice < Q_VOICE_STOP");
                if (ev.data == (void *)1 && !playing && pcm_is_playing())
                {
                    /* Aborting: Slight hack - flush PCM buffer if
                       only being used for voice */
                    pcmbuf_play_stop();
                }
                if (voice_is_playing)
                {
                    /* Clear the current buffer */
                    voice_is_playing = false;
                    voice_getmore = NULL;
                    voice_remaining = 0;
                    voicebuf = NULL;

                    /* Force the codec to think it's changing tracks */
                    ci_voice.new_track = 1; 
                    *realsize = 0;
                    return NULL;
                }
                else
                    break;

            case SYS_USB_CONNECTED:
                LOGFQUEUE("voice < SYS_USB_CONNECTED");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                if (audio_codec_loaded)
                    swap_codec();
                usb_wait_for_disconnect(&voice_queue);
                break;

            case Q_VOICE_PLAY:
                LOGFQUEUE("voice < Q_VOICE_PLAY");
                if (!voice_is_playing)
                {
                    /* Set up new voice data */
                    struct voice_info *voice_data;
#ifdef IRAM_STEAL
                    if (voice_iram_stolen)
                    {
                        logf("voice: iram restore");
                        memcpy((void*)CODEC_IRAM_ORIGIN,
                               iram_buf[CODEC_IDX_VOICE],
                               CODEC_IRAM_SIZE);
                        voice_iram_stolen = false;
                    }
#endif
                    /* must reset the buffer before any playback
                       begins if needed */
                    if (buffer_state == BUFFER_STATE_TRASHED)
                        audio_reset_buffer(pcmbuf_get_bufsize());

                    voice_is_playing = true;
                    trigger_cpu_boost();
                    voice_data = ev.data;
                    voice_remaining = voice_data->size;
                    voicebuf = voice_data->buf;
                    voice_getmore = voice_data->callback;
                }
                goto voice_play_clip;

            case SYS_TIMEOUT:
                LOGFQUEUE_SYS_TIMEOUT("voice < SYS_TIMEOUT");
                goto voice_play_clip;

            default:
                LOGFQUEUE("voice < default");
        }
    }

voice_play_clip:

    if (voice_remaining == 0 || voicebuf == NULL)
    {
        if (voice_getmore)
            voice_getmore((unsigned char **)&voicebuf, (int *)&voice_remaining);

        /* If this clip is done */
        if (voice_remaining == 0)
        {
            LOGFQUEUE("voice > voice Q_VOICE_STOP");
            queue_post(&voice_queue, Q_VOICE_STOP, 0);
            /* Force pcm playback. */
            if (!pcm_is_playing())
                pcmbuf_play_start();
        }
    }
    
    *realsize = MIN(voice_remaining, reqsize);

    if (*realsize == 0)
        return NULL;

    return voicebuf;
} /* voice_request_buffer_callback */

static void voice_advance_buffer_callback(size_t amount)
{
    amount = MIN(amount, voice_remaining);
    voicebuf += amount;
    voice_remaining -= amount;
}

static void voice_advance_buffer_loc_callback(void *ptr)
{
    size_t amount = (size_t)ptr - (size_t)voicebuf;
    
    voice_advance_buffer_callback(amount);
}

static off_t voice_mp3_get_filepos_callback(int newtime)
{
    (void)newtime;
    
    return 0;
}

static void voice_do_nothing(void)
{
    return;
}

static bool voice_seek_buffer_callback(size_t newpos)
{
    (void)newpos;
    
    return false;
}

static bool voice_request_next_track_callback(void)
{
    ci_voice.new_track = 0;
    return true;
}

static void voice_thread(void)
{
    while (1)
    {
        logf("Loading voice codec");
        voice_codec_loaded = true;
        mutex_lock(&mutex_codecthread);
        current_codec = CODEC_IDX_VOICE;
        dsp_configure(DSP_RESET, 0);
        voice_remaining = 0;
        voice_getmore = NULL;

        codec_load_file(get_codec_filename(AFMT_MPA_L3), &ci_voice);

        logf("Voice codec finished");
        voice_codec_loaded = false;
        mutex_unlock(&mutex_codecthread);
    }
} /* voice_thread */

#endif /* PLAYBACK_VOICE */

/* --- Codec thread --- */

static bool codec_pcmbuf_insert_split_callback(
        const void *ch1, const void *ch2, size_t length)
{
    const char* src[2];
    char *dest;
    long input_size;
    size_t output_size;

    src[0] = ch1;
    src[1] = ch2;

    if (dsp_stereo_mode() == STEREO_NONINTERLEAVED)
        length *= 2;    /* Length is per channel */

    while (length)
    {
        long est_output_size = dsp_output_size(length);
        /* Prevent audio from a previous track from playing */
        if (ci.new_track || ci.stop_codec)
            return true;

        while ((dest = pcmbuf_request_buffer(est_output_size,
                        &output_size)) == NULL) 
        {
            sleep(1);
            if (ci.seek_time || ci.new_track || ci.stop_codec)
                return true;
        }

        /* Get the real input_size for output_size bytes, guarding
         * against resampling buffer overflows. */
        input_size = dsp_input_size(output_size);

        if (input_size <= 0) 
        {
            DEBUGF("Error: dsp_input_size(%ld=dsp_output_size(%ld))=%ld<=0\n",
                    output_size, length, input_size);
            /* If this happens, there are samples of codec data that don't
             * become a number of pcm samples, and something is broken */
            return false;
        }

        /* Input size has grown, no error, just don't write more than length */
        if ((size_t)input_size > length)
            input_size = length;

        output_size = dsp_process(dest, src, input_size);

        pcmbuf_write_complete(output_size);

#ifdef PLAYBACK_VOICE
        if ((voice_is_playing || voice_thread_start) 
            && pcm_is_playing() && voice_codec_loaded &&
            pcmbuf_usage() > 30 && pcmbuf_mix_free() > 80)
        {
            voice_thread_start = false;
            swap_codec();
        }
#endif
        
        length -= input_size;
    }

    return true;
} /* codec_pcmbuf_insert_split_callback */

static bool codec_pcmbuf_insert_callback(const char *buf, size_t length)
{
    /* TODO: The audiobuffer API should probably be updated, and be based on
     *       pcmbuf_insert_split().  */
    long real_length = length;

    if (dsp_stereo_mode() == STEREO_NONINTERLEAVED)
        length /= 2;    /* Length is per channel */

    /* Second channel is only used for non-interleaved stereo. */
    return codec_pcmbuf_insert_split_callback(buf, buf + (real_length / 2),
        length);
}

static void* codec_get_memory_callback(size_t *size)
{
    *size = MALLOC_BUFSIZE;
    return &audiobuf[talk_get_bufsize()];
}

static void codec_pcmbuf_position_callback(size_t size) ICODE_ATTR;
static void codec_pcmbuf_position_callback(size_t size) 
{
    /* This is called from an ISR, so be quick */
    unsigned int time = size * 1000 / 4 / NATIVE_FREQUENCY +
        prev_ti->id3.elapsed;
    
    if (time >= prev_ti->id3.length) 
    {
        pcmbuf_set_position_callback(NULL);
        prev_ti->id3.elapsed = prev_ti->id3.length;
    } 
    else 
        prev_ti->id3.elapsed = time;
}

static void codec_set_elapsed_callback(unsigned int value)
{
    unsigned int latency;
    if (ci.seek_time)
        return;

#ifdef AB_REPEAT_ENABLE
    ab_position_report(value);
#endif

    latency = pcmbuf_get_latency();
    if (value < latency)
        CUR_TI->id3.elapsed = 0;
    else if (value - latency > CUR_TI->id3.elapsed ||
            value - latency < CUR_TI->id3.elapsed - 2)
    {
        CUR_TI->id3.elapsed = value - latency;
    }
}

static void codec_set_offset_callback(size_t value)
{
    unsigned int latency;
    
    if (ci.seek_time)
        return;

    latency = pcmbuf_get_latency() * CUR_TI->id3.bitrate / 8;
    if (value < latency)
        CUR_TI->id3.offset = 0;
    else
        CUR_TI->id3.offset = value - latency;
}

static void codec_advance_buffer_counters(size_t amount) 
{
    buf_ridx = RINGBUF_ADD(buf_ridx, amount);
    ci.curpos += amount;
    CUR_TI->available -= amount;

    /* Start buffer filling as necessary. */
    if (!pcmbuf_is_lowdata() && !filling)
    {
        if (FILEBUFUSED < conf_watermark && playing && !playlist_end)
        {
            LOGFQUEUE("codec > audio Q_AUDIO_FILL_BUFFER");
            queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);
        }
    }
}

/* copy up-to size bytes into ptr and return the actual size copied */
static size_t codec_filebuf_callback(void *ptr, size_t size)
{
    char *buf = (char *)ptr;
    size_t copy_n;
    size_t part_n;

    if (ci.stop_codec || !playing)
        return 0;

    /* The ammount to copy is the lesser of the requested amount and the
     * amount left of the current track (both on disk and already loaded) */
    copy_n = MIN(size, CUR_TI->available + CUR_TI->filerem);

    /* Nothing requested OR nothing left */
    if (copy_n == 0)
        return 0;

    /* Let the disk buffer catch fill until enough data is available */
    while (copy_n > CUR_TI->available) 
    {
        if (!filling)
        {
            LOGFQUEUE("codec > audio Q_AUDIO_FILL_BUFFER");
            queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);
        }
        
        sleep(1);
        if (ci.stop_codec || ci.new_track)
            return 0;
    }

    /* Copy as much as possible without wrapping */
    part_n = MIN(copy_n, filebuflen - buf_ridx);
    memcpy(buf, &filebuf[buf_ridx], part_n);
    /* Copy the rest in the case of a wrap */
    if (part_n < copy_n) {
        memcpy(&buf[part_n], &filebuf[0], copy_n - part_n);
    }

    /* Update read and other position pointers */
    codec_advance_buffer_counters(copy_n);

    /* Return the actual amount of data copied to the buffer */
    return copy_n;
} /* codec_filebuf_callback */

static void* codec_request_buffer_callback(size_t *realsize, size_t reqsize)
{
    size_t short_n, copy_n, buf_rem;

    if (!playing) 
    {
        *realsize = 0;
        return NULL;
    }

    copy_n = MIN(reqsize, CUR_TI->available + CUR_TI->filerem);
    if (copy_n == 0) 
    {
        *realsize = 0;
        return NULL;
    }

    while (copy_n > CUR_TI->available) 
    {
        if (!filling)
        {
            LOGFQUEUE("codec > audio Q_AUDIO_FILL_BUFFER");
            queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);
        }
        
        sleep(1);
        if (ci.stop_codec || ci.new_track) 
        {
            *realsize = 0;
            return NULL;
        }
    }

    /* How much is left at the end of the file buffer before wrap? */
    buf_rem = filebuflen - buf_ridx;
    
    /* If we can't satisfy the request without wrapping */
    if (buf_rem < copy_n) 
    {
        /* How short are we? */
        short_n = copy_n - buf_rem;
        
        /* If we can fudge it with the guardbuf */
        if (short_n < GUARD_BUFSIZE)
            memcpy(&filebuf[filebuflen], &filebuf[0], short_n);
        else
            copy_n = buf_rem;
    }

    *realsize = copy_n;
    
    return (char *)&filebuf[buf_ridx];
} /* codec_request_buffer_callback */

static int get_codec_base_type(int type)
{
    switch (type) {
        case AFMT_MPA_L1:
        case AFMT_MPA_L2:
        case AFMT_MPA_L3:
            return AFMT_MPA_L3;
    }

    return type;
}

static void codec_advance_buffer_callback(size_t amount)
{
    if (amount > CUR_TI->available + CUR_TI->filerem)
        amount = CUR_TI->available + CUR_TI->filerem;

    while (amount > CUR_TI->available && filling)
        sleep(1);

    if (amount > CUR_TI->available) 
    {
        struct event ev;
        
        LOGFQUEUE("codec > audio Q_AUDIO_REBUFFER_SEEK");
        queue_post(&audio_queue,
                Q_AUDIO_REBUFFER_SEEK, (void *)(ci.curpos + amount));
        
        queue_wait(&codec_callback_queue, &ev);
        switch (ev.id)
        {
            case Q_CODEC_REQUEST_FAILED:
                LOGFQUEUE("codec < Q_CODEC_REQUEST_FAILED");
                ci.stop_codec = true;
                return;

            case Q_CODEC_REQUEST_COMPLETE:
                LOGFQUEUE("codec < Q_CODEC_REQUEST_COMPLETE");
                return;           

            default:
                LOGFQUEUE("codec < default");
                ci.stop_codec = true;
                return;
        }
    }

    codec_advance_buffer_counters(amount);

    codec_set_offset_callback(ci.curpos);
}

static void codec_advance_buffer_loc_callback(void *ptr)
{
    size_t amount = (size_t)ptr - (size_t)&filebuf[buf_ridx];
    
    codec_advance_buffer_callback(amount);
}

/* Copied from mpeg.c. Should be moved somewhere else. */
static int codec_get_file_pos(void)
{
    int pos = -1;
    struct mp3entry *id3 = audio_current_track();

    if (id3->vbr)
    {
        if (id3->has_toc)
        {
            /* Use the TOC to find the new position */
            unsigned int percent, remainder;
            int curtoc, nexttoc, plen;

            percent = (id3->elapsed*100)/id3->length;
            if (percent > 99)
                percent = 99;

            curtoc = id3->toc[percent];

            if (percent < 99)
                nexttoc = id3->toc[percent+1];
            else
                nexttoc = 256;

            pos = (id3->filesize/256)*curtoc;

            /* Use the remainder to get a more accurate position */
            remainder   = (id3->elapsed*100)%id3->length;
            remainder   = (remainder*100)/id3->length;
            plen        = (nexttoc - curtoc)*(id3->filesize/256);
            pos        += (plen/100)*remainder;
        }
        else
        {
            /* No TOC exists, estimate the new position */
            pos = (id3->filesize / (id3->length / 1000)) *
                (id3->elapsed / 1000);
        }
    }
    else if (id3->bitrate)
        pos = id3->elapsed * (id3->bitrate / 8);
    else
        return -1;

    pos += id3->first_frame_offset;

    /* Don't seek right to the end of the file so that we can
       transition properly to the next song */
    if (pos >= (int)(id3->filesize - id3->id3v1len))
        pos = id3->filesize - id3->id3v1len - 1;

    return pos;
}

static off_t codec_mp3_get_filepos_callback(int newtime)
{
    off_t newpos;

    CUR_TI->id3.elapsed = newtime;
    newpos = codec_get_file_pos();

    return newpos;
}

static void codec_seek_complete_callback(void)
{
    logf("seek_complete");
    if (pcm_is_paused()) 
    {
        /* If this is not a seamless seek, clear the buffer */
        pcmbuf_play_stop();
        dsp_configure(DSP_FLUSH, NULL);

        /* If playback was not 'deliberately' paused, unpause now */
        if (!paused)
            pcmbuf_pause(false);
    }
    ci.seek_time = 0;
}

static bool codec_seek_buffer_callback(size_t newpos)
{
    int difference;

    logf("codec_seek_buffer_callback");

    if (newpos >= CUR_TI->filesize)
        newpos = CUR_TI->filesize - 1;

    difference = newpos - ci.curpos;
    if (difference >= 0) 
    {
        /* Seeking forward */
        logf("seek: +%d", difference);
        codec_advance_buffer_callback(difference);
        return true;
    }

    /* Seeking backward */
    difference = -difference;
    if (ci.curpos - difference < 0)
        difference = ci.curpos;

    /* We need to reload the song. */
    if (newpos < CUR_TI->start_pos)
    {
        struct event ev;
        
        LOGFQUEUE("codec > audio Q_AUDIO_REBUFFER_SEEK");
        queue_post(&audio_queue, Q_AUDIO_REBUFFER_SEEK, (void *)newpos);
        
        queue_wait(&codec_callback_queue, &ev);
        switch (ev.id)
        {
            case Q_CODEC_REQUEST_COMPLETE:
                LOGFQUEUE("codec < Q_CODEC_REQUEST_COMPLETE");
                return true;
            
            case Q_CODEC_REQUEST_FAILED:
                LOGFQUEUE("codec < Q_CODEC_REQUEST_FAILED");
                ci.stop_codec = true;
                return false;
            
            default:
                LOGFQUEUE("codec < default");
                return false;
        }
    }

    /* Seeking inside buffer space. */
    logf("seek: -%d", difference);
    CUR_TI->available += difference;
    buf_ridx = RINGBUF_SUB(buf_ridx, (unsigned)difference);
    ci.curpos -= difference;

    return true;
}

static void codec_configure_callback(int setting, void *value)
{
    switch (setting) {
    case CODEC_SET_FILEBUF_WATERMARK:
        conf_watermark = (unsigned long)value;
        set_filebuf_watermark(buffer_margin);
        break;

    case CODEC_SET_FILEBUF_CHUNKSIZE:
        conf_filechunk = (unsigned long)value;
        break;

    case CODEC_SET_FILEBUF_PRESEEK:
        conf_preseek = (unsigned long)value;
        break;

    default:
        if (!dsp_configure(setting, value)) { logf("Illegal key:%d", setting); }
    }
}

static void codec_track_changed(void)
{
    automatic_skip = false;
    LOGFQUEUE("codec > audio Q_AUDIO_TRACK_CHANGED");
    queue_post(&audio_queue, Q_AUDIO_TRACK_CHANGED, 0);
}

static void codec_pcmbuf_track_changed_callback(void)
{
    pcmbuf_set_position_callback(NULL);
    codec_track_changed();
}

static void codec_discard_codec_callback(void)
{
    if (CUR_TI->has_codec) 
    {
        CUR_TI->has_codec = false;
        buf_ridx = RINGBUF_ADD(buf_ridx, CUR_TI->codecsize);
    }

#if 0
    /* Check if a buffer desync has happened, log it and stop playback. */
    if (buf_ridx != CUR_TI->buf_idx)
    {
        int offset = CUR_TI->buf_idx - buf_ridx;
        size_t new_used = FILEBUFUSED - offset;
        
        logf("Buf off :%d=%d-%d", offset, CUR_TI->buf_idx, buf_ridx);
        logf("Used off:%d",FILEBUFUSED - new_used);
        
        /* This is a fatal internal error and it's not safe to 
         * continue playback. */
        ci.stop_codec = true;
        queue_post(&audio_queue, Q_AUDIO_STOP, 0);
    }
#endif
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
             && global_settings.crossfade != 2)
    {
        pcmbuf_crossfade_init(false);
        codec_track_changed();
    }
    /* Gapless playback. */
    else
    {
        pcmbuf_set_position_callback(codec_pcmbuf_position_callback);
        pcmbuf_set_event_handler(codec_pcmbuf_track_changed_callback);
    }
}

static bool codec_load_next_track(void) 
{
    struct event ev;

    prev_track_elapsed = CUR_TI->id3.elapsed;

    if (ci.seek_time)
        codec_seek_complete_callback();

#ifdef AB_REPEAT_ENABLE
    ab_end_of_track_report();
#endif

    logf("Request new track");

    if (ci.new_track == 0)
    {
        ci.new_track++;
        automatic_skip = true;
    }
    
    trigger_cpu_boost();
    LOGFQUEUE("codec > audio Q_AUDIO_CHECK_NEW_TRACK");
    queue_post(&audio_queue, Q_AUDIO_CHECK_NEW_TRACK, 0);
    while (1) 
    {
        queue_wait(&codec_callback_queue, &ev);
        if (ev.id == Q_CODEC_REQUEST_PENDING)
        {
            if (!automatic_skip)
                pcmbuf_play_stop();
        }
        else
            break;
    }

    switch (ev.id)
    {
        case Q_CODEC_REQUEST_COMPLETE:
            LOGFQUEUE("codec < Q_CODEC_REQUEST_COMPLETE");
            codec_track_skip_done(!automatic_skip);
            return true;

        case Q_CODEC_REQUEST_FAILED:
            LOGFQUEUE("codec < Q_CODEC_REQUEST_FAILED");
            ci.new_track = 0;
            ci.stop_codec = true;
            return false;

        default:
            LOGFQUEUE("codec < default");
            ci.stop_codec = true;
            return false;
    }
}

static bool codec_request_next_track_callback(void)
{
    int prev_codectype;

    if (ci.stop_codec || !playing)
        return false;

    prev_codectype = get_codec_base_type(CUR_TI->id3.codectype);

    if (!codec_load_next_track())
        return false;

    /* Check if the next codec is the same file. */
    if (prev_codectype == get_codec_base_type(CUR_TI->id3.codectype))
    {
        logf("New track loaded");
        codec_discard_codec_callback();
        return true;
    }
    else
    {
        logf("New codec:%d/%d", CUR_TI->id3.codectype, prev_codectype);
        return false;
    }
}

static void codec_thread(void)
{
    struct event ev;
    int status;
    size_t wrap;

    while (1) {
        status = 0;
        queue_wait(&codec_queue, &ev);

        switch (ev.id) {
            case Q_CODEC_LOAD_DISK:
                LOGFQUEUE("codec < Q_CODEC_LOAD_DISK");
                audio_codec_loaded = true;
#ifdef PLAYBACK_VOICE
                /* Don't sent messages to voice codec if it's not current */
                if (voice_codec_loaded && current_codec == CODEC_IDX_VOICE)
                {
                    LOGFQUEUE("codec > voice Q_AUDIO_PLAY");
                    queue_post(&voice_queue, Q_AUDIO_PLAY, 0);
                }
                mutex_lock(&mutex_codecthread);
#endif
                current_codec = CODEC_IDX_AUDIO;
                ci.stop_codec = false;
                status = codec_load_file((const char *)ev.data, &ci);
#ifdef PLAYBACK_VOICE
                mutex_unlock(&mutex_codecthread);
#endif
                break ;

            case Q_CODEC_LOAD:
                LOGFQUEUE("codec < Q_CODEC_LOAD");
                if (!CUR_TI->has_codec) {
                    logf("Codec slot is empty!");
                    /* Wait for the pcm buffer to go empty */
                    while (pcm_is_playing())
                        yield();
                    /* This must be set to prevent an infinite loop */
                    ci.stop_codec = true;
                    LOGFQUEUE("codec > codec Q_AUDIO_PLAY");
                    queue_post(&codec_queue, Q_AUDIO_PLAY, 0);
                    break ;
                }

                audio_codec_loaded = true;
#ifdef PLAYBACK_VOICE
                if (voice_codec_loaded && current_codec == CODEC_IDX_VOICE)
                {
                    LOGFQUEUE("codec > voice Q_AUDIO_PLAY");
                    queue_post(&voice_queue, Q_AUDIO_PLAY, 0);
                }
                mutex_lock(&mutex_codecthread);
#endif
                current_codec = CODEC_IDX_AUDIO;
                ci.stop_codec = false;
                wrap = (size_t)&filebuf[filebuflen] - (size_t)CUR_TI->codecbuf;
                status = codec_load_ram(CUR_TI->codecbuf, CUR_TI->codecsize,
                        &filebuf[0], wrap, &ci);
#ifdef PLAYBACK_VOICE
                mutex_unlock(&mutex_codecthread);
#endif
                break ;

#ifdef AUDIO_HAVE_RECORDING
            case Q_ENCODER_LOAD_DISK:
                LOGFQUEUE("codec < Q_ENCODER_LOAD_DISK");
                audio_codec_loaded = false; /* Not audio codec! */
#ifdef PLAYBACK_VOICE
                if (voice_codec_loaded && current_codec == CODEC_IDX_VOICE)
                {
                    LOGFQUEUE("codec > voice Q_ENCODER_RECORD");
                    queue_post(&voice_queue, Q_ENCODER_RECORD, NULL);
                }
                mutex_lock(&mutex_codecthread);
#endif
                logf("loading encoder");
                current_codec = CODEC_IDX_AUDIO;
                ci.stop_codec = false;
                status = codec_load_file((const char *)ev.data, &ci);
#ifdef PLAYBACK_VOICE
                mutex_unlock(&mutex_codecthread);
#endif
                logf("encoder stopped");
                break;
#endif /* AUDIO_HAVE_RECORDING */

#ifndef SIMULATOR
            case SYS_USB_CONNECTED:  
                LOGFQUEUE("codec < SYS_USB_CONNECTED");
                queue_clear(&codec_queue);
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&codec_queue);
                break;
#endif
                
            default:
                LOGFQUEUE("codec < default");
        }

        if (audio_codec_loaded)
        {
            if (ci.stop_codec)
            {
                status = CODEC_OK;
                if (!playing)
                    pcmbuf_play_stop();

            }
            audio_codec_loaded = false;
        }

        switch (ev.id) {
            case Q_CODEC_LOAD_DISK:
            case Q_CODEC_LOAD:
                LOGFQUEUE("codec < Q_CODEC_LOAD");
                if (playing) 
                {
                    if (ci.new_track || status != CODEC_OK) 
                    {
                        if (!ci.new_track) 
                        {
                            logf("Codec failure");
                            gui_syncsplash(HZ*2, true, "Codec failure");
                        }
                        
                        if (!codec_load_next_track())
                        {
                            // queue_post(&codec_queue, Q_AUDIO_STOP, 0);
                            LOGFQUEUE("codec > audio Q_AUDIO_STOP");
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
                                CUR_TI->id3.elapsed = CUR_TI->id3.length - pcmbuf_get_latency();
                                sleep(1);
                            }
                            LOGFQUEUE("codec > audio Q_AUDIO_STOP");
                            queue_post(&audio_queue, Q_AUDIO_STOP, 0);
                            break;
                        }
                    }
                    
                    if (CUR_TI->has_codec)
                    {
                        LOGFQUEUE("codec > codec Q_CODEC_LOAD");
                        queue_post(&codec_queue, Q_CODEC_LOAD, 0);
                    }
                    else
                    {
                        const char *codec_fn = get_codec_filename(CUR_TI->id3.codectype);
                        LOGFQUEUE("codec > codec Q_CODEC_LOAD_DISK");
                        queue_post(&codec_queue, Q_CODEC_LOAD_DISK,
                            (void *)codec_fn);
                    }
                }
                break;

#ifdef AUDIO_HAVE_RECORDING
            case Q_ENCODER_LOAD_DISK:
                LOGFQUEUE("codec < Q_ENCODER_LOAD_DISK");

                if (status == CODEC_OK)
                    break;

                logf("Encoder failure");
                gui_syncsplash(HZ*2, true, "Encoder failure");

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


/* --- Audio thread --- */

static bool audio_filebuf_is_lowdata(void)
{
    return FILEBUFUSED < AUDIO_FILEBUF_CRITICAL;
}

static bool audio_have_tracks(void)
{
    return track_ridx != track_widx || CUR_TI->filesize;
}

static bool audio_have_free_tracks(void)
{
    if (track_widx < track_ridx)
        return track_widx + 1 < track_ridx;
    else if (track_ridx == 0)
        return track_widx < MAX_TRACK - 1;
    
    return true;
}
        
int audio_track_count(void)
{
    if (audio_have_tracks())
    {
        int relative_track_widx = track_widx;
        
        if (track_ridx > track_widx)
            relative_track_widx += MAX_TRACK;
        
        return relative_track_widx - track_ridx + 1;
    }
    
    return 0;
}

long audio_filebufused(void)
{
    return (long) FILEBUFUSED;
}

/* Count the data BETWEEN the selected tracks */
static size_t audio_buffer_count_tracks(int from_track, int to_track) 
{
    size_t amount = 0;
    bool need_wrap = to_track < from_track;

    while (1)
    {
        if (++from_track >= MAX_TRACK)
        {
            from_track -= MAX_TRACK;
            need_wrap = false;
        }
        
        if (from_track >= to_track && !need_wrap)
            break;
        
        amount += tracks[from_track].codecsize + tracks[from_track].filesize;
    }
    return amount;
}

static bool audio_buffer_wind_forward(int new_track_ridx, int old_track_ridx)
{
    size_t amount;

    /* Start with the remainder of the previously playing track */
    amount = tracks[old_track_ridx].filesize - ci.curpos;
    /* Then collect all data from tracks in between them */
    amount += audio_buffer_count_tracks(old_track_ridx, new_track_ridx);
    logf("bwf:%ldB", (long) amount);
    
    if (amount > FILEBUFUSED)
        return false;

    /* Wind the buffer to the beginning of the target track or its codec */
    buf_ridx = RINGBUF_ADD(buf_ridx, amount);
    
    return true;
}

static bool audio_buffer_wind_backward(int new_track_ridx, int old_track_ridx) 
{
    /* Available buffer data */
    size_t buf_back;
    /* Start with the previously playing track's data and our data */
    size_t amount;
    
    amount = ci.curpos;
    buf_back = RINGBUF_SUB(buf_ridx, buf_widx);
    
    /* If we're not just resetting the current track */
    if (new_track_ridx != old_track_ridx)
    {
        /* Need to wind to before the old track's codec and our filesize */
        amount += tracks[old_track_ridx].codecsize;
        amount += tracks[new_track_ridx].filesize;

        /* Rewind the old track to its beginning */
        tracks[old_track_ridx].available =
            tracks[old_track_ridx].filesize - tracks[old_track_ridx].filerem;
    }

    /* If the codec was ever buffered */
    if (tracks[new_track_ridx].codecsize)
    {
        /* Add the codec to the needed size */
        amount += tracks[new_track_ridx].codecsize;
        tracks[new_track_ridx].has_codec = true;
    }

    /* Then collect all data from tracks between new and old */
    amount += audio_buffer_count_tracks(new_track_ridx, old_track_ridx);

    /* Do we have space to make this skip? */
    if (amount > buf_back)
        return false;

    logf("bwb:%ldB",amount);

    /* Rewind the buffer to the beginning of the target track or its codec */
    buf_ridx = RINGBUF_SUB(buf_ridx, amount);

    /* Reset to the beginning of the new track */
    tracks[new_track_ridx].available = tracks[new_track_ridx].filesize;

    return true;
}

static void audio_update_trackinfo(void)
{
    ci.filesize = CUR_TI->filesize;
    CUR_TI->id3.elapsed = 0;
    CUR_TI->id3.offset = 0;
    ci.id3 = &CUR_TI->id3;
    ci.curpos = 0;
    ci.taginfo_ready = &CUR_TI->taginfo_ready;
}

/* Yield to codecs for as long as possible if they are in need of data
 * return true if the caller should break to let the audio thread process
 * new events */
static bool audio_yield_codecs(void)
{
    yield();
    
    if (!queue_empty(&audio_queue)) 
        return true;

    while ((pcmbuf_is_crossfade_active() || pcmbuf_is_lowdata())
            && !ci.stop_codec && playing && !audio_filebuf_is_lowdata())
    {
        if (filling)
            yield();
        else
            sleep(2);

        if (!queue_empty(&audio_queue)) 
            return true;
    }
    
    return false;
}

static void audio_clear_track_entries(bool clear_unbuffered)
{
    int cur_idx = track_widx;
    int last_idx = -1;
    
    logf("Clearing tracks:%d/%d, %d", track_ridx, track_widx, clear_unbuffered);
    
    /* Loop over all tracks from write-to-read */
    while (1) 
    {
        cur_idx++;
        cur_idx &= MAX_TRACK_MASK;

        if (cur_idx == track_ridx)
            break;

        /* If the track is buffered, conditionally clear/notify,
         * otherwise clear the track if that option is selected */
        if (tracks[cur_idx].event_sent) 
        {
            if (last_idx >= 0)
            {
                /* If there is an unbuffer callback, call it, otherwise,
                 * just clear the track */
                if (track_unbuffer_callback)
                    track_unbuffer_callback(&tracks[last_idx].id3, false);
                
                memset(&tracks[last_idx], 0, sizeof(struct track_info));
            }
            last_idx = cur_idx;
        } 
        else if (clear_unbuffered)
            memset(&tracks[cur_idx], 0, sizeof(struct track_info));
    }

    /* We clear the previous instance of a buffered track throughout
     * the above loop to facilitate 'last' detection.  Clear/notify
     * the last track here */
    if (last_idx >= 0)
    {
        if (track_unbuffer_callback)
            track_unbuffer_callback(&tracks[last_idx].id3, true);
        memset(&tracks[last_idx], 0, sizeof(struct track_info));
    }
}

/* FIXME: This code should be made more generic and move to metadata.c */
static void audio_strip_tags(void)
{
    int i;
    static const unsigned char tag[] = "TAG";
    static const unsigned char apetag[] = "APETAGEX";    
    size_t tag_idx;
    size_t cur_idx;
    size_t len, version;

    tag_idx = RINGBUF_SUB(buf_widx, 128);

    if (FILEBUFUSED > 128 && tag_idx > buf_ridx)
    {
        cur_idx = tag_idx;
        for(i = 0;i < 3;i++)
        {
            if(filebuf[cur_idx] != tag[i])
                goto strip_ape_tag;

            cur_idx = RINGBUF_ADD(cur_idx, 1);
        }

        /* Skip id3v1 tag */
        logf("Skipping ID3v1 tag");
        buf_widx = tag_idx;
        tracks[track_widx].available -= 128;
        tracks[track_widx].filesize -= 128;
    }

strip_ape_tag:
    /* Check for APE tag (look for the APE tag footer) */
    tag_idx = RINGBUF_SUB(buf_widx, 32);

    if (FILEBUFUSED > 32 && tag_idx > buf_ridx)
    {
        cur_idx = tag_idx;
        for(i = 0;i < 8;i++)
        {
            if(filebuf[cur_idx] != apetag[i])
                return;

            cur_idx = RINGBUF_ADD(cur_idx, 1);
        }

        /* Read the version and length from the footer */
        version = filebuf[tag_idx+8] | (filebuf[tag_idx+9] << 8) |
                  (filebuf[tag_idx+10] << 16) | (filebuf[tag_idx+11] << 24);
        len = filebuf[tag_idx+12] | (filebuf[tag_idx+13] << 8) |
              (filebuf[tag_idx+14] << 16) | (filebuf[tag_idx+15] << 24);
        if (version == 2000)
            len += 32; /* APEv2 has a 32 byte header */

        /* Skip APE tag */
        if (FILEBUFUSED > len)
        {
            logf("Skipping APE tag (%dB)", len);
            buf_widx = RINGBUF_SUB(buf_widx, len);
            tracks[track_widx].available -= len;
            tracks[track_widx].filesize -= len;
        }
    }
}

/* Returns true if a whole file is read, false otherwise */
static bool audio_read_file(size_t minimum)
{
    bool ret_val = false;

    /* If we're called and no file is open, this is an error */
    if (current_fd < 0) 
    {
        logf("Bad fd in arf");
        /* Give some hope of miraculous recovery by forcing a track reload */
        tracks[track_widx].filesize = 0;
        /* Stop this buffering run */
        return ret_val;
    }

    trigger_cpu_boost();
    while (tracks[track_widx].filerem > 0) 
    {
        size_t copy_n;
        int overlap;
        int rc;

        /* copy_n is the largest chunk that is safe to read */
        copy_n = MIN(conf_filechunk, filebuflen - buf_widx);

        /* buf_widx == buf_ridx is defined as buffer empty, not buffer full */
        if (RINGBUF_ADD_CROSS(buf_widx,copy_n,buf_ridx) >= 0)
            break;

        /* rc is the actual amount read */
        rc = read(current_fd, &filebuf[buf_widx], copy_n);

        if (rc < 0) 
        {
            logf("File ended %dB early", tracks[track_widx].filerem);
            tracks[track_widx].filesize -= tracks[track_widx].filerem;
            tracks[track_widx].filerem = 0;
            break;
        }

        /* How much of the playing track did we overwrite */
        if (buf_widx == CUR_TI->buf_idx)
        {
            /* Special handling; zero or full overlap? */
            if (track_widx == track_ridx && CUR_TI->available == 0)
                overlap = 0;
            else
                overlap = rc;
        }
        else
            overlap = RINGBUF_ADD_CROSS(buf_widx,rc,CUR_TI->buf_idx);

        if ((unsigned)rc > tracks[track_widx].filerem)
        {
            logf("Bad: rc-filerem=%d, fixing", rc-tracks[track_widx].filerem);
            tracks[track_widx].filesize += rc - tracks[track_widx].filerem;
            tracks[track_widx].filerem = rc;
        }

        /* Advance buffer */
        buf_widx = RINGBUF_ADD(buf_widx, rc);
        tracks[track_widx].available += rc;
        tracks[track_widx].filerem -= rc;

        /* If we write into the playing track, adjust it's buffer info */
        if (overlap > 0)
        {
            CUR_TI->buf_idx += overlap;
            CUR_TI->start_pos += overlap;
        }

        /* For a rebuffer, fill at least this minimum */
        if (minimum > (unsigned)rc)
            minimum -= rc;
        /* Let the codec process up to the watermark */
        /* Break immediately if this is a quick buffer, or there is an event */
        else if (minimum || audio_yield_codecs())
        {
            /* Exit quickly, but don't stop the overall buffering process */
            ret_val = true;
            break;
        }
    }

    if (tracks[track_widx].filerem == 0) 
    {
        logf("Finished buf:%dB", tracks[track_widx].filesize);
        close(current_fd);
        current_fd = -1;
        audio_strip_tags();

        track_widx++;
        track_widx &= MAX_TRACK_MASK;

        tracks[track_widx].filesize = 0;
        return true;
    } 
    else 
    {
        logf("%s buf:%dB", ret_val?"Quick":"Partially",
                tracks[track_widx].filesize - tracks[track_widx].filerem);
        return ret_val;
    }
}

static bool audio_loadcodec(bool start_play)
{
    size_t size = 0;
    int fd;
    int rc;
    size_t copy_n;
    int prev_track;
    char codec_path[MAX_PATH]; /* Full path to codec */

    const char * codec_fn = get_codec_filename(tracks[track_widx].id3.codectype);
    if (codec_fn == NULL)
        return false;

    tracks[track_widx].has_codec = false;

    if (start_play)
    {
        /* Load the codec directly from disk and save some memory. */
        track_ridx = track_widx;
        ci.filesize = CUR_TI->filesize;
        ci.id3 = &CUR_TI->id3;
        ci.taginfo_ready = &CUR_TI->taginfo_ready;
        ci.curpos = 0;
        LOGFQUEUE("codec > codec Q_CODEC_LOAD_DISK");
        queue_post(&codec_queue, Q_CODEC_LOAD_DISK, (void *)codec_fn);
        return true;
    }
    else
    {
        /* If we already have another track than this one buffered */
        if (track_widx != track_ridx) 
        {
            prev_track = (track_widx - 1) & MAX_TRACK_MASK;
            
            /* If the previous codec is the same as this one, there is no need
             * to put another copy of it on the file buffer */
            if (get_codec_base_type(tracks[track_widx].id3.codectype) ==
                    get_codec_base_type(tracks[prev_track].id3.codectype)
                && audio_codec_loaded)
            {
                logf("Reusing prev. codec");
                return true;
            }
        }
    }

    codec_get_full_path(codec_path, codec_fn);

    fd = open(codec_path, O_RDONLY);
    if (fd < 0)
    {
        logf("Codec doesn't exist!");
        return false;
    }

    tracks[track_widx].codecsize = filesize(fd);
    
    /* Never load a partial codec */
    if (RINGBUF_ADD_CROSS(buf_widx,tracks[track_widx].codecsize,buf_ridx) >= 0)
    {
        logf("Not enough space");
        close(fd);
        return false;
    }

    while (size < tracks[track_widx].codecsize) 
    {
        copy_n = MIN(conf_filechunk, filebuflen - buf_widx);
        rc = read(fd, &filebuf[buf_widx], copy_n);
        if (rc < 0)
        {
            close(fd);
            /* This is an error condition, likely the codec file is corrupt */
            logf("Partial codec loaded");
            /* Must undo the buffer write of the partial codec */
            buf_widx = RINGBUF_SUB(buf_widx, size);
            tracks[track_widx].codecsize = 0;
            return false;
        }
        
        buf_widx = RINGBUF_ADD(buf_widx, rc);

        size += rc;       
    }

    tracks[track_widx].has_codec = true;

    close(fd);
    logf("Done: %dB", size);

    return true;
}

/* TODO: Copied from mpeg.c. Should be moved somewhere else. */
static void audio_set_elapsed(struct mp3entry* id3)
{
    unsigned long offset = id3->offset > id3->first_frame_offset ? 
        id3->offset - id3->first_frame_offset : 0;

    if ( id3->vbr ) {
        if ( id3->has_toc ) {
            /* calculate elapsed time using TOC */
            int i;
            unsigned int remainder, plen, relpos, nextpos;

            /* find wich percent we're at */
            for (i=0; i<100; i++ )
                if ( offset < id3->toc[i] * (id3->filesize / 256) )
                    break;

            i--;
            if (i < 0)
                i = 0;

            relpos = id3->toc[i];

            if (i < 99)
                nextpos = id3->toc[i+1];
            else
                nextpos = 256;

            remainder = offset - (relpos * (id3->filesize / 256));

            /* set time for this percent (divide before multiply to prevent
               overflow on long files. loss of precision is negligible on
               short files) */
            id3->elapsed = i * (id3->length / 100);

            /* calculate remainder time */
            plen = (nextpos - relpos) * (id3->filesize / 256);
            id3->elapsed += (((remainder * 100) / plen) *
                             (id3->length / 10000));
        }
        else {
            /* no TOC exists. set a rough estimate using average bitrate */
            int tpk = id3->length / 
                ((id3->filesize - id3->first_frame_offset - id3->id3v1len) /
                1024);
            id3->elapsed = offset / 1024 * tpk;
        }
    }
    else
    {
        /* constant bitrate, use exact calculation */
        if (id3->bitrate != 0)
            id3->elapsed = offset / (id3->bitrate / 8);
    }
}

static bool audio_load_track(int offset, bool start_play, bool rebuffer)
{
    char *trackname;
    off_t size;
    char msgbuf[80];

    /* Stop buffer filling if there is no free track entries.
       Don't fill up the last track entry (we wan't to store next track
       metadata there). */
    if (!audio_have_free_tracks()) 
    {
        logf("No free tracks");
        return false;
    }

    if (current_fd >= 0)
    {
        logf("Nonzero fd in alt");
        close(current_fd);
        current_fd = -1;
    }

    last_peek_offset++;
    peek_again:
    logf("Buffering track:%d/%d", track_widx, track_ridx);
    /* Get track name from current playlist read position. */
    while ((trackname = playlist_peek(last_peek_offset)) != NULL) 
    {
        /* Handle broken playlists. */
        current_fd = open(trackname, O_RDONLY);
        if (current_fd < 0) 
        {
            logf("Open failed");
            /* Skip invalid entry from playlist. */
            playlist_skip_entry(NULL, last_peek_offset);
        } 
        else
            break;
    }

    if (!trackname) 
    {
        logf("End-of-playlist");
        playlist_end = true;
        return false;
    }

    /* Initialize track entry. */
    size = filesize(current_fd);
    tracks[track_widx].filerem = size;
    tracks[track_widx].filesize = size;
    tracks[track_widx].available = 0;

    /* Set default values */
    if (start_play) 
    {
        int last_codec = current_codec;
        
        current_codec = CODEC_IDX_AUDIO;
        conf_watermark = AUDIO_DEFAULT_WATERMARK;
        conf_filechunk = AUDIO_DEFAULT_FILECHUNK;
        conf_preseek = AUDIO_REBUFFER_GUESS_SIZE;
        dsp_configure(DSP_RESET, 0);
        current_codec = last_codec;
    }

    /* Get track metadata if we don't already have it. */
    if (!tracks[track_widx].taginfo_ready) 
    {
        if (get_metadata(&tracks[track_widx],current_fd,trackname,v1first)) 
        {
            if (start_play) 
            {
                track_changed = true;
                playlist_update_resume_info(audio_current_track());
            }
        } 
        else 
        {
            logf("mde:%s!",trackname);
            
            /* Set filesize to zero to indicate no file was loaded. */
            tracks[track_widx].filesize = 0;
            tracks[track_widx].filerem = 0;
            close(current_fd);
            current_fd = -1;
            
            /* Skip invalid entry from playlist. */
            playlist_skip_entry(NULL, last_peek_offset);
            tracks[track_widx].taginfo_ready = false;
            goto peek_again;
        }

    }

    /* Load the codec. */
    tracks[track_widx].codecbuf = &filebuf[buf_widx];
    if (!audio_loadcodec(start_play)) 
    {
        /* Set filesize to zero to indicate no file was loaded. */
        tracks[track_widx].filesize = 0;
        tracks[track_widx].filerem = 0;
        close(current_fd);
        current_fd = -1;

        if (tracks[track_widx].codecsize)
        {
            /* No space for codec on buffer, not an error */
            tracks[track_widx].codecsize = 0;
            return false;
        }

        /* This is an error condition, either no codec was found, or reading
         * the codec file failed part way through, either way, skip the track */
        snprintf(msgbuf, sizeof(msgbuf)-1, "No codec for: %s", trackname);
        /* We should not use gui_syncplash from audio thread! */
        gui_syncsplash(HZ*2, true, msgbuf);
        /* Skip invalid entry from playlist. */
        playlist_skip_entry(NULL, last_peek_offset);
        tracks[track_widx].taginfo_ready = false;
        goto peek_again;
    }

    tracks[track_widx].start_pos = 0;
    set_filebuf_watermark(buffer_margin);
    tracks[track_widx].id3.elapsed = 0;

    if (offset > 0) 
    {
        switch (tracks[track_widx].id3.codectype) {
        case AFMT_MPA_L1:
        case AFMT_MPA_L2:
        case AFMT_MPA_L3:
            lseek(current_fd, offset, SEEK_SET);
            tracks[track_widx].id3.offset = offset;
            audio_set_elapsed(&tracks[track_widx].id3);
            tracks[track_widx].filerem = size - offset;
            ci.curpos = offset;
            tracks[track_widx].start_pos = offset;
            break;

        case AFMT_WAVPACK:
            lseek(current_fd, offset, SEEK_SET);
            tracks[track_widx].id3.offset = offset;
            tracks[track_widx].id3.elapsed =
                tracks[track_widx].id3.length / 2;
            tracks[track_widx].filerem = size - offset;
            ci.curpos = offset;
            tracks[track_widx].start_pos = offset;
            break;

        case AFMT_OGG_VORBIS:
        case AFMT_FLAC:
        case AFMT_PCM_WAV:
        case AFMT_A52:
        case AFMT_AAC:
            tracks[track_widx].id3.offset = offset;
            break;
        }
    }
    
    logf("alt:%s", trackname);
    tracks[track_widx].buf_idx = buf_widx;

    return audio_read_file(rebuffer);
}

static bool audio_read_next_metadata(void)
{
    int fd;
    char *trackname;
    int next_idx;
    int status;

    next_idx = track_widx;
    if (tracks[next_idx].taginfo_ready)
    {
        next_idx++;
        next_idx &= MAX_TRACK_MASK;

        if (tracks[next_idx].taginfo_ready)
            return true;
    }

    trackname = playlist_peek(last_peek_offset + 1);
    if (!trackname)
        return false;

    fd = open(trackname, O_RDONLY);
    if (fd < 0)
        return false;

    status = get_metadata(&tracks[next_idx],fd,trackname,v1first);
    /* Preload the glyphs in the tags */
    if (status) 
    {
        if (tracks[next_idx].id3.title)
            lcd_getstringsize(tracks[next_idx].id3.title, NULL, NULL);
        if (tracks[next_idx].id3.artist)
            lcd_getstringsize(tracks[next_idx].id3.artist, NULL, NULL);
        if (tracks[next_idx].id3.album)
            lcd_getstringsize(tracks[next_idx].id3.album, NULL, NULL);
    }
    close(fd);

    return status;
}

/* Send callback events to notify about new tracks. */
static void audio_generate_postbuffer_events(void)
{
    int cur_idx;
    int last_idx = -1;

    logf("Postbuffer:%d/%d",track_ridx,track_widx);

    if (audio_have_tracks()) 
    {
        cur_idx = track_ridx;
        
        while (1) {
            if (!tracks[cur_idx].event_sent) 
            {
                if (last_idx >= 0 && !tracks[last_idx].event_sent)
                {
                    /* Mark the event 'sent' even if we don't really send one */
                    tracks[last_idx].event_sent = true;
                    if (track_buffer_callback)
                        track_buffer_callback(&tracks[last_idx].id3, false);
                }
                last_idx = cur_idx;
            }
            if (cur_idx == track_widx)
                break;
            cur_idx++;
            cur_idx &= MAX_TRACK_MASK;
        }

        if (last_idx >= 0 && !tracks[last_idx].event_sent)
        {
            tracks[last_idx].event_sent = true;
            if (track_buffer_callback)
                track_buffer_callback(&tracks[last_idx].id3, true);
        }
        
        /* Force WPS reload. */
        track_changed = true;
    }
}

static bool audio_initialize_buffer_fill(bool clear_tracks)
{
    /* Don't initialize if we're already initialized */
    if (filling)
        return true;

    logf("Starting buffer fill");

    /* Set the filling flag true before calling audio_clear_tracks as that
     * function can yield and we start looping. */
    filling = true;
    
    if (clear_tracks)
        audio_clear_track_entries(false);

    /* Save the current resume position once. */
    playlist_update_resume_info(audio_current_track());
    
    return true;
}

static void audio_fill_file_buffer(
        bool start_play, bool rebuffer, size_t offset)
{
    bool had_next_track = audio_next_track() != NULL;
    bool continue_buffering;

    /* must reset the buffer before use if trashed */
    if (buffer_state != BUFFER_STATE_NORMAL)
        audio_reset_buffer(pcmbuf_get_bufsize());

    if (!audio_initialize_buffer_fill(!start_play))
        return ;

    /* If we have a partially buffered track, continue loading,
     * otherwise load a new track */
    if (tracks[track_widx].filesize > 0)
        continue_buffering = audio_read_file(rebuffer);
    else 
        continue_buffering = audio_load_track(offset, start_play, rebuffer);

    if (!had_next_track && audio_next_track())
        track_changed = true;

    /* If we're done buffering */
    if (!continue_buffering)
    {
        audio_read_next_metadata();

        audio_generate_postbuffer_events();
        filling = false;
    }
#ifndef SIMULATOR
    ata_sleep();
#endif

}

static void audio_rebuffer(void)
{
    logf("Forcing rebuffer");
    
    /* Notify the codec that this will take a while */
    /* Currently this can cause some problems (logf in reverse order):
     * Codec load error:-1
     * Codec load disk
     * Codec: Unsupported
     * Codec finished
     * New codec:0/3
     * Clearing tracks:7/7, 1
     * Forcing rebuffer
     * Check new track buffer
     * Request new track
     * Clearing tracks:5/5, 0
     * Starting buffer fill
     * Clearing tracks:5/5, 1
     * Re-buffering song w/seek
     */
    //if (!filling)
    //    queue_post(&codec_callback_queue, Q_CODEC_REQUEST_PENDING, 0);
    
    /* Stop in progress fill, and clear open file descriptor */
    if (current_fd >= 0)
    {
        close(current_fd);
        current_fd = -1;
    }
    filling = false;

    /* Reset buffer and track pointers */
    CUR_TI->buf_idx = buf_ridx = buf_widx = 0;
    track_widx = track_ridx;
    audio_clear_track_entries(true);
    CUR_TI->available = 0;

    /* Fill the buffer */
    last_peek_offset = -1;
    CUR_TI->filesize = 0;
    CUR_TI->start_pos = 0;
    ci.curpos = 0;

    if (!CUR_TI->taginfo_ready)
        memset(&CUR_TI->id3, 0, sizeof(struct mp3entry));

    audio_fill_file_buffer(false, true, 0);
}

static void audio_check_new_track(void)
{
    int track_count = audio_track_count();
    int old_track_ridx = track_ridx;
    bool forward;

    if (dir_skip)
    {
        dir_skip = false;
        if (playlist_next_dir(ci.new_track))
        {
            ci.new_track = 0;
            CUR_TI->taginfo_ready = false;
            audio_rebuffer();
            goto skip_done;
        }
        else
        {
            LOGFQUEUE("audio > codec Q_CODEC_REQUEST_FAILED");
            queue_post(&codec_callback_queue, Q_CODEC_REQUEST_FAILED, 0);
            return;
        }
    }

    if (new_playlist)
        ci.new_track = 0;

    /* If the playlist isn't that big */
    if (!playlist_check(ci.new_track))
    {
        if (ci.new_track >= 0)
        {
            LOGFQUEUE("audio > codec Q_CODEC_REQUEST_FAILED");
            queue_post(&codec_callback_queue, Q_CODEC_REQUEST_FAILED, 0);
            return;
        }
        /* Find the beginning backward if the user over-skips it */
        while (!playlist_check(++ci.new_track))
            if (ci.new_track >= 0)
            {
                LOGFQUEUE("audio > codec Q_CODEC_REQUEST_FAILED");
                queue_post(&codec_callback_queue, Q_CODEC_REQUEST_FAILED, 0);
                return;
            }
    }
    /* Update the playlist */
    last_peek_offset -= ci.new_track;

    if (playlist_next(ci.new_track) < 0)
    {
        LOGFQUEUE("audio > codec Q_CODEC_REQUEST_FAILED");
        queue_post(&codec_callback_queue, Q_CODEC_REQUEST_FAILED, 0);
        return;
    }

    if (new_playlist)
    {
        ci.new_track = 1;
        new_playlist = false;
    }

    /* Save the old track */
    prev_ti = CUR_TI;

    /* Move to the new track */
    track_ridx += ci.new_track;
    track_ridx &= MAX_TRACK_MASK;

    if (automatic_skip)
        playlist_end = false;

    track_changed = !automatic_skip;

    /* If it is not safe to even skip this many track entries */
    if (ci.new_track >= track_count || ci.new_track <= track_count - MAX_TRACK)
    {
        ci.new_track = 0;
        CUR_TI->taginfo_ready = false;
        audio_rebuffer();
        goto skip_done;
    }

    forward = ci.new_track > 0;
    ci.new_track = 0;

    /* If the target track is clearly not in memory */
    if (CUR_TI->filesize == 0 || !CUR_TI->taginfo_ready)
    {
        audio_rebuffer();
        goto skip_done;
    }

    /* The track may be in memory, see if it really is */
    if (forward)
    {
        if (!audio_buffer_wind_forward(track_ridx, old_track_ridx))
            audio_rebuffer();
    }
    else
    {
        int cur_idx = track_ridx;
        bool taginfo_ready = true;
        bool wrap = track_ridx > old_track_ridx;
        
        while (1) 
        {
            cur_idx++;
            cur_idx &= MAX_TRACK_MASK;
            if (!(wrap || cur_idx < old_track_ridx))
                break;

            /* If we hit a track in between without valid tag info, bail */
            if (!tracks[cur_idx].taginfo_ready)
            {
                taginfo_ready = false;
                break;
            }

            tracks[cur_idx].available = tracks[cur_idx].filesize;
            if (tracks[cur_idx].codecsize)
                tracks[cur_idx].has_codec = true;
        }
        if (taginfo_ready)
        {
            if (!audio_buffer_wind_backward(track_ridx, old_track_ridx))
                audio_rebuffer();
        }
        else
        {
            CUR_TI->taginfo_ready = false;
            audio_rebuffer();
        }
    }

skip_done:
    audio_update_trackinfo();
    LOGFQUEUE("audio > codec Q_CODEC_REQUEST_COMPLETE");
    queue_post(&codec_callback_queue, Q_CODEC_REQUEST_COMPLETE, 0);
}

static void audio_rebuffer_and_seek(size_t newpos)
{
    size_t real_preseek;
    int fd;
    char *trackname;

    /* (Re-)open current track's file handle. */
    trackname = playlist_peek(0);
    fd = open(trackname, O_RDONLY);
    if (fd < 0) 
    {
        LOGFQUEUE("audio > codec Q_CODEC_REQUEST_FAILED");
        queue_post(&codec_callback_queue, Q_CODEC_REQUEST_FAILED, 0);
        return;
    }
    
    if (current_fd >= 0)
        close(current_fd);
    current_fd = fd;

    playlist_end = false;

    ci.curpos = newpos;

    /* Clear codec buffer. */
    track_widx = track_ridx;
    tracks[track_widx].buf_idx = buf_widx = buf_ridx = 0;

    last_peek_offset = 0;
    filling = false;
    audio_initialize_buffer_fill(true);

    /* This may have been tweaked by the id3v1 code */
    CUR_TI->filesize=filesize(fd);
    if (newpos > conf_preseek)
    {
        CUR_TI->start_pos = newpos - conf_preseek;
        lseek(current_fd, CUR_TI->start_pos, SEEK_SET);
        CUR_TI->filerem = CUR_TI->filesize - CUR_TI->start_pos;
        real_preseek = conf_preseek;
    } 
    else 
    {
        CUR_TI->start_pos = 0;
        CUR_TI->filerem = CUR_TI->filesize;
        real_preseek = newpos;
    }

    CUR_TI->available = 0;

    audio_read_file(real_preseek);

    /* Account for the data we just read that is 'behind' us now */
    CUR_TI->available -= real_preseek;

    buf_ridx = RINGBUF_ADD(buf_ridx, real_preseek);

    LOGFQUEUE("audio > codec Q_CODEC_REQUEST_COMPLETE");
    queue_post(&codec_callback_queue, Q_CODEC_REQUEST_COMPLETE, 0);
}

void audio_set_track_buffer_event(void (*handler)(struct mp3entry *id3,
                                                  bool last_track))
{
    track_buffer_callback = handler;
}

void audio_set_track_unbuffer_event(void (*handler)(struct mp3entry *id3,
                                                    bool last_track))
{
    track_unbuffer_callback = handler;
}

void audio_set_track_changed_event(void (*handler)(struct mp3entry *id3))
{
    track_changed_callback = handler;
}

unsigned long audio_prev_elapsed(void)
{
    return prev_track_elapsed;
}

static void audio_stop_codec_flush(void)
{
    ci.stop_codec = true;
    pcmbuf_pause(true);
    while (audio_codec_loaded)
        yield();
    /* If the audio codec is not loaded any more, and the audio is still
     * playing, it is now and _only_ now safe to call this function from the
     * audio thread */
    if (pcm_is_playing())
        pcmbuf_play_stop();
    pcmbuf_pause(paused);
}

static void audio_stop_playback(void)
{
    /* If we were playing, save resume information */
    if (playing)
    {
        /* Save the current playing spot, or NULL if the playlist has ended */
        playlist_update_resume_info(
            (playlist_end && ci.stop_codec)?NULL:audio_current_track());
    }

    filling = false;
    paused = false;
    audio_stop_codec_flush();
    playing = false;
    
    if (current_fd >= 0) 
    {
        close(current_fd);
        current_fd = -1;
    }

    /* Mark all entries null. */
    audio_clear_track_entries(false);
}

static void audio_play_start(size_t offset)
{
#if defined(HAVE_RECORDING) || defined(CONFIG_TUNER)
    rec_set_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
#endif

    /* Wait for any previously playing audio to flush - TODO: Not necessary? */
    audio_stop_codec_flush();

    track_changed = true;
    playlist_end = false;

    playing = true;
    ci.new_track = 0;
    ci.seek_time = 0;
    wps_offset = 0;
    
    if (current_fd >= 0) 
    {
        close(current_fd);
        current_fd = -1;
    }

    sound_set_volume(global_settings.volume);
    track_widx = track_ridx = 0;
    buf_ridx = buf_widx = 0;

    /* Mark all entries null. */
    memset(tracks, 0, sizeof(struct track_info) * MAX_TRACK);
    
    last_peek_offset = -1;

    audio_fill_file_buffer(true, false, offset);
}


/* Invalidates all but currently playing track. */
static void audio_invalidate_tracks(void)
{
    if (audio_have_tracks()) {
        last_peek_offset = 0;

        playlist_end = false;
        track_widx = track_ridx;
        /* Mark all other entries null (also buffered wrong metadata). */
        audio_clear_track_entries(true);

        /* If the current track is fully buffered, advance the write pointer */
        if (tracks[track_widx].filerem == 0)
            track_widx = (track_widx + 1) & MAX_TRACK_MASK;

        buf_widx = RINGBUF_ADD(buf_ridx, CUR_TI->available);

        audio_read_next_metadata();
    }
}

static void audio_new_playlist(void)
{
    /* Prepare to start a new fill from the beginning of the playlist */
    last_peek_offset = -1;
    if (audio_have_tracks()) {
        playlist_end = false;
        track_widx = track_ridx;
        audio_clear_track_entries(true);

        track_widx++;
        track_widx &= MAX_TRACK_MASK;

        /* Stop reading the current track */
        CUR_TI->filerem = 0;
        close(current_fd);
        current_fd = -1;

        /* Mark the current track as invalid to prevent skipping back to it */
        CUR_TI->taginfo_ready = false;

        /* Invalidate the buffer other than the playing track */
        buf_widx = RINGBUF_ADD(buf_ridx, CUR_TI->available);
    }
    
    /* Signal the codec to initiate a track change forward */
    new_playlist = true;
    ci.new_track = 1;
    audio_fill_file_buffer(false, true, 0);
}

static void audio_initiate_track_change(long direction)
{
    playlist_end = false;
    ci.new_track += direction;
    wps_offset -= direction;
}

static void audio_initiate_dir_change(long direction)
{
    playlist_end = false;
    dir_skip = true;
    ci.new_track = direction;
}

/*
 * Layout audio buffer as follows:
 * [|TALK]|MALLOC|FILE|GUARD|PCM|AUDIOCODEC|[VOICECODEC|]
 */
static void audio_reset_buffer(size_t pcmbufsize)
{
    /* see audio_get_recording_buffer if this is modified */
    size_t offset;

    logf("audio_reset_buffer");
    logf("  size:%08X", pcmbufsize);

    /* Initially set up file buffer as all space available */
    filebuf    = audiobuf + MALLOC_BUFSIZE + talk_get_bufsize();
    filebuflen = audiobufend - filebuf;

    /* Allow for codec(s) at end of audio buffer */
    if (talk_voice_required())
    {
#ifdef PLAYBACK_VOICE
#ifdef IRAM_STEAL
        filebuflen -= CODEC_IRAM_SIZE + 2*CODEC_SIZE;
#else
        filebuflen -= 2*(CODEC_IRAM_SIZE + CODEC_SIZE);
#endif
        /* Allow 2 codecs at end of audio buffer */
        /* If using IRAM for plugins voice IRAM swap buffer must be dedicated
           and out of the way of buffer usage or else a call to audio_get_buffer
           and subsequent buffer use might trash the swap space. A plugin
           initializing IRAM after getting the full buffer would present similar
           problem. Options include: failing the request if the other buffer
           has been obtained already or never allowing use of the voice IRAM
           buffer within the audio buffer. Using buffer_alloc basically
           implements the second in a more convenient way. */
        iram_buf[CODEC_IDX_AUDIO] = filebuf + filebuflen;
        dram_buf[CODEC_IDX_AUDIO] = iram_buf[CODEC_IDX_AUDIO] + CODEC_IRAM_SIZE;

#ifdef IRAM_STEAL
        /* Allocate voice IRAM swap buffer once */
        if (iram_buf[CODEC_IDX_VOICE] == NULL)
            iram_buf[CODEC_IDX_VOICE] = buffer_alloc(CODEC_IRAM_SIZE);
        dram_buf[CODEC_IDX_VOICE] = dram_buf[CODEC_IDX_AUDIO] + CODEC_SIZE;
#else
        iram_buf[CODEC_IDX_VOICE] = dram_buf[CODEC_IDX_AUDIO] + CODEC_SIZE;
        dram_buf[CODEC_IDX_VOICE] = iram_buf[CODEC_IDX_VOICE] + CODEC_IRAM_SIZE;
#endif /* IRAM_STEAL */

#endif /* PLAYBACK_VOICE */
    }
    else
    {
#ifdef PLAYBACK_VOICE
        /* Allow for 1 codec at end of audio buffer */
        filebuflen -= CODEC_IRAM_SIZE + CODEC_SIZE;

        iram_buf[CODEC_IDX_AUDIO] = filebuf + filebuflen;
        dram_buf[CODEC_IDX_AUDIO] = iram_buf[CODEC_IDX_AUDIO] + CODEC_IRAM_SIZE;
        iram_buf[CODEC_IDX_VOICE] = NULL;
        dram_buf[CODEC_IDX_VOICE] = NULL;
#endif
    }

    filebuflen -= pcmbuf_init(pcmbufsize, filebuf + filebuflen) + GUARD_BUFSIZE;

    /* Ensure that file buffer is aligned */
    offset      = -(size_t)filebuf & 3;
    filebuf += offset;
    filebuflen -= offset;
    filebuflen &= ~3;

    /* Clear any references to the file buffer */
    buffer_state = BUFFER_STATE_NORMAL;
}

#ifdef ROCKBOX_HAS_LOGF
static void audio_test_track_changed_event(struct mp3entry *id3)
{
    (void)id3;

    logf("tce:%s", id3->path);
}
#endif

static void audio_playback_init(void)
{
#ifdef PLAYBACK_VOICE
    static bool voicetagtrue = true;
    static struct mp3entry id3_voice;
#endif
    struct event ev;

    logf("playback api init");
    pcm_init();

#ifdef AUDIO_HAVE_RECORDING
    rec_set_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
#endif

#ifdef ROCKBOX_HAS_LOGF
    audio_set_track_changed_event(audio_test_track_changed_event);
#endif

    /* Initialize codec api. */
    ci.read_filebuf = codec_filebuf_callback;
    ci.pcmbuf_insert = codec_pcmbuf_insert_callback;
    ci.pcmbuf_insert_split = codec_pcmbuf_insert_split_callback;
    ci.get_codec_memory = codec_get_memory_callback;
    ci.request_buffer = codec_request_buffer_callback;
    ci.advance_buffer = codec_advance_buffer_callback;
    ci.advance_buffer_loc = codec_advance_buffer_loc_callback;
    ci.request_next_track = codec_request_next_track_callback;
    ci.mp3_get_filepos = codec_mp3_get_filepos_callback;
    ci.seek_buffer = codec_seek_buffer_callback;
    ci.seek_complete = codec_seek_complete_callback;
    ci.set_elapsed = codec_set_elapsed_callback;
    ci.set_offset = codec_set_offset_callback;
    ci.configure = codec_configure_callback;
    ci.discard_codec = codec_discard_codec_callback;

    /* Initialize voice codec api. */
#ifdef PLAYBACK_VOICE
    memcpy(&ci_voice, &ci, sizeof(struct codec_api));
    memset(&id3_voice, 0, sizeof(struct mp3entry));
    ci_voice.read_filebuf = voice_filebuf_callback;
    ci_voice.pcmbuf_insert = voice_pcmbuf_insert_callback;
    ci_voice.pcmbuf_insert_split = voice_pcmbuf_insert_split_callback;
    ci_voice.get_codec_memory = voice_get_memory_callback;
    ci_voice.request_buffer = voice_request_buffer_callback;
    ci_voice.advance_buffer = voice_advance_buffer_callback;
    ci_voice.advance_buffer_loc = voice_advance_buffer_loc_callback;
    ci_voice.request_next_track = voice_request_next_track_callback;
    ci_voice.mp3_get_filepos = voice_mp3_get_filepos_callback;
    ci_voice.seek_buffer = voice_seek_buffer_callback;
    ci_voice.seek_complete = voice_do_nothing;
    ci_voice.set_elapsed = voice_set_elapsed_callback;
    ci_voice.set_offset = voice_set_offset_callback;
    ci_voice.discard_codec = voice_do_nothing;
    ci_voice.taginfo_ready = &voicetagtrue;
    ci_voice.id3 = &id3_voice;
    id3_voice.frequency = 11200;
    id3_voice.length = 1000000L;
#endif

    codec_thread_p = create_thread(codec_thread, codec_stack, 
                                   sizeof(codec_stack),
                                   codec_thread_name IF_PRIO(, PRIORITY_PLAYBACK));

    while (1)
    {
        queue_wait(&audio_queue, &ev);
        if (ev.id == Q_AUDIO_POSTINIT)
            break ;

#ifndef SIMULATOR
        if (ev.id == SYS_USB_CONNECTED)
        {
            logf("USB: Audio preinit");
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            usb_wait_for_disconnect(&audio_queue);
        }
#endif
    }

    /* initialize the buffer */
    filebuf = audiobuf; /* must be non-NULL for audio_set_crossfade */
    buffer_state = BUFFER_STATE_TRASHED; /* force it */
    audio_set_crossfade(global_settings.crossfade);

    audio_is_initialized = true;

    sound_settings_apply();
}
#if MEM > 8
/* we dont want this rebuffering on targets with little ram
   because the disk may never spin down */
bool ata_fillbuffer_callback(void)
{
    queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER_IF_ACTIVE_ATA, 0);
    return true;
}
#endif

static void audio_thread(void)
{
    struct event ev;
#if MEM > 8
    size_t high_watermark;
#endif
    /* At first initialize audio system in background. */
    audio_playback_init();
#if MEM > 8
    high_watermark = (3*filebuflen)/4;
#endif
    
    while (1) 
    {
        if (filling)
        {
            queue_wait_w_tmo(&audio_queue, &ev, 0);
            if (ev.id == SYS_TIMEOUT)
                ev.id = Q_AUDIO_FILL_BUFFER;
        }
#if MEM > 8
        else
        {
            queue_wait_w_tmo(&audio_queue, &ev, HZ/2);
            if ( (ev.id == SYS_TIMEOUT) &&
                (FILEBUFUSED < high_watermark))
                register_ata_idle_func(ata_fillbuffer_callback);
        }
#else
            queue_wait_w_tmo(&audio_queue, &ev, HZ/2);
#endif
        switch (ev.id) {
#if MEM > 8
            case Q_AUDIO_FILL_BUFFER_IF_ACTIVE_ATA:
                 /* only fill if the disk is still spining */
#ifndef SIMULATOR
                 if (!ata_disk_is_active())
                    break;
#endif
#endif /* MEM > 8 */
                  /* else fall through to Q_AUDIO_FILL_BUFFER */
            case Q_AUDIO_FILL_BUFFER:
                LOGFQUEUE("audio < Q_AUDIO_FILL_BUFFER");
                if (!filling)
                    if (!playing || playlist_end || ci.stop_codec)
                        break;
                audio_fill_file_buffer(false, false, 0);
                break;

            case Q_AUDIO_PLAY:
                LOGFQUEUE("audio < Q_AUDIO_PLAY");
                audio_clear_track_entries(false);
                audio_play_start((size_t)ev.data);
                break ;

            case Q_AUDIO_STOP:
                LOGFQUEUE("audio < Q_AUDIO_STOP");
                audio_stop_playback();
                break ;

            case Q_AUDIO_PAUSE:
                LOGFQUEUE("audio < Q_AUDIO_PAUSE");
                pcmbuf_pause((bool)ev.data);
                paused = (bool)ev.data;
                break ;

            case Q_AUDIO_SKIP:
                LOGFQUEUE("audio < Q_AUDIO_SKIP");
                audio_initiate_track_change((long)ev.data);
                break;

            case Q_AUDIO_PRE_FF_REWIND:
                LOGFQUEUE("audio < Q_AUDIO_PRE_FF_REWIND");
                if (!playing)
                    break;
                pcmbuf_pause(true);
                break;

            case Q_AUDIO_FF_REWIND:
                LOGFQUEUE("audio < Q_AUDIO_FF_REWIND");
                if (!playing)
                    break ;
                ci.seek_time = (long)ev.data+1;
                break ;

            case Q_AUDIO_REBUFFER_SEEK:
                LOGFQUEUE("audio < Q_AUDIO_REBUFFER_SEEK");
                audio_rebuffer_and_seek((size_t)ev.data);
                break;

            case Q_AUDIO_CHECK_NEW_TRACK:
                LOGFQUEUE("audio < Q_AUDIO_CHECK_NEW_TRACK");
                audio_check_new_track();
                break;

            case Q_AUDIO_DIR_SKIP:
                LOGFQUEUE("audio < Q_AUDIO_DIR_SKIP");
                playlist_end = false;
                audio_initiate_dir_change((long)ev.data);
                break;

            case Q_AUDIO_NEW_PLAYLIST:
                LOGFQUEUE("audio < Q_AUDIO_NEW_PLAYLIST");
                audio_new_playlist();
                break;

            case Q_AUDIO_FLUSH:
                LOGFQUEUE("audio < Q_AUDIO_FLUSH");
                audio_invalidate_tracks();
                break ;

            case Q_AUDIO_TRACK_CHANGED:
                LOGFQUEUE("audio < Q_AUDIO_TRACK_CHANGED");
                if (track_changed_callback)
                    track_changed_callback(&CUR_TI->id3);
                track_changed = true;
                playlist_update_resume_info(audio_current_track());
                break ;

#ifdef AUDIO_HAVE_RECORDING
            case Q_AUDIO_LOAD_ENCODER:
                LOGFQUEUE("audio < Q_AUDIO_LOAD_ENCODER");
                LOGFQUEUE("audio > codec Q_ENCODER_LOAD_DISK");
                queue_post(&codec_queue, Q_ENCODER_LOAD_DISK, ev.data);
                break;
#endif

#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                LOGFQUEUE("audio < SYS_USB_CONNECTED");
                audio_stop_playback();
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&audio_queue);
                break ;
#endif

            case SYS_TIMEOUT:
                LOGFQUEUE_SYS_TIMEOUT("audio < SYS_TIMEOUT");
                break;

            default:
                LOGFQUEUE("audio < default");
        }
    }
}

