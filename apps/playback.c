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

/* TODO: Fast codecs seem to cause badness on track skipping (stop, old audio,
 * then new audio).  Investigate the CFL_FLUSH mode used for all track skips */
/* TODO: Check for a possibly broken codepath on a rapid skip, stop event */
/* TODO: same in reverse ^^ */
/* TODO: Can use the track changed callback to detect end of track and seek
 * in the previous track until this happens */
/* Design: we have prev_ti already, have a conditional for what type of seek
 * to do on a seek request, if it is a previous track seek, skip previous,
 * and in the request_next_track callback set the offset up the same way that
 * starting from an offset works. */
/* This is also necesary to prevent the problem with buffer overwriting on
 * automatic track changes */

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
    Q_AUDIO_FLUSH,
    Q_AUDIO_TRACK_CHANGED,
    Q_AUDIO_DIR_SKIP,
    Q_AUDIO_POSTINIT,
    Q_AUDIO_FILL_BUFFER,

    Q_CODEC_REQUEST_PENDING,
    Q_CODEC_REQUEST_COMPLETE,
    Q_CODEC_REQUEST_FAILED,

    Q_VOICE_PLAY,
    Q_VOICE_STOP,

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
struct voice_info {
    void (*callback)(unsigned char **start, int *size);
    int size;
    char *buf;
};


static struct mutex mutex_codecthread;
static struct event_queue codec_callback_queue;

static struct mp3entry id3_voice;

static char *voicebuf;
static size_t voice_remaining;
static bool voice_is_playing;
static void (*voice_getmore)(unsigned char** start, int* size);
static int voice_thread_num = -1;

/* Is file buffer currently being refilled? */
static volatile bool filling IDATA_ATTR;

volatile int current_codec IDATA_ATTR;
extern unsigned char codecbuf[];

/* Ring buffer where tracks and codecs are loaded. */
static char *filebuf;

/* Total size of the ring buffer. */
size_t filebuflen;

/* Bytes available in the buffer. */
size_t filebufused;

/* Ring buffer read and write indexes. */
static volatile size_t buf_ridx IDATA_ATTR;
static volatile size_t buf_widx IDATA_ATTR;

#ifndef SIMULATOR
static unsigned char *iram_buf[2];
#endif
static unsigned char *dram_buf[2];

/* Step count to the next unbuffered track. */
static int last_peek_offset;

/* Track information (count in file buffer, read/write indexes for
   track ring structure. */
static int track_ridx;
static int track_widx;
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

/* Was the skip being executed manual or automatic? */
static bool manual_skip;
static bool dir_skip = false;

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
static int mp3_get_file_pos(void);

static void audio_clear_track_entries(
        bool clear_buffered, bool clear_unbuffered);
static void initialize_buffer_fill(bool clear_tracks);
static void audio_fill_file_buffer(bool start_play, size_t offset);

static void swap_codec(void)
{
    int my_codec = current_codec;

    logf("swapping out codec:%d", my_codec);

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

    logf("resuming codec:%d", my_codec);
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
static void voice_boost_cpu(bool state)
{
    static bool voice_cpu_boosted = false;

    if (state != voice_cpu_boosted)
    {
        cpu_boost(state);
        voice_cpu_boosted = state;
    }
}
#else
#define voice_boost_cpu(state)   do { } while(0)
#endif

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

    do {
        long est_output_size = dsp_output_size(length);
        while ((dest = pcmbuf_request_voice_buffer(est_output_size,
                        &output_size, playing)) == NULL)
            if (playing)
                swap_codec();
            else
                yield();

        /* Get the real input_size for output_size bytes, guarding
         * against resampling buffer overflows. */
        input_size = dsp_input_size(output_size);

        if (input_size <= 0) {
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
            if (pcmbuf_usage() < 10 || pcmbuf_mix_free() < 30)
                swap_codec();
        }
        else
            pcmbuf_write_complete(output_size);

        length -= input_size;

    } while (length > 0);


    return true;
}

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

    do {
        long est_output_size = dsp_output_size(length);
        /* Prevent audio from a previous track from playing */
        if (ci.new_track || ci.stop_codec)
            return true;

        while ((dest = pcmbuf_request_buffer(est_output_size,
                        &output_size)) == NULL) {
            sleep(1);
            if (ci.seek_time || ci.new_track || ci.stop_codec)
                return true;
        }

        /* Get the real input_size for output_size bytes, guarding
         * against resampling buffer overflows. */
        input_size = dsp_input_size(output_size);

        if (input_size <= 0) {
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

        if (voice_is_playing && pcm_is_playing() &&
                pcmbuf_usage() > 30 && pcmbuf_mix_free() > 80)
            swap_codec();

        length -= input_size;

    } while (length > 0);

    return true;
}

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

static void* get_voice_memory_callback(size_t *size)
{
    *size = 0;
    return NULL;
}

