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

/* TODO: Pause should be handled in here, rather than PCMBUF so that voice can
 * play whilst audio is paused */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "system.h"
#include "thread.h"
#include "file.h"
#include "panic.h"
#include "memory.h"
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
#include "buffering.h"
#include "voice_thread.h"
#include "mp3_playback.h"
#include "usb.h"
#include "status.h"
#include "ata.h"
#include "screens.h"
#include "playlist.h"
#include "playback.h"
#include "pcmbuf.h"
#include "buffer.h"
#include "dsp.h"
#include "abrepeat.h"
#include "cuesheet.h"
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#endif
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "peakmeter.h"
#include "action.h"
#include "albumart.h"
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
/* amount of guess-space to allow for codecs that must hunt and peck
 * for their correct seeek target, 32k seems a good size */
#define AUDIO_REBUFFER_GUESS_SIZE    (1024*32)

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


/* Define one constant that includes recording related functionality */
#if defined(HAVE_RECORDING) && !defined(SIMULATOR)
#define AUDIO_HAVE_RECORDING
#endif

enum {
    Q_NULL = 0,
    Q_AUDIO_PLAY = 1,
    Q_AUDIO_STOP,
    Q_AUDIO_PAUSE,
    Q_AUDIO_SKIP,
    Q_AUDIO_PRE_FF_REWIND,
    Q_AUDIO_FF_REWIND,
    Q_AUDIO_CHECK_NEW_TRACK,
    Q_AUDIO_FLUSH,
    Q_AUDIO_TRACK_CHANGED,
    Q_AUDIO_DIR_SKIP,
    Q_AUDIO_POSTINIT,
    Q_AUDIO_FILL_BUFFER,
    Q_CODEC_REQUEST_COMPLETE,
    Q_CODEC_REQUEST_FAILED,

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
#define CODEC_IRAM_ORIGIN   ((unsigned char *)0x4000c000)
#define CODEC_IRAM_SIZE     ((size_t)0xc000)
#elif defined(IAUDIO_X5) || defined(IAUDIO_M5)
#define CODEC_IRAM_ORIGIN   ((unsigned char *)0x10010000)
#define CODEC_IRAM_SIZE     ((size_t)0x10000)
#else
#define CODEC_IRAM_ORIGIN   ((unsigned char *)0x1000c000)
#define CODEC_IRAM_SIZE     ((size_t)0xc000)
#endif

static bool audio_is_initialized = false;
static bool audio_thread_ready NOCACHEBSS_ATTR = false;

/* Variables are commented with the threads that use them: *
 * A=audio, C=codec, V=voice. A suffix of - indicates that *
 * the variable is read but not updated on that thread.    */
/* TBD: Split out "audio" and "playback" (ie. calling) threads */

/* Main state control */
static volatile bool audio_codec_loaded NOCACHEBSS_ATTR = false; /* Codec loaded? (C/A-) */
static volatile bool playing NOCACHEBSS_ATTR = false; /* Is audio playing? (A) */
static volatile bool paused NOCACHEBSS_ATTR = false; /* Is audio paused? (A/C-) */

/* Ring buffer where compressed audio and codecs are loaded */
static unsigned char *filebuf = NULL;       /* Start of buffer (A/C-) */
static unsigned char *malloc_buf = NULL;    /* Start of malloc buffer (A/C-) */
/* FIXME: make filebuflen static */
size_t filebuflen = 0;                      /* Size of buffer (A/C-) */
/* FIXME: make buf_ridx (C/A-) */

/* Possible arrangements of the buffer */
#define BUFFER_STATE_TRASHED        -1          /* trashed; must be reset */
#define BUFFER_STATE_INITIALIZED     0          /* voice+audio OR audio-only */
#define BUFFER_STATE_VOICED_ONLY     1          /* voice-only */
static int buffer_state = BUFFER_STATE_TRASHED; /* Buffer state */

/* Used to keep the WPS up-to-date during track transtition */
static struct mp3entry prevtrack_id3;

/* Used to provide the codec with a pointer */
static struct mp3entry curtrack_id3;

/* Used to make next track info available while playing last track on buffer */
static struct mp3entry lasttrack_id3;

/* Track info structure about songs in the file buffer (A/C-) */
struct track_info {
    int audio_hid;             /* The ID for the track's buffer handle */
    int id3_hid;               /* The ID for the track's metadata handle */
    int codec_hid;             /* The ID for the track's codec handle */
#ifdef HAVE_ALBUMART
    int aa_hid;                /* The ID for the track's album art handle */
#endif

    size_t filesize;           /* File total length */

    bool taginfo_ready;        /* Is metadata read */
};

static struct track_info tracks[MAX_TRACK];
static volatile int track_ridx = 0;  /* Track being decoded (A/C-) */
static int track_widx = 0;           /* Track being buffered (A) */

#define CUR_TI (&tracks[track_ridx]) /* Playing track info pointer (A/C-) */
static struct track_info *prev_ti = NULL;  /* Pointer to the previously played
                                              track */

/* Set by the audio thread when the current track information has updated
 * and the WPS may need to update its cached information */
static bool track_changed = false;

/* Information used only for filling the buffer */
/* Playlist steps from playing track to next track to be buffered (A) */
static int last_peek_offset = 0;

/* Scrobbler support */
static unsigned long prev_track_elapsed = 0; /* Previous track elapsed time (C/A-)*/

/* Track change controls */
static bool automatic_skip = false; /* Who initiated in-progress skip? (C/A-) */
static bool playlist_end = false;   /* Has the current playlist ended? (A) */
static bool auto_dir_skip = false;  /* Have we changed dirs automatically? */
static bool dir_skip = false;       /* Is a directory skip pending? (A) */
static bool new_playlist = false;   /* Are we starting a new playlist? (A) */
static int wps_offset = 0;          /* Pending track change offset, to keep WPS responsive (A) */
static bool skipped_during_pause = false; /* Do we need to clear the PCM buffer when playback resumes (A) */

/* Set to true if the codec thread should send an audio stop request
 * (typically because the end of the playlist has been reached).
 */
static bool codec_requested_stop = false;

static size_t buffer_margin  = 0; /* Buffer margin aka anti-skip buffer (A/C-) */

/* Multiple threads */
/* Set the watermark to trigger buffer fill (A/C) FIXME */
static void set_filebuf_watermark(int seconds, size_t max);

/* Audio thread */
static struct event_queue       audio_queue NOCACHEBSS_ATTR;
static struct queue_sender_list audio_queue_sender_list NOCACHEBSS_ATTR;
static long audio_stack[(DEFAULT_STACK_SIZE + 0x1000)/sizeof(long)];
static const char audio_thread_name[] = "audio";

static void audio_thread(void);
static void audio_initiate_track_change(long direction);
static bool audio_have_tracks(void);
static void audio_reset_buffer(void);

/* Codec thread */
extern struct codec_api ci;
static struct event_queue codec_queue NOCACHEBSS_ATTR;
static struct queue_sender_list codec_queue_sender_list;
static long codec_stack[(DEFAULT_STACK_SIZE + 0x2000)/sizeof(long)]
IBSS_ATTR;
static const char codec_thread_name[] = "codec";
struct thread_entry *codec_thread_p; /* For modifying thread priority later. */

/* PCM buffer messaging */
static struct event_queue pcmbuf_queue NOCACHEBSS_ATTR;

/* Function to be called by pcm buffer callbacks.
 * Permissible Context(s): Audio interrupt
 */
static void pcmbuf_callback_queue_post(long id, intptr_t data)
{
    /* No lock since we're already in audio interrupt context */
    queue_post(&pcmbuf_queue, id, data);
}

/* Scan the pcmbuf queue and return true if a message pulled.
 * Permissible Context(s): Thread
 */
static bool pcmbuf_queue_scan(struct queue_event *ev)
{
    if (!queue_empty(&pcmbuf_queue))
    {
        /* Transfer message to audio queue */
        pcm_play_lock();
        /* Pull message - never, ever any blocking call! */
        queue_wait_w_tmo(&pcmbuf_queue, ev, 0);
        pcm_play_unlock();
        return true;
    }

    return false;
}

/* Clear the pcmbuf queue of messages
 * Permissible Context(s): Thread
 */
static void pcmbuf_queue_clear(void)
{
    pcm_play_lock();
    queue_clear(&pcmbuf_queue);
    pcm_play_unlock();
}

/* --- Helper functions --- */

static struct mp3entry *bufgetid3(int handle_id)
{
    if (handle_id < 0)
        return NULL;

