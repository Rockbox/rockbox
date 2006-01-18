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

#define AUDIO_DEFAULT_FIRST_LIMIT    (1024*1024*10)
#define AUDIO_FILL_CYCLE             (1024*256)
#define AUDIO_DEFAULT_WATERMARK      (1024*512)
#define AUDIO_DEFAULT_FILECHUNK      (1024*32)

#define AUDIO_PLAY         1
#define AUDIO_STOP         2
#define AUDIO_PAUSE        3
#define AUDIO_RESUME       4
#define AUDIO_NEXT         5
#define AUDIO_PREV         6
#define AUDIO_FF_REWIND    7
#define AUDIO_FLUSH_RELOAD 8
#define AUDIO_CODEC_DONE   9
#define AUDIO_FLUSH        10
#define AUDIO_TRACK_CHANGED 11
#define AUDIO_DIR_NEXT      12
#define AUDIO_DIR_PREV      13

#define CODEC_LOAD       1
#define CODEC_LOAD_DISK  2

/* As defined in plugins/lib/xxx2wav.h */
#define MALLOC_BUFSIZE (512*1024)
#define GUARD_BUFSIZE  (32*1024)

/* As defined in plugin.lds */
#define CODEC_IRAM_ORIGIN   0x1000c000
#define CODEC_IRAM_SIZE     0xc000

extern bool audio_is_initialized;

/* Buffer control thread. */
static struct event_queue audio_queue;
static long audio_stack[(DEFAULT_STACK_SIZE + 0x1000)/sizeof(long)];
static const char audio_thread_name[] = "audio";

/* Codec thread. */
static struct event_queue codec_queue;
static long codec_stack[(DEFAULT_STACK_SIZE + 0x2000)/sizeof(long)] IBSS_ATTR;
static const char codec_thread_name[] = "codec";

/* Voice codec thread. */
static struct event_queue voice_codec_queue;
/* Not enough IRAM for this. */
static long voice_codec_stack[(DEFAULT_STACK_SIZE + 0x2000)/sizeof(long)] IBSS_ATTR;
static const char voice_codec_thread_name[] = "voice codec";

static struct mutex mutex_bufferfill;
static struct mutex mutex_codecthread;

static struct mp3entry id3_voice;

static char *voicebuf;
static int voice_remaining;
static bool voice_is_playing;
static void (*voice_getmore)(unsigned char** start, int* size);

/* Is file buffer currently being refilled? */
static volatile bool filling;

volatile int current_codec;
extern unsigned char codecbuf[];

/* Ring buffer where tracks and codecs are loaded. */
static char *filebuf;

/* Total size of the ring buffer. */
int filebuflen;

/* Bytes available in the buffer. */
int filebufused;

/* Ring buffer read and write indexes. */
static volatile int buf_ridx;
static volatile int buf_widx;

/* Step count to the next unbuffered track. */
static int last_peek_offset;

/* Track information (count in file buffer, read/write indexes for
   track ring structure. */
int track_count;
static volatile int track_ridx;
static volatile int track_widx;
static bool track_changed;

/* Partially loaded song's file handle to continue buffering later. */
static int current_fd;

/* Information about how many bytes left on the buffer re-fill run. */
static long fill_bytesleft;

/* Track info structure about songs in the file buffer. */
static struct track_info tracks[MAX_TRACK];

/* Pointer to track info structure about current song playing. */
static struct track_info *cur_ti;

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

/* Configuration */
static int conf_bufferlimit;
static int conf_watermark;
static int conf_filechunk;
static int buffer_margin;

static bool v1first = false;

static void mp3_set_elapsed(struct mp3entry* id3);
int mp3_get_file_pos(void);

static void do_swap(int idx_old, int idx_new)
{
#ifndef SIMULATOR
    unsigned char *iram_p = (unsigned char *)(CODEC_IRAM_ORIGIN);
    unsigned char *iram_buf[2];
#endif
    unsigned char *dram_buf[2];


#ifndef SIMULATOR
    iram_buf[0] = &filebuf[filebuflen];
    iram_buf[1] = &filebuf[filebuflen+CODEC_IRAM_SIZE];
    memcpy(iram_buf[idx_old], iram_p, CODEC_IRAM_SIZE);
    memcpy(iram_p, iram_buf[idx_new], CODEC_IRAM_SIZE);
#endif

    dram_buf[0] = (unsigned char *)&filebuf[filebuflen+CODEC_IRAM_SIZE*2];
    dram_buf[1] = (unsigned char *)&filebuf[filebuflen+CODEC_IRAM_SIZE*2+CODEC_SIZE];
    memcpy(dram_buf[idx_old], codecbuf, CODEC_SIZE);
    memcpy(codecbuf, dram_buf[idx_new], CODEC_SIZE);
}

static void swap_codec(void)
{
    int last_codec;
    
    logf("swapping codec:%d", current_codec);
    
    /* We should swap codecs' IRAM contents and code space. */
    do_swap(current_codec, !current_codec);
    
    last_codec = current_codec;
    current_codec = !current_codec;

    /* Release the semaphore and force a task switch. */
    mutex_unlock(&mutex_codecthread);
    sleep(1);

    /* Waiting until we are ready to run again. */
    mutex_lock(&mutex_codecthread);

    /* Check if codec swap did not happen. */
    if (current_codec != last_codec)
    {
        logf("no codec switch happened!");
        do_swap(current_codec, !current_codec);
        current_codec = !current_codec;
    }
    
    invalidate_icache();
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

bool codec_pcmbuf_insert_split_callback(void *ch1, void *ch2,
                                        long length)
{
    char* src[2];
    char *dest;
    long input_size;
    long output_size;

    src[0] = ch1;
    src[1] = ch2;

    while (paused)
    {
        if (pcm_is_playing())
            pcm_play_pause(false);
        sleep(1);
        if (ci.stop_codec || ci.reload_codec || ci.seek_time)
            return true;
    }
    
    if (dsp_stereo_mode() == STEREO_NONINTERLEAVED)
    {
        length *= 2;    /* Length is per channel */
    }

    while (length > 0) {
        /* This will prevent old audio from playing when skipping tracks. */
        if ((ci.reload_codec || ci.stop_codec) && current_codec != CODEC_IDX_VOICE)
            return true;
    
        while ((dest = pcmbuf_request_buffer(dsp_output_size(length), 
            &output_size)) == NULL) {
            sleep(1);
            if ((ci.reload_codec || ci.stop_codec) && current_codec != CODEC_IDX_VOICE)
                return true;
        }

        /* Get the real input_size for output_size bytes, guarding
         * against resampling buffer overflows. */
        input_size = dsp_input_size(output_size);
        if (input_size > length) {
            DEBUGF("Error: dsp_input_size(%ld=dsp_output_size(%ld))=%ld > %ld\n",
                   output_size, length, input_size, length);
            input_size = length;
        }
        
        if (input_size <= 0) {
            pcmbuf_flush_buffer(0);
            DEBUGF("Warning: dsp_input_size(%ld=dsp_output_size(%ld))=%ld <= 0\n",
                   output_size, length, input_size);
            /* should we really continue, or should we break?
             * We should probably continue because calling pcmbuf_flush_buffer(0)
             * will wrap the buffer if it was fully filled and so next call to
             * pcmbuf_request_buffer should give the requested output_size. */
            continue;
        }

        output_size = dsp_process(dest, src, input_size);

        /* Hotswap between audio and voice codecs as necessary. */
        switch (current_codec)
        {
            case CODEC_IDX_AUDIO:
                pcmbuf_flush_buffer(output_size);
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
                } else {
                    pcmbuf_flush_buffer(output_size);
                }
                break ;
        }
            
        length -= input_size;
    }

    return true;
}