static void* get_codec_memory_callback(size_t *size)
{
    *size = MALLOC_BUFSIZE;
    if (voice_codec_loaded)
        return &audiobuf[talk_get_bufsize()];
    else
        return audiobuf;
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

static void voice_set_elapsed_callback(unsigned int value)
{
    (void)value;
}

static void codec_set_elapsed_callback(unsigned int value)
{
    unsigned int latency;

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

static void voice_set_offset_callback(size_t value)
{
    (void)value;
}

static void codec_set_offset_callback(size_t value)
{
    unsigned int latency = pcmbuf_get_latency() * cur_ti->id3.bitrate / 8;
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
    return track_ridx != track_widx || cur_ti->filesize;
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
    if (have_tracks())
    {
        int relative_track_widx = track_widx;
        if (track_ridx > track_widx)
            relative_track_widx += MAX_TRACK;
        return relative_track_widx - track_ridx + 1;
    } else
        return 0;
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

static size_t voice_filebuf_callback(void *ptr, size_t size)
{
    (void)ptr;
    (void)size;

    return 0;
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
    copy_n = MIN(size, cur_ti->available + cur_ti->filerem);

    /* Nothing requested OR nothing left */
    if (copy_n == 0)
        return 0;

    /* Let the disk buffer catch fill until enough data is available */
    while (copy_n > cur_ti->available) {
        if (!filling)
            queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);
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
    advance_buffer_counters(copy_n);

    /* Return the actual amount of data copied to the buffer */
    return copy_n;
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
        if (voice_is_playing)
            queue_wait_w_tmo(&voice_codec_queue, &ev, 0);
        else if (playing)
        {
            queue_wait_w_tmo(&voice_codec_queue, &ev, 0);
            if (ev.id == SYS_TIMEOUT)
                ev.id = Q_AUDIO_PLAY;
        }
        else
            queue_wait(&voice_codec_queue, &ev);

        switch (ev.id) {
            case Q_AUDIO_PLAY:
                swap_codec();
                break;

            case Q_VOICE_STOP:
                if (voice_is_playing)
                {
                    /* Clear the current buffer */
                    voice_is_playing = false;
                    voice_getmore = NULL;
                    voice_remaining = 0;
                    voicebuf = NULL;
                    voice_boost_cpu(false);
                    ci_voice.new_track = 1;
                    /* Force the codec to think it's changing tracks */
                    *realsize = 0;
                    return NULL;
                }
                else
                    break;

            case SYS_USB_CONNECTED:
                logf("USB: Audio core");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                if (audio_codec_loaded)
                    swap_codec();
                usb_wait_for_disconnect(&voice_codec_queue);
                break;

            case Q_VOICE_PLAY:
                {
                    struct voice_info *voice_data;
                    voice_is_playing = true;
                    voice_boost_cpu(true);
                    voice_data = ev.data;
                    voice_remaining = voice_data->size;
                    voicebuf = voice_data->buf;
                    voice_getmore = voice_data->callback;
                }
            case SYS_TIMEOUT:
                goto voice_play_clip;
        }
    }

voice_play_clip:

    if (voice_remaining == 0 || voicebuf == NULL)
    {
        if (voice_getmore)
            voice_getmore((unsigned char **)&voicebuf, (int *)&voice_remaining);

        /* If this clip is done */
        if (!voice_remaining)
        {
            queue_post(&voice_codec_queue, Q_VOICE_STOP, 0);
            /* Force pcm playback. */
            if (!pcm_is_playing())
                pcmbuf_play_start();
        }
    }
    
    *realsize = MIN(voice_remaining, reqsize);

    if (*realsize == 0)
        return NULL;

    return voicebuf;
}

static void* codec_request_buffer_callback(size_t *realsize, size_t reqsize)
{
    size_t short_n, copy_n, buf_rem;

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
        if (!filling)
            queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);
        sleep(1);
        if (ci.stop_codec || ci.new_track) {
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

/* Count the data BETWEEN the selected tracks */
static size_t buffer_count_tracks(int from_track, int to_track) {
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

static bool buffer_wind_forward(int new_track_ridx, int old_track_ridx)
{
    size_t amount;

    /* Start with the remainder of the previously playing track */
    amount = tracks[old_track_ridx].filesize - ci.curpos;
    /* Then collect all data from tracks in between them */
    amount += buffer_count_tracks(old_track_ridx, new_track_ridx);

    if (amount > filebufused)
        return false;

    logf("bwf:%ldB",amount);

    /* Wind the buffer to the beginning of the target track or its codec */
    buf_ridx += amount;
    filebufused -= amount;
    /* Check and handle buffer wrapping */
    if (buf_ridx >= filebuflen)
        buf_ridx -= filebuflen;

    return true;
}

static bool buffer_wind_backward(int new_track_ridx, int old_track_ridx) {
    /* Available buffer data */
    size_t buf_back = buf_ridx;
    if (buf_ridx < buf_widx)
        buf_back += filebuflen;
    buf_back -= buf_widx;
    /* Start with the previously playing track's data and our data */
    size_t amount = ci.curpos;

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
    amount += buffer_count_tracks(new_track_ridx, old_track_ridx);

    /* Do we have space to make this skip? */
    if (amount > buf_back)
        return false;

    logf("bwb:%ldB",amount);

    /* Check and handle buffer wrapping */
    if (amount > buf_ridx)
        buf_ridx += filebuflen;
    /* Rewind the buffer to the beginning of the target track or its codec */
    buf_ridx -= amount;
    filebufused += amount;

    /* Reset to the beginning of the new track */
    tracks[new_track_ridx].available = tracks[new_track_ridx].filesize;

    return true;
}

static void audio_update_trackinfo(void)
{
    ci.filesize = cur_ti->filesize;
    cur_ti->id3.elapsed = 0;
    cur_ti->id3.offset = 0;
    ci.id3 = &cur_ti->id3;
    ci.curpos = 0;
    ci.seek_time = 0;
    ci.taginfo_ready = &cur_ti->taginfo_ready;
}

static void audio_rebuffer(void)
{
    logf("Forcing rebuffer");
    /* Notify the codec that this will take a while */
    if (!filling)
        queue_post(&codec_callback_queue, Q_CODEC_REQUEST_PENDING, 0);
    /* Stop in progress fill, and clear open file descriptor */
    close(current_fd);
    current_fd = -1;
    filling = false;

    /* Reset buffer and track pointers */
    buf_ridx = buf_widx = 0;
    track_widx = track_ridx;
    audio_clear_track_entries(false, true);
    filebufused = 0;

    /* Cause the buffer fill to return as soon as the codec is loaded */
    queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);
    /* Fill the buffer */
    last_peek_offset = -1;
    cur_ti->filesize = 0;
    cur_ti->start_pos = 0;
    audio_fill_file_buffer(false, 0);
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
            cur_ti->taginfo_ready = false;
            audio_rebuffer();
            goto skip_done;
        }
        else
        {
            queue_post(&codec_callback_queue, Q_CODEC_REQUEST_FAILED, 0);
            return;
        }
    }

    /* If the playlist isn't that big */
    if (!playlist_check(ci.new_track))
    {
        if (ci.new_track >= 0)
        {
            queue_post(&codec_callback_queue, Q_CODEC_REQUEST_FAILED, 0);
            return;
        }
        /* Find the beginning backward if the user over-skips it */
        while (!playlist_check(++ci.new_track))
            if (ci.new_track >= 0)
            {
                queue_post(&codec_callback_queue, Q_CODEC_REQUEST_FAILED, 0);
                return;
            }
    }
    /* Update the playlist */
    last_peek_offset -= ci.new_track;
    playlist_next(ci.new_track);

    track_ridx+=ci.new_track;
    if (track_ridx >= MAX_TRACK)
        track_ridx -= MAX_TRACK;
    else if (track_ridx < 0)
        track_ridx += MAX_TRACK;

    forward = ci.new_track > 0;
    ci.new_track = 0;

    /* Save the old track */
    prev_ti = cur_ti;
    /* Move to the new track */
    cur_ti = &tracks[track_ridx];

    track_changed = manual_skip;

    /* If it is not safe to even skip this many track entries */
    if (ci.new_track >= track_count || ci.new_track <= track_count - MAX_TRACK)
    {
        cur_ti->taginfo_ready = false;
        audio_rebuffer();
        goto skip_done;
    }

    /* If the target track is clearly not in memory */
    if (cur_ti->filesize == 0 || !cur_ti->taginfo_ready)
    {
        audio_rebuffer();
        goto skip_done;
    }

    /* The track may be in memory, see if it really is */
    if (forward)
    {
        if (!buffer_wind_forward(track_ridx, old_track_ridx))
            audio_rebuffer();
    }
    else
    {
        int cur_idx = track_ridx;
        bool taginfo_ready = true;
        bool wrap = track_ridx > old_track_ridx;
        while (1) {
            if (++cur_idx >= MAX_TRACK)
                cur_idx -= MAX_TRACK;
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
            if (!buffer_wind_backward(track_ridx, old_track_ridx))
                audio_rebuffer();
        }
        else
        {
            cur_ti->taginfo_ready = false;
            audio_rebuffer();
        }
    }

skip_done:
    audio_update_trackinfo();
    queue_post(&codec_callback_queue, Q_CODEC_REQUEST_COMPLETE, 0);
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
        queue_post(&codec_callback_queue, Q_CODEC_REQUEST_FAILED, 0);
        return;
    }
    if (current_fd >= 0)
        close(current_fd);
    current_fd = fd;

    playlist_end = false;

    ci.curpos = newpos;

    /* Clear codec buffer. */
    filebufused = 0;
    buf_widx = cur_ti->buf_idx + newpos;
    while (buf_widx >= filebuflen)
        buf_widx -= filebuflen;
    buf_ridx = buf_widx;

    /* Write to the now current track */
    track_widx = track_ridx;

    last_peek_offset = 0;
    initialize_buffer_fill(true);

    if (newpos > AUDIO_REBUFFER_GUESS_SIZE)
        cur_ti->start_pos = newpos - AUDIO_REBUFFER_GUESS_SIZE;
    else
        cur_ti->start_pos = 0;

    cur_ti->filerem = cur_ti->filesize - cur_ti->start_pos;
    cur_ti->available = 0;

    lseek(current_fd, cur_ti->start_pos, SEEK_SET);

    queue_post(&codec_callback_queue, Q_CODEC_REQUEST_COMPLETE, 0);
}