    struct mp3entry *id3;
    ssize_t ret = bufgetdata(handle_id, 0, (void *)&id3);

    if (ret < 0 || ret != sizeof(struct mp3entry))
        return NULL;

    return id3;
}

static bool clear_track_info(struct track_info *track)
{
    /* bufclose returns true if the handle is not found, or if it is closed
     * successfully, so these checks are safe on non-existant handles */
    if (!track)
        return false;

    if (track->codec_hid >= 0) {
        if (bufclose(track->codec_hid))
            track->codec_hid = -1;
        else
            return false;
    }

    if (track->id3_hid >= 0) {
        if (bufclose(track->id3_hid))
            track->id3_hid = -1;
        else
            return false;
    }

    if (track->audio_hid >= 0) {
        if (bufclose(track->audio_hid))
            track->audio_hid = -1;
        else
            return false;
    }

#ifdef HAVE_ALBUMART
    if (track->aa_hid >= 0) {
        if (bufclose(track->aa_hid))
            track->aa_hid = -1;
        else
            return false;
    }
#endif

    track->filesize = 0;
    track->taginfo_ready = false;

    return true;
}

/* --- External interfaces --- */

/* This sends a stop message and the audio thread will dump all it's
   subsequenct messages */
void audio_hard_stop(void)
{
    /* Stop playback */
    LOGFQUEUE("audio >| audio Q_AUDIO_STOP: 1");
    queue_send(&audio_queue, Q_AUDIO_STOP, 1);
#ifdef PLAYBACK_VOICE
    voice_stop();
#endif
}

bool audio_restore_playback(int type)
{
    switch (type)
    {
    case AUDIO_WANT_PLAYBACK:
        if (buffer_state != BUFFER_STATE_INITIALIZED)
            audio_reset_buffer();
        return true;
    case AUDIO_WANT_VOICE:
        if (buffer_state == BUFFER_STATE_TRASHED)
            audio_reset_buffer();
        return true;
    default:
        return false;
    }
}

unsigned char *audio_get_buffer(bool talk_buf, size_t *buffer_size)
{
    unsigned char *buf, *end;

    if (audio_is_initialized)
    {
        audio_hard_stop();
    }
    /* else buffer_state will be BUFFER_STATE_TRASHED at this point */

    if (buffer_size == NULL)
    {
        /* Special case for talk_init to use since it already knows it's
           trashed */
        buffer_state = BUFFER_STATE_TRASHED;
        return NULL;
    }

    if (talk_buf || buffer_state == BUFFER_STATE_TRASHED
           || !talk_voice_required())
    {
        logf("get buffer: talk, audio");
        /* Ok to use everything from audiobuf to audiobufend - voice is loaded,
           the talk buffer is not needed because voice isn't being used, or
           could be BUFFER_STATE_TRASHED already. If state is
           BUFFER_STATE_VOICED_ONLY, no problem as long as memory isn't written
           without the caller knowing what's going on. Changing certain settings
           may move it to a worse condition but the memory in use by something
           else will remain undisturbed.
         */
        if (buffer_state != BUFFER_STATE_TRASHED)
        {
            talk_buffer_steal();
            buffer_state = BUFFER_STATE_TRASHED;
        }

        buf = audiobuf;
        end = audiobufend;
    }
    else
    {
        /* Safe to just return this if already BUFFER_STATE_VOICED_ONLY or
           still BUFFER_STATE_INITIALIZED */
        /* Skip talk buffer and move pcm buffer to end to maximize available
           contiguous memory - no audio running means voice will not need the
           swap space */
        logf("get buffer: audio");
        buf = audiobuf + talk_get_bufsize();
        end = audiobufend - pcmbuf_init(audiobufend);
        buffer_state = BUFFER_STATE_VOICED_ONLY;
    }

    *buffer_size = end - buf;

    return buf;
}

#ifdef HAVE_RECORDING
unsigned char *audio_get_recording_buffer(size_t *buffer_size)
{
    /* Stop audio, voice and obtain all available buffer space */
    audio_hard_stop();
    talk_buffer_steal();

    unsigned char *end = audiobufend;
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

    LOGFQUEUE("codec > Q_ENCODER_LOAD_DISK");
    queue_post(&codec_queue, Q_ENCODER_LOAD_DISK, (intptr_t)enc_fn);

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

    ci.stop_encoder = true;
    while (ci.enc_codec_loaded > 0)
        yield();
#endif
} /* audio_remove_encoder */

#endif /* HAVE_RECORDING */

#ifdef HAVE_ALBUMART
int audio_current_aa_hid(void)
{
    int cur_idx;
    int offset = ci.new_track + wps_offset;

    cur_idx = track_ridx + offset;
    cur_idx &= MAX_TRACK_MASK;

    return tracks[cur_idx].aa_hid;
}
#endif

struct mp3entry* audio_current_track(void)
{
    const char *filename;
    const char *p;
    static struct mp3entry temp_id3;
    int cur_idx;
    int offset = ci.new_track + wps_offset;

    cur_idx = (track_ridx + offset) & MAX_TRACK_MASK;

    if (cur_idx == track_ridx && *curtrack_id3.path)
    {
        /* The usual case */
        return &curtrack_id3;
    }
    else if (offset == -1 && *prevtrack_id3.path)
    {
        /* We're in a track transition. The codec has moved on to the nex track,
           but the audio being played is still the same (now previous) track.
           prevtrack_id3.elapsed is being updated in an ISR by
           codec_pcmbuf_position_callback */
        return &prevtrack_id3;
    }
    else if (tracks[cur_idx].id3_hid >= 0)
    {
        /* Get the ID3 metadata from the main buffer */
        return bufgetid3(tracks[cur_idx].id3_hid);
    }

    /* We didn't find the ID3 metadata, so we fill temp_id3 with the little info
       we have and return that. */

    memset(&temp_id3, 0, sizeof(struct mp3entry));

    filename = playlist_peek(0);
    if (!filename)
        filename = "No file!";

#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
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
    int next_idx;
    int offset = ci.new_track + wps_offset;

