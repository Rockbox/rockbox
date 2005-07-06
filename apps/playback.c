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
#include "wps.h"
#include "wps-display.h"
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
#include "pcm_playback.h"
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

static volatile bool codec_loaded;
static volatile bool playing;
static volatile bool paused;

#define CODEC_VORBIS   "/.rockbox/codecs/vorbis.codec";
#define CODEC_MPA_L3   "/.rockbox/codecs/mpa.codec";
#define CODEC_FLAC     "/.rockbox/codecs/flac.codec";
#define CODEC_WAV      "/.rockbox/codecs/wav.codec";
#define CODEC_A52      "/.rockbox/codecs/a52.codec";
#define CODEC_MPC      "/.rockbox/codecs/mpc.codec";
#define CODEC_WAVPACK  "/.rockbox/codecs/wavpack.codec";

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

#define CODEC_LOAD       1
#define CODEC_LOAD_DISK  2

/* As defined in plugins/lib/xxx2wav.h */
#define MALLOC_BUFSIZE (512*1024)
#define GUARD_BUFSIZE  (8*1024)

/* TODO:
    Handle playlist_peek in mpeg.c
    Track changing
*/

extern bool audio_is_initialized;

/* Buffer control thread. */
static struct event_queue audio_queue;
static long audio_stack[(DEFAULT_STACK_SIZE + 0x1000)/sizeof(long)];
static const char audio_thread_name[] = "audio";

/* Codec thread. */
static struct event_queue codec_queue;
static long codec_stack[(DEFAULT_STACK_SIZE + 0x2500)/sizeof(long)] IDATA_ATTR;
static const char codec_thread_name[] = "codec";

static struct mutex mutex_bufferfill;

/* Is file buffer currently being refilled? */
static volatile bool filling;

/* Ring buffer where tracks and codecs are loaded. */
static char *codecbuf;

/* Total size of the ring buffer. */
int codecbuflen;

/* Bytes available in the buffer. */
int codecbufused;

/* Ring buffer read and write indexes. */
static volatile int buf_ridx;
static volatile int buf_widx;

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

/* Codec API including function callbacks. */
extern struct codec_api ci;

/* When we change a song and buffer is not in filling state, this
   variable keeps information about whether to go a next/previous track. */
static int new_track;

/* Callback function to call when current track has really changed. */
void (*track_changed_callback)(struct track_info *ti);
void (*track_buffer_callback)(struct mp3entry *id3, bool last_track);
void (*track_unbuffer_callback)(struct mp3entry *id3, bool last_track);

/* Configuration */
static int conf_bufferlimit;
static int conf_watermark;
static int conf_filechunk;

static bool v1first = false;

static void mp3_set_elapsed(struct mp3entry* id3);
int mp3_get_file_pos(void);

/* Simulator stubs. */
#ifdef SIMULATOR
bool pcm_insert_buffer(char *buf, long length)
{
    (void)buf;
    (void)length;
    
    return true;
}

void pcm_flush_buffer(long length)
{
    (void)length;
}


void* pcm_request_buffer(long length, long *realsize)
{
    (void)length;
    (void)realsize;
    
    return NULL;
}

void audiobuffer_add_event(void (*event_handler)(void))
{
    (void)event_handler;
}

unsigned int audiobuffer_get_latency()
{
    return 0;
}

void pcm_play_stop(void)
{
}

bool pcm_is_playing(void)
{
    return false;
}

bool pcm_is_crossfade_active(void)
{
    return false;
}

bool pcm_is_lowdata(void)
{
    return false;
}

void pcm_flush_audio(void)
{
}

bool pcm_crossfade_init(void)
{
    return false;
}

void pcm_set_boost_mode(bool state)
{
    (void)state;
}

bool pcm_is_crossfade_enabled(void)
{
    return false;
}

void pcm_play_pause(bool state)
{
    (void)state;
}

int ata_sleep(void)
{
    return 0;
}
#endif

bool codec_audiobuffer_insert_callback(char *buf, long length)
{
    char *dest;
    long realsize;
    int factor;
    int next_channel = 0;
    int processed_length;
    int mono = 0;
    
    /* If non-interleaved stereo mode. */
    if (dsp_config.stereo_mode == STEREO_NONINTERLEAVED)
        next_channel = length / 2;
    else if (dsp_config.stereo_mode == STEREO_MONO) {
        length *= 2;
        mono = 1;
    }
    
    if (dsp_config.sample_depth > 16) {
        length /= 2;
        factor = 1;
    } else {
        factor = 0;
    }
            
    while (length > 0) {
        /* Request a few extra bytes for resampling. */
        /* FIXME: Required extra bytes SHOULD be calculated. */
        while ((dest = pcm_request_buffer(length+16384, &realsize)) == NULL)
            yield();
        
        if (realsize < 16384) {
            pcm_flush_buffer(0);
            continue ;
        }
        
        realsize -= 16384;
        
        if (next_channel) {
            processed_length = dsp_process(dest, buf, realsize / 4) * 2;
            dsp_process(dest, buf + next_channel, realsize / 4);
        } else {
            processed_length = dsp_process(dest, buf, realsize >> (mono + 1));
        }
        pcm_flush_buffer(processed_length);
        length -= realsize;
        buf += realsize << (factor + mono);
    }
    
    return true;
}

