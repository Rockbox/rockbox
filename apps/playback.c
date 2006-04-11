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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "system.h"
#include "thread.h"
#include "file.h"
#include "lcd.h"
#include "font.h"
#include "backlight.h"
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
#include "pcm_playback.h"
#include "pcm_record.h"
#include "buffer.h"
#include "dsp.h"
#include "abrepeat.h"
#include "tagcache.h"
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
#include "talk.h"
#ifdef CONFIG_TUNER
#include "radio.h"
#endif
#include "splash.h"

static volatile bool audio_codec_loaded;
static volatile bool voice_codec_loaded;
static volatile bool playing;
static volatile bool paused;

#define CODEC_VORBIS   "/.rockbox/codecs/vorbis.codec"
#define CODEC_MPA_L3   "/.rockbox/codecs/mpa.codec"
#define CODEC_FLAC     "/.rockbox/codecs/flac.codec"
#define CODEC_WAV      "/.rockbox/codecs/wav.codec"
#define CODEC_A52      "/.rockbox/codecs/a52.codec"
#define CODEC_MPC      "/.rockbox/codecs/mpc.codec"
#define CODEC_WAVPACK  "/.rockbox/codecs/wavpack.codec"
#define CODEC_ALAC     "/.rockbox/codecs/alac.codec"
#define CODEC_AAC      "/.rockbox/codecs/aac.codec"
#define CODEC_SHN      "/.rockbox/codecs/shorten.codec"
#define CODEC_AIFF     "/.rockbox/codecs/aiff.codec"

/* default point to start buffer refill */
#define AUDIO_DEFAULT_WATERMARK      (1024*512)
/* amount of data to read in one read() call */
#define AUDIO_DEFAULT_FILECHUNK      (1024*32)
/* point at which the file buffer will fight for CPU time */
#define AUDIO_FILEBUF_CRITICAL       (1024*128)
/* amount of guess-space to allow for codecs that must hunt and peck
 * for their correct seeek target, 32k seems a good size */
#define AUDIO_REBUFFER_GUESS_SIZE    (1024*32)

enum {
    Q_AUDIO_PLAY = 1,
    Q_AUDIO_STOP,
    Q_AUDIO_PAUSE,
    Q_AUDIO_SKIP,
    Q_AUDIO_PRE_FF_REWIND,
    Q_AUDIO_FF_REWIND,
    Q_AUDIO_REBUFFER_SEEK,
    Q_AUDIO_CHECK_NEW_TRACK,
    Q_AUDIO_CODEC_DONE,
    Q_AUDIO_FLUSH,
    Q_AUDIO_TRACK_CHANGED,
    Q_AUDIO_DIR_SKIP,
    Q_AUDIO_POSTINIT,
    Q_AUDIO_FILL_BUFFER,

    Q_CODEC_LOAD,
    Q_CODEC_LOAD_DISK,
};

/* As defined in plugins/lib/xxx2wav.h */
#define MALLOC_BUFSIZE (512*1024)
#define GUARD_BUFSIZE  (32*1024)

/* As defined in plugin.lds */
#if CONFIG_CPU == PP5020 || CONFIG_CPU == PP5002
#define CODEC_IRAM_ORIGIN   0x4000c000
#else
#define CODEC_IRAM_ORIGIN   0x1000c000
#endif
#define CODEC_IRAM_SIZE     0xc000

extern bool audio_is_initialized;

/* Buffer control thread. */
static struct event_queue audio_queue;
static long audio_stack[(DEFAULT_STACK_SIZE + 0x1000)/sizeof(long)];
static const char audio_thread_name[] = "audio";

/* Codec thread. */
static struct event_queue codec_queue;
static long codec_stack[(DEFAULT_STACK_SIZE + 0x2000)/sizeof(long)]
IBSS_ATTR;
static const char codec_thread_name[] = "codec";

/* Voice codec thread. */
static struct event_queue voice_codec_queue;
static long voice_codec_stack[(DEFAULT_STACK_SIZE + 0x2000)/sizeof(long)]
IBSS_ATTR;
static const char voice_codec_thread_name[] = "voice codec";

static struct mutex mutex_codecthread;
static struct mutex mutex_interthread;

static struct mp3entry id3_voice;

static char *voicebuf;
static size_t voice_remaining;
static bool voice_is_playing;
static void (*voice_getmore)(unsigned char** start, int* size);

/* Is file buffer currently being refilled? */
static volatile bool filling;
static volatile bool filling_short;

volatile int current_codec;
extern unsigned char codecbuf[];

/* Ring buffer where tracks and codecs are loaded. */
static char *filebuf;

/* Total size of the ring buffer. */
size_t filebuflen;

/* Bytes available in the buffer. */
size_t filebufused;

/* Ring buffer read and write indexes. */
static volatile size_t buf_ridx;
static volatile size_t buf_widx;

#ifndef SIMULATOR
static unsigned char *iram_buf[2];
#endif
static unsigned char *dram_buf[2];

/* Step count to the next unbuffered track. */
static int last_peek_offset;

/* Track information (count in file buffer, read/write indexes for
   track ring structure. */
static volatile int track_ridx;
static volatile int track_widx;
static bool track_changed;

/* Partially loaded song's file handle to continue buffering later. */
static int current_fd;

/* Information about how many bytes left on the buffer re-fill run. */
static size_t fill_bytesleft;

/* Track info structure about songs in the file buffer. */
static struct track_info tracks[MAX_TRACK];

/* Pointer to track info structure about current song playing. */
static struct track_info *cur_ti;
static struct track_info *prev_ti;

/* Have we reached end of the current playlist. */
static bool playlist_end = false;

/* Codec API including function callbacks. */
extern struct codec_api ci;
extern struct codec_api ci_voice;

/* When we change a song and buffer is not in filling state, this
   variable keeps information about whether to go a next/previous track. */
static int new_track;

/* Callback function to call when current track has really changed. */
void (*track_changed_callback)(struct mp3entry *id3);
void (*track_buffer_callback)(struct mp3entry *id3, bool last_track);
void (*track_unbuffer_callback)(struct mp3entry *id3, bool last_track);

static void playback_init(void);

/* Configuration */
static size_t conf_watermark;
static size_t conf_filechunk;
static size_t buffer_margin;

static bool v1first = false;

static void mp3_set_elapsed(struct mp3entry* id3);
int mp3_get_file_pos(void);

static void audio_clear_track_entries(bool clear_unbuffered);
static void initialize_buffer_fill(bool start_play, bool short_fill);
static void audio_fill_file_buffer(
        bool start_play, bool short_fill, size_t offset);