    if (!audio_have_tracks())
        return NULL;

    if (wps_offset == -1 && *prevtrack_id3.path)
    {
        /* We're in a track transition. The next track for the WPS is the one
           currently being decoded. */
        return &curtrack_id3;
    }

    next_idx = (track_ridx + offset + 1) & MAX_TRACK_MASK;

    if (next_idx == track_widx)
    {
        /* The next track hasn't been buffered yet, so we return the static
           version of its metadata. */
        return &lasttrack_id3;
    }

    if (tracks[next_idx].id3_hid < 0)
        return NULL;
    else
        return bufgetid3(tracks[next_idx].id3_hid);
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
    talk_force_shutup();
#endif

    /* Start playback */
    LOGFQUEUE("audio >| audio Q_AUDIO_PLAY: %ld", offset);
    /* Don't return until playback has actually started */
    queue_send(&audio_queue, Q_AUDIO_PLAY, offset);
}

void audio_stop(void)
{
    /* Stop playback */
    LOGFQUEUE("audio >| audio Q_AUDIO_STOP");
    /* Don't return until playback has actually stopped */
    queue_send(&audio_queue, Q_AUDIO_STOP, 0);
}

void audio_pause(void)
{
    LOGFQUEUE("audio >| audio Q_AUDIO_PAUSE");
    /* Don't return until playback has actually paused */
    queue_send(&audio_queue, Q_AUDIO_PAUSE, true);
}

void audio_resume(void)
{
    LOGFQUEUE("audio >| audio Q_AUDIO_PAUSE resume");
    /* Don't return until playback has actually resumed */
    queue_send(&audio_queue, Q_AUDIO_PAUSE, false);
}

static void audio_skip(int direction)
{
    if (playlist_check(ci.new_track + wps_offset + direction))
    {
        if (global_settings.beep)
            pcmbuf_beep(5000, 100, 2500*global_settings.beep);

        LOGFQUEUE("audio > audio Q_AUDIO_SKIP %d", direction);
        queue_post(&audio_queue, Q_AUDIO_SKIP, direction);
        /* Update wps while our message travels inside deep playback queues. */
        wps_offset += direction;
        /* Immediately update the playlist index */
        playlist_next(direction);
        track_changed = true;
    }
    else
    {
        /* No more tracks. */
        if (global_settings.beep)
            pcmbuf_beep(1000, 100, 1000*global_settings.beep);
    }
}

void audio_next(void)
{
    audio_skip(1);
}

void audio_prev(void)
{
    audio_skip(-1);
}

void audio_next_dir(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_DIR_SKIP 1");
    queue_post(&audio_queue, Q_AUDIO_DIR_SKIP, 1);
}

void audio_prev_dir(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_DIR_SKIP -1");
    queue_post(&audio_queue, Q_AUDIO_DIR_SKIP, -1);
}

void audio_pre_ff_rewind(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_PRE_FF_REWIND");
    queue_post(&audio_queue, Q_AUDIO_PRE_FF_REWIND, 0);
}

void audio_ff_rewind(long newpos)
{
    LOGFQUEUE("audio > audio Q_AUDIO_FF_REWIND");
    queue_post(&audio_queue, Q_AUDIO_FF_REWIND, newpos);
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

#ifndef HAVE_FLASH_STORAGE
void audio_set_buffer_margin(int setting)
{
    static const int lookup[] = {5, 15, 30, 60, 120, 180, 300, 600};
    buffer_margin = lookup[setting];
    logf("buffer margin: %ld", (long)buffer_margin);
    set_filebuf_watermark(buffer_margin, 0);
}
#endif

/* Take nescessary steps to enable or disable the crossfade setting */
void audio_set_crossfade(int enable)
{
    size_t offset;
    bool was_playing;
    size_t size;

    /* Tell it the next setting to use */
    pcmbuf_crossfade_enable(enable);

    /* Return if size hasn't changed or this is too early to determine
       which in the second case there's no way we could be playing
       anything at all */
    if (pcmbuf_is_same_size())
    {
        /* This function is a copout and just syncs some variables -
           to be removed at a later date */
        pcmbuf_crossfade_enable_finished();
        return;
    }

    offset = 0;
    was_playing = playing;

    /* Playback has to be stopped before changing the buffer size */
    if (was_playing)
    {
        /* Store the track resume position */
        offset = curtrack_id3.offset;
        gui_syncsplash(0, str(LANG_RESTARTING_PLAYBACK));
    }

    /* Blast it - audio buffer will have to be setup again next time
       something plays */
    audio_get_buffer(true, &size);

    /* Restart playback if audio was running previously */
    if (was_playing)
        audio_play(offset);
}

/* --- Routines called from multiple threads --- */

static void set_filebuf_watermark(int seconds, size_t max)
{
    size_t bytes;

    if (!filebuf)
        return;     /* Audio buffers not yet set up */

    bytes = seconds?MAX(curtrack_id3.bitrate * seconds * (1000/8), max):max;
    bytes = MIN(bytes, filebuflen / 2);
    buf_set_watermark(bytes);
}

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
} /* get_codec_filename */

/* --- Codec thread --- */
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

static void* codec_get_memory_callback(size_t *size)
{
    *size = MALLOC_BUFSIZE;
    return malloc_buf;
}

/* Between the codec and PCM track change, we need to keep updating the
   "elapsed" value of the previous (to the codec, but current to the
   user/PCM/WPS) track, so that the progressbar reaches the end.
   During that transition, the WPS will display prevtrack_id3. */
static void codec_pcmbuf_position_callback(size_t size) ICODE_ATTR;
static void codec_pcmbuf_position_callback(size_t size)
{
    /* This is called from an ISR, so be quick */
    unsigned int time = size * 1000 / 4 / NATIVE_FREQUENCY +
        prevtrack_id3.elapsed;

    if (time >= prevtrack_id3.length)
    {
        pcmbuf_set_position_callback(NULL);
        prevtrack_id3.elapsed = prevtrack_id3.length;
    }
    else
        prevtrack_id3.elapsed = time;
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
        curtrack_id3.elapsed = 0;
    else if (value - latency > curtrack_id3.elapsed ||
            value - latency < curtrack_id3.elapsed - 2)
    {
        curtrack_id3.elapsed = value - latency;
    }
}

static void codec_set_offset_callback(size_t value)
{
    unsigned int latency;

    if (ci.seek_time)
        return;

    latency = pcmbuf_get_latency() * curtrack_id3.bitrate / 8;
    if (value < latency)
        curtrack_id3.offset = 0;
    else
        curtrack_id3.offset = value - latency;
}

static void codec_advance_buffer_counters(size_t amount)
{
    bufadvance(CUR_TI->audio_hid, amount);
    ci.curpos += amount;
}

/* copy up-to size bytes into ptr and return the actual size copied */
static size_t codec_filebuf_callback(void *ptr, size_t size)
{
    ssize_t copy_n;

    if (ci.stop_codec || !playing)
        return 0;

    copy_n = bufread(CUR_TI->audio_hid, size, ptr);

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

    if (!playing)
    {
        *realsize = 0;
        return NULL;
    }

    ret = bufgetdata(CUR_TI->audio_hid, reqsize, &ptr);
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
    codec_advance_buffer_counters(amount);
    codec_set_offset_callback(ci.curpos);
}

static void codec_advance_buffer_loc_callback(void *ptr)
{
    size_t amount = buf_get_offset(CUR_TI->audio_hid, ptr);
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

    curtrack_id3.elapsed = newtime;
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
        dsp_configure(ci.dsp, DSP_FLUSH, 0);

        /* If playback was not 'deliberately' paused, unpause now */
        if (!paused)
            pcmbuf_pause(false);
    }
    ci.seek_time = 0;
}