bool codec_audiobuffer_insert_split_callback(void *ch1, void *ch2, 
                                             long length)
{
    char *dest;
    long realsize;
    int factor;
    int processed_length;
    
    /* non-interleaved stereo mode. */
    if (dsp_config.sample_depth > 16) {
        factor = 0;
    } else {
        length /= 2;
        factor = 1;
    }
    
    while (length > 0) {
        /* Request a few extra bytes for resampling. */
        while ((dest = pcm_request_buffer(length+4096, &realsize)) == NULL)
            yield();
        
        if (realsize < 4096) {
            pcm_flush_buffer(0);
            continue ;
        }
        
        realsize -= 4096;
        
        processed_length = dsp_process(dest, ch1, realsize / 4) * 2;
        dsp_process(dest, ch2, realsize / 4);
        pcm_flush_buffer(processed_length);
        length -= realsize;
        ch1 += realsize >> factor;
        ch2 += realsize >> factor;
    }
    
    return true;
}

void* get_codec_memory_callback(long *size)
{
    *size = MALLOC_BUFSIZE;
    return &audiobuf[0];
}

void codec_set_elapsed_callback(unsigned int value)
{
    unsigned int latency;

    if (ci.stop_codec)
        return ;
        
    latency = audiobuffer_get_latency();
    
    if (value < latency) {
        cur_ti->id3.elapsed = 0;
    } else if (value - latency > cur_ti->id3.elapsed 
            || value - latency < cur_ti->id3.elapsed - 2) {
        cur_ti->id3.elapsed = value - latency;
    }
}

long codec_filebuf_callback(void *ptr, long size)
{
    char *buf = (char *)ptr;
    int copy_n;
    int part_n;
    
    if (ci.stop_codec || !playing)
        return 0;
    
    copy_n = MIN((off_t)size, (off_t)cur_ti->available + cur_ti->filerem);
    
    while (copy_n > cur_ti->available) {
        yield();
        if (ci.stop_codec)
            return 0;
    }
    
    if (copy_n == 0)
        return 0;
    
    part_n = MIN(copy_n, codecbuflen - buf_ridx);
    memcpy(buf, &codecbuf[buf_ridx], part_n);
    if (part_n < copy_n) {
        memcpy(&buf[part_n], &codecbuf[0], copy_n - part_n);
    }
    
    buf_ridx += copy_n;
    if (buf_ridx >= codecbuflen)
        buf_ridx -= codecbuflen;
    ci.curpos += copy_n;
    cur_ti->available -= copy_n;
    codecbufused -= copy_n;
    
    return copy_n;
}

void* codec_request_buffer_callback(long *realsize, long reqsize)
{
    long part_n;
    
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
        if (ci.stop_codec) {
            *realsize = 0;
            return NULL;
        }
    }
    
    part_n = MIN((int)*realsize, codecbuflen - buf_ridx);
    if (part_n < *realsize) {
        part_n += GUARD_BUFSIZE;
        if (part_n < *realsize)
            *realsize = part_n;
        memcpy(&codecbuf[codecbuflen], &codecbuf[0], *realsize - 
            (codecbuflen - buf_ridx));
    }
    
    return (char *)&codecbuf[buf_ridx];
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
    codecbufused = 0;
    buf_ridx = buf_widx = 0;
    cur_ti->filerem = cur_ti->filesize - newpos;
    cur_ti->filepos = newpos;
    cur_ti->start_pos = newpos;
    ci.curpos = newpos;
    cur_ti->available = 0;
    lseek(current_fd, newpos, SEEK_SET);
    pcm_flush_audio();
        
    mutex_unlock(&mutex_bufferfill);

    while (cur_ti->available == 0 && cur_ti->filerem > 0) {
        yield();
        if (ci.stop_codec)
            return false;
    }
    
    return true;
}

void codec_advance_buffer_callback(long amount)
{
    if (amount > cur_ti->available + cur_ti->filerem)
        amount = cur_ti->available + cur_ti->filerem;
    
    if (amount > cur_ti->available) {
        if (!rebuffer_and_seek(ci.curpos + amount))
            ci.stop_codec = true;
        return ;
    }
    
    buf_ridx += amount;
    if (buf_ridx >= codecbuflen)
        buf_ridx -= codecbuflen;
    cur_ti->available -= amount;
    codecbufused -= amount;
    ci.curpos += amount;
    cur_ti->id3.offset = ci.curpos;
}