static void swap_codec(void)
{
    int my_codec = current_codec;

    logf("swapping out codec:%d", current_codec);

    /* Save our current IRAM and DRAM */
#ifndef SIMULATOR
    memcpy(iram_buf[my_codec], (unsigned char *)CODEC_IRAM_ORIGIN,
            CODEC_IRAM_SIZE);
#endif
    memcpy(dram_buf[my_codec], codecbuf, CODEC_SIZE);

    do {
        /* Release my semaphore and force a task switch. */
        mutex_unlock(&mutex_codecthread);
        yield();
        mutex_lock(&mutex_codecthread);
    /* Loop until the other codec has locked and run */
    } while (my_codec == current_codec);
    current_codec = my_codec;

    /* Reload our IRAM and DRAM */
#ifndef SIMULATOR
    memcpy((unsigned char *)CODEC_IRAM_ORIGIN, iram_buf[my_codec],
            CODEC_IRAM_SIZE);
#endif
    invalidate_icache();
    memcpy(codecbuf, dram_buf[my_codec], CODEC_SIZE);

    logf("codec resuming:%d", current_codec);
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
static void voice_boost_cpu(bool state)
{
    static bool voice_cpu_boosted = false;

    if (!voice_codec_loaded)
        state = false;

    if (state != voice_cpu_boosted)
    {
        cpu_boost(state);
        voice_cpu_boosted = state;
    }
}
#else
#define voice_boost_cpu(state)   do { } while(0)
#endif

bool codec_pcmbuf_insert_split_callback(const void *ch1, const void *ch2,
                                        size_t length)
{
    const char* src[2];
    char *dest;
    long input_size;
    size_t output_size;

    src[0] = ch1;
    src[1] = ch2;

    if (dsp_stereo_mode() == STEREO_NONINTERLEAVED)
        length *= 2;    /* Length is per channel */

    while (length > 0) {
        long est_output_size = dsp_output_size(length);
        if (current_codec == CODEC_IDX_VOICE) {
            while ((dest = pcmbuf_request_voice_buffer(est_output_size,
                            &output_size, audio_codec_loaded)) == NULL)
                sleep(1);
        }
        else
        {
            /* Prevent audio from a previous position from hitting the buffer */
            if (ci.reload_codec || ci.stop_codec)
                return true;

            while ((dest = pcmbuf_request_buffer(est_output_size,
                            &output_size)) == NULL) {
                sleep(1);
                if (ci.seek_time || ci.reload_codec || ci.stop_codec)
                    return true;
            }
        }

        /* Get the real input_size for output_size bytes, guarding
         * against resampling buffer overflows. */
        input_size = dsp_input_size(output_size);

        if (input_size <= 0) {
            DEBUGF("Error: dsp_input_size(%ld=dsp_output_size(%ld))=%ld<=0\n",
                    output_size, length, input_size);
            /* If this happens, then some samples have been lost */
            break;
        }

        if ((size_t)input_size > length) {
            DEBUGF("Error: dsp_input_size(%ld=dsp_output_size(%ld))=%ld>%ld\n",
                   output_size, length, input_size, length);
            input_size = length;
        }

        output_size = dsp_process(dest, src, input_size);

        /* Hotswap between audio and voice codecs as necessary. */
        switch (current_codec)
        {
            case CODEC_IDX_AUDIO:
                pcmbuf_write_complete(output_size);
                if (voice_is_playing && pcmbuf_usage() > 30
                    && pcmbuf_mix_usage() < 20)
                {
                    voice_boost_cpu(true);
                    swap_codec();
                    voice_boost_cpu(false);
                }
                break ;

            case CODEC_IDX_VOICE:
                if (audio_codec_loaded) {
                    pcmbuf_mix(dest, output_size);
                    if ((pcmbuf_usage() < 10)
                        || pcmbuf_mix_usage() > 70)
                        swap_codec();
                } else
                    pcmbuf_write_complete(output_size);
                break ;
        }

        length -= input_size;
    }

    return true;
}

bool codec_pcmbuf_insert_callback(const char *buf, size_t length)
{
    /* TODO: The audiobuffer API should probably be updated, and be based on
     *       pcmbuf_insert_split().  */
    long real_length = length;

    if (dsp_stereo_mode() == STEREO_NONINTERLEAVED)
    {
        length /= 2;    /* Length is per channel */
    }

    /* Second channel is only used for non-interleaved stereo. */
    return codec_pcmbuf_insert_split_callback(buf, buf + (real_length / 2),
        length);
}

void* get_codec_memory_callback(size_t *size)
{
    *size = MALLOC_BUFSIZE;
    if (voice_codec_loaded)
        return &audiobuf[talk_get_bufsize()];

    return &audiobuf[0];
}

static void pcmbuf_position_callback(size_t size) ICODE_ATTR;
static void pcmbuf_position_callback(size_t size) {
    unsigned int time = size * 1000 / 4 / NATIVE_FREQUENCY +
        prev_ti->id3.elapsed;
    if (time >= prev_ti->id3.length) {
        pcmbuf_set_position_callback(NULL);
        prev_ti->id3.elapsed = prev_ti->id3.length;
    } else {
        prev_ti->id3.elapsed = time;
    }
}

void codec_set_elapsed_callback(unsigned int value)
{
    unsigned int latency;

    /* We don't save or display offsets for voice */
    if (current_codec == CODEC_IDX_VOICE)
        return ;

#ifdef AB_REPEAT_ENABLE
    ab_position_report(value);
#endif
    latency = pcmbuf_get_latency();

    if (value < latency)
        cur_ti->id3.elapsed = 0;
    else if (value - latency > cur_ti->id3.elapsed ||
            value - latency < cur_ti->id3.elapsed - 2)
        cur_ti->id3.elapsed = value - latency;
}

void codec_set_offset_callback(size_t value)
{
    unsigned int latency;

    /* We don't save or display offsets for voice */
    if (current_codec == CODEC_IDX_VOICE)
        return ;

    latency = pcmbuf_get_latency() * cur_ti->id3.bitrate / 8;

    if (value < latency)
        cur_ti->id3.offset = 0;
    else
        cur_ti->id3.offset = value - latency;
}

static bool filebuf_is_lowdata(void)
{
    return filebufused < AUDIO_FILEBUF_CRITICAL;
}

static bool have_tracks(void)
{
    return track_ridx != track_widx || tracks[track_ridx].filesize;
}

static bool have_free_tracks(void)
{
    if (track_widx < track_ridx)
        return track_widx + 1 < track_ridx;
    else if (track_ridx == 0)
        return track_widx < MAX_TRACK - 1;
    else
        return true;
}
        
int audio_track_count(void)
{
    int track_count = 0;
    if (have_tracks())
    {
        int cur_idx = track_ridx;
        track_count++;
        while (cur_idx != track_widx)
        {
            track_count++;
            if (++cur_idx > MAX_TRACK)
                cur_idx -= MAX_TRACK;
        }
    }
    return track_count;
}

static void advance_buffer_counters(size_t amount) {
    buf_ridx += amount;
    if (buf_ridx >= filebuflen)
        buf_ridx -= filebuflen;
    ci.curpos += amount;
    cur_ti->available -= amount;
    filebufused -= amount;

    /* Start buffer filling as necessary. */
    if (!pcmbuf_is_lowdata() && !filling)
        if (conf_watermark && filebufused <= conf_watermark && playing)
            queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);
}

/* copy up-to size bytes into ptr and return the actual size copied */
size_t codec_filebuf_callback(void *ptr, size_t size)
{
    char *buf = (char *)ptr;
    size_t copy_n;
    size_t part_n;

    if (ci.stop_codec || !playing || current_codec == CODEC_IDX_VOICE)
        return 0;

    /* The ammount to copy is the lesser of the requested amount and the
     * amount left of the current track (both on disk and already loaded) */
    copy_n = MIN(size, cur_ti->available + cur_ti->filerem);

    /* Nothing requested OR nothing left */
    if (copy_n == 0)
        return 0;

    /* Let the disk buffer catch fill until enough data is available */
    while (copy_n > cur_ti->available) {
        queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);
        yield();
        if (ci.stop_codec || ci.reload_codec)
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
    advance_buffer_counters(copy_n);

    /* Return the actual amount of data copied to the buffer */
    return copy_n;
}

void* voice_request_data(size_t *realsize, size_t reqsize)
{
    while (queue_empty(&voice_codec_queue) && (voice_remaining == 0
            || voicebuf == NULL) && !ci_voice.stop_codec)
    {
        yield();
        if (audio_codec_loaded && (pcmbuf_usage() < 30
            || !voice_is_playing || voicebuf == NULL))
            swap_codec();
        else if (!voice_is_playing)
        {
            voice_boost_cpu(false);
            if (!pcm_is_playing())
                pcmbuf_boost(false);
            sleep(HZ/16);
        }

        if (voice_remaining)
            voice_is_playing = true;
        else if (voice_getmore != NULL)
        {
            voice_getmore((unsigned char **)&voicebuf, (int *)&voice_remaining);

            if (!voice_remaining)
            {
                voice_is_playing = false;
                /* Force pcm playback. */
                pcmbuf_play_start();
            }
        }
    }

    voice_is_playing = true;
    *realsize = MIN(voice_remaining, reqsize);

    if (*realsize == 0)
        return NULL;

    return voicebuf;
}