static bool codec_seek_buffer_callback(size_t newpos)
{
    logf("codec_seek_buffer_callback");

    int ret = bufseek(CUR_TI->audio_hid, newpos);
    if (ret == 0) {
        ci.curpos = newpos;
        return true;
    }
    else {
        return false;
    }
}

static void codec_configure_callback(int setting, intptr_t value)
{
    switch (setting) {
    case CODEC_SET_FILEBUF_WATERMARK:
        set_filebuf_watermark(buffer_margin, value);
        break;

    default:
        if (!dsp_configure(ci.dsp, setting, value))
            { logf("Illegal key:%d", setting); }
    }
}

static void codec_track_changed(void)
{
    LOGFQUEUE("codec > audio Q_AUDIO_TRACK_CHANGED");
    queue_post(&audio_queue, Q_AUDIO_TRACK_CHANGED, 0);
}

static void codec_pcmbuf_track_changed_callback(void)
{
    pcmbuf_set_position_callback(NULL);
    pcmbuf_callback_queue_post(Q_AUDIO_TRACK_CHANGED, 0);
}

static void codec_discard_codec_callback(void)
{
    if (CUR_TI->codec_hid >= 0)
    {
        bufclose(CUR_TI->codec_hid);
        CUR_TI->codec_hid = -1;
    }
}

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
    codec_track_changed();
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

    prev_track_elapsed = curtrack_id3.elapsed;

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

static bool codec_request_next_track_callback(void)
{
    int prev_codectype;

    if (ci.stop_codec || !playing)
        return false;

    prev_codectype = get_codec_base_type(curtrack_id3.codectype);

    if (!codec_load_next_track())
        return false;

    /* Check if the next codec is the same file. */
    if (prev_codectype == get_codec_base_type(curtrack_id3.codectype))
    {
        logf("New track loaded");
        codec_discard_codec_callback();
        return true;
    }
    else
    {
        logf("New codec:%d/%d", curtrack_id3.codectype, prev_codectype);
        return false;
    }
}