void codec_advance_buffer_loc_callback(void *ptr)
{
    long amount;
    
    amount = (int)ptr - (int)&codecbuf[buf_ridx];
    codec_advance_buffer_callback(amount);
}

off_t codec_mp3_get_filepos_callback(int newtime)
{
    off_t newpos;
    
    cur_ti->id3.elapsed = newtime;
    newpos = mp3_get_file_pos();
    
    return newpos;
}

bool codec_seek_buffer_callback(off_t newpos)
{
    int difference;
    
    if (newpos < 0)
        newpos = 0;
    
    if (newpos >= cur_ti->filesize)
        newpos = cur_ti->filesize - 1;
     
    difference = newpos - ci.curpos;
    /* Seeking forward */
    if (difference >= 0) {
        logf("seek: +%d", difference);
        codec_advance_buffer_callback(difference);
        if (!pcm_is_crossfade_active())
            pcm_play_stop();
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
    codecbufused += difference;
    cur_ti->available += difference;
    buf_ridx -= difference;
    if (buf_ridx < 0)
        buf_ridx = codecbuflen + buf_ridx;
    ci.curpos -= difference;
    if (!pcm_is_crossfade_active())
        pcm_play_stop();
    
    return true;
}

void codec_configure_callback(int setting, void *value)
{
    switch (setting) {
    case CODEC_SET_FILEBUF_WATERMARK:
        conf_watermark = (unsigned int)value;
        break;
        
    case CODEC_SET_FILEBUF_CHUNKSIZE:
        conf_filechunk = (unsigned int)value;
        break;
        
    case CODEC_SET_FILEBUF_LIMIT:
        conf_bufferlimit = (unsigned int)value;
        break;
        
    case CODEC_DSP_ENABLE:
        if ((bool)value)
            ci.audiobuffer_insert = codec_audiobuffer_insert_callback;
        else
            ci.audiobuffer_insert = pcm_insert_buffer;
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

void audio_set_track_changed_event(void (*handler)(struct track_info *ti))
{
    track_changed_callback = handler;
}

void codec_track_changed(void)
{
    track_changed = true;
    queue_post(&audio_queue, AUDIO_TRACK_CHANGED, 0);
}

/* Give codecs or file buffering the right amount of processing time
   to prevent pcm audio buffer from going empty. */
void yield_codecs(void)
{
    yield();
    if (!pcm_is_playing())
        sleep(5);
    while ((pcm_is_crossfade_active() || pcm_is_lowdata())
            && !ci.stop_codec && playing && queue_empty(&audio_queue)
            && codecbufused > (128*1024))
        yield();
}

void audio_fill_file_buffer(void)
{
    long i, size;
    int rc;

    if (current_fd < 0)
        return ;

    /* Throw away buffered codec. */
    if (tracks[track_widx].start_pos != 0)
        tracks[track_widx].codecsize = 0;
    
    i = 0;
    size = MIN(tracks[track_widx].filerem, AUDIO_FILL_CYCLE);
    while (i < size) {
        /* Give codecs some processing time. */
        yield_codecs();
            
        if (fill_bytesleft == 0)
            break ;
        rc = MIN(conf_filechunk, codecbuflen - buf_widx);
        rc = MIN(rc, (off_t)fill_bytesleft);
        rc = read(current_fd, &codecbuf[buf_widx], rc);
        if (rc <= 0) {
            tracks[track_widx].filerem = 0;
            break ;
        }
        
        buf_widx += rc;
        if (buf_widx >= codecbuflen)
            buf_widx -= codecbuflen;
        i += rc;
        tracks[track_widx].available += rc;
        codecbufused += rc;
        fill_bytesleft -= rc;
    }
    
    tracks[track_widx].filerem -= i;
    tracks[track_widx].filepos += i;
    /*logf("Filled:%d/%d", tracks[track_widx].available,
                       tracks[track_widx].filerem);*/
}

bool loadcodec(const char *trackname, bool start_play)
{
    char msgbuf[80];
    off_t size;
    int filetype;
    int fd;
    int i, rc;
    const char *codec_path;
    int copy_n;
    int prev_track;
    
    filetype = probe_file_format(trackname);
    switch (filetype) {
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
    default:
        logf("Codec: Unsupported");
        snprintf(msgbuf, sizeof(msgbuf)-1, "No codec for: %s", trackname);
        splash(HZ*2, true, msgbuf);
        codec_path = NULL;
    }
    
    tracks[track_widx].id3.codectype = filetype;
    tracks[track_widx].codecsize = 0;
    if (codec_path == NULL)
        return false;
    
    if (!start_play) {
        prev_track = track_widx - 1;
        if (prev_track < 0)
            prev_track = MAX_TRACK-1;
        if (track_count > 0 && filetype == tracks[prev_track].id3.codectype) {
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
        snprintf(msgbuf, sizeof(msgbuf)-1, "Couldn't load codec: %s", codec_path);
        splash(HZ*2, true, msgbuf);
        return false;
    }
    
    size = filesize(fd);
    if ((off_t)fill_bytesleft < size + conf_watermark) {
        logf("Not enough space");
        fill_bytesleft = 0;
        close(fd);
        return false;
    }
    
    i = 0;
    while (i < size) {
        yield_codecs();
        
        copy_n = MIN(conf_filechunk, codecbuflen - buf_widx);
        rc = read(fd, &codecbuf[buf_widx], copy_n);
        if (rc < 0)
            return false;
        buf_widx += rc;
        if (buf_widx >= codecbuflen)
            buf_widx -= codecbuflen;
        i += rc;
    }
    close(fd);
    logf("Done: %dB", i);
    
    codecbufused += size;
    fill_bytesleft -= size;
    tracks[track_widx].codecsize = size;
    
    return true;
}

bool read_next_metadata(void)
{
    int fd;
    char *trackname;
    int next_track;
    int status;

    trackname = playlist_peek(last_peek_offset);
    if (!trackname)
        return false;

    fd = open(trackname, O_RDONLY);
    if (fd < 0)
        return false;

    next_track = track_ridx + 1;
    if (next_track >= MAX_TRACK)
        next_track -= MAX_TRACK;

    /* Start buffer refilling also because we need to spin-up the disk. */
    filling = true;
    status = get_metadata(&tracks[next_track],fd,trackname,v1first);
    track_changed = true;
    close(fd);

    return status;
}

bool audio_load_track(int offset, bool start_play, int peek_offset)
{
    char *trackname;
    int fd;
    off_t size;
    int rc, i;
    int copy_n;

    /* Stop buffer filling if there is no free track entries. */
    if (track_count >= MAX_TRACK) {
        fill_bytesleft = 0;
        return false;
    }

    /* Don't start loading track if the current write position already
       contains a BUFFERED track. The entry may contain the metadata
       which is ok. */
    if (tracks[track_widx].filesize != 0)
        return false;

    /* Get track name from current playlist read position. */
    logf("Buffering track:%d/%d", track_widx, track_ridx);
    trackname = playlist_peek(peek_offset);
    if (!trackname) {
        logf("End-of-playlist");
        conf_watermark = 0;
        return false;
    }
    
    fd = open(trackname, O_RDONLY);
    if (fd < 0) {
        logf("Open failed");
        return false;
    }

    /* Initialize track entry. */
    size = filesize(fd);
    tracks[track_widx].filerem = size;
    tracks[track_widx].filesize = size;
    tracks[track_widx].filepos = 0;
    tracks[track_widx].available = 0;
    tracks[track_widx].taginfo_ready = false;
    tracks[track_widx].playlist_offset = offset;
    
    if (buf_widx >= codecbuflen)
        buf_widx -= codecbuflen;
    
    /* Set default values */
    if (start_play) {
        conf_bufferlimit = 0;
        conf_watermark = AUDIO_DEFAULT_WATERMARK;
        conf_filechunk = AUDIO_DEFAULT_FILECHUNK;
        dsp_configure(DSP_RESET, 0);
        ci.configure(CODEC_DSP_ENABLE, false);
    }

    /* Load the codec. */
    tracks[track_widx].codecbuf = &codecbuf[buf_widx];
    if (!loadcodec(trackname, start_play)) {
        close(fd);
        /* Stop buffer filling if codec load failed. */
        fill_bytesleft = 0;
        return false;
    }
    // tracks[track_widx].filebuf = &codecbuf[buf_widx];
    tracks[track_widx].start_pos = 0;
        
    /* Get track metadata if we don't already have it. */
    if (!tracks[track_widx].taginfo_ready) {
        if (!get_metadata(&tracks[track_widx],fd,trackname,v1first)) {
            close(fd);
            return false;
        }
    }
    tracks[track_widx].id3.elapsed = 0;

    /* Starting playback from an offset is only support in MPA at the moment */
    if (offset > 0) {
      if ((tracks[track_widx].id3.codectype==AFMT_MPA_L2) ||
          (tracks[track_widx].id3.codectype==AFMT_MPA_L3)) {
        lseek(fd, offset, SEEK_SET);
        tracks[track_widx].id3.offset = offset;
        mp3_set_elapsed(&tracks[track_widx].id3);
        tracks[track_widx].filepos = offset;
        tracks[track_widx].filerem = tracks[track_widx].filesize - offset;
        ci.curpos = offset;
        tracks[track_widx].start_pos = offset;
      }
      else if (tracks[track_widx].id3.codectype==AFMT_WAVPACK) {
        lseek(fd, offset, SEEK_SET);
        tracks[track_widx].id3.offset = offset;
        tracks[track_widx].id3.elapsed = tracks[track_widx].id3.length / 2;
        tracks[track_widx].filepos = offset;
        tracks[track_widx].filerem = tracks[track_widx].filesize - offset;
        ci.curpos = offset;
        tracks[track_widx].start_pos = offset;
      }
    } 
    
    if (start_play) {
        track_count++;
        audiobuffer_add_event(codec_track_changed);
    }

    /* Do some initial file buffering. */
    i = tracks[track_widx].start_pos;
    size = MIN(size, AUDIO_FILL_CYCLE);
    while (i < size) {
        /* Give codecs some processing time to prevent glitches. */
        yield_codecs();
        
        if (fill_bytesleft == 0)
            break ;
        
        copy_n = MIN(conf_filechunk, codecbuflen - buf_widx);
        copy_n = MIN(size - i, copy_n);
        copy_n = MIN((int)fill_bytesleft, copy_n);
        rc = read(fd, &codecbuf[buf_widx], copy_n);
        if (rc <= 0) {
            logf("File error!");
            close(fd);
            return false;
        }
        buf_widx += rc;
        if (buf_widx >= codecbuflen)
            buf_widx -= codecbuflen;
        i += rc;
        tracks[track_widx].available += rc;
        tracks[track_widx].filerem -= rc;
        codecbufused += rc;
        fill_bytesleft -= rc;
    }
    
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
        if (++track_widx >= MAX_TRACK) {
            track_widx = 0;
        }
        tracks[track_widx].filerem = 0;
    }
    
    return true;
}

void audio_play_start(int offset)
{
    if (current_fd >= 0) {
        close(current_fd);
        current_fd = -1;
    }
    
    memset(&tracks, 0, sizeof(struct track_info) * MAX_TRACK);
    sound_set(SOUND_VOLUME, global_settings.volume);
    track_count = 0;
    track_widx = 0;
    track_ridx = 0;
    buf_ridx = 0;
    buf_widx = 0;
    codecbufused = 0;
    pcm_set_boost_mode(true);
    
    fill_bytesleft = codecbuflen;
    filling = true;
    last_peek_offset = 0;
    if (audio_load_track(offset, true, 0)) {
        last_peek_offset++;
        if (track_buffer_callback) {
            cur_ti->event_sent = true;
            track_buffer_callback(&cur_ti->id3, true);
        }
        ata_sleep();
    } else {
        logf("Failure");
    }
    pcm_set_boost_mode(false);
}

void audio_clear_track_entries(void)
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
            tracks[cur_idx].event_sent = true;
            event_count--;
            track_unbuffer_callback(&tracks[cur_idx].id3, event_count == 0);
        }
        
        memset(&tracks[cur_idx], 0, sizeof(struct track_info));
    }
}