static void voice_advance_buffer_callback(size_t amount)
{
    amount = MIN(amount, voice_remaining);
    voicebuf += amount;
    voice_remaining -= amount;
}

static void codec_advance_buffer_callback(size_t amount)
{
    if (amount > cur_ti->available + cur_ti->filerem)
        amount = cur_ti->available + cur_ti->filerem;

    while (amount > cur_ti->available && filling)
        sleep(1);

    if (amount > cur_ti->available) {
        struct event ev;
        queue_post(&audio_queue,
                Q_AUDIO_REBUFFER_SEEK, (void *)(ci.curpos + amount));
        queue_wait(&codec_callback_queue, &ev);
        switch (ev.id)
        {
            case Q_CODEC_REQUEST_FAILED:
                ci.stop_codec = true;
            case Q_CODEC_REQUEST_COMPLETE:
                return;
            default:
                logf("Bad event on ccq");
                ci.stop_codec = true;
                return;
        }
    }

    advance_buffer_counters(amount);

    codec_set_offset_callback(ci.curpos);
}

static void voice_advance_buffer_loc_callback(void *ptr)
{
    size_t amount = (size_t)ptr - (size_t)voicebuf;
    voice_advance_buffer_callback(amount);
}

static void codec_advance_buffer_loc_callback(void *ptr)
{
    size_t amount = (size_t)ptr - (size_t)&filebuf[buf_ridx];
    codec_advance_buffer_callback(amount);
}