bool codec_pcmbuf_insert_callback(char *buf, long length)
{
    /* TODO: The audiobuffer API should probably be updated, and be based on
     *       pcmbuf_insert_split().
     */
    long real_length = length;

    if (dsp_stereo_mode() == STEREO_NONINTERLEAVED)
    {
        length /= 2;    /* Length is per channel */
    }

    /* Second channel is only used for non-interleaved stereo. */
    return codec_pcmbuf_insert_split_callback(buf, buf + (real_length / 2),
        length);
}

void* get_codec_memory_callback(long *size)
{
    *size = MALLOC_BUFSIZE;
    if (voice_codec_loaded)
        return &audiobuf[talk_get_bufsize()];

    return &audiobuf[0];
}

void codec_set_elapsed_callback(unsigned int value)
{
    unsigned int latency;

    if (ci.stop_codec || current_codec == CODEC_IDX_VOICE)
        return ;
        
    latency = pcmbuf_get_latency();
    
    if (value < latency) {
        cur_ti->id3.elapsed = 0;
    } else if (value - latency > cur_ti->id3.elapsed 
            || value - latency < cur_ti->id3.elapsed - 2) {
        cur_ti->id3.elapsed = value - latency;
    }
}

void codec_set_offset_callback(unsigned int value)
{
    unsigned int latency;

    if (ci.stop_codec || current_codec == CODEC_IDX_VOICE)
        return ;
        
    latency = pcmbuf_get_latency() * cur_ti->id3.bitrate / 8;
    
    if (value < latency) {
        cur_ti->id3.offset = 0;
    } else {
        cur_ti->id3.offset = value - latency;
    }
}

long codec_filebuf_callback(void *ptr, long size)
{
    char *buf = (char *)ptr;
    int copy_n;
    int part_n;
    
    if (ci.stop_codec || !playing || current_codec == CODEC_IDX_VOICE)
        return 0;
    
    copy_n = MIN((off_t)size, (off_t)cur_ti->available + cur_ti->filerem);
    
    while (copy_n > cur_ti->available) {
        yield();
        if (ci.stop_codec || ci.reload_codec)
            return 0;
    }
    
    if (copy_n == 0)
        return 0;
    
    part_n = MIN(copy_n, filebuflen - buf_ridx);
    memcpy(buf, &filebuf[buf_ridx], part_n);
    if (part_n < copy_n) {
        memcpy(&buf[part_n], &filebuf[0], copy_n - part_n);
    }
    
    buf_ridx += copy_n;
    if (buf_ridx >= filebuflen)
        buf_ridx -= filebuflen;
    ci.curpos += copy_n;
    cur_ti->available -= copy_n;
    filebufused -= copy_n;
    
    return copy_n;
}