/* Send callback events to notify about new tracks. */
static void generate_postbuffer_events(void)
{
    int i;
    int cur_ridx, event_count;
    
    if (!track_buffer_callback)
        return ;

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
            track_buffer_callback(&tracks[cur_ridx].id3, event_count == 0);
        }
        if (++cur_ridx >= MAX_TRACK)
            cur_ridx -= MAX_TRACK;
    }
}

void initialize_buffer_fill(void)
{
    int cur_idx, i;
    
    
    fill_bytesleft = codecbuflen - codecbufused;
    cur_ti->start_pos = ci.curpos;

    if (filling)
        return ;

    filling = true;
    pcm_set_boost_mode(true);
    
    /* Calculate real track count after throwing away old tracks. */
    cur_idx = track_ridx;
    for (i = 0; i < track_count; i++) {
        if (cur_idx == track_widx)
            break ;
            
        if (++cur_idx >= MAX_TRACK)
            cur_idx = 0;
    }
    
    track_count = i;
    if (tracks[track_widx].filesize != 0)
        track_count++;
    
    /* Mark all other entries null. */
    audio_clear_track_entries();
}

void audio_check_buffer(void)
{
    /* Start buffer filling as necessary. */
    if ((codecbufused > conf_watermark || !queue_empty(&audio_queue) 
        || !playing || ci.stop_codec || ci.reload_codec) && !filling)
        return ;
    
    initialize_buffer_fill();
    
    /* Limit buffering size at first run. */
    if (conf_bufferlimit && (int)fill_bytesleft >= conf_bufferlimit) {
        fill_bytesleft = conf_bufferlimit - codecbufused;
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
    if (audio_load_track(0, false, last_peek_offset)) {
        last_peek_offset++;
    } else if (tracks[track_widx].filerem == 0 || fill_bytesleft == 0) {
        if (track_count <= 1)
            read_next_metadata();
            
        generate_postbuffer_events();
        filling = false;
        conf_bufferlimit = 0;
        pcm_set_boost_mode(false);
            
        if (playing)
            ata_sleep();
    }
}

void audio_update_trackinfo(void)
{
    if (new_track >= 0) {
        buf_ridx += cur_ti->available;
        codecbufused -= cur_ti->available;
        
        cur_ti = &tracks[track_ridx];
        buf_ridx += cur_ti->codecsize;
        codecbufused -= cur_ti->codecsize;
        if (buf_ridx >= codecbuflen)
            buf_ridx -= codecbuflen;
            
        if (!filling)
            pcm_set_boost_mode(false);
    } else {
        buf_ridx -= ci.curpos + cur_ti->codecsize;
        codecbufused += ci.curpos + cur_ti->codecsize;
        cur_ti->available = cur_ti->filesize;
        
        cur_ti = &tracks[track_ridx];
        buf_ridx -= cur_ti->filesize;
        codecbufused += cur_ti->filesize;
        cur_ti->available = cur_ti->filesize;
        if (buf_ridx < 0)
            buf_ridx = codecbuflen + buf_ridx;
    }
    
    ci.filesize = cur_ti->filesize;
    cur_ti->id3.elapsed = 0;
    cur_ti->id3.offset = 0;
    ci.id3 = (struct mp3entry *)&cur_ti->id3;
    ci.curpos = 0;
    cur_ti->start_pos = 0;
    ci.taginfo_ready = (bool *)&cur_ti->taginfo_ready;
    if (!pcm_crossfade_init())
        audiobuffer_add_event(codec_track_changed);
    else
        codec_track_changed();
}

static void audio_stop_playback(void)
{
    paused = false;
    playing = false;
    ci.stop_codec = true;
    if (current_fd >= 0) {
        close(current_fd);
        current_fd = -1;
    }
    pcm_play_stop();
    pcm_play_pause(true);
    track_count = 0;
    audio_clear_track_entries();
    filling = false;
}

/* Request the next track with new codec. */
void audio_change_track(void)
{
    logf("change track");
    
    /* Wait for new track data. */
    while (track_count <= 1 && filling)
        yield();

    /* If we are not filling, then it must be end-of-playlist. */
    if (track_count <= 1) {
        logf("No more tracks");
        while (pcm_is_playing())
            yield();
        audio_stop_playback();
        return ;
    }
    
    if (++track_ridx >= MAX_TRACK)
        track_ridx = 0;
    
    audio_update_trackinfo();
    queue_post(&codec_queue, CODEC_LOAD, 0);
}

bool codec_request_next_track_callback(void)
{
    if (ci.stop_codec || !playing || !queue_empty(&audio_queue))
        return false;
    
    logf("Request new track");
    
    /* Advance to next track. */
    if (ci.reload_codec && new_track > 0) {
        if (!playlist_check(1))
            return false;
        last_peek_offset--;
        playlist_next(1);
        if (++track_ridx == MAX_TRACK)
            track_ridx = 0;
            
        /* Wait for new track data (codectype 0 is invalid). When a correct
           codectype is set, we can assume that the filesize is correct. */
        while (tracks[track_ridx].id3.codectype == 0 && filling
               && !ci.stop_codec)
            yield();
            
        if (tracks[track_ridx].filesize == 0) {
            logf("Loading from disk...");
            new_track = 0;
            queue_post(&audio_queue, AUDIO_PLAY, 0);
            return false;
        }
    }
    
    /* Advance to previous track. */
    else if (ci.reload_codec && new_track < 0) {
        if (!playlist_check(-1))
            return false;
        last_peek_offset++;
        playlist_next(-1);
        if (--track_ridx < 0)
            track_ridx = MAX_TRACK-1;
        if (tracks[track_ridx].filesize == 0 || 
            codecbufused+ci.curpos+tracks[track_ridx].filesize
            /*+ (off_t)tracks[track_ridx].codecsize*/ > codecbuflen) {
            logf("Loading from disk...");
            new_track = 0;
            queue_post(&audio_queue, AUDIO_PLAY, 0);
            return false;
        }
    }
    
    /* Codec requested track change (next track). */
    else {
        if (!playlist_check(1))
            return false;
        last_peek_offset--;
        playlist_next(1);
        if (++track_ridx >= MAX_TRACK)
            track_ridx = 0;
        
        /* Wait for new track data (codectype 0 is invalid). When a correct
           codectype is set, we can assume that the filesize is correct. */
        while (tracks[track_ridx].id3.codectype == 0 && filling
               && !ci.stop_codec)
            yield();
            
        if (tracks[track_ridx].filesize == 0) {
            logf("No more tracks");
            new_track = 0;
            return false;
        }
    }
    
    ci.reload_codec = false;
    
    if (cur_ti->id3.codectype != tracks[track_ridx].id3.codectype) {
        if (--track_ridx < 0)
            track_ridx = MAX_TRACK-1;
        logf("New codec");
        new_track = 0;
        return false;
    }
    
    logf("On-the-fly change");
    audio_update_trackinfo();
    new_track = 0;
    
    return true;    
}

/* Invalidates all but currently playing track. */
void audio_invalidate_tracks(void)
{
    if (track_count == 0) {
        queue_post(&audio_queue, AUDIO_PLAY, 0);
        return ;
    }
    
    track_count = 1;
    last_peek_offset = 1;
    track_widx = track_ridx;
    audio_clear_track_entries();
    codecbufused = cur_ti->available;
    buf_widx = buf_ridx + cur_ti->available;
    if (buf_widx >= codecbuflen)
        buf_widx -= codecbuflen;
    read_next_metadata();
}

static void initiate_track_change(int peek_index)
{
    if (!playlist_check(peek_index))
        return ;
            
    new_track = peek_index;
    ci.reload_codec = true;
    
    /* Detect if disk is spinning.. */
    if (filling) {
        ci.stop_codec = true;
        playlist_next(peek_index);
        queue_post(&audio_queue, AUDIO_PLAY, 0);
    } 
    
    else if (!pcm_crossfade_init())
        pcm_flush_audio();
    else
        codec_track_changed();
}

void audio_thread(void)
{
    struct event ev;
    
    while (1) {
        yield_codecs();

        mutex_lock(&mutex_bufferfill);
        audio_check_buffer();
        mutex_unlock(&mutex_bufferfill);
        
        queue_wait_w_tmo(&audio_queue, &ev, 0);
        switch (ev.id) {
            case AUDIO_PLAY:
                logf("starting...");
                playing = true;
                ci.stop_codec = true;
                ci.reload_codec = false;
                ci.seek_time = 0;
                pcm_crossfade_init();
                audio_play_start((int)ev.data);
                playlist_update_resume_info(audio_current_track());
                break ;
                
            case AUDIO_STOP:
                if (playing)
                    playlist_update_resume_info(audio_current_track());
                audio_stop_playback();
                break ;
                
            case AUDIO_PAUSE:
                logf("audio_pause");
                pcm_play_pause(false);
                paused = true;
                break ;
                
            case AUDIO_RESUME:
                logf("audio_resume");
                pcm_play_pause(true);
                paused = false;
                break ;
            
            case AUDIO_NEXT:
                logf("audio_next");
                initiate_track_change(1);
                break ;
                
            case AUDIO_PREV:
                logf("audio_prev");
                initiate_track_change(-1);
                break;
                
            case AUDIO_FLUSH:
                audio_invalidate_tracks();
                break ;
                
            case AUDIO_TRACK_CHANGED:
                if (track_changed_callback)
                    track_changed_callback(cur_ti);
                playlist_update_resume_info(audio_current_track());
                break ;
                
            case AUDIO_CODEC_DONE:
                //if (playing)
                //    audio_change_track();
                break ;

#ifndef SIMULATOR                
            case SYS_USB_CONNECTED:
                logf("USB Connection");
                audio_stop_playback();
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&audio_queue);
                break ;
#endif
            case SYS_TIMEOUT:
                if (playing)
                    playlist_update_resume_info(audio_current_track());
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
        switch (ev.id) {
            case CODEC_LOAD_DISK:
                ci.stop_codec = false;
                codec_loaded = true;
                status = codec_load_file((char *)ev.data);
                break ;
                
            case CODEC_LOAD:
                logf("Codec start");
                codecsize = cur_ti->codecsize;
                if (codecsize == 0) {
                    logf("Codec slot is empty!");
                    audio_stop_playback();
                    break ;
                }
                
                ci.stop_codec = false;
                wrap = (int)&codecbuf[codecbuflen] - (int)cur_ti->codecbuf;
                codec_loaded = true;
                status = codec_load_ram(cur_ti->codecbuf,  codecsize, 
                                        &codecbuf[0], wrap);
                break ;

#ifndef SIMULATOR                
            case SYS_USB_CONNECTED:
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&codec_queue);
                break ;
#endif
        }

        codec_loaded = false;
        
        switch (ev.id) {
        case CODEC_LOAD_DISK:
        case CODEC_LOAD:
            if (status != CODEC_OK) {
                logf("Codec failure");
                splash(HZ*2, true, "Codec failure");
                audio_stop_playback();
            } else {
                logf("Codec finished");
            }
                
            if (playing && !ci.stop_codec && !ci.reload_codec) {
                audio_change_track();
                continue ;
            } else if (ci.stop_codec) {
                //playing = false;
            }
            //queue_post(&audio_queue, AUDIO_CODEC_DONE, (void *)status);
        }
    }
}

