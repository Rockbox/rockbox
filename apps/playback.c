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
#include "plugin.h"
#include "wps.h"
#include "wps-display.h"
#include "audio.h"
#include "logf.h"
#include "mp3_playback.h"
#include "mp3data.h"
#include "usb.h"
#include "status.h"
#include "main_menu.h"
#include "ata.h"
#include "screens.h"
#include "playlist.h"
#include "playback.h"
#include "pcm_playback.h"
#include "buffer.h"
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

static volatile bool playing;
static volatile bool paused;

#define CODEC_VORBIS   "/.rockbox/codecs/codecvorbis.rock";
#define CODEC_MPA_L3   "/.rockbox/codecs/codecmpa.rock";
#define CODEC_FLAC     "/.rockbox/codecs/codecflac.rock";
#define CODEC_WAV      "/.rockbox/codecs/codecwav.rock";
#define CODEC_A52      "/.rockbox/codecs/codeca52.rock";
#define CODEC_MPC      "/.rockbox/codecs/codecmpc.rock";
#define CODEC_WAVPACK  "/.rockbox/codecs/codecwavpack.rock";

#define AUDIO_DEFAULT_WATERMARK      (1024*256)
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
static long codec_stack[(DEFAULT_STACK_SIZE + 0x2500)/sizeof(long)] __attribute__ ((section(".idata")));
static const char codec_thread_name[] = "codec";

/* Is file buffer currently being refilled? */
static volatile bool filling;

/* Ring buffer where tracks and codecs are loaded. */
char *codecbuf;

/* Total size of the ring buffer. */
int codecbuflen;

/* Bytes available in the buffer. */
static volatile int codecbufused;

/* Ring buffer read and write indexes. */
static volatile int buf_ridx;
static volatile int buf_widx;

/* Track information (count in file buffer, read/write indexes for
   track ring structure. */
static int track_count;
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
static volatile struct track_info *cur_ti;

/* Codec API including function callbacks. */
static struct codec_api ci;

/* When we change a song and buffer is not in filling state, this
   variable keeps information about whether to go a next/previous track. */
static int new_track;

/* Configuration */
static int conf_bufferlimit;
static int conf_watermark;
static int conf_filechunk;

static bool v1first = false;

static void mp3_set_elapsed(struct mp3entry* id3);
int mp3_get_file_pos(void);

#ifdef SIMULATOR
bool audiobuffer_insert_sim(char *buf, size_t length)
{
    (void)buf;
    (void)length;
    
    return true;
}
#endif

void* get_codec_memory_callback(size_t *size)
{
    *size = MALLOC_BUFSIZE;
    return &audiobuf[0];
}

void codec_set_elapsed_callback(unsigned int value)
{
    unsigned int latency;

#ifndef SIMULATOR
    latency = audiobuffer_get_latency();
#else
    latency = 0;
#endif
    if (value < latency) {
        cur_ti->id3.elapsed = 0;
    } else if (value - latency > cur_ti->id3.elapsed 
            || value - latency < cur_ti->id3.elapsed - 2) {
        cur_ti->id3.elapsed = value - latency;
    }
}

size_t codec_filebuf_callback(void *ptr, size_t size)
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