void* codec_request_buffer_callback(size_t *realsize, size_t reqsize)
{
    size_t short_n, copy_n, buf_rem;

    /* Voice codec. */
    if (current_codec == CODEC_IDX_VOICE)
        return voice_request_data(realsize, reqsize);

    if (!playing) {
        *realsize = 0;
        return NULL;
    }

    copy_n = MIN(reqsize, cur_ti->available + cur_ti->filerem);
    if (copy_n == 0) {
        *realsize = 0;
        return NULL;
    }

    while (copy_n > cur_ti->available) {
        queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);
        yield();
        if (ci.stop_codec || ci.reload_codec) {
            *realsize = 0;
            return NULL;
        }
    }

    /* How much is left at the end of the file buffer before wrap? */
    buf_rem = filebuflen - buf_ridx;
    /* If we can't satisfy the request without wrapping */
    if (buf_rem < copy_n) {
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
}

static void buffer_wind_forward(void)
{
    /* Wind the buffer forward to the end of the current track */
    buf_ridx += prev_ti->available;
    filebufused -= prev_ti->available;

    if (buf_ridx >= filebuflen)
        buf_ridx -= filebuflen;
}

static void buffer_wind_backward(size_t rewind)
{
    /* Check and handle buffer wrapping */
    if (rewind > buf_ridx)
        buf_ridx += filebuflen;
    /* Rewind the buffer to the beginning of the target track (or its codec) */
    buf_ridx -= rewind;
    filebufused += rewind;

    /* Rewind the old track to its beginning */
    prev_ti->available = prev_ti->filesize - prev_ti->filerem;
    /* Reset to the beginning of the new track */
    cur_ti->available = cur_ti->filesize;
}

static void audio_update_trackinfo(void)
{
    ci.filesize = cur_ti->filesize;
    cur_ti->id3.elapsed = 0;
    cur_ti->id3.offset = 0;
    ci.id3 = (struct mp3entry *)&cur_ti->id3;
    ci.curpos = 0;
    cur_ti->start_pos = 0;
    ci.taginfo_ready = (bool *)&cur_ti->taginfo_ready;
    track_changed = true;
}

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

static void audio_rebuffer(void)
{
    logf("Forcing rebuffer");
    /* Stop in progress fill, and clear open file descriptor */
    close(current_fd);
    current_fd = -1;
    filling = false;

    /* Reset buffer and track pointers */
    buf_ridx = buf_widx = 0;
    track_widx = track_ridx;
    filebufused = 0;

    /* Cause the buffer fill to return as soon as the codec is loaded */
    queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);
    /* Fill the buffer */
    last_peek_offset = -1;
    tracks[track_ridx].filesize = 0;
    audio_fill_file_buffer(false, true, 0);
}

static void audio_check_new_track(long direction)
{
    /* Move to the new track */
    prev_ti = cur_ti;
    cur_ti = &tracks[track_ridx];

    audio_update_trackinfo();

    if (tracks[track_ridx].filesize == 0)
        audio_rebuffer();
    else
    {
        if (direction > 0)
            buffer_wind_forward();
        else if (direction < 0)
        {
            /* This is how much 'back' data must remain uncleared for us
             * to safely backup to the beginning of the previous track */
            size_t req_size = ci.curpos + prev_ti->codecsize + cur_ti->filesize;
            /* This is the amount of 'back' data available on the buffer */
            size_t buf_back = buf_ridx;
            if (buf_back < buf_widx)
                buf_back += filebuflen;
            buf_back -= buf_widx;

            /* If the track isn't on the buffer */
            if (req_size > buf_back)
                audio_rebuffer();
            /* If the track needs to load its codec from the buffer */
            else if (get_codec_base_type(prev_ti->id3.codectype) !=
                    get_codec_base_type(cur_ti->id3.codectype))
            {
                /* If the codec was never buffered */
                if (!cur_ti->codecsize)
                    audio_rebuffer();
                else
                {
                    req_size += cur_ti->codecsize;

                    /* If the codec was overwritten */
                    if (req_size > buf_back)
                        audio_rebuffer();
                    else
                    {
                        cur_ti->has_codec = true;
                        buffer_wind_backward(req_size);
                    }
                }
            }
            else
                buffer_wind_backward(req_size);
        }
        else { logf("Impossible lack of direction"); }
    }
    mutex_unlock(&mutex_interthread);
}

static void rebuffer_and_seek(size_t newpos)
{
    int fd;
    char *trackname;

    trackname = playlist_peek(0);
    /* (Re-)open current track's file handle. */

    fd = open(trackname, O_RDONLY);
    if (fd < 0) {
        logf("Open failed!");
        mutex_unlock(&mutex_interthread);
        return;
    }
    if (current_fd >= 0)
        close(current_fd);
    current_fd = fd;

    playlist_end = false;

    ci.curpos = newpos;

    /* Clear codec buffer. */
    filebufused = 0;
    buf_ridx = buf_widx = 0;

    /* Make sure we are reading the cur_ti */
    while (&tracks[track_ridx] != cur_ti)
        if (--track_ridx < 0)
            track_ridx += MAX_TRACK;
    /* Write to the now current track */
    track_widx = track_ridx;

    last_peek_offset = 0;
    initialize_buffer_fill(false, true);

    if (newpos > AUDIO_REBUFFER_GUESS_SIZE)
        cur_ti->start_pos = newpos - AUDIO_REBUFFER_GUESS_SIZE;
    else
        cur_ti->start_pos = 0;

    cur_ti->filerem = cur_ti->filesize - cur_ti->start_pos;
    cur_ti->available = 0;

    lseek(current_fd, cur_ti->start_pos, SEEK_SET);

    mutex_unlock(&mutex_interthread);
}

void codec_advance_buffer_callback(size_t amount)
{
    if (current_codec == CODEC_IDX_VOICE) {
        amount = MIN(amount, voice_remaining);
        voicebuf += amount;
        voice_remaining -= amount;

        return ;
    }

    if (amount > cur_ti->available + cur_ti->filerem)
        amount = cur_ti->available + cur_ti->filerem;

    while (amount > cur_ti->available && filling)
        sleep(1);

    /* This should not happen */
    if (amount > cur_ti->available) {
        mutex_lock(&mutex_interthread);
        queue_post(&audio_queue,
                Q_AUDIO_REBUFFER_SEEK, (void *)(ci.curpos + amount));
        mutex_lock(&mutex_interthread);
        mutex_unlock(&mutex_interthread);
        return ;
    }

    advance_buffer_counters(amount);

    codec_set_offset_callback(ci.curpos);
}

void codec_advance_buffer_loc_callback(void *ptr)
{
    size_t amount;

    if (current_codec == CODEC_IDX_VOICE)
        amount = (size_t)ptr - (size_t)voicebuf;
    else
        amount = (size_t)ptr - (size_t)&filebuf[buf_ridx];
    codec_advance_buffer_callback(amount);
}

off_t codec_mp3_get_filepos_callback(int newtime)
{
    off_t newpos;

    cur_ti->id3.elapsed = newtime;
    newpos = mp3_get_file_pos();

    return newpos;
}

void codec_seek_complete_callback(void)
{
    logf("seek_complete");
    if (pcm_is_paused()) {
        /* If this is not a seamless seek, clear the buffer */
        pcmbuf_play_stop();
        /* If playback was not 'deliberately' paused, unpause now */
        if (!paused)
            pcmbuf_pause(false);
    }
    ci.seek_time = 0;
}