struct mp3entry* audio_current_track(void)
{
    // logf("audio_current_track");
    
    if (track_count > 0 && cur_ti->taginfo_ready)
        return (struct mp3entry *)&cur_ti->id3;
    else
        return NULL;
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
        
    //logf("audio_next_track");
    
    return &tracks[next_idx].id3;
}

bool audio_has_changed_track(void)
{
    if (track_changed && track_count > 0 && playing) {
        if (!cur_ti->taginfo_ready)
            return false;
        track_changed = false;
        return true;
    }
    
    return false;
}

void audio_play(int offset)
{
    logf("audio_play");
    ci.stop_codec = true;
    if (!pcm_crossfade_init())
        pcm_flush_audio();
    else
        codec_track_changed();
        
    pcm_play_pause(true);
    paused = false;
    queue_post(&audio_queue, AUDIO_PLAY, (void *)offset);
}

void audio_stop(void)
{
    logf("audio_stop");
    queue_post(&audio_queue, AUDIO_STOP, 0);
    while (playing || codec_loaded)
        yield();
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
    queue_post(&audio_queue, AUDIO_NEXT, 0);
}

void audio_prev(void)
{
    queue_post(&audio_queue, AUDIO_PREV, 0);
}

void audio_ff_rewind(int newpos)
{
    int counter;
    
    logf("rewind: %d", newpos);
    /* Keep playback paused until seek is complete. */
    if (playing) {
        ci.seek_time = newpos+1;
        counter = 30;
        pcm_flush_audio();
        while (ci.seek_time) {
            sleep(10);
            if (counter-- == 0)
                break ;
        }
        pcm_play_stop();
    }
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
                if ( id3->offset < (int)(id3->toc[i] * (id3->filesize / 256)) )
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
        /* constant bitrate == simple frame calculation */
        id3->elapsed = id3->offset / id3->bpf * id3->tpf;
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
    else if (id3->bpf && id3->tpf)
        pos = (id3->elapsed/id3->tpf)*id3->bpf;
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

#ifndef SIMULATOR
void audio_set_buffer_margin(int seconds)
{
    (void)seconds;
    logf("bufmargin: %d", seconds);
}
#endif

void mpeg_id3_options(bool _v1first)
{
   v1first = _v1first;
}

void test_buffer_event(struct mp3entry *id3, bool last_track)
{
    (void)id3;
    (void)last_track;
    
    logf("be:%d%s", last_track, id3->title);
}

void test_unbuffer_event(struct mp3entry *id3, bool last_track)
{
    (void)id3;
    (void)last_track;
    
    logf("ube:%d%s", last_track, id3->title);
}

void audio_init(void)
{
    logf("audio api init");
    codecbuflen = audiobufend - audiobuf - PCMBUF_SIZE - PCMBUF_GUARD
                  - MALLOC_BUFSIZE - GUARD_BUFSIZE;
    //codecbuflen = 2*512*1024;
    codecbufused = 0;
    filling = false;
    codecbuf = &audiobuf[MALLOC_BUFSIZE];
    playing = false;
    codec_loaded = false;
    paused = false;
    track_changed = false;
    current_fd = -1;
    track_buffer_callback = NULL;
    track_unbuffer_callback = NULL;
    track_changed_callback = NULL;
    
    logf("abuf:%0x", PCMBUF_SIZE);
    logf("fbuf:%0x", codecbuflen);
    logf("mbuf:%0x", MALLOC_BUFSIZE);

    audio_set_track_buffer_event(test_buffer_event);
    audio_set_track_unbuffer_event(test_unbuffer_event);
    
    /* Initialize codec api. */    
    ci.read_filebuf = codec_filebuf_callback;
    ci.audiobuffer_insert = pcm_insert_buffer;
    ci.audiobuffer_insert_split = codec_audiobuffer_insert_split_callback;
    ci.get_codec_memory = get_codec_memory_callback;
    ci.request_buffer = codec_request_buffer_callback;
    ci.advance_buffer = codec_advance_buffer_callback;
    ci.advance_buffer_loc = codec_advance_buffer_loc_callback;
    ci.request_next_track = codec_request_next_track_callback;
    ci.mp3_get_filepos = codec_mp3_get_filepos_callback;
    ci.seek_buffer = codec_seek_buffer_callback;
    ci.set_elapsed = codec_set_elapsed_callback;
    ci.configure = codec_configure_callback;

    mutex_init(&mutex_bufferfill);
    queue_init(&audio_queue);
    queue_init(&codec_queue);
    
    create_thread(codec_thread, codec_stack, sizeof(codec_stack),
                  codec_thread_name);
    create_thread(audio_thread, audio_stack, sizeof(audio_stack),
                  audio_thread_name);
#ifndef SIMULATOR
    audio_is_initialized = true;
#endif
}