void* codec_request_buffer_callback(size_t *realsize, size_t reqsize)
{
    size_t part_n;
    
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

void codec_advance_buffer_callback(size_t amount)
{
    if ((int)amount > cur_ti->available + cur_ti->filerem)
        amount = cur_ti->available + cur_ti->filerem;
    
    try_again:
    if ((int)amount > cur_ti->available) {
        if (filling) {
            while (filling)
                yield();
            goto try_again;
        }
        codecbufused = 0;
        buf_ridx = 0;
        buf_widx = 0;
        cur_ti->start_pos = ci.curpos + amount;
        amount -= cur_ti->available;
        ci.curpos += cur_ti->available;
        cur_ti->available = 0;
        while ((int)amount > cur_ti->available && !ci.stop_codec)
            yield();
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
    size_t amount;
    
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
    if (difference >= 0) {
        logf("seek: +%d", difference);
        codec_advance_buffer_callback(difference);
#ifndef SIMULATOR
        pcm_play_stop();
#endif
        return true;
    }
    
    difference = -difference;
    if (ci.curpos - difference < 0)
        difference = ci.curpos;
    
    if (ci.curpos - difference < cur_ti->start_pos) {
        logf("Seek failed (reload song)");
        /* We need to reload the song. FIX THIS! */
        return false;
    }
    
    logf("seek: -%d", difference);
    codecbufused += difference;
    cur_ti->available += difference;
    buf_ridx -= difference;
    if (buf_ridx < 0)
        buf_ridx = codecbuflen + buf_ridx;
    ci.curpos -= difference;
#ifndef SIMULATOR
    pcm_play_stop();
#endif
    
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
        
    default:
        logf("Illegal key: %d", setting);
    }
}

void yield_codecs(void)
{
    yield();
#ifndef SIMULATOR
    if (!pcm_is_playing())
        sleep(5);
    while (pcm_is_lowdata() && !ci.stop_codec && 
           playing && queue_empty(&audio_queue))
        yield();
#endif
}

void audio_fill_file_buffer(void)
{
    size_t i;
    int rc;
    
    tracks[track_widx].start_pos = ci.curpos;
    logf("Filling buffer...");
    i = 0;
    while ((off_t)i < tracks[track_widx].filerem) {
        /* Give codecs some processing time. */
        yield_codecs();
            
        if (!queue_empty(&audio_queue)) {
            logf("Filling interrupted");
            return ;
        }
        
        if (fill_bytesleft < MIN((unsigned int)conf_filechunk, 
                                 tracks[track_widx].filerem - i))
            break ;
        rc = MIN(conf_filechunk, codecbuflen - buf_widx);
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
        fill_bytesleft -= rc;
    }
    
    codecbufused += i;
    tracks[track_widx].filerem -= i;
    tracks[track_widx].filepos += i;
    logf("Done:%d", tracks[track_widx].available);
    if (tracks[track_widx].filerem == 0) {
        close(current_fd);
        current_fd = -1;
    }
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
    case AFMT_MPA_L2:
    case AFMT_MPA_L3:
        logf("Codec: MPA L2/L3");
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
    
    tracks[track_widx].codectype = filetype;
    tracks[track_widx].codecsize = 0;
    if (codec_path == NULL)
        return false;
    
    if (!start_play) {
        prev_track = track_widx - 1;
        if (prev_track < 0)
            prev_track = MAX_TRACK-1;
        if (track_count > 0 && filetype == tracks[prev_track].codectype) {
            logf("Reusing prev. codec");
            return true;
        }
    } else {
        /* Load the codec directly from disk and save some memory. */
        cur_ti = &tracks[track_widx];
        ci.filesize = cur_ti->filesize;
        ci.id3 = (struct mp3entry *)&cur_ti->id3;
        ci.mp3data = (struct mp3info *)&cur_ti->mp3data;
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
        close(fd);
        return false;
    }
    
    i = 0;
    while (i < size) {
        yield_codecs();
        if (!queue_empty(&audio_queue)) {
            logf("Buffering interrupted");
            close(fd);
            return false;
        }
        
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

bool audio_load_track(int offset, bool start_play, int peek_offset)
{
    char *trackname;
    int fd;
    off_t size;
    int rc, i;
    int copy_n;
    
    if (track_count >= MAX_TRACK)
        return false;
        
    trackname = playlist_peek(peek_offset);
    if (!trackname) {
        return false;
    }
    
    fd = open(trackname, O_RDONLY);
    if (fd < 0)
        return false;
    
    size = filesize(fd);
    tracks[track_widx].filerem = size;
    tracks[track_widx].filesize = size;
    tracks[track_widx].filepos = 0;
    tracks[track_widx].available = 0;
    tracks[track_widx].taginfo_ready = false;
    tracks[track_widx].playlist_offset = offset;
    
    /* Load the codec */
    if (buf_widx >= codecbuflen)
        buf_widx -= codecbuflen;
    
    /* Set default values */
    if (start_play) {
        conf_bufferlimit = 0;
        conf_watermark = AUDIO_DEFAULT_WATERMARK;
        conf_filechunk = AUDIO_DEFAULT_FILECHUNK;
    }
    
    tracks[track_widx].codecbuf = &codecbuf[buf_widx];
    if (!loadcodec(trackname, start_play)) {
        close(fd);
        return false;
    }
    // tracks[track_widx].filebuf = &codecbuf[buf_widx];
    tracks[track_widx].start_pos = 0;
        
    //logf("%s", trackname);
    logf("Buffering track:%d/%d", track_widx, track_ridx);
    
    if (!queue_empty(&audio_queue)) {
        logf("Interrupted!");
        //ci.stop_codec = true;
        close(fd);
        return false;
    }

    if (!get_metadata(&tracks[track_widx],fd,trackname,v1first)) {
      close(fd);
      return false;
    }

    /* Starting playback from an offset is only support in MPA at the moment */
    if (offset > 0) {
      if ((tracks[track_widx].codectype==AFMT_MPA_L2) ||
          (tracks[track_widx].codectype==AFMT_MPA_L3)) {
        lseek(fd, offset, SEEK_SET);
        tracks[track_widx].id3.offset = offset;
        mp3_set_elapsed(&tracks[track_widx].id3);
        tracks[track_widx].filepos = offset;
        tracks[track_widx].filerem = tracks[track_widx].filesize - offset;
        ci.curpos = offset;
        tracks[track_widx].start_pos = offset;
      }
    } 
    
    if (start_play) {
        track_count++;
        track_changed = true;
    }
        
    i = tracks[track_widx].start_pos;
    while (i < size) {
        /* Give codecs some processing time to prevent glitches. */
        yield_codecs();
        
        /* Limit buffering size at first run. */
        if (conf_bufferlimit && (int)fill_bytesleft >= conf_bufferlimit) {
            fill_bytesleft = conf_bufferlimit;
            conf_bufferlimit = 0;
        }
        
        if (!queue_empty(&audio_queue)) {
            logf("Buffering interrupted");
            close(fd);
            return false;
        }
        
        if (fill_bytesleft == 0)
            break ;
        
        copy_n = MIN(conf_filechunk, codecbuflen - buf_widx);
        copy_n = MIN(size - i, copy_n);
        copy_n = MIN((int)fill_bytesleft, copy_n);
        rc = read(fd, &codecbuf[buf_widx], copy_n);
        if (rc < 0) {
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
        return false;
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

void audio_insert_tracks(int offset, bool start_playing, int peek_offset)
{
    fill_bytesleft = codecbuflen - codecbufused;
    filling = true;
    while (audio_load_track(offset, start_playing, peek_offset)) {
        start_playing = false;
        offset = 0;
        peek_offset++;
    }
    filling = false;
}

void audio_play_start(int offset)
{
    memset(&tracks, 0, sizeof(struct track_info) * MAX_TRACK);
    sound_set(SOUND_VOLUME, global_settings.volume);
    track_count = 0;
    track_widx = 0;
    track_ridx = 0;
    buf_ridx = 0;
    buf_widx = 0;
    codecbufused = 0;
#ifndef SIMULATOR
    pcm_set_boost_mode(true);
#endif
    audio_insert_tracks(offset, true, 0);
#ifndef SIMULATOR
    pcm_set_boost_mode(false);
    ata_sleep();
#endif
}

void audio_clear_track_entries(void)
{
    int cur_idx;
    int i;
    
    cur_idx = track_widx;
    for (i = 0; i < MAX_TRACK - track_count; i++) {
        if (++cur_idx >= MAX_TRACK)
            cur_idx = 0;
        memset(&tracks[cur_idx], 0, sizeof(struct track_info));
    }
}

void audio_check_buffer(void)
{
    int i;
    int cur_idx;
    
    /* Fill buffer as full as possible for cross-fader. */
#ifndef SIMULATOR
    if (pcm_is_crossfade_enabled() && cur_ti->id3.length > 0
        && cur_ti->id3.length - cur_ti->id3.elapsed < 20000 && playing)
        pcm_set_boost_mode(true);
#endif
    
    /* Start buffer filling as necessary. */
    if (codecbufused > conf_watermark || !queue_empty(&audio_queue) 
        || !playing || ci.stop_codec || ci.reload_codec)
        return ;
    
    filling = true;
#ifndef SIMULATOR
    pcm_set_boost_mode(true);
#endif
    
    fill_bytesleft = codecbuflen - codecbufused;
    
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
    
    /* Try to load remainings of the file. */
    if (tracks[track_widx].filerem > 0)
        audio_fill_file_buffer();
    
    /* Increase track write index as necessary. */
    if (tracks[track_widx].filerem == 0 && tracks[track_widx].filesize != 0) {
        if (++track_widx == MAX_TRACK)
            track_widx = 0;
        logf("new ti: %d", track_widx);
    }
    
    logf("ti: %d", track_widx);
    /* Load new files to fill the entire buffer. */
    if (tracks[track_widx].filerem == 0)
        audio_insert_tracks(0, false, 1);

#ifndef SIMULATOR    
    pcm_set_boost_mode(false);
    if (playing)
        ata_sleep();
#endif
    filling = false;
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
#ifndef SIMULATOR        
        pcm_crossfade_start();
        if (!filling)
            pcm_set_boost_mode(false);
#endif
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
    ci.mp3data = (struct mp3info *)&cur_ti->mp3data;
    ci.curpos = 0;
    ci.taginfo_ready = (bool *)&cur_ti->taginfo_ready;
    track_changed = true;
}

void audio_change_track(void)
{
    if (track_ridx == track_widx) {
        logf("No more tracks");
        playing = false;
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
        playlist_next(1);
        if (++track_ridx == MAX_TRACK)
            track_ridx = 0;
        if (tracks[track_ridx].filesize == 0) {
            logf("Loading from disk...");
            new_track = 0;
            queue_post(&audio_queue, AUDIO_PLAY, 0);
            return false;
        }
    }
    
    /* Advance to previous track. */
    else if (ci.reload_codec && new_track < 0) {
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
        playlist_next(1);
        if (++track_ridx >= MAX_TRACK)
            track_ridx = 0;
        
        if (track_ridx == track_widx && tracks[track_ridx].filerem == 0) {
            logf("No more tracks");
            new_track = 0;
            return false;
        }
    }
    
    ci.reload_codec = false;
    
    if (cur_ti->codectype != tracks[track_ridx].codectype) {
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
    track_widx = track_ridx;
    audio_clear_track_entries();
    codecbufused = cur_ti->available;
    buf_widx = buf_ridx + cur_ti->available;
    if (buf_widx >= codecbuflen)
        buf_widx -= codecbuflen;
}

void audio_thread(void)
{
    struct event ev;
    
    while (1) {
        audio_check_buffer();
        queue_wait_w_tmo(&audio_queue, &ev, 100);
        switch (ev.id) {
            case AUDIO_PLAY:
                ci.stop_codec = true;
                ci.reload_codec = false;
                ci.seek_time = 0;
#ifndef SIMULATOR
                pcm_play_stop();
#endif
                audio_play_start((int)ev.data);
                break ;
                
            case AUDIO_STOP:
                paused = false;
#ifndef SIMULATOR
                pcm_play_stop();
                pcm_play_pause(true);
#endif
                break ;
                
            case AUDIO_PAUSE:
                break ;
                
            case AUDIO_RESUME:
                break ;
            
            case AUDIO_NEXT:
                break ;
                
            case AUDIO_FLUSH:
                audio_invalidate_tracks();
                break ;
                
            case AUDIO_CODEC_DONE:
                //if (playing)
                //    audio_change_track();
                break ;

#ifndef SIMULATOR                
            case SYS_USB_CONNECTED:
                playing = false;
                ci.stop_codec = true;
                logf("USB Connection");
                pcm_play_stop();
                pcm_play_pause(true);
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&audio_queue);
                break ;
#endif
        }
    }
}

void codec_thread(void)
{
    struct event ev;
    size_t codecsize;
    int status;
    int wrap;
    
    while (1) {
        status = 0;
        queue_wait(&codec_queue, &ev);
        switch (ev.id) {
            case CODEC_LOAD_DISK:
                ci.stop_codec = false;
                status = codec_load_file((char *)ev.data, &ci);
                break ;
                
            case CODEC_LOAD:
                logf("Codec start");
                codecsize = cur_ti->codecsize;
                if (codecsize == 0) {
                    logf("Codec slot is empty!");
                    playing = false;
                    break ;
                }
                
                ci.stop_codec = false;
                wrap = (int)&codecbuf[codecbuflen] - (int)cur_ti->codecbuf;
                status = codec_load_ram(cur_ti->codecbuf,  codecsize, 
                                        &ci, &codecbuf[0], wrap);
                break ;

#ifndef SIMULATOR                
            case SYS_USB_CONNECTED:
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&codec_queue);
                break ;
#endif
        }
        
        switch (ev.id) {
        case CODEC_LOAD_DISK:
        case CODEC_LOAD:
            if (status != PLUGIN_OK) {
                logf("Codec failure");
                splash(HZ*2, true, "Codec failure");
                playing = false;
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
    logf("audio_current_track");
    
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
    if (track_changed && track_count > 0) {
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
#ifndef SIMULATOR
    pcm_play_pause(true);
#endif
    paused = false;
    playing = true;
    queue_post(&audio_queue, AUDIO_PLAY, (void *)offset);
}

void audio_stop(void)
{
    logf("audio_stop");
    playing = false;
    paused = false;
    ci.stop_codec = true;
    if (current_fd >= 0) {
        close(current_fd);
        current_fd = -1;
    }
    queue_post(&audio_queue, AUDIO_STOP, 0);
#ifndef SIMULATOR
    pcm_play_pause(true);
#endif
}

void audio_pause(void)
{
    logf("audio_pause");
#ifndef SIMULATOR
    pcm_play_pause(false);
#endif
    paused = true;
    //queue_post(&audio_queue, AUDIO_PAUSE, 0);
}

void audio_resume(void)
{
    logf("audio_resume");
#ifndef SIMULATOR
    pcm_play_pause(true);
#endif
    paused = false;
    //queue_post(&audio_queue, AUDIO_RESUME, 0);
}

void audio_next(void)
{
    logf("audio_next");
    new_track = 1;
    ci.reload_codec = true;
    
    /* Detect if disk is spinning.. */
    if (filling) {
        ci.stop_codec = true;
        playlist_next(1);
        queue_post(&audio_queue, AUDIO_PLAY, 0);
    } 
#ifndef SIMULATOR    
    else if (!pcm_crossfade_start()) {
        pcm_play_stop();
    }
#endif
}

void audio_prev(void)
{
    logf("audio_prev");
    new_track = -1;
    ci.reload_codec = true;
#ifndef SIMULATOR
    pcm_play_stop();
#endif
        
    if (filling) {
        ci.stop_codec = true;
        playlist_next(-1);
        queue_post(&audio_queue, AUDIO_PLAY, 0);
    }
    //queue_post(&audio_queue, AUDIO_PREV, 0);
}

void audio_ff_rewind(int newpos)
{
    logf("rewind: %d", newpos);
    /* Does not work yet. */
    if (playing)
        ci.seek_time = newpos+1;
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
}
#endif

void mpeg_id3_options(bool _v1first)
{
   v1first = _v1first;
}

void audio_init(void)
{
    logf("audio api init");
    codecbuflen = audiobufend - audiobuf - PCMBUF_SIZE 
                  - MALLOC_BUFSIZE - GUARD_BUFSIZE;
    //codecbuflen = 2*512*1024;
    codecbufused = 0;
    filling = false;
    codecbuf = &audiobuf[MALLOC_BUFSIZE];
    playing = false;
    paused = false;
    track_changed = false;
    current_fd = -1;
    
    logf("abuf:%0x", PCMBUF_SIZE);
    logf("fbuf:%0x", codecbuflen);
    logf("mbuf:%0x", MALLOC_BUFSIZE);
    
    /* Initialize codec api. */    
    ci.read_filebuf = codec_filebuf_callback;
#ifndef SIMULATOR
    ci.audiobuffer_insert = audiobuffer_insert;
#else
    ci.audiobuffer_insert = audiobuffer_insert_sim;
#endif
    ci.get_codec_memory = get_codec_memory_callback;
    ci.request_buffer = codec_request_buffer_callback;
    ci.advance_buffer = codec_advance_buffer_callback;
    ci.advance_buffer_loc = codec_advance_buffer_loc_callback;
    ci.request_next_track = codec_request_next_track_callback;
    ci.mp3_get_filepos = codec_mp3_get_filepos_callback;
    ci.seek_buffer = codec_seek_buffer_callback;
    ci.set_elapsed = codec_set_elapsed_callback;
    ci.configure = codec_configure_callback;
    
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