static off_t voice_mp3_get_filepos_callback(int newtime)
{
    (void)newtime;
    return 0;
}

static off_t codec_mp3_get_filepos_callback(int newtime)
{
    off_t newpos;

    cur_ti->id3.elapsed = newtime;
    newpos = mp3_get_file_pos();

    return newpos;
}

static void voice_do_nothing(void)
{
    return;
}

static void codec_seek_complete_callback(void)
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

static bool voice_seek_buffer_callback(size_t newpos)
{
    (void)newpos;
    return false;
}

static bool codec_seek_buffer_callback(size_t newpos)
{
    int difference;

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
        struct event ev;
        queue_post(&audio_queue, Q_AUDIO_REBUFFER_SEEK, (void *)newpos);
        queue_wait(&codec_callback_queue, &ev);
        switch (ev.id)
        {
            case Q_CODEC_REQUEST_COMPLETE:
                return true;
            case Q_CODEC_REQUEST_FAILED:
                ci.stop_codec = true;
                return false;
            default:
                logf("Bad event on ccq");
                return false;
        }
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
        return;

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
static void strip_id3v1_tag(void)
{
    int i;
    static const unsigned char tag[] = "TAG";
    size_t tag_idx;
    size_t cur_idx;

    tag_idx = buf_widx;
    if (tag_idx < 128)
        tag_idx += filebuflen;
    tag_idx -= 128;

    if (filebufused > 128 && tag_idx > buf_ridx)
    {
        cur_idx = tag_idx;
        for(i = 0;i < 3;i++)
        {
            if(filebuf[cur_idx] != tag[i])
                return;

            if(++cur_idx >= filebuflen)
                cur_idx -= filebuflen;
        }

        /* Skip id3v1 tag */
        logf("Skipping ID3v1 tag");
        buf_widx = tag_idx;
        tracks[track_widx].available -= 128;
        tracks[track_widx].filesize -= 128;
        filebufused -= 128;
    }
}

static void audio_read_file(void)
{
    size_t copy_n;
    int rc;

    /* If we're called and no file is open, this is an error */
    if (current_fd < 0) {
        logf("Bad fd in arf");
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
        logf("Finished buf:%dB", tracks[track_widx].filesize);
        close(current_fd);
        current_fd = -1;
        strip_id3v1_tag();

        if (++track_widx >= MAX_TRACK)
            track_widx = 0;

        tracks[track_widx].filesize = 0;
    } else {
        logf("Partially buf:%dB",
                tracks[track_widx].filesize - tracks[track_widx].filerem);
    }
}

static void codec_discard_codec_callback(void)
{
    if (cur_ti->has_codec) {
        cur_ti->has_codec = false;
        filebufused -= cur_ti->codecsize;
        buf_ridx += cur_ti->codecsize;
        if (buf_ridx >= filebuflen)
            buf_ridx -= filebuflen;
    }
    /* Check if a buffer desync has happened, and log it */
    if (buf_ridx != cur_ti->buf_idx)
    {
        int offset = cur_ti->buf_idx - buf_ridx;
        size_t new_used = filebufused - offset;
        logf("Buf off :%d=%d-%d", offset, cur_ti->buf_idx, buf_ridx);
        buf_ridx += offset;
        /* Reset the buffer used amount based on the read and write pointers */
        filebufused = track_widx;
        if (track_widx < track_ridx)
            filebufused += filebuflen;
        filebufused -= track_ridx;
        /* If that was not the same amount as the track was off, log it */
        if (new_used != filebufused) {
            logf("Used off:%d",filebufused - new_used);
        }
    }
}

static const char *get_codec_path(int codectype) {
    switch (codectype) {
        case AFMT_OGG_VORBIS:
            logf("Codec: Vorbis");
            return CODEC_VORBIS;
        case AFMT_MPA_L1:
        case AFMT_MPA_L2:
        case AFMT_MPA_L3:
            logf("Codec: MPA L1/L2/L3");
            return CODEC_MPA_L3;
        case AFMT_PCM_WAV:
            logf("Codec: PCM WAV");
            return CODEC_WAV;
        case AFMT_FLAC:
            logf("Codec: FLAC");
            return CODEC_FLAC;
        case AFMT_A52:
            logf("Codec: A52");
            return CODEC_A52;
        case AFMT_MPC:
            logf("Codec: Musepack");
            return CODEC_MPC;
        case AFMT_WAVPACK:
            logf("Codec: WAVPACK");
            return CODEC_WAVPACK;
        case AFMT_ALAC:
            logf("Codec: ALAC");
            return CODEC_ALAC;
        case AFMT_AAC:
            logf("Codec: AAC");
            return CODEC_AAC;
        case AFMT_SHN:
            logf("Codec: SHN");
            return CODEC_SHN;
        case AFMT_AIFF:
            logf("Codec: PCM AIFF");
            return CODEC_AIFF;
        default:
            logf("Codec: Unsupported");
            return NULL;
    }
}

static bool loadcodec(bool start_play)
{
    size_t size;
    int fd;
    int rc;
    size_t copy_n;
    int prev_track;

    const char *codec_path = get_codec_path(tracks[track_widx].id3.codectype);
    if (codec_path == NULL)
        return false;

    tracks[track_widx].has_codec = false;
    tracks[track_widx].codecsize = 0;

    if (start_play)
    {
        /* Load the codec directly from disk and save some memory. */
        track_ridx = track_widx;
        cur_ti = &tracks[track_ridx];
        ci.filesize = cur_ti->filesize;
        ci.id3 = &cur_ti->id3;
        ci.taginfo_ready = &cur_ti->taginfo_ready;
        ci.curpos = 0;
        playing = true;
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
    if (fill_bytesleft < size) {
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
        logf("Nonzero fd in alt");
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

    /* Set default values */
    if (start_play) {
        int last_codec = current_codec;
        current_codec = CODEC_IDX_AUDIO;
        conf_watermark = AUDIO_DEFAULT_WATERMARK;
        conf_filechunk = AUDIO_DEFAULT_FILECHUNK;
        dsp_configure(DSP_RESET, 0);
        current_codec = last_codec;
    }

    /* Get track metadata if we don't already have it. */
    if (!tracks[track_widx].taginfo_ready) {
        if (get_metadata(&tracks[track_widx],current_fd,trackname,v1first)) {
            track_changed = true;
            if (start_play)
                playlist_update_resume_info(audio_current_track());
        } else {
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
    if (!loadcodec(start_play)) {

        if (tracks[track_widx].codecsize)
        {
            /* Must undo the buffer write of the partial codec */
            logf("Partial codec loaded");
            fill_bytesleft += tracks[track_widx].codecsize;
            filebufused -= tracks[track_widx].codecsize;
            if (buf_widx < tracks[track_widx].codecsize)
                buf_widx += filebuflen;
            buf_widx -= tracks[track_widx].codecsize;
            tracks[track_widx].codecsize = 0;
        }

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
    
    logf("alt:%s", trackname);
    tracks[track_widx].buf_idx = buf_widx;
    audio_read_file();

    return true;
}

static void audio_clear_track_entries(
        bool clear_buffered, bool clear_unbuffered)
{
    int cur_idx = track_widx;
    int last_idx = -1;
    
    logf("Clearing tracks:%d/%d, %d", track_ridx, track_widx, clear_unbuffered);
    /* Loop over all tracks from write-to-read */
    while (1) {
        if (++cur_idx >= MAX_TRACK)
            cur_idx -= MAX_TRACK;
        if (cur_idx == track_ridx)
            break;

        /* If the track is buffered, conditionally clear/notify,
         * otherwise clear the track if that option is selected */
        if (tracks[cur_idx].event_sent) {
            if (clear_buffered) {
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

static void audio_stop_playback(void)
{
    /* If we were playing, save resume information */
    if (playing)
    {
        /* Save the current playing spot, or NULL if the playlist has ended */
        playlist_update_resume_info(playlist_end?NULL:audio_current_track());
    }
    filebufused = 0;
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
#ifdef CONFIG_TUNER
    /* check if radio is playing */
    if (get_radio_status() != FMRADIO_OFF)
        radio_stop();
#endif

    /* Wait for any previously playing audio to flush - TODO: Not necessary? */
    while (audio_codec_loaded)
        stop_codec_flush();

    track_changed = true;
    playlist_end = false;

    playing = true;
    ci.new_track = 0;
    ci.seek_time = 0;

    if (current_fd >= 0) {
        close(current_fd);
        current_fd = -1;
    }

    sound_set_volume(global_settings.volume);
    track_widx = track_ridx = 0;
    buf_ridx = buf_widx = 0;
    filebufused = 0;

    /* Mark all entries null. */
    memset(tracks, 0, sizeof(struct track_info) * MAX_TRACK);
    
    last_peek_offset = -1;

    audio_fill_file_buffer(true, offset);
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
            if (++cur_idx >= MAX_TRACK)
                cur_idx -= MAX_TRACK;
        }

        if (last_idx >= 0 && !tracks[last_idx].event_sent)
        {
            tracks[last_idx].event_sent = true;
            if (track_buffer_callback)
                track_buffer_callback(&tracks[last_idx].id3, true);
        }
    }
}

static void initialize_buffer_fill(bool clear_tracks)
{
    fill_bytesleft = filebuflen - filebufused;
    if (buf_ridx > cur_ti->buf_idx)
        cur_ti->start_pos = buf_ridx - cur_ti->buf_idx;

    /* Don't initialize if we're already initialized */
    if (filling)
        return ;

    logf("Starting buffer fill");

    if (clear_tracks)
        audio_clear_track_entries(true, false);

    /* Save the current resume position once. */
    playlist_update_resume_info(audio_current_track());

    filling = true;
}

static void audio_fill_file_buffer(bool start_play, size_t offset)
{
    initialize_buffer_fill(!start_play);

    /* If we have a partially buffered track, continue loading,
     * otherwise load a new track */
    if (tracks[track_widx].filesize > 0)
        audio_read_file();
    else if (!audio_load_track(offset, start_play))
        fill_bytesleft = 0;

    /* If we're done buffering */
    if (fill_bytesleft == 0)
    {
        read_next_metadata();

        generate_postbuffer_events();
        filling = false;

#ifndef SIMULATOR
        if (playing)
            ata_sleep();
#endif
    }
}

static void track_skip_done(bool was_manual)
{
    /* Manual track change (always crossfade or flush audio). */
    if (was_manual)
    {
        pcmbuf_crossfade_init(true);
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
        pcmbuf_set_position_callback(pcmbuf_position_callback);
        pcmbuf_set_event_handler(pcmbuf_track_changed_callback);
    }
}

static bool load_next_track(void) {
    struct event ev;

    if (ci.seek_time)
        codec_seek_complete_callback();

#ifdef AB_REPEAT_ENABLE
    ab_end_of_track_report();
#endif

    logf("Request new track");

    if (ci.new_track == 0)
    {
        ci.new_track++;
        manual_skip = false;
    }
    else
        manual_skip = true;
    
    cpu_boost(true);
    queue_post(&audio_queue, Q_AUDIO_CHECK_NEW_TRACK, 0);
    while (1) {
        queue_wait(&codec_callback_queue, &ev);
        if (ev.id == Q_CODEC_REQUEST_PENDING)
            if (manual_skip)
                pcmbuf_play_stop();
        else
            break;
    }
    cpu_boost(false);
    switch (ev.id)
    {
        case Q_CODEC_REQUEST_COMPLETE:
            track_skip_done(manual_skip);
            return true;
        case Q_CODEC_REQUEST_FAILED:
            ci.new_track = 0;
            ci.stop_codec = true;
            return false;
        default:
            logf("Bad event on ccq");
            ci.stop_codec = true;
            return false;
    }
}

static bool voice_request_next_track_callback(void)
{
    ci_voice.new_track = 0;
    return true;
}

static bool codec_request_next_track_callback(void)
{
    int prev_codectype;

    if (ci.stop_codec || !playing)
        return false;

    prev_codectype = get_codec_base_type(cur_ti->id3.codectype);

    if (!load_next_track())
        return false;

    /* Check if the next codec is the same file. */
    if (prev_codectype == get_codec_base_type(cur_ti->id3.codectype))
    {
        logf("New track loaded");
        codec_discard_codec_callback();
        return true;
    }
    else
    {
        logf("New codec:%d/%d", cur_ti->id3.codectype, prev_codectype);
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

        audio_clear_track_entries(true, true);

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

static void initiate_track_change(long direction)
{
    playlist_end = false;
    ci.new_track += direction;
}

static void initiate_dir_change(long direction)
{
    playlist_end = false;
    dir_skip = true;
    ci.new_track = direction;
}

void audio_thread(void)
{
    struct event ev;

    /* At first initialize audio system in background. */
    playback_init();

    while (1) {
        if (filling)
        {
            queue_wait_w_tmo(&audio_queue, &ev, 0);
            if (ev.id == SYS_TIMEOUT)
                ev.id = Q_AUDIO_FILL_BUFFER;
        }
        else
            queue_wait_w_tmo(&audio_queue, &ev, HZ);

        switch (ev.id) {
            case Q_AUDIO_FILL_BUFFER:
                if (!filling)
                    if (!playing || playlist_end || ci.stop_codec)
                        break;
                audio_fill_file_buffer(false, 0);
                break;

            case Q_AUDIO_PLAY:
                logf("starting...");
                audio_play_start((size_t)ev.data);
                break ;

            case Q_AUDIO_STOP:
                logf("audio_stop");
                audio_stop_playback();
                break ;

            case Q_AUDIO_PAUSE:
                logf("audio_%s",ev.data?"pause":"resume");
                pcmbuf_pause((bool)ev.data);
                paused = (bool)ev.data;
                break ;

            case Q_AUDIO_SKIP:
                logf("audio_skip");
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
                audio_check_new_track();
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
                track_changed = true;
                playlist_update_resume_info(audio_current_track());
                break ;

#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                logf("USB: Audio core");
                audio_stop_playback();
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&audio_queue);
                break ;
#endif
            case SYS_TIMEOUT:
                break;
        }
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
                logf("Codec load disk");
                audio_codec_loaded = true;
                if (voice_codec_loaded)
                    queue_post(&voice_codec_queue, Q_AUDIO_PLAY, 0);
                mutex_lock(&mutex_codecthread);
                current_codec = CODEC_IDX_AUDIO;
                ci.stop_codec = false;
                status = codec_load_file((const char *)ev.data, &ci);
                mutex_unlock(&mutex_codecthread);
                break ;

            case Q_CODEC_LOAD:
                logf("Codec load ram");
                if (!cur_ti->has_codec) {
                    logf("Codec slot is empty!");
                    /* Wait for the pcm buffer to go empty */
                    while (pcm_is_playing())
                        yield();
                    /* This must be set to prevent an infinite loop */
                    ci.stop_codec = true;
                    queue_post(&codec_queue, Q_AUDIO_PLAY, 0);
                    break ;
                }

                audio_codec_loaded = true;
                if (voice_codec_loaded)
                    queue_post(&voice_codec_queue, Q_AUDIO_PLAY, 0);
                mutex_lock(&mutex_codecthread);
                current_codec = CODEC_IDX_AUDIO;
                ci.stop_codec = false;
                wrap = (size_t)&filebuf[filebuflen] - (size_t)cur_ti->codecbuf;
                status = codec_load_ram(cur_ti->codecbuf, cur_ti->codecsize,
                        &filebuf[0], wrap, &ci);
                mutex_unlock(&mutex_codecthread);
                break ;

#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                queue_clear(&codec_queue);
                logf("USB: Audio codec");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                if (voice_codec_loaded)
                    swap_codec();
                usb_wait_for_disconnect(&codec_queue);
                break ;
#endif
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
                if (playing) {
                    const char *codec_path;
                    if (ci.new_track || status != CODEC_OK) {
                        if (!ci.new_track) {
                            logf("Codec failure");
                            gui_syncsplash(HZ*2, true, "Codec failure");
                        }
                        if (!load_next_track())
                        {
                            queue_post(&codec_queue, Q_AUDIO_STOP, 0);
                            break;
                        }
                    } else {
                        logf("Codec finished");
                        if (ci.stop_codec)
                        {
                            /* Wait for the audio to stop playing before
                             * triggering the WPS exit */
                            while(pcm_is_playing())
                                sleep(1);
                            queue_post(&audio_queue, Q_AUDIO_STOP, 0);
                            break;
                        }
                    }
                    if (cur_ti->has_codec)
                        queue_post(&codec_queue, Q_CODEC_LOAD, 0);
                    else
                    {
                        codec_path = get_codec_path(cur_ti->id3.codectype);
                        queue_post(&codec_queue,
                                Q_CODEC_LOAD_DISK, (void *)codec_path);
                    }
                }
        }
    }
}

static void reset_buffer(void)
{
    size_t offset;

    filebuf = (char *)&audiobuf[MALLOC_BUFSIZE];
    filebuflen = audiobufend - audiobuf - MALLOC_BUFSIZE - GUARD_BUFSIZE -
        (pcmbuf_get_bufsize() + get_pcmbuf_descsize() + PCMBUF_MIX_CHUNK * 2);

    if (talk_get_bufsize())
    {
        filebuf = &filebuf[talk_get_bufsize()];
        filebuflen -= 2*CODEC_IRAM_SIZE + 2*CODEC_SIZE + talk_get_bufsize();

#ifndef SIMULATOR
        iram_buf[0] = &filebuf[filebuflen];
        iram_buf[1] = &filebuf[filebuflen+CODEC_IRAM_SIZE];
#endif
        dram_buf[0] = (unsigned char *)&filebuf[filebuflen+CODEC_IRAM_SIZE*2];
        dram_buf[1] =
            (unsigned char *)&filebuf[filebuflen+CODEC_IRAM_SIZE*2+CODEC_SIZE];
    }

    /* Ensure that everything is aligned */
    offset = (-(size_t)filebuf) & 3;
    filebuf += offset;
    filebuflen -= offset;
    filebuflen &= ~3;
}

static void voice_codec_thread(void)
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

        codec_load_file(CODEC_MPA_L3, &ci_voice);

        logf("Voice codec finished");
        mutex_unlock(&mutex_codecthread);
        voice_codec_loaded = false;
    }
}

void voice_init(void)
{
    if (!filebuf)
        return;     /* Audio buffers not yet set up */

    if (voice_thread_num >= 0)
    {
        logf("Terminating voice codec");
        remove_thread(voice_thread_num);
        queue_delete(&voice_codec_queue);
        voice_thread_num = -1;
        voice_codec_loaded = false;
    }

    if (!talk_get_bufsize())
        return ;

    logf("Starting voice codec");
    queue_init(&voice_codec_queue);
    voice_thread_num = create_thread(voice_codec_thread, voice_codec_stack,
            sizeof(voice_codec_stack), voice_codec_thread_name);
    while (!voice_codec_loaded)
        yield();
}

struct mp3entry* audio_current_track(void)
{
    const char *filename;
    const char *p;
    static struct mp3entry temp_id3;
    int cur_idx = track_ridx + ci.new_track;

    if (cur_idx >= MAX_TRACK)
        cur_idx += MAX_TRACK;
    else if (cur_idx < 0)
        cur_idx += MAX_TRACK;

    if (tracks[cur_idx].taginfo_ready)
        return &tracks[cur_idx].id3;

    memset(&temp_id3, 0, sizeof(struct mp3entry));
    
    filename = playlist_peek(ci.new_track);
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

    playing = true;
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

void audio_next(void)
{
    if (global_settings.beep)
        pcmbuf_beep(5000, 100, 2500*global_settings.beep);

    queue_post(&audio_queue, Q_AUDIO_SKIP, (void *)1);
}

void audio_prev(void)
{
    if (global_settings.beep)
        pcmbuf_beep(5000, 100, 2500*global_settings.beep);

    queue_post(&audio_queue, Q_AUDIO_SKIP, (void *)-1);
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


/* TODO: Copied from mpeg.c. Should be moved somewhere else. */
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
static int mp3_get_file_pos(void)
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
    static struct voice_info voice_clip;
    voice_clip.callback = get_more;
    voice_clip.buf = (char *)start;
    voice_clip.size = size;
    queue_post(&voice_codec_queue, Q_VOICE_STOP, 0);
    queue_post(&voice_codec_queue, Q_VOICE_PLAY, &voice_clip);
    voice_is_playing = true;
    voice_boost_cpu(true);
}

void mp3_play_stop(void)
{
    queue_post(&voice_codec_queue, Q_VOICE_STOP, 0);
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
    size_t offset = 0;
    int seconds = 1;

    if (!filebuf)
        return;     /* Audio buffers not yet set up */

    if (enable)
        seconds = global_settings.crossfade_fade_out_delay
                + global_settings.crossfade_fade_out_duration;

    /* Buffer has to be at least 2s long. */
    seconds += 2;
    logf("buf len: %d", seconds);
    size = seconds * (NATIVE_FREQUENCY*4);
    if (pcmbuf_get_bufsize() == size)
        return ;

    if (was_playing)
    {
        /* Store the track resume position */
        offset = cur_ti->id3.offset;
        /* Playback has to be stopped before changing the buffer size. */
        queue_post(&audio_queue, Q_AUDIO_STOP, 0);
        while (audio_codec_loaded)
            yield();
        gui_syncsplash(0, true, (char *)str(LANG_RESTARTING_PLAYBACK));
    }

    /* Re-initialize audio system. */
    pcmbuf_init(size);
    pcmbuf_crossfade_enable(enable);
    reset_buffer();
    logf("abuf:%dB", pcmbuf_get_bufsize());
    logf("fbuf:%dB", filebuflen);

    voice_init();

    /* Restart playback. */
    if (was_playing) {
        playing = true;
        queue_post(&audio_queue, Q_AUDIO_PLAY, (void *)offset);

        /* Wait for the playback to start again (and display the splash
           screen during that period. */
        while (playing && !audio_codec_loaded)
            yield();
    }
}

void mpeg_id3_options(bool _v1first)
{
   v1first = _v1first;
}

#if (ROCKBOX_HAS_LOGF == 1)
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

void test_track_changed_event(struct mp3entry *id3)
{
    (void)id3;

    logf("tce:%s", id3->path);
}
#endif

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

#if (ROCKBOX_HAS_LOGF == 1)
    audio_set_track_buffer_event(test_buffer_event);
    audio_set_track_unbuffer_event(test_unbuffer_event);
    audio_set_track_changed_event(test_track_changed_event);
#endif

    /* Initialize codec api. */
    ci.read_filebuf = codec_filebuf_callback;
    ci.pcmbuf_insert = codec_pcmbuf_insert_callback;
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

    /* Initialize voice codec api. */
    memcpy(&ci_voice, &ci, sizeof(struct codec_api));
    memset(&id3_voice, 0, sizeof(struct mp3entry));
    ci_voice.read_filebuf = voice_filebuf_callback;
    ci_voice.pcmbuf_insert = voice_pcmbuf_insert_callback;
    ci_voice.pcmbuf_insert_split = voice_pcmbuf_insert_split_callback;
    ci_voice.get_codec_memory = get_voice_memory_callback;
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

    create_thread(codec_thread, codec_stack, sizeof(codec_stack),
            codec_thread_name);

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

    /* FIXME: This call will infinite loop if called on the audio thread
     * while playing, fortunately this is an init call so that should be
     * impossible. */
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

    queue_init(&audio_queue);
    queue_init(&codec_queue);
    /* clear, not init to create a private queue */
    queue_clear(&codec_callback_queue);

    create_thread(audio_thread, audio_stack, sizeof(audio_stack),
                  audio_thread_name);
}

void audio_init(void)
{
    logf("playback system post-init");

    queue_post(&audio_queue, Q_AUDIO_POSTINIT, 0);
}