void* voice_request_data(long *realsize, long reqsize)
{
    while (queue_empty(&voice_codec_queue) && (voice_remaining == 0
            || voicebuf == NULL) && !ci_voice.stop_codec)
    {
        yield();
        if (audio_codec_loaded && (pcmbuf_usage() < 30
            || !voice_is_playing || voicebuf == NULL))
        {
            swap_codec();
        }
        else if (!voice_is_playing)
        {
            voice_boost_cpu(false);
            if (!pcm_is_playing())
                pcmbuf_boost(false);
            sleep(HZ/16);
        }
            
        if (voice_remaining)
        {
            voice_is_playing = true;
            break ;
        }
            
        if (voice_getmore != NULL)
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

    if (reqsize < 0)
        reqsize = 0;
        
    voice_is_playing = true;
    *realsize = voice_remaining;
    if (*realsize > reqsize)
        *realsize = reqsize;

    if (*realsize == 0)
        return NULL;

    return voicebuf;
}

void* codec_request_buffer_callback(long *realsize, long reqsize)
{
    long part_n;

    /* Voice codec. */
    if (current_codec == CODEC_IDX_VOICE) {
        return voice_request_data(realsize, reqsize);
    }
   
    if (ci.stop_codec || !playing) {
        *realsize = 0;
        return NULL;
    }
    
    *realsize = MIN((off_t)reqsize, (off_t)cur_ti->available + cur_ti->filerem);
    if (*realsize == 0) {
        return NULL;
    }
    
    while ((int)*realsize > cur_ti->available) {
        yield();
        if (ci.stop_codec || ci.reload_codec) {
            *realsize = 0;
            return NULL;
        }
    }
    
    part_n = MIN((int)*realsize, filebuflen - buf_ridx);
    if (part_n < *realsize) {
        part_n += GUARD_BUFSIZE;
        if (part_n < *realsize)
            *realsize = part_n;
        memcpy(&filebuf[filebuflen], &filebuf[0], *realsize -
            (filebuflen - buf_ridx));
    }
    
    return (char *)&filebuf[buf_ridx];
}

static bool rebuffer_and_seek(int newpos)
{
    int fd;

    logf("Re-buffering song");
    mutex_lock(&mutex_bufferfill);

    /* (Re-)open current track's file handle. */
    fd = open(playlist_peek(0), O_RDONLY);
    if (fd < 0) {
        logf("Open failed!");
        mutex_unlock(&mutex_bufferfill);
        return false;
    }
    if (current_fd >= 0)
        close(current_fd);
    current_fd = fd;
        
    /* Clear codec buffer. */
    audio_invalidate_tracks();
    filebufused = 0;
    playlist_end = false;
    buf_ridx = buf_widx = 0;
    cur_ti->filerem = cur_ti->filesize - newpos;
    cur_ti->filepos = newpos;
    cur_ti->start_pos = newpos;
    ci.curpos = newpos;
    cur_ti->available = 0;
    lseek(current_fd, newpos, SEEK_SET);

    mutex_unlock(&mutex_bufferfill);

    while (cur_ti->available == 0 && cur_ti->filerem > 0) {
        sleep(1);
        if (ci.stop_codec || ci.reload_codec || !queue_empty(&audio_queue))
            return false;
    }
    
    return true;
}

void codec_advance_buffer_callback(long amount)
{
    if (current_codec == CODEC_IDX_VOICE) {
        //logf("voice ad.buf:%d", amount);
        amount = MAX(0, MIN(amount, voice_remaining));
        voicebuf += amount;
        voice_remaining -= amount;
        
        return ;
    }
    
    if (amount > cur_ti->available + cur_ti->filerem)
        amount = cur_ti->available + cur_ti->filerem;

    while (amount > cur_ti->available && filling)
        sleep(1);
    
    if (amount > cur_ti->available) {
        if (!rebuffer_and_seek(ci.curpos + amount))
            ci.stop_codec = true;
        return ;
    }
    
    buf_ridx += amount;
    if (buf_ridx >= filebuflen)
        buf_ridx -= filebuflen;
    cur_ti->available -= amount;
    filebufused -= amount;
    ci.curpos += amount;
    codec_set_offset_callback(ci.curpos);
}

void codec_advance_buffer_loc_callback(void *ptr)
{
    long amount;

    if (current_codec == CODEC_IDX_VOICE)
        amount = (int)ptr - (int)voicebuf;
    else
        amount = (int)ptr - (int)&filebuf[buf_ridx];
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
    /* assume we're called from non-voice codec, as they shouldn't seek */
    ci.seek_time = 0;
    pcmbuf_flush_audio();
}

bool codec_seek_buffer_callback(off_t newpos)
{
    int difference;

    if (current_codec == CODEC_IDX_VOICE)
        return false;
        
    if (newpos < 0)
        newpos = 0;
    
    if (newpos >= cur_ti->filesize)
        newpos = cur_ti->filesize - 1;
     
    difference = newpos - ci.curpos;
    /* Seeking forward */
    if (difference >= 0) {
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
        return rebuffer_and_seek(newpos);

    /* Seeking inside buffer space. */
    logf("seek: -%d", difference);
    filebufused += difference;
    cur_ti->available += difference;
    buf_ridx -= difference;
    if (buf_ridx < 0)
        buf_ridx = filebuflen + buf_ridx;
    ci.curpos -= difference;
    
    return true;
}

static void set_filebuf_watermark(int seconds)
{
    long bytes;

    if (current_codec == CODEC_IDX_VOICE)
        return ;

    if (!filebuf)
        return;     /* Audio buffers not yet set up */
        
    bytes = MAX((int)cur_ti->id3.bitrate * seconds * (1000/8), conf_watermark);
    bytes = MIN(bytes, filebuflen / 2);
    conf_watermark = bytes;
}

static void codec_configure_callback(int setting, void *value)
{
    switch (setting) {
    case CODEC_SET_FILEBUF_WATERMARK:
        conf_watermark = (unsigned int)value;
        set_filebuf_watermark(buffer_margin);
        break;
        
    case CODEC_SET_FILEBUF_CHUNKSIZE:
        conf_filechunk = (unsigned int)value;
        break;
        
    case CODEC_DSP_ENABLE:
        if ((bool)value)
            ci.pcmbuf_insert = codec_pcmbuf_insert_callback;
        else
            ci.pcmbuf_insert = pcmbuf_insert_buffer;
        break ;

    default:
        if (!dsp_configure(setting, value)) {
            logf("Illegal key: %d", setting);
        }
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
    queue_post(&audio_queue, AUDIO_TRACK_CHANGED, 0);
}

/* Give codecs or file buffering the right amount of processing time
   to prevent pcm audio buffer from going empty. */
static void yield_codecs(void)
{
    yield();
    if (!pcm_is_playing() && !paused)
        sleep(5);
    while ((pcmbuf_is_crossfade_active() || pcmbuf_is_lowdata())
            && !ci.stop_codec && playing && queue_empty(&audio_queue)
            && filebufused > (128*1024))
        sleep(1);
}

/* FIXME: This code should be made more generic and move to metadata.c */
void strip_id3v1_tag(void)
{
    int i;
    static const unsigned char tag[] = "TAG";
    int tagptr;
    bool found = true;

    if (filebufused >= 128)
    {
        tagptr = buf_widx - 128;
        if (tagptr < 0)
            tagptr += filebuflen;
        
        for(i = 0;i < 3;i++)
        {
            if(tagptr >= filebuflen)
                tagptr -= filebuflen;
            
            if(filebuf[tagptr] != tag[i])
            {
                found = false;
                break;
            }
            
            tagptr++;
        }
        
        if(found)
        {
            /* Skip id3v1 tag */
            logf("Skipping ID3v1 tag\n");
            buf_widx -= 128;
            tracks[track_widx].available -= 128;
            filebufused -= 128;
        }
    }
}

static void audio_fill_file_buffer(void)
{
    long i, size;
    int rc;

    if (current_fd < 0)
        return ;

    /* Throw away buffered codec. */
    if (tracks[track_widx].start_pos != 0)
        tracks[track_widx].codecsize = 0;
    
    mutex_lock(&mutex_bufferfill);
    i = 0;
    size = MIN(tracks[track_widx].filerem, AUDIO_FILL_CYCLE);
    while (i < size) {
        /* Give codecs some processing time. */
        yield_codecs();
            
        if (fill_bytesleft == 0)
            break ;
        rc = MIN(conf_filechunk, filebuflen - buf_widx);
        rc = MIN(rc, fill_bytesleft);
        rc = read(current_fd, &filebuf[buf_widx], rc);
        if (rc <= 0) {
            tracks[track_widx].filerem = 0;
            break ;
        }
        
        buf_widx += rc;
        if (buf_widx >= filebuflen)
            buf_widx -= filebuflen;
        i += rc;
        tracks[track_widx].available += rc;
        tracks[track_widx].filerem -= rc;
        tracks[track_widx].filepos += rc;
        filebufused += rc;
        fill_bytesleft -= rc;
    }

    if (tracks[track_widx].filerem == 0) {
        strip_id3v1_tag();
    }

    mutex_unlock(&mutex_bufferfill);
    
    /*logf("Filled:%d/%d", tracks[track_widx].available,
                       tracks[track_widx].filerem);*/
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

static bool loadcodec(bool start_play)
{
    off_t size;
    int fd;
    int i, rc;
    const char *codec_path;
    int copy_n;
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
    default:
        logf("Codec: Unsupported");
        codec_path = NULL;
        return false;
    }
    
    tracks[track_widx].codecsize = 0;
    
    if (!start_play) {
        prev_track = track_widx - 1;
        if (prev_track < 0)
            prev_track = MAX_TRACK-1;
        if (track_count > 0 && 
            get_codec_base_type(tracks[track_widx].id3.codectype) ==
            get_codec_base_type(tracks[prev_track].id3.codectype))
        {
            logf("Reusing prev. codec");
            return true;
        }
    } else {
        /* Load the codec directly from disk and save some memory. */
        cur_ti = &tracks[track_widx];
        ci.filesize = cur_ti->filesize;
        ci.id3 = (struct mp3entry *)&cur_ti->id3;
        ci.taginfo_ready = (bool *)&cur_ti->taginfo_ready;
        ci.curpos = 0;
        playing = true;
        logf("Starting codec");
        queue_post(&codec_queue, CODEC_LOAD_DISK, (void *)codec_path);
        return true;
    }
    
    fd = open(codec_path, O_RDONLY);
    if (fd < 0) {
        logf("Codec doesn't exist!");
        return false;
    }
    
    size = filesize(fd);
    if ((off_t)fill_bytesleft < size + conf_watermark) {
        logf("Not enough space");
        /* Set codectype back to zero to indicate no codec was loaded. */
        tracks[track_widx].id3.codectype = 0;
        fill_bytesleft = 0;
        close(fd);
        return false;
    }
    
    i = 0;
    while (i < size) {
        yield_codecs();
        
        copy_n = MIN(conf_filechunk, filebuflen - buf_widx);
        rc = read(fd, &filebuf[buf_widx], copy_n);
        if (rc < 0)
            return false;
        buf_widx += rc;
        filebufused += rc;
        fill_bytesleft -= rc;
        if (buf_widx >= filebuflen)
            buf_widx -= filebuflen;
        i += rc;
    }
    close(fd);
    logf("Done: %dB", i);
    
    tracks[track_widx].codecsize = size;
    
    return true;
}

static bool read_next_metadata(void)
{
    int fd;
    char *trackname;
    int next_track;
    int status;

    next_track = track_widx;
    if (tracks[track_widx].taginfo_ready)
        next_track++;
        
    if (next_track >= MAX_TRACK)
        next_track -= MAX_TRACK;

    if (tracks[next_track].taginfo_ready)
        return true;
        
    trackname = playlist_peek(last_peek_offset + 1);
    if (!trackname)
        return false;

    fd = open(trackname, O_RDONLY);
    if (fd < 0)
        return false;
    
    /** Start buffer refilling also because we need to spin-up the disk.
     * In fact, it might be better not to start filling here, because if user
     * is manipulating the playlist a lot, we will just lose battery. */
    // filling = true;
    status = get_metadata(&tracks[next_track],fd,trackname,v1first);
    /* Preload the glyphs in the tags */
    if (status) {
        if (tracks[next_track].id3.title)
            lcd_getstringsize(tracks[next_track].id3.title, NULL, NULL);
        if (tracks[next_track].id3.artist)
            lcd_getstringsize(tracks[next_track].id3.artist, NULL, NULL);
        if (tracks[next_track].id3.album)
            lcd_getstringsize(tracks[next_track].id3.album, NULL, NULL);
    }
    track_changed = true;
    close(fd);

    return status;
}

static bool audio_load_track(int offset, bool start_play, int peek_offset)
{
    char *trackname;
    int fd = -1;
    off_t size;
    int rc, i;
    int copy_n;
    char msgbuf[80];

    /* Stop buffer filling if there is no free track entries.
       Don't fill up the last track entry (we wan't to store next track
       metadata there). */
    if (track_count >= MAX_TRACK - 1) {
        fill_bytesleft = 0;
        return false;
    }

    /* Don't start loading track if the current write position already
       contains a BUFFERED track. The entry may contain the metadata
       which is ok. */
    if (tracks[track_widx].filesize != 0)
        return false;

    peek_again:
    /* Get track name from current playlist read position. */
    logf("Buffering track:%d/%d", track_widx, track_ridx);
    /* Handle broken playlists. */
    while ( (trackname = playlist_peek(peek_offset)) != NULL) {
        fd = open(trackname, O_RDONLY);
        if (fd < 0) {
            logf("Open failed");
            /* Skip invalid entry from playlist. */
            playlist_skip_entry(NULL, peek_offset);
            continue ;
        }
        break ;
    }
    
    if (!trackname) {
        logf("End-of-playlist");
        playlist_end = true;
        return false;
    }
    
    /* Initialize track entry. */
    size = filesize(fd);
    tracks[track_widx].filerem = size;
    tracks[track_widx].filesize = size;
    tracks[track_widx].filepos = 0;
    tracks[track_widx].available = 0;
    //tracks[track_widx].taginfo_ready = false;
    tracks[track_widx].playlist_offset = peek_offset;
    last_peek_offset = peek_offset;
    
    if (buf_widx >= filebuflen)
        buf_widx -= filebuflen;
    
    /* Set default values */
    if (start_play) {
        int last_codec = current_codec;
        current_codec = CODEC_IDX_AUDIO;
        conf_bufferlimit = AUDIO_DEFAULT_FIRST_LIMIT;
        conf_watermark = AUDIO_DEFAULT_WATERMARK;
        conf_filechunk = AUDIO_DEFAULT_FILECHUNK;
        dsp_configure(DSP_RESET, 0);
        ci.configure(CODEC_DSP_ENABLE, false);
        current_codec = last_codec;
    }

    /* Get track metadata if we don't already have it. */
    if (!tracks[track_widx].taginfo_ready) {
        if (!get_metadata(&tracks[track_widx],fd,trackname,v1first)) {
            logf("Metadata error!");
            tracks[track_widx].filesize = 0;
            tracks[track_widx].filerem = 0;
            tracks[track_widx].taginfo_ready = false;
            close(fd);
            /* Skip invalid entry from playlist. */
            playlist_skip_entry(NULL, peek_offset);
            goto peek_again;
        }
    }
    
    /* Load the codec. */
    tracks[track_widx].codecbuf = &filebuf[buf_widx];
    if (!loadcodec(start_play)) {
        /* We should not use gui_syncplash from audio thread! */
        snprintf(msgbuf, sizeof(msgbuf)-1, "No codec for: %s", trackname);
        gui_syncsplash(HZ*2, true, msgbuf);
        close(fd);
        
        /* Set filesize to zero to indicate no file was loaded. */
        tracks[track_widx].filesize = 0;
        tracks[track_widx].filerem = 0;
        tracks[track_widx].taginfo_ready = false;

        /* Try skipping to next track. */
        if (fill_bytesleft > 0) {
            /* Skip invalid entry from playlist. */
            playlist_skip_entry(NULL, peek_offset);
            goto peek_again;
        }
        return false;
    }

    tracks[track_widx].start_pos = 0;
    set_filebuf_watermark(buffer_margin);
    tracks[track_widx].id3.elapsed = 0;

    /* Starting playback from an offset is only support in MPA at the moment */
    if (offset > 0) {
        switch (tracks[track_widx].id3.codectype) {
        case AFMT_MPA_L2:
        case AFMT_MPA_L3:
            lseek(fd, offset, SEEK_SET);
            tracks[track_widx].id3.offset = offset;
            mp3_set_elapsed(&tracks[track_widx].id3);
            tracks[track_widx].filepos = offset;
            tracks[track_widx].filerem = tracks[track_widx].filesize - offset;
            ci.curpos = offset;
            tracks[track_widx].start_pos = offset;
            break;

        case AFMT_WAVPACK:
            lseek(fd, offset, SEEK_SET);
            tracks[track_widx].id3.offset = offset;
            tracks[track_widx].id3.elapsed = tracks[track_widx].id3.length / 2;
            tracks[track_widx].filepos = offset;
            tracks[track_widx].filerem = tracks[track_widx].filesize - offset;
            ci.curpos = offset;
            tracks[track_widx].start_pos = offset;
            break;
        case AFMT_OGG_VORBIS:
        case AFMT_FLAC:
            tracks[track_widx].id3.offset = offset;
            break;
        }
    }
    
    if (start_play) {
        track_count++;
        codec_track_changed();
    }

    /* Do some initial file buffering. */
    mutex_lock(&mutex_bufferfill);
    i = tracks[track_widx].start_pos;
    size = MIN(size, AUDIO_FILL_CYCLE);
    while (i < size) {
        /* Give codecs some processing time to prevent glitches. */
        yield_codecs();
        
        if (fill_bytesleft == 0)
            break ;
        
        copy_n = MIN(conf_filechunk, filebuflen - buf_widx);
        copy_n = MIN(size - i, copy_n);
        copy_n = MIN((int)fill_bytesleft, copy_n);
        rc = read(fd, &filebuf[buf_widx], copy_n);
        if (rc < copy_n) {
            logf("File error!");
            tracks[track_widx].filesize = 0;
            tracks[track_widx].filerem = 0;
            close(fd);
            mutex_unlock(&mutex_bufferfill);
            return false;
        }
        buf_widx += rc;
        if (buf_widx >= filebuflen)
            buf_widx -= filebuflen;
        i += rc;
        tracks[track_widx].available += rc;
        tracks[track_widx].filerem -= rc;
        filebufused += rc;
        fill_bytesleft -= rc;
    }
    mutex_unlock(&mutex_bufferfill);
    
    if (!start_play)
        track_count++;
        
    tracks[track_widx].filepos = i;
    
    if (current_fd >= 0) {
        close(current_fd);
        current_fd = -1;
    }
    
    /* Leave the file handle open for faster buffer refill. */
    if (tracks[track_widx].filerem != 0) {
        current_fd = fd;
        logf("Partially buf:%d", tracks[track_widx].available);
    } else {
        logf("Completely buf.");
        close(fd);

        strip_id3v1_tag();
        
        if (++track_widx >= MAX_TRACK) {
            track_widx = 0;
        }
        tracks[track_widx].filerem = 0;
    }
    
    return true;
}

static void audio_clear_track_entries(bool buffered_only)
{
    int cur_idx, event_count;
    int i;
    
    cur_idx = track_widx;
    event_count = 0;
    for (i = 0; i < MAX_TRACK - track_count; i++) {
        if (++cur_idx >= MAX_TRACK)
            cur_idx = 0;

        if (tracks[cur_idx].event_sent)
            event_count++;
            
        if (!track_unbuffer_callback)
            memset(&tracks[cur_idx], 0, sizeof(struct track_info));
    }

    if (!track_unbuffer_callback)
        return ;
        
    cur_idx = track_widx;
    for (i = 0; i < MAX_TRACK - track_count; i++) {
        if (++cur_idx >= MAX_TRACK)
            cur_idx = 0;

        /* Send an event to notify that track has finished. */
        if (tracks[cur_idx].event_sent) {
            event_count--;
            track_unbuffer_callback(&tracks[cur_idx].id3, event_count == 0);
        }

        if (tracks[cur_idx].event_sent || !buffered_only)
            memset(&tracks[cur_idx], 0, sizeof(struct track_info));
    }
}

static void audio_stop_playback(bool resume)
{
    paused = false;
    if (playing)
        playlist_update_resume_info(resume ? audio_current_track() : NULL);
    playing = false;
    filling = false;
    ci.stop_codec = true;
    if (current_fd >= 0) {
        close(current_fd);
        current_fd = -1;
    }
    pcmbuf_play_stop();
    while (audio_codec_loaded)
        yield();
    
    track_count = 0;
    /* Mark all entries null. */
    audio_clear_track_entries(false);
}

static void audio_play_start(int offset)
{
    if (current_fd >= 0) {
        close(current_fd);
        current_fd = -1;
    }

    memset(&tracks, 0, sizeof(struct track_info) * MAX_TRACK);
    sound_set_volume(global_settings.volume);
    track_count = 0;
    track_widx = 0;
    track_ridx = 0;
    buf_ridx = 0;
    buf_widx = 0;
    filebufused = 0;
    pcmbuf_set_boost_mode(true);
    
    fill_bytesleft = filebuflen;
    filling = true;
    last_peek_offset = -1;
    
    if (audio_load_track(offset, true, 0)) {
        if (track_buffer_callback) {
            cur_ti->event_sent = true;
            track_buffer_callback(&cur_ti->id3, true);
        }
    } else {
        logf("Failure");
        audio_stop_playback(false);
    }

    pcmbuf_set_boost_mode(false);
}

/* Send callback events to notify about new tracks. */
static void generate_postbuffer_events(void)
{
    int i;
    int cur_ridx, event_count;
    
    /* At first determine how many unsent events we have. */
    cur_ridx = track_ridx;
    event_count = 0;
    for (i = 0; i < track_count; i++) {
        if (!tracks[cur_ridx].event_sent)
            event_count++;
        if (++cur_ridx >= MAX_TRACK)
            cur_ridx -= MAX_TRACK;
    }

    /* Now sent these events. */
    cur_ridx = track_ridx;
    for (i = 0; i < track_count; i++) {
        if (!tracks[cur_ridx].event_sent) {
            tracks[cur_ridx].event_sent = true;
            event_count--;
            /* We still want to set event_sent flags even if not using
               event callbacks. */
            if (track_buffer_callback)
                track_buffer_callback(&tracks[cur_ridx].id3, event_count == 0);
        }
        if (++cur_ridx >= MAX_TRACK)
            cur_ridx -= MAX_TRACK;
    }
}

static void initialize_buffer_fill(void)
{
    int cur_idx, i;
    
    /* Initialize only once; do not truncate the tracks. */
    if (filling)
        return ;

    /* Save the current resume position once. */
    playlist_update_resume_info(audio_current_track());
    
    fill_bytesleft = filebuflen - filebufused;
    cur_ti->start_pos = ci.curpos;

    pcmbuf_set_boost_mode(true);
    
    filling = true;
    
    /* Calculate real track count after throwing away old tracks. */
    cur_idx = track_ridx;
    for (i = 0; i < track_count; i++) {
        if (cur_idx == track_widx)
            break ;
            
        if (++cur_idx >= MAX_TRACK)
            cur_idx = 0;
    }
    
    track_count = i;
    if (tracks[track_widx].filesize == 0) {
        if (--track_widx < 0)
            track_widx = MAX_TRACK - 1;
    } else {
        track_count++;
    }

    /* Mark all buffered entries null (not metadata for next track). */
    audio_clear_track_entries(true);
}

static void audio_check_buffer(void)
{
    /* Start buffer filling as necessary. */
    if ((!conf_watermark || filebufused > conf_watermark
        || !queue_empty(&audio_queue) || !playing || ci.stop_codec
        || ci.reload_codec || playlist_end) && !filling)
        return ;
    
    mutex_lock(&mutex_bufferfill);
    initialize_buffer_fill();
    mutex_unlock(&mutex_bufferfill);
    
    /* Limit buffering size at first run. */
    if (conf_bufferlimit && fill_bytesleft > conf_bufferlimit
            - filebufused) {
        fill_bytesleft = MAX(0, conf_bufferlimit - filebufused);
    }
    
    /* Try to load remainings of the file. */
    if (tracks[track_widx].filerem > 0)
        audio_fill_file_buffer();
    
    /* Increase track write index as necessary. */
    if (tracks[track_widx].filerem == 0 && tracks[track_widx].filesize != 0) {
        if (++track_widx == MAX_TRACK)
            track_widx = 0;
    }
    
    /* Load new files to fill the entire buffer. */
    if (audio_load_track(0, false, last_peek_offset + 1)) {
        if (conf_bufferlimit)
            fill_bytesleft = 0;
    }
    else if (tracks[track_widx].filerem == 0)
        fill_bytesleft = 0;

    if (fill_bytesleft <= 0)
    {
        /* Read next unbuffered track's metadata as necessary. */
        read_next_metadata();
        
        generate_postbuffer_events();
        filling = false;
        conf_bufferlimit = 0;
        pcmbuf_set_boost_mode(false);

#ifndef SIMULATOR                    
        if (playing)
            ata_sleep();
#endif
    }
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
    if (pcmbuf_is_crossfade_enabled() && !pcmbuf_is_crossfade_active()) {
        pcmbuf_crossfade_init();
        codec_track_changed();
    } else {
        pcmbuf_add_event(codec_track_changed);
    }

    /* Manual track change. */
    if (new_track)
        codec_track_changed();
}

enum {
    SKIP_FAIL,
    SKIP_OK_DISK,
    SKIP_OK_RAM,
};

/* Should handle all situations. */
static int skip_next_track(void)
{
    logf("skip next");
    /* Manual track skipping. */
    if (new_track > 0)
        last_peek_offset--;
    
    /* Automatic track skipping. */
    else
    {
        if (!playlist_check(1)) {
            ci.reload_codec = false;
            return SKIP_FAIL;
        }
        last_peek_offset--;
        playlist_next(1);
    }
    
    if (++track_ridx >= MAX_TRACK)
        track_ridx = 0;
    
    /* Wait for new track data. */
    while (tracks[track_ridx].filesize == 0 && filling
           && !ci.stop_codec)
        yield();

    if (tracks[track_ridx].filesize <= 0)
    {
        logf("Loading from disk...");
        ci.reload_codec = true;
        queue_post(&audio_queue, AUDIO_PLAY, 0);
        return SKIP_OK_DISK;
    }
    
    buf_ridx += cur_ti->available;
    filebufused -= cur_ti->available;
    
    cur_ti = &tracks[track_ridx];
    buf_ridx += cur_ti->codecsize;
    filebufused -= cur_ti->codecsize;
    if (buf_ridx >= filebuflen)
        buf_ridx -= filebuflen;
    audio_update_trackinfo();
    
    if (!filling)
        pcmbuf_set_boost_mode(false);
    
    return SKIP_OK_RAM;
}

static int skip_previous_track(void)
{
    logf("skip previous");
    last_peek_offset++;
    if (--track_ridx < 0)
        track_ridx += MAX_TRACK;
    
    if (tracks[track_ridx].filesize == 0 || 
        filebufused+ci.curpos+tracks[track_ridx].filesize
        /*+ (off_t)tracks[track_ridx].codecsize*/ > filebuflen) {
        logf("Loading from disk...");
        ci.reload_codec = true;
        queue_post(&audio_queue, AUDIO_PLAY, 0);
        return SKIP_OK_DISK;
    }
    
    buf_ridx -= ci.curpos + cur_ti->codecsize;
    filebufused += ci.curpos + cur_ti->codecsize;
    cur_ti->available = cur_ti->filesize - cur_ti->filerem;
    
    cur_ti = &tracks[track_ridx];
    buf_ridx -= cur_ti->filesize;
    filebufused += cur_ti->filesize;
    cur_ti->available = cur_ti->filesize;
    if (buf_ridx < 0)
        buf_ridx += filebuflen;
    audio_update_trackinfo();
    
    return SKIP_OK_RAM;
}

/* Request the next track with new codec. */
static void audio_change_track(void)
{
    logf("change track");

    if (!ci.reload_codec)
    {
        if (skip_next_track() == SKIP_FAIL)
        {
            logf("No more tracks");
            while (pcm_is_playing())
            sleep(1);
            audio_stop_playback(false);
            return ;
        }
    }
    
    ci.reload_codec = false;
    /* Needed for fast skipping. */
    if (cur_ti->codecsize > 0)
        queue_post(&codec_queue, CODEC_LOAD, 0);
}

bool codec_request_next_track_callback(void)
{
    struct track_info *prev_ti = cur_ti;
    
    if (current_codec == CODEC_IDX_VOICE) {
        voice_remaining = 0;
        /* Terminate the codec if there are messages waiting on the queue or
           the core has been requested the codec to be terminated. */
        return !ci_voice.stop_codec && queue_empty(&voice_codec_queue);
    }
        
    if (ci.stop_codec || !playing)
        return false;
    
    logf("Request new track");

    /* Advance to next track. */
    if (new_track >= 0 || !ci.reload_codec) {
        if (skip_next_track() != SKIP_OK_RAM)
            return false;
    }
    
    /* Advance to previous track. */
    else  {
        if (skip_previous_track() != SKIP_OK_RAM)
            return false;
    }
    
    new_track = 0;
    ci.reload_codec = false;
    
    logf("On-the-fly change");
    
    /* Check if the next codec is the same file. */
    if (get_codec_base_type(prev_ti->id3.codectype) !=
        get_codec_base_type(cur_ti->id3.codectype))
    {
        logf("New codec:%d/%d", cur_ti->id3.codectype,
                tracks[track_ridx].id3.codectype);
        
        if (cur_ti->codecsize == 0)
        {
            logf("Loading from disk [2]...");
            queue_post(&audio_queue, AUDIO_PLAY, 0);
        }
        else
            ci.reload_codec = true;
        
        return false;
    }
        
    return true;    
}

/* Invalidates all but currently playing track. */
void audio_invalidate_tracks(void)
{
    if (track_count == 0) {
        /* This call doesn't seem necessary anymore. Uncomment it
           if things break */
        /* queue_post(&audio_queue, AUDIO_PLAY, 0); */
        return ;
    }
    
    playlist_end = false;
    track_count = 1;
    last_peek_offset = 0;
    track_widx = track_ridx;
    /* Mark all other entries null (also buffered wrong metadata). */
    audio_clear_track_entries(false);
    filebufused = cur_ti->available;
    buf_widx = buf_ridx + cur_ti->available;
    if (buf_widx >= filebuflen)
        buf_widx -= filebuflen;
    read_next_metadata();
}

static void initiate_track_change(int peek_index)
{
    /* Detect if disk is spinning or already loading. */
    if (filling || ci.reload_codec || !audio_codec_loaded) {
        queue_post(&audio_queue, AUDIO_PLAY, 0);
    } else {
        new_track = peek_index;
        ci.reload_codec = true;
        if (!pcmbuf_is_crossfade_enabled())
            pcmbuf_flush_audio();
    }

    codec_track_changed();
}

static void initiate_dir_change(int direction)
{
    if(!playlist_next_dir(direction))
        return;

    queue_post(&audio_queue, AUDIO_PLAY, 0);

    codec_track_changed();
}

void audio_thread(void)
{
    struct event ev;
    int last_tick = 0;
    bool play_pending = false;
    
    while (1) {
        if (!play_pending && queue_empty(&audio_queue))
        {
            yield_codecs();
            audio_check_buffer();
        }
        else
        {
            // ata_spin();
            sleep(1);
        }
        
        queue_wait_w_tmo(&audio_queue, &ev, 0);
        if (ev.id == SYS_TIMEOUT && play_pending)
        {
            ev.id = AUDIO_PLAY;
            ev.data = 0;
        }
        
        switch (ev.id) {
            case AUDIO_PLAY:
                /* Don't start playing immediately if user is skipping tracks
                 * fast to prevent UI lag. */
                track_count = 0;
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
            
                /* Do not start crossfading if audio is paused. */
                if (paused)
                    pcmbuf_play_stop();

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
                pcmbuf_crossfade_init();
                while (audio_codec_loaded)
                    yield();
                audio_play_start((int)ev.data);

                playlist_update_resume_info(audio_current_track());

                /* If there are no tracks in the playlist, then the playlist
                   was empty or none of the filenames were valid.  No point
                   in playing an empty playlist. */
                if (playlist_amount() == 0) {
                    audio_stop_playback(false);
                }
                break ;
                
            case AUDIO_STOP:
                audio_stop_playback(true);
                break ;
                
            case AUDIO_PAUSE:
                logf("audio_pause");
                /* We will pause the pcm playback in audiobuffer insert function
                   to prevent a loop inside the pcm buffer. */
                // pcm_play_pause(false);
                paused = true;
                break ;
                
            case AUDIO_RESUME:
                logf("audio_resume");
                pcm_play_pause(true);
                paused = false;
                break ;
            
            case AUDIO_NEXT:
                logf("audio_next");
                last_tick = current_tick;
                playlist_end = false;
                initiate_track_change(1);
                break ;
                
            case AUDIO_PREV:
                logf("audio_prev");
                last_tick = current_tick;
                playlist_end = false;
                initiate_track_change(-1);
                break;
                
            case AUDIO_FF_REWIND:
                if (!playing)
                    break ;
                pcmbuf_play_stop();
                ci.seek_time = (int)ev.data+1;
                break ;

            case AUDIO_DIR_NEXT:
                logf("audio_dir_next");
                playlist_end = false;
                if (global_settings.beep)
                    pcmbuf_beep(5000, 100, 2500*global_settings.beep);
                initiate_dir_change(1);
                break;
            
            case AUDIO_DIR_PREV:
                logf("audio_dir_prev");
                playlist_end = false;
                if (global_settings.beep)
                    pcmbuf_beep(5000, 100, 2500*global_settings.beep);
                initiate_dir_change(-1);
                break;

            case AUDIO_FLUSH:
                audio_invalidate_tracks();
                break ;
                
            case AUDIO_TRACK_CHANGED:
                if (track_changed_callback)
                    track_changed_callback(&cur_ti->id3);
                playlist_update_resume_info(audio_current_track());
                break ;
                
            case AUDIO_CODEC_DONE:
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
    long codecsize;
    int status;
    int wrap;
    
    while (1) {
        status = 0;
        queue_wait(&codec_queue, &ev);
        new_track = 0;
        
        switch (ev.id) {
            case CODEC_LOAD_DISK:
                ci.stop_codec = false;
                audio_codec_loaded = true;
                mutex_lock(&mutex_codecthread);
                current_codec = CODEC_IDX_AUDIO;
                status = codec_load_file((char *)ev.data, &ci);
                mutex_unlock(&mutex_codecthread);
                break ;
                
            case CODEC_LOAD:
                logf("Codec start");
                codecsize = cur_ti->codecsize;
                if (codecsize == 0) {
                    logf("Codec slot is empty!");
                    /* Wait for the pcm buffer to go empty */
                    while (pcm_is_playing())
                        yield();
                    audio_stop_playback(true);
                    break ;
                }
                
                ci.stop_codec = false;
                wrap = (int)&filebuf[filebuflen] - (int)cur_ti->codecbuf;
                audio_codec_loaded = true;
                mutex_lock(&mutex_codecthread);
                current_codec = CODEC_IDX_AUDIO;
                status = codec_load_ram(cur_ti->codecbuf,  codecsize,
                                        &filebuf[0], wrap, &ci);
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

        audio_codec_loaded = false;
        
        switch (ev.id) {
        case CODEC_LOAD_DISK:
        case CODEC_LOAD:
            if (status != CODEC_OK) {
                logf("Codec failure");
                // audio_stop_playback();
                ci.reload_codec = false;
                gui_syncsplash(HZ*2, true, "Codec failure");
            } else {
                logf("Codec finished");
            }
                
            if (playing && !ci.stop_codec)
                audio_change_track();
            
            // queue_post(&audio_queue, AUDIO_CODEC_DONE, (void *)status);
        }
    }
}

static void reset_buffer(void)
{
    filebuf = (char *)&audiobuf[MALLOC_BUFSIZE];
    filebuflen = audiobufend - audiobuf - pcmbuf_get_bufsize()
                  - PCMBUF_GUARD - MALLOC_BUFSIZE - GUARD_BUFSIZE;
                  
    if (talk_get_bufsize() && voice_codec_loaded)
    {
        filebuf = &filebuf[talk_get_bufsize()];
        filebuflen -= 2*CODEC_IRAM_SIZE + 2*CODEC_SIZE + talk_get_bufsize();
    }
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
            case CODEC_LOAD_DISK:
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
        if (current_codec != CODEC_IDX_VOICE)
            swap_codec();
        sleep(1);
    }

    if (!talk_get_bufsize())
        return ;
        
    logf("Starting voice codec");
    queue_post(&voice_codec_queue, CODEC_LOAD_DISK, (void *)CODEC_MPA_L3);
    while (!voice_codec_loaded)
        sleep(1);
}

struct mp3entry* audio_current_track(void)
{
    const char *filename;
    const char *p;
    static struct mp3entry temp_id3;
    
    if (track_count > 0 && cur_ti->taginfo_ready)
        return (struct mp3entry *)&cur_ti->id3;
    else {
        filename = playlist_peek(0);
        if (!filename)
            filename = "No file!";
        p = strrchr(filename, '/');
        if (!p)
            p = filename;
        else
            p++;
        
        memset(&temp_id3, 0, sizeof(struct mp3entry));
        strncpy(temp_id3.path, p, sizeof(temp_id3.path)-1);
        temp_id3.title = &temp_id3.path[0];

        return &temp_id3;
    }
}

struct mp3entry* audio_next_track(void)
{
    int next_idx = track_ridx + 1;
    
    if (track_count == 0)
        return NULL;
    
    if (next_idx >= MAX_TRACK)
        next_idx = 0;
        
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

void audio_play(int offset)
{
    logf("audio_play");
    queue_post(&audio_queue, AUDIO_PLAY, (void *)offset);
}

void audio_stop(void)
{
    logf("audio_stop");
    queue_post(&audio_queue, AUDIO_STOP, 0);
    while (playing || audio_codec_loaded)
        yield();
}

bool mp3_pause_done(void)
{
    return paused;
}

void audio_pause(void)
{
    queue_post(&audio_queue, AUDIO_PAUSE, 0);
}

void audio_resume(void)
{
    queue_post(&audio_queue, AUDIO_RESUME, 0);
}

void audio_next(void)
{
    /* Prevent UI lag and update the WPS immediately. */
    if (global_settings.beep)
        pcmbuf_beep(5000, 100, 2500*global_settings.beep);

    if (!playlist_check(1))
        return ;
    playlist_next(1);
    track_changed = true;
    
    /* Force WPS to update even if audio thread is blocked spinning. */
    if (mutex_bufferfill.locked)
        cur_ti->taginfo_ready = false;
    
    queue_post(&audio_queue, AUDIO_NEXT, 0);
}

void audio_prev(void)
{
    /* Prevent UI lag and update the WPS immediately. */
    if (global_settings.beep)
        pcmbuf_beep(5000, 100, 2500*global_settings.beep);

    if (!playlist_check(-1))
        return ;
    playlist_next(-1);
    track_changed = true;
    
    /* Force WPS to update even if audio thread is blocked spinning. */
    if (mutex_bufferfill.locked)
        cur_ti->taginfo_ready = false;
    
    queue_post(&audio_queue, AUDIO_PREV, 0);
}

void audio_next_dir(void)
{
    queue_post(&audio_queue, AUDIO_DIR_NEXT, 0);
}

void audio_prev_dir(void)
{
    queue_post(&audio_queue, AUDIO_DIR_PREV, 0);
}

void audio_ff_rewind(int newpos)
{
    logf("rewind: %d", newpos);
    queue_post(&audio_queue, AUDIO_FF_REWIND, (int *)newpos);
}

void audio_flush_and_reload_tracks(void)
{
    logf("flush & reload");
    queue_post(&audio_queue, AUDIO_FLUSH, 0);
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
            {
                if ( id3->offset < id3->toc[i] * (id3->filesize / 256) )
                {
                    break;
                }
            }
            
            i--;
            if (i < 0)
                i = 0;

            relpos = id3->toc[i];

            if (i < 99)
            {
                nextpos = id3->toc[i+1];
            }
            else
            {
                nextpos = 256; 
            }

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
        /* constant bitrate, use exact calculation */
        id3->elapsed = id3->offset / (id3->bitrate / 8);
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
    {
        return -1;
    }

    if (pos >= (int)(id3->filesize - id3->id3v1len))
    {
        /* Don't seek right to the end of the file so that we can
           transition properly to the next song */
        pos = id3->filesize - id3->id3v1len - 1;
    }
    else if (pos < (int)id3->first_frame_offset)
    {
        /* skip past id3v2 tag and other leading garbage */
        pos = id3->first_frame_offset;
    }
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
    long size;
    bool was_playing = playing;
    int offset = 0;
    int seconds = 1;

    if (!filebuf)
        return;     /* Audio buffers not yet set up */

    /* Store the track resume position */
    if (playing)
        offset = cur_ti->id3.offset;

    if (enable)
    {
        seconds = global_settings.crossfade_fade_out_delay
                + global_settings.crossfade_fade_out_duration;
    }
        
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

void audio_init(void)
{
    static bool voicetagtrue = true;

    logf("audio api init");
    pcm_init();

#if defined(HAVE_RECORDING) && !defined(SIMULATOR)
    /* Set the input multiplexer to Line In */
    pcm_rec_mux(0);
#endif
    
    filebufused = 0;
    filling = false;
    current_codec = CODEC_IDX_AUDIO;
    filebuf = (char *)&audiobuf[MALLOC_BUFSIZE];
    playing = false;
    audio_codec_loaded = false;
    voice_is_playing = false;
    paused = false;
    track_changed = false;
    current_fd = -1;
    track_buffer_callback = NULL;
    track_unbuffer_callback = NULL;
    track_changed_callback = NULL;
    /* Just to prevent cur_ti never be anything random. */
    cur_ti = &tracks[0];
    
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

    memcpy(&ci_voice, &ci, sizeof(struct codec_api));
    memset(&id3_voice, 0, sizeof(struct mp3entry));
    ci_voice.taginfo_ready = &voicetagtrue;
    ci_voice.id3 = &id3_voice;
    ci_voice.pcmbuf_insert = codec_pcmbuf_insert_callback;
    id3_voice.frequency = 11200;
    id3_voice.length = 1000000L;
    
    mutex_init(&mutex_bufferfill);
    mutex_init(&mutex_codecthread);
    
    queue_init(&audio_queue);
    queue_init(&codec_queue);
    queue_init(&voice_codec_queue);
    
    create_thread(codec_thread, codec_stack, sizeof(codec_stack),
                  codec_thread_name);
    create_thread(voice_codec_thread, voice_codec_stack,
                  sizeof(voice_codec_stack), voice_codec_thread_name);
    create_thread(audio_thread, audio_stack, sizeof(audio_stack),
                  audio_thread_name);

    /* Apply relevant settings */
    audio_set_buffer_margin(global_settings.buffer_margin);
    audio_set_crossfade(global_settings.crossfade);
}