bool codec_seek_buffer_callback(size_t newpos)
{
    int difference;

    if (current_codec == CODEC_IDX_VOICE)
        return false;

    if (newpos >= cur_ti->filesize)
        newpos = cur_ti->filesize - 1;

    difference = newpos - ci.curpos;
    if (difference >= 0) {
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
    if (newpos < cur_ti->start_pos)
    {
        mutex_lock(&mutex_interthread);
        queue_post(&audio_queue, Q_AUDIO_REBUFFER_SEEK, (void *)newpos);
        mutex_lock(&mutex_interthread);
        mutex_unlock(&mutex_interthread);
        return true;
    }

    /* Seeking inside buffer space. */
    logf("seek: -%d", difference);
    filebufused += difference;
    cur_ti->available += difference;
    if (buf_ridx < (unsigned)difference)
        buf_ridx += filebuflen;
    buf_ridx -= difference;
    ci.curpos -= difference;

    return true;
}

static void set_filebuf_watermark(int seconds)
{
    size_t bytes;

    if (current_codec == CODEC_IDX_VOICE)
        return ;

    if (!filebuf)
        return;     /* Audio buffers not yet set up */

    bytes = MAX(cur_ti->id3.bitrate * seconds * (1000/8), conf_watermark);
    bytes = MIN(bytes, filebuflen / 2);
    conf_watermark = bytes;
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

    case CODEC_DSP_ENABLE:
        if ((bool)value)
            ci.pcmbuf_insert = codec_pcmbuf_insert_callback;
        else
            ci.pcmbuf_insert = pcmbuf_insert_buffer;
        break ;

    default:
        if (!dsp_configure(setting, value)) { logf("Illegal key:%d", setting); }
    }
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

static void codec_track_changed(void)
{
    track_changed = true;
    queue_post(&audio_queue, Q_AUDIO_TRACK_CHANGED, 0);
}

static void pcmbuf_track_changed_callback(void)
{
    track_changed = true;
    pcmbuf_set_position_callback(NULL);
    queue_post(&audio_queue, Q_AUDIO_TRACK_CHANGED, 0);
}

/* Yield to codecs for as long as possible if they are in need of data
 * return true if the caller should break to let the audio thread process
 * new events */
static bool yield_codecs(void)
{
    yield();
    if (!queue_empty(&audio_queue)) return true;

    while ((pcmbuf_is_crossfade_active() || pcmbuf_is_lowdata())
            && !ci.stop_codec && playing && !filebuf_is_lowdata())
    {
        sleep(1);
        if (!queue_empty(&audio_queue)) return true;
    }
    return false;
}

/* FIXME: This code should be made more generic and move to metadata.c */
void strip_id3v1_tag(void)
{
    int i;
    static const unsigned char tag[] = "TAG";
    size_t tagptr;
    bool found = true;

    if (filebufused >= 128)
    {
        tagptr = buf_widx;
        if (tagptr < 128)
            tagptr += filebuflen;
        tagptr -= 128;

        for(i = 0;i < 3;i++)
        {
            if(filebuf[tagptr] != tag[i])
            {
                found = false;
                break;
            }

            if(++tagptr >= filebuflen)
                tagptr -= filebuflen;
        }

        if(found)
        {
            /* Skip id3v1 tag */
            logf("Skipping ID3v1 tag\n");
            if (buf_widx < 128)
                buf_widx += filebuflen;
            buf_widx -= 128;
            tracks[track_widx].available -= 128;
            filebufused -= 128;
        }
    }
}

static void audio_read_file(void)
{
    size_t copy_n;
    int rc;

    /* If we're called and no file is open, this is an error */
    if (current_fd < 0) {
        logf("audio_read_file fd < 0");
        /* Stop this buffer cycle immediately */
        fill_bytesleft = 0;
        /* Give some hope of miraculous recovery by forcing a track reload */
        tracks[track_widx].filesize = 0;
        return ;
    }

    while (tracks[track_widx].filerem > 0) {
        if (fill_bytesleft == 0)
            break ;

        /* copy_n is the largest chunk that is safe to read */
        copy_n = MIN(conf_filechunk, filebuflen - buf_widx);
        copy_n = MIN(copy_n, fill_bytesleft);
        /* rc is the actual amount read */
        rc = read(current_fd, &filebuf[buf_widx], copy_n);

        if (rc <= 0) {
            /* Reached the end of the file */
            tracks[track_widx].filerem = 0;
            break ;
        }

        buf_widx += rc;
        if (buf_widx >= filebuflen)
            buf_widx -= filebuflen;
        tracks[track_widx].available += rc;
        tracks[track_widx].filerem -= rc;

        filebufused += rc;
        if (fill_bytesleft > (unsigned)rc)
            fill_bytesleft -= rc;
        else
            fill_bytesleft = 0;

        /* Let the codec process until it is out of the danger zone, or there
         * is an event to handle.  In the latter case, break this fill cycle
         * immediately */
        if (yield_codecs())
            break;
    }

    if (tracks[track_widx].filerem == 0) {
        logf("Finished buf.");
        strip_id3v1_tag();
        close(current_fd);
        current_fd = -1;
        if (++track_widx >= MAX_TRACK)
            track_widx = 0;

        tracks[track_widx].filesize = 0;
        /* If we're short filling, and have at least twice the watermark
         * of data, stop filling after this track */
        if (filling_short && filebufused > conf_watermark * 2)
            fill_bytesleft = 0;
    } else { logf("Partially buf:%d", tracks[track_widx].available); }
}

static void codec_discard_codec_callback(void)
{
    if (tracks[track_ridx].has_codec) {
        tracks[track_ridx].has_codec = false;
        filebufused -= tracks[track_ridx].codecsize;
        buf_ridx += tracks[track_ridx].codecsize;
        if (buf_ridx >= filebuflen)
            buf_ridx -= filebuflen;
    }
}

static bool loadcodec(bool start_play)
{
    size_t size;
    int fd;
    int rc;
    const char *codec_path;
    size_t copy_n;
    int prev_track;

    switch (tracks[track_widx].id3.codectype) {
    case AFMT_OGG_VORBIS:
        logf("Codec: Vorbis");
        codec_path = CODEC_VORBIS;
        break;
    case AFMT_MPA_L1:
    case AFMT_MPA_L2:
    case AFMT_MPA_L3:
        logf("Codec: MPA L1/L2/L3");
        codec_path = CODEC_MPA_L3;
        break;
    case AFMT_PCM_WAV:
        logf("Codec: PCM WAV");
        codec_path = CODEC_WAV;
        break;
    case AFMT_FLAC:
        logf("Codec: FLAC");
        codec_path = CODEC_FLAC;
        break;
    case AFMT_A52:
        logf("Codec: A52");
        codec_path = CODEC_A52;
        break;
    case AFMT_MPC:
        logf("Codec: Musepack");
        codec_path = CODEC_MPC;
        break;
    case AFMT_WAVPACK:
        logf("Codec: WAVPACK");
        codec_path = CODEC_WAVPACK;
        break;
    case AFMT_ALAC:
        logf("Codec: ALAC");
        codec_path = CODEC_ALAC;
        break;
    case AFMT_AAC:
        logf("Codec: AAC");
        codec_path = CODEC_AAC;
        break;
    case AFMT_SHN:
        logf("Codec: SHN");
        codec_path = CODEC_SHN;
        break;
    case AFMT_AIFF:
        logf("Codec: PCM AIFF");
        codec_path = CODEC_AIFF;
        break;
    default:
        logf("Codec: Unsupported");
        codec_path = NULL;
        return false;
    }

    tracks[track_widx].has_codec = false;
    tracks[track_widx].codecsize = 0;

    if (start_play)
    {
        /* Load the codec directly from disk and save some memory. */
        cur_ti = &tracks[track_widx];
        ci.filesize = cur_ti->filesize;
        ci.id3 = (struct mp3entry *)&cur_ti->id3;
        ci.taginfo_ready = (bool *)&cur_ti->taginfo_ready;
        ci.curpos = 0;
        playing = true;
        logf("Starting codec");
        queue_post(&codec_queue, Q_CODEC_LOAD_DISK, (void *)codec_path);
        return true;
    }
    else
    {
        /* If we already have another track than this one buffered */
        if (track_widx != track_ridx) {
            prev_track = track_widx - 1;
            if (prev_track < 0)
                prev_track += MAX_TRACK;
            /* If the previous codec is the same as this one, there is no need
             * to put another copy of it on the file buffer */
            if (get_codec_base_type(tracks[track_widx].id3.codectype) ==
                    get_codec_base_type(tracks[prev_track].id3.codectype))
            {
                logf("Reusing prev. codec");
                return true;
            }
        }
    }

    fd = open(codec_path, O_RDONLY);
    if (fd < 0)
    {
        logf("Codec doesn't exist!");
        return false;
    }

    size = filesize(fd);
    /* Never load a partial codec */
    if (filebuflen - filebufused < size + AUDIO_REBUFFER_GUESS_SIZE) {
        logf("Not enough space");
        fill_bytesleft = 0;
        close(fd);
        return false;
    }

    while (tracks[track_widx].codecsize < size) {
        copy_n = MIN(conf_filechunk, filebuflen - buf_widx);
        rc = read(fd, &filebuf[buf_widx], copy_n);
        if (rc < 0)
            return false;
        
        filebufused += rc;
        if (fill_bytesleft > (unsigned)rc)
            fill_bytesleft -= rc;
        else
            fill_bytesleft = 0;

        buf_widx += rc;
        if (buf_widx >= filebuflen)
            buf_widx -= filebuflen;

        tracks[track_widx].codecsize += rc;
        
        /* FIXME: This will spin around pretty quickly, but still requires
         * full read of the codec when an event is posted to the audio
         * queue during this loop */
        yield_codecs();
    }

    tracks[track_widx].has_codec = true;

    close(fd);
    logf("Done: %dB", size);

    return true;
}

static bool read_next_metadata(void)
{
    int fd;
    char *trackname;
    int next_idx;
    int status;

    next_idx = track_widx;
    if (tracks[next_idx].taginfo_ready)
    {
        if (++next_idx >= MAX_TRACK)
            next_idx -= MAX_TRACK;
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
    if (status) {
        if (tracks[next_idx].id3.title)
            lcd_getstringsize(tracks[next_idx].id3.title, NULL, NULL);
        if (tracks[next_idx].id3.artist)
            lcd_getstringsize(tracks[next_idx].id3.artist, NULL, NULL);
        if (tracks[next_idx].id3.album)
            lcd_getstringsize(tracks[next_idx].id3.album, NULL, NULL);
    }
    track_changed = true;
    close(fd);

    return status;
}

static bool audio_load_track(int offset, bool start_play)
{
    char *trackname;
    off_t size;
    char msgbuf[80];

    /* Stop buffer filling if there is no free track entries.
       Don't fill up the last track entry (we wan't to store next track
       metadata there). */
    if (!have_free_tracks()) {
        logf("No free tracks");
        return false;
    }

    if (current_fd >= 0)
    {
        close(current_fd);
        current_fd = -1;
    }

    last_peek_offset++;
    peek_again:
    logf("Buffering track:%d/%d", track_widx, track_ridx);
    /* Get track name from current playlist read position. */
    while ((trackname = playlist_peek(last_peek_offset)) != NULL) {
        /* Handle broken playlists. */
        current_fd = open(trackname, O_RDONLY);
        if (current_fd < 0) {
            logf("Open failed");
            /* Skip invalid entry from playlist. */
            playlist_skip_entry(NULL, last_peek_offset);
        } else
            break;
    }

    if (!trackname) {
        logf("End-of-playlist");
        playlist_end = true;
        return false;
    }

    /* Initialize track entry. */
    size = filesize(current_fd);
    tracks[track_widx].filerem = size;
    tracks[track_widx].filesize = size;
    tracks[track_widx].available = 0;
    tracks[track_widx].playlist_offset = last_peek_offset;

    /* Set default values */
    if (start_play) {
        int last_codec = current_codec;
        current_codec = CODEC_IDX_AUDIO;
        conf_watermark = AUDIO_DEFAULT_WATERMARK;
        conf_filechunk = AUDIO_DEFAULT_FILECHUNK;
        dsp_configure(DSP_RESET, 0);
        ci.configure(CODEC_DSP_ENABLE, false);
        current_codec = last_codec;
    }

    /* Get track metadata if we don't already have it. */
    if (!tracks[track_widx].taginfo_ready) {
        if (!get_metadata(&tracks[track_widx],current_fd,trackname,v1first)) {
            logf("Metadata error:%s!",trackname);
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
        track_changed = true;
    }

    /* Load the codec. */
    tracks[track_widx].codecbuf = &filebuf[buf_widx];
    if (!loadcodec(start_play)) {
        /* Set filesize to zero to indicate no file was loaded. */
        tracks[track_widx].filesize = 0;
        tracks[track_widx].filerem = 0;
        close(current_fd);
        current_fd = -1;

        /* Try skipping to next track if there is space. */
        if (fill_bytesleft > 0) {
            /* This is an error condition unless the fill_bytesleft is 0 */
            snprintf(msgbuf, sizeof(msgbuf)-1, "No codec for: %s", trackname);
            /* We should not use gui_syncplash from audio thread! */
            gui_syncsplash(HZ*2, true, msgbuf);
            /* Skip invalid entry from playlist. */
            playlist_skip_entry(NULL, last_peek_offset);
            tracks[track_widx].taginfo_ready = false;
            goto peek_again;
        }
        return false;
    }

    tracks[track_widx].start_pos = 0;
    set_filebuf_watermark(buffer_margin);
    tracks[track_widx].id3.elapsed = 0;

    if (offset > 0) {
        switch (tracks[track_widx].id3.codectype) {
        case AFMT_MPA_L2:
        case AFMT_MPA_L3:
            lseek(current_fd, offset, SEEK_SET);
            tracks[track_widx].id3.offset = offset;
            mp3_set_elapsed(&tracks[track_widx].id3);
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
            tracks[track_widx].id3.offset = offset;
            break;
        }

    }
    
    if (start_play)
        codec_track_changed();

    audio_read_file();

    return true;
}

static void audio_clear_track_entries(bool clear_unbuffered)
{
    int cur_idx = track_widx;
    int last_idx = -1;
    
    logf("Clearing tracks:%d/%d, %d", track_ridx, track_widx, buffered_only);
    /* Loop over all tracks from write-to-read */
    while (1) {
        if (++cur_idx >= MAX_TRACK)
            cur_idx -= MAX_TRACK;
        if (cur_idx == track_ridx)
            break;

        /* If the track is buffered, conditionally clear/notify,
         * otherwise clear the track if that option is selected */
        if (tracks[cur_idx].event_sent) {
            if (last_idx >= 0)
            {
                /* If there is an unbuffer callback, call it, otherwise, just
                 * clear the track */
                if (track_unbuffer_callback)
                    track_unbuffer_callback(&tracks[last_idx].id3, false);

                memset(&tracks[last_idx], 0, sizeof(struct track_info));
            }
            last_idx = cur_idx;
        } else if (clear_unbuffered)
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

static void stop_codec_flush(void)
{
    ci.stop_codec = true;
    pcmbuf_pause(true);
    while (audio_codec_loaded)
        yield();
    pcmbuf_pause(paused);
}

static void audio_stop_playback(bool resume)
{
    if (playing)
        playlist_update_resume_info(resume ? audio_current_track() : NULL);
    playing = false;
    filling = false;
    paused = false;
    stop_codec_flush();
    if (current_fd >= 0) {
        close(current_fd);
        current_fd = -1;
    }

    /* Mark all entries null. */
    memset(tracks, 0, sizeof(struct track_info) * MAX_TRACK);
}

static void audio_play_start(size_t offset)
{
    long parameter;

    if (current_fd >= 0) {
        close(current_fd);
        current_fd = -1;
    }

    sound_set_volume(global_settings.volume);
    track_widx = track_ridx = 0;
    buf_ridx = buf_widx = 0;
    filebufused = 0;

    memset(&tracks[0], 0, sizeof(struct track_info));
    
    last_peek_offset = -1;

    if (offset == 0) parameter = -1;
    else parameter = offset;

    queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, (void *)parameter);
}

/* Send callback events to notify about new tracks. */
static void generate_postbuffer_events(void)
{
    int cur_idx;
    int last_idx = -1;

    logf("Postbuffer:%d/%d",track_ridx,track_widx);

    if (have_tracks()) {
        cur_idx = track_ridx;
        while (1) {
            if (!tracks[cur_idx].event_sent) {
                if (last_idx >= 0) {
                    /* Mark the event 'sent' even if we don't really send one */
                    tracks[last_idx].event_sent = true;
                    if (track_buffer_callback)
                        track_buffer_callback(&tracks[last_idx].id3, false);
                }
                last_idx = cur_idx;
            }
            if (cur_idx == track_widx)
                break;
            if (++cur_idx >= MAX_TRACK)
                cur_idx -= MAX_TRACK;
        }

        if (last_idx >= 0) {
            tracks[last_idx].event_sent = true;
            if (track_buffer_callback)
                track_buffer_callback(&tracks[last_idx].id3, true);
        }
    }
}

static void initialize_buffer_fill(bool start_play, bool short_fill)
{
    if (short_fill) {
        filling_short = true;
        fill_bytesleft = filebuflen >> 2;
    }
    /* Recalculate remaining bytes to buffer */
    else if (!filling_short)
        fill_bytesleft = filebuflen - filebufused;

    /* Don't initialize if we're already initialized */
    if (filling)
        return ;

    audio_clear_track_entries(start_play);

    logf("Starting buffer fill");
    pcmbuf_set_boost_mode(true);

    /* Save the current resume position once. */
    playlist_update_resume_info(audio_current_track());

    filling = true;
}

static void audio_fill_file_buffer(
        bool start_play, bool short_fill, size_t offset)
{
    initialize_buffer_fill(start_play, short_fill);

    /* If we have a partially buffered track, continue loading,
     * otherwise load a new track */
    if (tracks[track_widx].filesize > 0)
        audio_read_file();
    else if (!audio_load_track(offset, start_play))
        fill_bytesleft = 0;

    /* Read next unbuffered track's metadata as soon as playback begins */
    if (pcm_is_playing() || fill_bytesleft == 0)
    {
        read_next_metadata();

        /* If we're done buffering */
        if (fill_bytesleft == 0)
        {
            generate_postbuffer_events();
            filling = false;
            filling_short = false;
            pcmbuf_set_boost_mode(false);

#ifndef SIMULATOR
            if (playing)
                ata_sleep();
#endif
        }
    }
}

static void track_skip_done(void)
{
    /* Manual track change (always crossfade or flush audio). */
    if (new_track)
    {
        pcmbuf_crossfade_init(true);
        codec_track_changed();
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
        pcmbuf_set_event_handler(pcmbuf_track_changed_callback);
}

static bool skip_next_track(void)
{
    logf("skip next");

    /* Automatic track skipping. */
    if (new_track == 0)
    {
        pcmbuf_set_position_callback(pcmbuf_position_callback);
        if (!playlist_check(1)) {
            ci.stop_codec = true;
            ci.reload_codec = false;
            return false;
        }
        last_peek_offset--;
        playlist_next(1);
    }

    if (++track_ridx >= MAX_TRACK)
        track_ridx = 0;

    mutex_lock(&mutex_interthread);
    queue_post(&audio_queue, Q_AUDIO_CHECK_NEW_TRACK, (void *)1);
    mutex_lock(&mutex_interthread);
    mutex_unlock(&mutex_interthread);
    
    track_skip_done();

    return true;
}

static void skip_previous_track(void)
{
    logf("skip previous");

    if (--track_ridx < 0)
        track_ridx += MAX_TRACK;

    mutex_lock(&mutex_interthread);
    queue_post(&audio_queue, Q_AUDIO_CHECK_NEW_TRACK, (void *)-1);
    mutex_lock(&mutex_interthread);
    mutex_unlock(&mutex_interthread);

    track_skip_done();
}

bool codec_request_next_track_callback(void)
{
    if (current_codec == CODEC_IDX_VOICE) {
        voice_remaining = 0;
        /* Terminate the codec if there are messages waiting on the queue or
           the core has been requested the codec to be terminated. */
        return !ci_voice.stop_codec && queue_empty(&voice_codec_queue);
    }

    if (ci.stop_codec || !playing)
        return false;

#ifdef AB_REPEAT_ENABLE
    ab_end_of_track_report();
#endif

    logf("Request new track");

    
    if (new_track >= 0)
    {
        if (!skip_next_track())
            return false;
    }
    else
        skip_previous_track();
    
    /* Check if the next codec is the same file. */
    if (get_codec_base_type(prev_ti->id3.codectype) ==
        get_codec_base_type(cur_ti->id3.codectype))
    {
        logf("New track loaded");
        ci.reload_codec = false;
        codec_discard_codec_callback();
        return true;
    }
    else
    {
        logf("New codec:%d/%d", cur_ti->id3.codectype, prev_ti->id3.codectype);
        ci.reload_codec = true;
        return false;
    }
}

/* Invalidates all but currently playing track. */
void audio_invalidate_tracks(void)
{
    if (have_tracks()) {
        playlist_end = false;
        last_peek_offset = 0;

        track_widx = track_ridx;

        audio_clear_track_entries(true);

        /* If the current track is fully buffered, advance the write pointer */
        if (tracks[track_widx].filerem == 0)
            if (++track_widx >= MAX_TRACK)
                track_widx -= MAX_TRACK;

        /* Mark all other entries null (also buffered wrong metadata). */
        filebufused = cur_ti->available;
        buf_widx = buf_ridx + cur_ti->available;
        if (buf_widx >= filebuflen)
            buf_widx -= filebuflen;

        read_next_metadata();
    }
}

static void initiate_track_change(long peek_index)
{
    new_track = peek_index;
    ci.reload_codec = true;
}

static void initiate_dir_change(long direction)
{
    if(!playlist_next_dir(direction))
        return;

    queue_post(&audio_queue, Q_AUDIO_PLAY, 0);
}

void audio_thread(void)
{
    struct event ev;
    int last_tick = 0;
    bool play_pending = false;

    /* At first initialize audio system in background. */
    playback_init();

    while (1) {
        if (play_pending)
        {
            queue_wait_w_tmo(&audio_queue, &ev, 0);
            if (ev.id == SYS_TIMEOUT)
            {
                ev.id = Q_AUDIO_PLAY;
                ev.data = 0;
            }
        }
        else if (filling)
        {
            queue_wait_w_tmo(&audio_queue, &ev, 0);
            if (ev.id == SYS_TIMEOUT)
            {
                ev.id = Q_AUDIO_FILL_BUFFER;
                ev.data = (void *)0;
            }
        }
        else
            queue_wait_w_tmo(&audio_queue, &ev, HZ);


        switch (ev.id) {
            case Q_AUDIO_FILL_BUFFER:
                {
                    bool start_play = (bool)ev.data;
                    if (!filling && !start_play)
                        if (ci.stop_codec || ci.reload_codec || playlist_end)
                            break;
                    audio_fill_file_buffer(
                            start_play, start_play, abs((long)ev.data));
                    break;
                }
            case Q_AUDIO_PLAY:
                /* Don't start playing immediately if user is skipping tracks
                 * fast to prevent UI lag. */
                last_peek_offset = 0;
                track_changed = true;
                playlist_end = false;
                if (current_tick - last_tick < HZ/2)
                {
                    play_pending = true;
                    break ;
                }
                play_pending = false;
                last_tick = current_tick;

#ifdef CONFIG_TUNER
                /* check if radio is playing */
                if (get_radio_status() != FMRADIO_OFF) {
                    radio_stop();
                }
#endif

                logf("starting...");

                playing = true;
                ci.stop_codec = true;
                ci.reload_codec = false;
                ci.seek_time = 0;

                while (audio_codec_loaded)
                    yield();

                audio_play_start((size_t)ev.data);

                break ;

            case Q_AUDIO_STOP:
                logf("audio_stop");
                audio_stop_playback(true);
                break ;

            case Q_AUDIO_PAUSE:
                logf("audio_%s",ev.data?"pause":"resume");
                pcmbuf_pause((bool)ev.data);
                paused = (bool)ev.data;
                break ;

            case Q_AUDIO_SKIP:
                logf("audio_skip");
                last_tick = current_tick;
                playlist_end = false;
                initiate_track_change((long)ev.data);
                break;

            case Q_AUDIO_PRE_FF_REWIND:
                if (!playing)
                    break;
                logf("pre_ff_rewind");
                pcmbuf_pause(true);
                break;

            case Q_AUDIO_FF_REWIND:
                if (!playing)
                    break ;
                logf("ff_rewind");
                ci.seek_time = (long)ev.data+1;
                break ;

            case Q_AUDIO_REBUFFER_SEEK:
                logf("Re-buffering song w/seek");
                rebuffer_and_seek((size_t)ev.data);
                break;

            case Q_AUDIO_CHECK_NEW_TRACK:
                logf("Check new track buffer");
                audio_check_new_track((long)ev.data);
                break;

            case Q_AUDIO_DIR_SKIP:
                logf("audio_dir_skip");
                playlist_end = false;
                if (global_settings.beep)
                    pcmbuf_beep(5000, 100, 2500*global_settings.beep);
                initiate_dir_change((long)ev.data);
                break;

            case Q_AUDIO_FLUSH:
                logf("flush & reload");
                audio_invalidate_tracks();
                break ;

            case Q_AUDIO_TRACK_CHANGED:
                if (track_changed_callback)
                    track_changed_callback(&cur_ti->id3);
                playlist_update_resume_info(audio_current_track());
                pcmbuf_set_position_callback(NULL);
                track_changed = true;
                audio_clear_track_entries(false);
                break ;

            case Q_AUDIO_CODEC_DONE:
                break ;

#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                logf("USB: Audio core");
                audio_stop_playback(true);
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&audio_queue);
                break ;
#endif
            case SYS_TIMEOUT:
                break;
        }
    }
}

void codec_thread(void)
{
    struct event ev;
    int status;
    size_t wrap;

    while (1) {
        status = 0;
        queue_wait(&codec_queue, &ev);
        new_track = 0;

        switch (ev.id) {
            case Q_CODEC_LOAD_DISK:
                ci.stop_codec = false;
                audio_codec_loaded = true;
                mutex_lock(&mutex_codecthread);
                current_codec = CODEC_IDX_AUDIO;
                status = codec_load_file((char *)ev.data, &ci);
                mutex_unlock(&mutex_codecthread);
                break ;

            case Q_CODEC_LOAD:
                logf("Codec start");
                if (!tracks[track_ridx].has_codec) {
                    logf("Codec slot is empty!");
                    /* Wait for the pcm buffer to go empty */
                    while (pcm_is_playing())
                        yield();
                    /* This must be set to prevent an infinite loop */
                    ci.stop_codec = true;
                    queue_post(&codec_queue, Q_AUDIO_PLAY, 0);
                    break ;
                }

                ci.stop_codec = false;
                wrap = (size_t)&filebuf[filebuflen] -
                    (size_t)tracks[track_ridx].codecbuf;
                audio_codec_loaded = true;
                mutex_lock(&mutex_codecthread);
                current_codec = CODEC_IDX_AUDIO;
                status = codec_load_ram(tracks[track_ridx].codecbuf,
                        tracks[track_ridx].codecsize, &filebuf[0], wrap, &ci);
                mutex_unlock(&mutex_codecthread);
                break ;

#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                while (voice_codec_loaded) {
                    if (current_codec != CODEC_IDX_VOICE)
                        swap_codec();
                    sleep(1);
                }
                logf("USB: Audio codec");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&codec_queue);
                break ;
#endif
        }

        if (audio_codec_loaded)
            if (ci.stop_codec && pcm_is_paused())
                pcmbuf_play_stop();

        audio_codec_loaded = false;

        switch (ev.id) {
            case Q_CODEC_LOAD_DISK:
            case Q_CODEC_LOAD:
                ci.reload_codec = false;
                if (playing) {
                    if (new_track || status == CODEC_OK) {
                        logf("Codec finished");
                        if (ci.stop_codec)
                            queue_post(&audio_queue, Q_AUDIO_STOP, 0);
                        else
                            queue_post(&codec_queue, Q_CODEC_LOAD, 0);
                    } else {
                        logf("Codec failure");
                        gui_syncsplash(HZ*2, true, "Codec failure");
                        if (ci.stop_codec)
                            queue_post(&audio_queue, Q_AUDIO_STOP, 0);
                        else if (skip_next_track())
                            queue_post(&codec_queue, Q_CODEC_LOAD, 0);
                        else
                            queue_post(&audio_queue, Q_AUDIO_STOP, 0);
                    }
                }
        }
    }
}

static void reset_buffer(void)
{
    filebuf = (char *)&audiobuf[MALLOC_BUFSIZE];
    filebuflen = audiobufend - audiobuf - MALLOC_BUFSIZE - GUARD_BUFSIZE -
        (pcmbuf_get_bufsize() + get_pcmbuf_descsize() + PCMBUF_FADE_CHUNK);


    if (talk_get_bufsize() && voice_codec_loaded)
    {
        filebuf = &filebuf[talk_get_bufsize()];
        filebuflen -= 2*CODEC_IRAM_SIZE + 2*CODEC_SIZE + talk_get_bufsize();
    }

#ifndef SIMULATOR
    iram_buf[0] = &filebuf[filebuflen];
    iram_buf[1] = &filebuf[filebuflen+CODEC_IRAM_SIZE];
#endif
    dram_buf[0] = (unsigned char *)&filebuf[filebuflen+CODEC_IRAM_SIZE*2];
    dram_buf[1] = (unsigned char *)&filebuf[filebuflen+CODEC_IRAM_SIZE*2+CODEC_SIZE];

}

void voice_codec_thread(void)
{
    struct event ev;
    int status;

    current_codec = CODEC_IDX_AUDIO;
    voice_codec_loaded = false;
    while (1) {
        status = 0;
        voice_is_playing = false;
        queue_wait(&voice_codec_queue, &ev);
        switch (ev.id) {
            case Q_CODEC_LOAD_DISK:
                logf("Loading voice codec");
                audio_stop_playback(true);
                mutex_lock(&mutex_codecthread);
                current_codec = CODEC_IDX_VOICE;
                dsp_configure(DSP_RESET, 0);
                ci.configure(CODEC_DSP_ENABLE, (bool *)true);
                voice_remaining = 0;
                voice_getmore = NULL;
                voice_codec_loaded = true;
                reset_buffer();
                ci_voice.stop_codec = false;

                status = codec_load_file((char *)ev.data, &ci_voice);

                logf("Voice codec finished");
                audio_stop_playback(true);
                mutex_unlock(&mutex_codecthread);
                current_codec = CODEC_IDX_AUDIO;
                voice_codec_loaded = false;
                reset_buffer();
                break ;

#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                logf("USB: Voice codec");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&voice_codec_queue);
                break ;
#endif
        }
    }
}

void voice_init(void)
{
    if (!filebuf)
        return;     /* Audio buffers not yet set up */

    while (voice_codec_loaded)
    {
        logf("Terminating voice codec");
        ci_voice.stop_codec = true;
        sleep(1);
    }

    if (!talk_get_bufsize())
        return ;

    logf("Starting voice codec");
    queue_post(&voice_codec_queue, Q_CODEC_LOAD_DISK, (void *)CODEC_MPA_L3);
    while (!voice_codec_loaded)
        sleep(1);
}

struct mp3entry* audio_current_track(void)
{
    const char *filename;
    const char *p;
    static struct mp3entry temp_id3;

    if (have_tracks()  && cur_ti->taginfo_ready)
        return (struct mp3entry *)&cur_ti->id3;

    memset(&temp_id3, 0, sizeof(struct mp3entry));
    
    filename = playlist_peek(0);
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

    if (!have_tracks())
        return NULL;

    if (++next_idx >= MAX_TRACK)
        next_idx -= MAX_TRACK;

    if (!tracks[next_idx].taginfo_ready)
        return NULL;

    return &tracks[next_idx].id3;
}

bool audio_has_changed_track(void)
{
    if (track_changed) {
        track_changed = false;
        return true;
    }

    return false;
}

void audio_play(long offset)
{
    logf("audio_play");
    if (pcmbuf_is_crossfade_enabled())
    {
        ci.stop_codec = true;
        sleep(1);
        pcmbuf_crossfade_init(true);
    }
    else
        stop_codec_flush();

    queue_post(&audio_queue, Q_AUDIO_PLAY, (void *)offset);
}

void audio_stop(void)
{
    queue_post(&audio_queue, Q_AUDIO_STOP, 0);
    while (playing || audio_codec_loaded)
        yield();
}

bool mp3_pause_done(void)
{
    return pcm_is_paused();
}

void audio_pause(void)
{
    queue_post(&audio_queue, Q_AUDIO_PAUSE, (void *)true);
}

void audio_resume(void)
{
    queue_post(&audio_queue, Q_AUDIO_PAUSE, (void *)false);
}

static void audio_skip(long count) {
    /* Prevent UI lag and update the WPS immediately. */
    if (global_settings.beep)
        pcmbuf_beep(5000, 100, 2500*global_settings.beep);

    if (!playlist_check(count))
        return ;
    last_peek_offset += -count;
    playlist_next(count);
    

    queue_post(&audio_queue, Q_AUDIO_SKIP, (void *)count);
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
    queue_post(&audio_queue, Q_AUDIO_DIR_SKIP, (void *)1);
}

void audio_prev_dir(void)
{
    queue_post(&audio_queue, Q_AUDIO_DIR_SKIP, (void *)-1);
}

void audio_pre_ff_rewind(void)
{
    queue_post(&audio_queue, Q_AUDIO_PRE_FF_REWIND, 0);
}

void audio_ff_rewind(long newpos)
{
    queue_post(&audio_queue, Q_AUDIO_FF_REWIND, (int *)newpos);
}

void audio_flush_and_reload_tracks(void)
{
    queue_post(&audio_queue, Q_AUDIO_FLUSH, 0);
}

void audio_error_clear(void)
{
}

int audio_status(void)
{
    int ret = 0;

    if (playing)
        ret |= AUDIO_STATUS_PLAY;

    if (paused)
        ret |= AUDIO_STATUS_PAUSE;

    return ret;
}

int audio_get_file_pos(void)
{
    return 0;
}


/* Copied from mpeg.c. Should be moved somewhere else. */
static void mp3_set_elapsed(struct mp3entry* id3)
{
    if ( id3->vbr ) {
        if ( id3->has_toc ) {
            /* calculate elapsed time using TOC */
            int i;
            unsigned int remainder, plen, relpos, nextpos;

            /* find wich percent we're at */
            for (i=0; i<100; i++ )
                if ( id3->offset < id3->toc[i] * (id3->filesize / 256) )
                    break;

            i--;
            if (i < 0)
                i = 0;

            relpos = id3->toc[i];

            if (i < 99)
                nextpos = id3->toc[i+1];
            else
                nextpos = 256;

            remainder = id3->offset - (relpos * (id3->filesize / 256));

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
            int tpk = id3->length / (id3->filesize / 1024);
            id3->elapsed = id3->offset / 1024 * tpk;
        }
    }
    else
    {
        /* constant bitrate, use exact calculation */
        if (id3->bitrate != 0)
            id3->elapsed = id3->offset / (id3->bitrate / 8);
    }
}

/* Copied from mpeg.c. Should be moved somewhere else. */
int mp3_get_file_pos(void)
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
            pos     += (plen/100)*remainder;
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

    /* Don't seek right to the end of the file so that we can
       transition properly to the next song */
    if (pos >= (int)(id3->filesize - id3->id3v1len))
        pos = id3->filesize - id3->id3v1len - 1;
    /* skip past id3v2 tag and other leading garbage */
    else if (pos < (int)id3->first_frame_offset)
        pos = id3->first_frame_offset;

    return pos;
}

void mp3_play_data(const unsigned char* start, int size,
                   void (*get_more)(unsigned char** start, int* size))
{
    voice_getmore = get_more;
    voicebuf = (char *)start;
    voice_remaining = size;
    voice_is_playing = true;
    pcmbuf_reset_mixpos();
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
    bool was_playing = playing;
    int offset = 0;
    int seconds = 1;

    if (!filebuf)
        return;     /* Audio buffers not yet set up */

    /* Store the track resume position */
    if (playing)
        offset = cur_ti->id3.offset;

    if (enable)
        seconds = global_settings.crossfade_fade_out_delay
                + global_settings.crossfade_fade_out_duration;

    /* Buffer has to be at least 2s long. */
    seconds += 2;
    logf("buf len: %d", seconds);
    size = seconds * (NATIVE_FREQUENCY*4);
    if (pcmbuf_get_bufsize() == size)
        return ;

    /* Playback has to be stopped before changing the buffer size. */
    audio_stop_playback(true);

    /* Re-initialize audio system. */
    if (was_playing)
        gui_syncsplash(0, true, (char *)str(LANG_RESTARTING_PLAYBACK));
    pcmbuf_init(size);
    pcmbuf_crossfade_enable(enable);
    reset_buffer();
    logf("abuf:%dB", pcmbuf_get_bufsize());
    logf("fbuf:%dB", filebuflen);

    voice_init();

    /* Restart playback. */
    if (was_playing) {
        audio_play(offset);

        /* Wait for the playback to start again (and display the splash
           screen during that period. */
        playing = true;
        while (playing && !audio_codec_loaded)
            yield();
    }
}

void mpeg_id3_options(bool _v1first)
{
   v1first = _v1first;
}

void test_buffer_event(struct mp3entry *id3, bool last_track)
{
    (void)id3;
    (void)last_track;

    logf("be:%d%s", last_track, id3->path);
}

void test_unbuffer_event(struct mp3entry *id3, bool last_track)
{
    (void)id3;
    (void)last_track;

    logf("ube:%d%s", last_track, id3->path);
}

static void playback_init(void)
{
    static bool voicetagtrue = true;
    struct event ev;

    logf("playback api init");
    pcm_init();

#if defined(HAVE_RECORDING) && !defined(SIMULATOR)
    /* Set the input multiplexer to Line In */
    pcm_rec_mux(0);
#endif

    audio_set_track_buffer_event(test_buffer_event);
    audio_set_track_unbuffer_event(test_unbuffer_event);

    /* Initialize codec api. */
    ci.read_filebuf = codec_filebuf_callback;
    ci.pcmbuf_insert = pcmbuf_insert_buffer;
    ci.pcmbuf_insert_split = codec_pcmbuf_insert_split_callback;
    ci.get_codec_memory = get_codec_memory_callback;
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

    memcpy(&ci_voice, &ci, sizeof(struct codec_api));
    memset(&id3_voice, 0, sizeof(struct mp3entry));
    ci_voice.taginfo_ready = &voicetagtrue;
    ci_voice.id3 = &id3_voice;
    ci_voice.pcmbuf_insert = codec_pcmbuf_insert_callback;
    id3_voice.frequency = 11200;
    id3_voice.length = 1000000L;

    create_thread(codec_thread, codec_stack, sizeof(codec_stack),
            codec_thread_name);
    create_thread(voice_codec_thread, voice_codec_stack,
            sizeof(voice_codec_stack), voice_codec_thread_name);

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

    filebuf = (char *)&audiobuf[MALLOC_BUFSIZE];

    /* Apply relevant settings */
    audio_set_buffer_margin(global_settings.buffer_margin);
    audio_set_crossfade(global_settings.crossfade);

    sound_settings_apply();
}

void audio_preinit(void)
{
    logf("playback system pre-init");

    filebufused = 0;
    filling = false;
    current_codec = CODEC_IDX_AUDIO;
    playing = false;
    paused = false;
    audio_codec_loaded = false;
    voice_is_playing = false;
    track_changed = false;
    current_fd = -1;
    track_buffer_callback = NULL;
    track_unbuffer_callback = NULL;
    track_changed_callback = NULL;
    /* Just to prevent cur_ti never be anything random. */
    cur_ti = &tracks[0];

    mutex_init(&mutex_codecthread);
    mutex_init(&mutex_interthread);

    queue_init(&audio_queue);
    queue_init(&codec_queue);
    queue_init(&voice_codec_queue);

    create_thread(audio_thread, audio_stack, sizeof(audio_stack),
                  audio_thread_name);
}

void audio_init(void)
{
    logf("playback system post-init");

    queue_post(&audio_queue, Q_AUDIO_POSTINIT, 0);
}