static void codec_thread(void)
{
    struct queue_event ev;
    int status;

    while (1) {
        status = 0;
        cancel_cpu_boost();
        queue_wait(&codec_queue, &ev);
        codec_requested_stop = false;

        switch (ev.id) {
            case Q_CODEC_LOAD_DISK:
                LOGFQUEUE("codec < Q_CODEC_LOAD_DISK");
                queue_reply(&codec_queue, 1);
                audio_codec_loaded = true;
                ci.stop_codec = false;
                status = codec_load_file((const char *)ev.data, &ci);
                break;

            case Q_CODEC_LOAD:
                LOGFQUEUE("codec < Q_CODEC_LOAD");
                if (CUR_TI->codec_hid < 0) {
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
                status = codec_load_buf(CUR_TI->codec_hid, &ci);
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
                            gui_syncsplash(HZ*2, "Codec failure");
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
                                curtrack_id3.elapsed =
                                    curtrack_id3.length - pcmbuf_get_latency();
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

                    if (CUR_TI->codec_hid >= 0)
                    {
                        LOGFQUEUE("codec > codec Q_CODEC_LOAD");
                        queue_post(&codec_queue, Q_CODEC_LOAD, 0);
                    }
                    else
                    {
                        const char *codec_fn =
                            get_codec_filename(curtrack_id3.codectype);
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
                gui_syncsplash(HZ*2, "Encoder failure");

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

static bool audio_have_tracks(void)
{
    return (audio_track_count() != 0);
}

static int audio_free_track_count(void)
{
    /* Used tracks + free tracks adds up to MAX_TRACK - 1 */
    return MAX_TRACK - 1 - audio_track_count();
}

int audio_track_count(void)
{
    /* Calculate difference from track_ridx to track_widx
     * taking into account a possible wrap-around. */
    return (MAX_TRACK + track_widx - track_ridx) & MAX_TRACK_MASK;
}

long audio_filebufused(void)
{
    return (long) buf_used();
}

/* Update track info after successful a codec track change */
static void audio_update_trackinfo(void)
{
    /* Load the curent track's metadata into curtrack_id3 */
    CUR_TI->taginfo_ready = (CUR_TI->id3_hid >= 0);
    if (CUR_TI->id3_hid >= 0)
        copy_mp3entry(&curtrack_id3, bufgetid3(CUR_TI->id3_hid));

    int next_idx = (track_ridx + 1) & MAX_TRACK_MASK;
    tracks[next_idx].taginfo_ready = (tracks[next_idx].id3_hid >= 0);

    /* Reset current position */
    curtrack_id3.elapsed = 0;
    curtrack_id3.offset = 0;

    /* Update the codec API */
    ci.filesize = CUR_TI->filesize;
    ci.id3 = &curtrack_id3;
    ci.curpos = 0;
    ci.taginfo_ready = &CUR_TI->taginfo_ready;
}

static void buffering_audio_callback(enum callback_event ev, int value)
{
    (void)value;
    logf("buffering_audio_callback");

    switch (ev)
    {
        case EVENT_BUFFER_LOW:
            LOGFQUEUE("buffering > audio Q_AUDIO_FILL_BUFFER");
            queue_remove_from_head(&audio_queue, Q_AUDIO_FILL_BUFFER);
            queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);
            break;

        case EVENT_HANDLE_REBUFFER:
            LOGFQUEUE("audio >| audio Q_AUDIO_FLUSH");
            queue_send(&audio_queue, Q_AUDIO_FLUSH, 0);
            break;

        case EVENT_HANDLE_FINISHED:
            logf("handle %d finished buffering", value);
            strip_tags(value);
            break;

        default:
            break;
    }
}

/* Clear tracks between write and read, non inclusive */
static void audio_clear_track_entries(void)
{
    int cur_idx = track_widx;

    logf("Clearing tracks:%d/%d", track_ridx, track_widx);

    /* Loop over all tracks from write-to-read */
    while (1)
    {
        cur_idx = (cur_idx + 1) & MAX_TRACK_MASK;

        if (cur_idx == track_ridx)
            break;

        clear_track_info(&tracks[cur_idx]);
    }
}

/* Clear all tracks */
static bool audio_release_tracks(void)
{
    int i, cur_idx;

    logf("releasing all tracks");

    for(i = 0; i < MAX_TRACK; i++)
    {
        cur_idx = (track_ridx + i) & MAX_TRACK_MASK;
        if (!clear_track_info(&tracks[cur_idx]))
            return false;
    }

    return true;
}

static bool audio_loadcodec(bool start_play)
{
    int prev_track;
    char codec_path[MAX_PATH]; /* Full path to codec */

    if (tracks[track_widx].id3_hid < 0) {
        return false;
    }

    const char * codec_fn =
        get_codec_filename(bufgetid3(tracks[track_widx].id3_hid)->codectype);
    if (codec_fn == NULL)
        return false;

    tracks[track_widx].codec_hid = -1;

    if (start_play)
    {
        /* Load the codec directly from disk and save some memory. */
        track_ridx = track_widx;
        ci.filesize = CUR_TI->filesize;
        ci.id3 = &curtrack_id3;
        ci.taginfo_ready = &CUR_TI->taginfo_ready;
        ci.curpos = 0;
        LOGFQUEUE("codec > codec Q_CODEC_LOAD_DISK");
        queue_post(&codec_queue, Q_CODEC_LOAD_DISK, (intptr_t)codec_fn);
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
            if (get_codec_base_type(
                        bufgetid3(tracks[track_widx].id3_hid)->codectype) ==
                get_codec_base_type(
                        bufgetid3(tracks[prev_track].id3_hid)->codectype)
                && audio_codec_loaded)
            {
                logf("Reusing prev. codec");
                return true;
            }
        }
    }

    codec_get_full_path(codec_path, codec_fn);

    tracks[track_widx].codec_hid = bufopen(codec_path, 0, TYPE_CODEC);
    if (tracks[track_widx].codec_hid < 0)
        return false;

    logf("Loaded codec");

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

/* Load one track by making the appropriate bufopen calls. Return true if
   everything required was loaded correctly, false if not. */
static bool audio_load_track(int offset, bool start_play)
{
    const char *trackname;
    char msgbuf[80];
    int fd = -1;
    int file_offset = 0;
    struct mp3entry id3;

    /* Stop buffer filling if there is no free track entries.
       Don't fill up the last track entry (we wan't to store next track
       metadata there). */
    if (!audio_free_track_count())
    {
        logf("No free tracks");
        return false;
    }

    last_peek_offset++;
    tracks[track_widx].taginfo_ready = false;

    peek_again:
    logf("Buffering track:%d/%d", track_widx, track_ridx);
    /* Get track name from current playlist read position. */
    while ((trackname = playlist_peek(last_peek_offset)) != NULL)
    {
        /* Handle broken playlists. */
        fd = open(trackname, O_RDONLY);
        if (fd < 0)
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
        memset(&lasttrack_id3, 0, sizeof(struct mp3entry));
        return false;
    }

    tracks[track_widx].filesize = filesize(fd);

    if ((unsigned)offset > tracks[track_widx].filesize)
        offset = 0;

    /* Set default values */
    if (start_play)
    {
        buf_set_watermark(AUDIO_DEFAULT_WATERMARK);
        dsp_configure(ci.dsp, DSP_RESET, 0);
        track_changed = true;
        playlist_update_resume_info(audio_current_track());
    }

    /* Get track metadata if we don't already have it. */
    if (tracks[track_widx].id3_hid < 0)
    {
        if (get_metadata(&id3, fd, trackname))
        {
            send_event(PLAYBACK_EVENT_TRACK_BUFFER, &id3);
            
            tracks[track_widx].id3_hid =
                bufalloc(&id3, sizeof(struct mp3entry), TYPE_ID3);

            if (tracks[track_widx].id3_hid < 0)
            {
                last_peek_offset--;
                close(fd);
                copy_mp3entry(&lasttrack_id3, &id3);
                return false;
            }

            if (track_widx == track_ridx)
                copy_mp3entry(&curtrack_id3, &id3);

            if (start_play)
            {
                track_changed = true;
                playlist_update_resume_info(audio_current_track());
            }
        }
        else
        {
            logf("mde:%s!",trackname);

            /* Skip invalid entry from playlist. */
            playlist_skip_entry(NULL, last_peek_offset);
            close(fd);
            goto peek_again;
        }

    }

    close(fd);

#if 0
    if (cuesheet_is_enabled() && tracks[track_widx].id3.cuesheet_type == 1)
    {
        char cuepath[MAX_PATH];

        struct cuesheet *cue = start_play ? curr_cue : temp_cue;

        if (look_for_cuesheet_file(trackname, cuepath) &&
            parse_cuesheet(cuepath, cue))
        {
            strcpy((cue)->audio_filename, trackname);
            if (start_play)
                cue_spoof_id3(curr_cue, &tracks[track_widx].id3);
        }
    }
#endif

    struct mp3entry *track_id3;

    if (track_widx == track_ridx)
        track_id3 = &curtrack_id3;
    else
        track_id3 = bufgetid3(tracks[track_widx].id3_hid);

#ifdef HAVE_ALBUMART
    if (tracks[track_widx].aa_hid < 0 && gui_sync_wps_uses_albumart())
    {
        char aa_path[MAX_PATH];
        if (find_albumart(track_id3, aa_path, sizeof(aa_path)))
            tracks[track_widx].aa_hid = bufopen(aa_path, 0, TYPE_BITMAP);
    }
#endif

    /* Load the codec. */
    if (!audio_loadcodec(start_play))
    {
        if (tracks[track_widx].codec_hid == ERR_BUFFER_FULL)
        {
            /* No space for codec on buffer, not an error */
            return false;
        }

        /* This is an error condition, either no codec was found, or reading
         * the codec file failed part way through, either way, skip the track */
        snprintf(msgbuf, sizeof(msgbuf)-1, "No codec for: %s", trackname);
        /* We should not use gui_syncplash from audio thread! */
        gui_syncsplash(HZ*2, msgbuf);
        /* Skip invalid entry from playlist. */
        playlist_skip_entry(NULL, last_peek_offset);
        goto peek_again;
    }

    track_id3->elapsed = 0;

    enum data_type type = TYPE_PACKET_AUDIO;

    switch (track_id3->codectype) {
    case AFMT_MPA_L1:
    case AFMT_MPA_L2:
    case AFMT_MPA_L3:
        if (offset > 0) {
            file_offset = offset;
            track_id3->offset = offset;
            audio_set_elapsed(track_id3);
        }
        break;

    case AFMT_WAVPACK:
        if (offset > 0) {
            file_offset = offset;
            track_id3->offset = offset;
            track_id3->elapsed = track_id3->length / 2;
        }
        break;

    case AFMT_OGG_VORBIS:
    case AFMT_SPEEX:
    case AFMT_FLAC:
    case AFMT_PCM_WAV:
    case AFMT_A52:
    case AFMT_AAC:
    case AFMT_MPC:
    case AFMT_APE:
        if (offset > 0)
            track_id3->offset = offset;
        break;

    case AFMT_NSF:
    case AFMT_SPC:
    case AFMT_SID:
        logf("Loading atomic %d",track_id3->codectype);
        type = TYPE_ATOMIC_AUDIO;
        break;
    }

    logf("alt:%s", trackname);

    if (file_offset > AUDIO_REBUFFER_GUESS_SIZE)
        file_offset -= AUDIO_REBUFFER_GUESS_SIZE;
    else if (track_id3->first_frame_offset)
        file_offset = track_id3->first_frame_offset;
    else
        file_offset = 0;

    tracks[track_widx].audio_hid = bufopen(trackname, file_offset, type);

    if (tracks[track_widx].audio_hid < 0)
        return false;

    /* All required data is now available for the codec. */
    tracks[track_widx].taginfo_ready = true;

    if (start_play)
    {
        ci.curpos=file_offset;
        buf_request_buffer_handle(tracks[track_widx].audio_hid);
    }

    track_widx = (track_widx + 1) & MAX_TRACK_MASK;

    return true;
}

static void audio_fill_file_buffer(bool start_play, size_t offset)
{
    struct queue_event ev;
    bool had_next_track = audio_next_track() != NULL;
    bool continue_buffering;

    /* No need to rebuffer if there are track skips pending. */
    if (ci.new_track != 0)
        return;

    /* Must reset the buffer before use if trashed or voice only - voice
       file size shouldn't have changed so we can go straight from
       BUFFER_STATE_VOICED_ONLY to BUFFER_STATE_INITIALIZED */
    if (buffer_state != BUFFER_STATE_INITIALIZED)
        audio_reset_buffer();

    logf("Starting buffer fill");

    if (!start_play)
        audio_clear_track_entries();

    /* Save the current resume position once. */
    playlist_update_resume_info(audio_current_track());

    do {
        continue_buffering = audio_load_track(offset, start_play);
        start_play = false;
        offset = 0;
        sleep(1);
        if (queue_peek(&audio_queue, &ev)) {
            if (ev.id != Q_AUDIO_FILL_BUFFER)
            {
                /* There's a message in the queue. break the loop to treat it,
                and go back to filling after that. */
                LOGFQUEUE("buffering > audio Q_AUDIO_FILL_BUFFER");
                queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);
            }
            break;
        }
    } while (continue_buffering);

    if (!had_next_track && audio_next_track())
        track_changed = true;

}

static void audio_rebuffer(void)
{
    logf("Forcing rebuffer");

    clear_track_info(CUR_TI);

    /* Reset track pointers */
    track_widx = track_ridx;
    audio_clear_track_entries();

    /* Fill the buffer */
    last_peek_offset = -1;
    ci.curpos = 0;

    if (!CUR_TI->taginfo_ready)
        memset(&curtrack_id3, 0, sizeof(struct mp3entry));

    audio_fill_file_buffer(false, 0);
}

/* Called on request from the codec to get a new track. This is the codec part
   of the track transition. */
static int audio_check_new_track(void)
{
    int track_count = audio_track_count();
    int old_track_ridx = track_ridx;
    int i, idx;
    int next_playlist_index;
    bool forward;
    bool end_of_playlist;  /* Temporary flag, not the same as playlist_end */

    /* Now it's good time to send track unbuffer events. */
    send_event(PLAYBACK_EVENT_TRACK_FINISH, &curtrack_id3);
    
    if (dir_skip)
    {
        dir_skip = false;
        if (playlist_next_dir(ci.new_track))
        {
            ci.new_track = 0;
            audio_rebuffer();
            goto skip_done;
        }
        else
        {
            LOGFQUEUE("audio >|= codec Q_CODEC_REQUEST_FAILED");
            return Q_CODEC_REQUEST_FAILED;
        }
    }

    if (new_playlist)
        ci.new_track = 0;

    end_of_playlist = playlist_peek(automatic_skip ? ci.new_track : 0) == NULL;
    auto_dir_skip = end_of_playlist && global_settings.next_folder;

    /* If the playlist isn't that big */
    if (automatic_skip && !playlist_check(ci.new_track))
    {
        if (ci.new_track >= 0)
        {
            LOGFQUEUE("audio >|= codec Q_CODEC_REQUEST_FAILED");
            return Q_CODEC_REQUEST_FAILED;
        }
        /* Find the beginning backward if the user over-skips it */
        while (!playlist_check(++ci.new_track))
            if (ci.new_track >= 0)
            {
                LOGFQUEUE("audio >|= codec Q_CODEC_REQUEST_FAILED");
                return Q_CODEC_REQUEST_FAILED;
            }
    }

    /* Update the playlist */
    last_peek_offset -= ci.new_track;

    if (auto_dir_skip)
    {
        /* If the track change was the result of an auto dir skip,
           we need to update the playlist now */
        next_playlist_index = playlist_next(ci.new_track);

        if (next_playlist_index < 0)
        {
            LOGFQUEUE("audio >|= codec Q_CODEC_REQUEST_FAILED");
            return Q_CODEC_REQUEST_FAILED;
        }
    }

    if (new_playlist)
    {
        ci.new_track = 1;
        new_playlist = false;
    }

    /* Save the track metadata to allow the WPS to display it
       while PCM finishes playing that track */
    copy_mp3entry(&prevtrack_id3, &curtrack_id3);

    /* Update the main buffer copy of the track metadata with the one
       the codec has been using (for the unbuffer callbacks) */
    if (CUR_TI->id3_hid >= 0)
        copy_mp3entry(bufgetid3(CUR_TI->id3_hid), &curtrack_id3);

    /* Save a pointer to the old track to allow later clearing */
    prev_ti = CUR_TI;

    for (i = 0; i < ci.new_track; i++)
    {
        idx = (track_ridx + i) & MAX_TRACK_MASK;
        struct mp3entry *id3 = bufgetid3(tracks[idx].id3_hid);
        ssize_t offset = buf_handle_offset(tracks[idx].audio_hid);
        if (!id3 || offset < 0 || (unsigned)offset > id3->first_frame_offset)
        {
            /* We don't have all the audio data for that track, so clear it,
               but keep the metadata. */
            if (tracks[idx].audio_hid >= 0 && bufclose(tracks[idx].audio_hid))
            {
                tracks[idx].audio_hid = -1;
                tracks[idx].filesize = 0;
            }
        }
    }

    /* Move to the new track */
    track_ridx = (track_ridx + ci.new_track) & MAX_TRACK_MASK;

    buf_set_base_handle(CUR_TI->audio_hid);

    if (automatic_skip)
    {
        playlist_end = false;
        wps_offset = -ci.new_track;
        track_changed = true;
    }

    /* If it is not safe to even skip this many track entries */
    if (ci.new_track >= track_count || ci.new_track <= track_count - MAX_TRACK)
    {
        ci.new_track = 0;
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

    /* When skipping backwards, it is possible that we've found a track that's
     * buffered, but which is around the track-wrap and therefor not the track
     * we are looking for */
    if (!forward)
    {
        int cur_idx = track_ridx;
        bool taginfo_ready = true;
        /* We've wrapped the buffer backwards if new > old */
        bool wrap = track_ridx > old_track_ridx;

        while (1)
        {
            cur_idx = (cur_idx + 1) & MAX_TRACK_MASK;

            /* if we've advanced past the wrap when cur_idx is zeroed */
            if (!cur_idx)
                wrap = false;

            /* if we aren't still on the wrap and we've caught the old track */
            if (!(wrap || cur_idx < old_track_ridx))
                break;

            /* If we hit a track in between without valid tag info, bail */
            if (!tracks[cur_idx].taginfo_ready)
            {
                taginfo_ready = false;
                break;
            }
        }
        if (!taginfo_ready)
        {
            audio_rebuffer();
        }
    }

skip_done:
    audio_update_trackinfo();
    LOGFQUEUE("audio >|= codec Q_CODEC_REQUEST_COMPLETE");
    return Q_CODEC_REQUEST_COMPLETE;
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
    {
        pcmbuf_play_stop();
        pcmbuf_queue_clear();
    }
    pcmbuf_pause(paused);
}

static void audio_stop_playback(void)
{
    /* If we were playing, save resume information */
    if (playing)
    {
        struct mp3entry *id3 = NULL;

        if (!playlist_end || !ci.stop_codec)
        {
            /* Set this early, the outside code yields and may allow the codec
               to try to wait for a reply on a buffer wait */
            ci.stop_codec = true;
            id3 = audio_current_track();
        }

        /* Save the current playing spot, or NULL if the playlist has ended */
        playlist_update_resume_info(id3);

        /* TODO: Create auto bookmark too? */

        prev_track_elapsed = curtrack_id3.elapsed;
    }

    paused = false;
    audio_stop_codec_flush();
    playing = false;

    /* Mark all entries null. */
    audio_clear_track_entries();

    /* Close all tracks */
    audio_release_tracks();

    unregister_buffering_callback(buffering_audio_callback);

    memset(&curtrack_id3, 0, sizeof(struct mp3entry));
}

static void audio_play_start(size_t offset)
{
    int i;

#if INPUT_SRC_CAPS != 0
    audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif

    /* Wait for any previously playing audio to flush - TODO: Not necessary? */
    paused = false;
    audio_stop_codec_flush();

    track_changed = true;
    playlist_end = false;

    playing = true;

    ci.new_track = 0;
    ci.seek_time = 0;
    wps_offset = 0;

    sound_set_volume(global_settings.volume);
    track_widx = track_ridx = 0;

    /* Clear all track entries. */
    for (i = 0; i < MAX_TRACK; i++) {
        clear_track_info(&tracks[i]);
    }

    last_peek_offset = -1;

    /* Officially playing */
    queue_reply(&audio_queue, 1);

#ifndef HAVE_FLASH_STORAGE
    set_filebuf_watermark(buffer_margin, 0);
#endif
    audio_fill_file_buffer(true, offset);
    register_buffering_callback(buffering_audio_callback);

    LOGFQUEUE("audio > audio Q_AUDIO_TRACK_CHANGED");
    queue_post(&audio_queue, Q_AUDIO_TRACK_CHANGED, 0);
}


/* Invalidates all but currently playing track. */
static void audio_invalidate_tracks(void)
{
    if (audio_have_tracks())
    {
        last_peek_offset = 0;
        playlist_end = false;
        track_widx = track_ridx;

        /* Mark all other entries null (also buffered wrong metadata). */
        audio_clear_track_entries();

        track_widx = (track_widx + 1) & MAX_TRACK_MASK;

        audio_fill_file_buffer(false, 0);
    }
}

static void audio_new_playlist(void)
{
    /* Prepare to start a new fill from the beginning of the playlist */
    last_peek_offset = -1;
    if (audio_have_tracks())
    {
        if (paused)
            skipped_during_pause = true;
        playlist_end = false;
        track_widx = track_ridx;
        audio_clear_track_entries();

        track_widx = (track_widx + 1) & MAX_TRACK_MASK;

        /* Mark the current track as invalid to prevent skipping back to it */
        CUR_TI->taginfo_ready = false;
    }

    /* Signal the codec to initiate a track change forward */
    new_playlist = true;
    ci.new_track = 1;

    /* Officially playing */
    queue_reply(&audio_queue, 1);

    audio_fill_file_buffer(false, 0);
}

/* Called on manual track skip */
static void audio_initiate_track_change(long direction)
{
    logf("audio_initiate_track_change(%ld)", direction);

    playlist_end = false;
    ci.new_track += direction;
    wps_offset -= direction;
    if (paused)
        skipped_during_pause = true;
}

/* Called on manual dir skip */
static void audio_initiate_dir_change(long direction)
{
    playlist_end = false;
    dir_skip = true;
    ci.new_track = direction;
    if (paused)
        skipped_during_pause = true;
}

/* Called when PCM track change is complete */
static void audio_finalise_track_change(void)
{
    logf("audio_finalise_track_change");

    if (automatic_skip)
    {
        if (!auto_dir_skip)
            playlist_next(-wps_offset);

        wps_offset = 0;
        automatic_skip = false;
    }

    auto_dir_skip = false;

    /* Invalidate prevtrack_id3 */
    prevtrack_id3.path[0] = 0;

    if (prev_ti && prev_ti->audio_hid < 0)
    {
        /* No audio left so we clear all the track info. */
        clear_track_info(prev_ti);
    }

    if (prev_ti && prev_ti->id3_hid >= 0)
    {
        /* Reset the elapsed time to force the progressbar to be empty if
           the user skips back to this track */
        bufgetid3(prev_ti->id3_hid)->elapsed = 0;
    }

    send_event(PLAYBACK_EVENT_TRACK_CHANGE, &curtrack_id3);

    track_changed = true;
    playlist_update_resume_info(audio_current_track());
}

/*
 * Layout audio buffer as follows - iram buffer depends on target:
 * [|SWAP:iram][|TALK]|MALLOC|FILE|GUARD|PCM|[SWAP:dram[|iram]|]
 */
static void audio_reset_buffer(void)
{
    /* see audio_get_recording_buffer if this is modified */
    logf("audio_reset_buffer");

    /* If the setup of anything allocated before the file buffer is
       changed, do check the adjustments after the buffer_alloc call
       as it will likely be affected and need sliding over */

    /* Initially set up file buffer as all space available */
    malloc_buf = audiobuf + talk_get_bufsize();
    /* Align the malloc buf to line size. Especially important to cf
       targets that do line reads/writes. */
    malloc_buf = (unsigned char *)(((uintptr_t)malloc_buf + 15) & ~15);
    filebuf    = malloc_buf + MALLOC_BUFSIZE; /* filebuf line align implied */
    filebuflen = audiobufend - filebuf;

    filebuflen &= ~15;

    /* Subtract whatever the pcm buffer says it used plus the guard buffer */
    filebuflen -= pcmbuf_init(filebuf + filebuflen) + GUARD_BUFSIZE;

    /* Make sure filebuflen is a longword multiple after adjustment - filebuf
       will already be line aligned */
    filebuflen &= ~3;

    buffering_reset(filebuf, filebuflen);

    /* Clear any references to the file buffer */
    buffer_state = BUFFER_STATE_INITIALIZED;

#if defined(ROCKBOX_HAS_LOGF) && defined(LOGF_ENABLE)
    /* Make sure everything adds up - yes, some info is a bit redundant but
       aids viewing and the sumation of certain variables should add up to
       the location of others. */
    {
        size_t pcmbufsize;
        const unsigned char *pcmbuf = pcmbuf_get_meminfo(&pcmbufsize);
        logf("mabuf:  %08X", (unsigned)malloc_buf);
        logf("mabufe: %08X", (unsigned)(malloc_buf + MALLOC_BUFSIZE));
        logf("fbuf:   %08X", (unsigned)filebuf);
        logf("fbufe:  %08X", (unsigned)(filebuf + filebuflen));
        logf("gbuf:   %08X", (unsigned)(filebuf + filebuflen));
        logf("gbufe:  %08X", (unsigned)(filebuf + filebuflen + GUARD_BUFSIZE));
        logf("pcmb:   %08X", (unsigned)pcmbuf);
        logf("pcmbe:  %08X", (unsigned)(pcmbuf + pcmbufsize));
    }
#endif
}

static void audio_thread(void)
{
    struct queue_event ev;

    pcm_postinit();

    audio_thread_ready = true;

    while (1)
    {
        cancel_cpu_boost();
        if (!pcmbuf_queue_scan(&ev))
            queue_wait_w_tmo(&audio_queue, &ev, HZ/2);

        switch (ev.id) {
            case Q_AUDIO_FILL_BUFFER:
                LOGFQUEUE("audio < Q_AUDIO_FILL_BUFFER");
                if (!playing || playlist_end || ci.stop_codec)
                    break;
                audio_fill_file_buffer(false, 0);
                break;

            case Q_AUDIO_PLAY:
                LOGFQUEUE("audio < Q_AUDIO_PLAY");
                if (playing && ev.data <= 0)
                    audio_new_playlist();
                else
                {
                    audio_stop_playback();
                    audio_play_start((size_t)ev.data);
                }
                break;

            case Q_AUDIO_STOP:
                LOGFQUEUE("audio < Q_AUDIO_STOP");
                if (playing)
                    audio_stop_playback();
                if (ev.data != 0)
                    queue_clear(&audio_queue);
                break;

            case Q_AUDIO_PAUSE:
                LOGFQUEUE("audio < Q_AUDIO_PAUSE");
                if (!(bool) ev.data && skipped_during_pause && !pcmbuf_is_crossfade_active())
                    pcmbuf_play_stop(); /* Flush old track on resume after skip */
                skipped_during_pause = false;
                if (!playing)
                    break;
                pcmbuf_pause((bool)ev.data);
                paused = (bool)ev.data;
                break;

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
                    break;
                if (automatic_skip)
                {
                    /* An automatic track skip is in progress. Finalize it,
                       then go back to the previous track */
                    audio_finalise_track_change();
                    ci.new_track = -1;
                }
                ci.seek_time = (long)ev.data+1;
                break;

            case Q_AUDIO_CHECK_NEW_TRACK:
                LOGFQUEUE("audio < Q_AUDIO_CHECK_NEW_TRACK");
                queue_reply(&audio_queue, audio_check_new_track());
                break;

            case Q_AUDIO_DIR_SKIP:
                LOGFQUEUE("audio < Q_AUDIO_DIR_SKIP");
                playlist_end = false;
                audio_initiate_dir_change(ev.data);
                break;

            case Q_AUDIO_FLUSH:
                LOGFQUEUE("audio < Q_AUDIO_FLUSH");
                audio_invalidate_tracks();
                break;

            case Q_AUDIO_TRACK_CHANGED:
                /* PCM track change done */
                LOGFQUEUE("audio < Q_AUDIO_TRACK_CHANGED");
                audio_finalise_track_change();
                break;

#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                LOGFQUEUE("audio < SYS_USB_CONNECTED");
                if (playing)
                    audio_stop_playback();
#ifdef PLAYBACK_VOICE
                voice_stop();
#endif
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&audio_queue);

                /* Mark all entries null. */
                audio_clear_track_entries();

                /* release tracks to make sure all handles are closed */
                audio_release_tracks();
                break;
#endif

            case SYS_TIMEOUT:
                LOGFQUEUE_SYS_TIMEOUT("audio < SYS_TIMEOUT");
                break;

            default:
                LOGFQUEUE("audio < default");
                break;
        } /* end switch */
    } /* end while */
}

/* Initialize the audio system - called from init() in main.c.
 * Last function because of all the references to internal symbols
 */
void audio_init(void)
{
    struct thread_entry *audio_thread_p;

    /* Can never do this twice */
    if (audio_is_initialized)
    {
        logf("audio: already initialized");
        return;
    }

    logf("audio: initializing");

    /* Initialize queues before giving control elsewhere in case it likes
       to send messages. Thread creation will be delayed however so nothing
       starts running until ready if something yields such as talk_init. */
    queue_init(&audio_queue, true);
    queue_init(&codec_queue, false);
    queue_init(&pcmbuf_queue, false);

    pcm_init();

     /* Initialize codec api. */
    ci.read_filebuf        = codec_filebuf_callback;
    ci.pcmbuf_insert       = codec_pcmbuf_insert_callback;
    ci.get_codec_memory    = codec_get_memory_callback;
    ci.request_buffer      = codec_request_buffer_callback;
    ci.advance_buffer      = codec_advance_buffer_callback;
    ci.advance_buffer_loc  = codec_advance_buffer_loc_callback;
    ci.request_next_track  = codec_request_next_track_callback;
    ci.mp3_get_filepos     = codec_mp3_get_filepos_callback;
    ci.seek_buffer         = codec_seek_buffer_callback;
    ci.seek_complete       = codec_seek_complete_callback;
    ci.set_elapsed         = codec_set_elapsed_callback;
    ci.set_offset          = codec_set_offset_callback;
    ci.configure           = codec_configure_callback;
    ci.discard_codec       = codec_discard_codec_callback;
    ci.dsp                 = (struct dsp_config *)dsp_configure(NULL, DSP_MYDSP,
                                                                CODEC_IDX_AUDIO);

    /* initialize the buffer */
    filebuf = audiobuf;

    /* audio_reset_buffer must to know the size of voice buffer so init
       talk first */
    talk_init();

    codec_thread_p = create_thread(
            codec_thread, codec_stack, sizeof(codec_stack),
            CREATE_THREAD_FROZEN,
            codec_thread_name IF_PRIO(, PRIORITY_PLAYBACK)
            IF_COP(, CPU));

    queue_enable_queue_send(&codec_queue, &codec_queue_sender_list,
                            codec_thread_p);

    audio_thread_p = create_thread(audio_thread, audio_stack,
                  sizeof(audio_stack), CREATE_THREAD_FROZEN,
                  audio_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE)
                  IF_COP(, CPU));

    queue_enable_queue_send(&audio_queue, &audio_queue_sender_list,
                            audio_thread_p);

#ifdef PLAYBACK_VOICE
    voice_thread_init();
#endif

    /* Set crossfade setting for next buffer init which should be about... */
    pcmbuf_crossfade_enable(global_settings.crossfade);

    /* initialize the buffering system */

    buffering_init();
    /* ...now! Set up the buffers */
    audio_reset_buffer();

    int i;
    for(i = 0; i < MAX_TRACK; i++)
    {
        tracks[i].audio_hid = -1;
        tracks[i].id3_hid = -1;
        tracks[i].codec_hid = -1;
#ifdef HAVE_ALBUMART
        tracks[i].aa_hid = -1;
#endif
    }

    /* Probably safe to say */
    audio_is_initialized = true;

    sound_settings_apply();
#ifndef HAVE_FLASH_STORAGE
    audio_set_buffer_margin(global_settings.buffer_margin);
#endif

    /* it's safe to let the threads run now */
#ifdef PLAYBACK_VOICE
    voice_thread_resume();
#endif
    thread_thaw(codec_thread_p);
    thread_thaw(audio_thread_p);

} /* audio_init */

void audio_wait_for_init(void)
{
    /* audio thread will only set this once after it finished the final
     * audio hardware init so this little construct is safe - even
     * cross-core. */
    while (!audio_thread_ready)
    {
        sleep(0);
    }
}
