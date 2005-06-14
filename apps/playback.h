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

#ifndef _PLAYBACK_H
#define _PLAYBACK_H

#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "id3.h"
#include "mp3data.h"

/* Supported file types. */
#define AFMT_MPA_L1      0x0001  // MPEG Audio layer 1
#define AFMT_MPA_L2      0x0002  // MPEG Audio layer 2
#define AFMT_MPA_L3      0x0004  // MPEG Audio layer 3
  /* (MPEG-1, 2, 2.5 layers 1, 2 and 3 */
#define AFMT_PCM_WAV     0x0008  // Uncompressed PCM in a WAV file
#define AFMT_OGG_VORBIS  0x0010  // Ogg Vorbis
#define AFMT_FLAC        0x0020  // FLAC
#define AFMT_MPC         0x0040  // Musepack
#define AFMT_AAC         0x0080  // AAC
#define AFMT_APE         0x0100  // Monkey's Audio
#define AFMT_WMA         0x0200  // Windows Media Audio
#define AFMT_A52         0x0400  // A/52 (aka AC3) audio
#define AFMT_REAL        0x0800  // Realaudio
#define AFMT_UNKNOWN     0x1000  // Unknown file format
#define AFMT_WAVPACK     0x2000  // WavPack

/* File buffer configuration keys. */
#define CODEC_SET_FILEBUF_WATERMARK     1
#define CODEC_SET_FILEBUF_CHUNKSIZE     2
#define CODEC_SET_FILEBUF_LIMIT         3

/* Not yet implemented. */
#define CODEC_SET_AUDIOBUF_WATERMARK    4

#define MAX_TRACK 10
struct track_info {
    struct mp3entry id3;     /* TAG metadata */
    struct mp3info mp3data;  /* MP3 metadata */
    char *codecbuf;          /* Pointer to codec buffer */
    size_t codecsize;        /* Codec length in bytes */
    int codectype;           /* Codec type (example AFMT_MPA_L3) */
    
    off_t filerem;           /* Remaining bytes of file NOT in buffer */
    off_t filesize;          /* File total length */
    off_t filepos;           /* Read position of file for next buffer fill */
    off_t start_pos;         /* Position to first bytes of file in buffer */
    volatile int available;  /* Available bytes to read from buffer */
    bool taginfo_ready;      /* Is metadata read */
    int playlist_offset;     /* File location in playlist */
};


/* Codec Interface */
struct codec_api {
    off_t  filesize;          /* Total file length */
    off_t  curpos;            /* Current buffer position */
    
    /* For gapless mp3 */
    struct mp3entry *id3;     /* TAG metadata pointer */
    struct mp3info *mp3data;  /* MP3 metadata pointer */
    bool *taginfo_ready;      /* Is metadata read */
    
    /* Codec should periodically check if stop_codec is set to true.
       In case it's, codec must return with PLUGIN_OK status immediately. */
    bool stop_codec;
    /* Codec should periodically check if reload_codec is set to true.
       In case it's, codec should reload itself without exiting. */
    bool reload_codec;
    /* If seek_time != 0, codec should seek to that song position (in ms)
       if codec supports seeking. */
    int seek_time;
    
    /* Returns buffer to malloc array. Only codeclib should need this. */
    void* (*get_codec_memory)(size_t *size);
    /* Insert PCM data into audio buffer for playback. Playback will start
       automatically. */
    bool (*audiobuffer_insert)(char *data, size_t length);
    /* Set song position in WPS (value in ms). */
    void (*set_elapsed)(unsigned int value);
    
    /* Read next <size> amount bytes from file buffer to <ptr>.
       Will return number of bytes read or 0 if end of file. */
    size_t (*read_filebuf)(void *ptr, size_t size);
    /* Request pointer to file buffer which can be used to read
       <realsize> amount of data. <reqsize> tells the buffer system
       how much data it should try to allocate. If <realsize> is 0,
       end of file is reached. */
    void* (*request_buffer)(size_t *realsize, size_t reqsize);
    /* Advance file buffer position by <amount> amount of bytes. */
    void (*advance_buffer)(size_t amount);
    /* Advance file buffer to a pointer location inside file buffer. */
    void (*advance_buffer_loc)(void *ptr);
    /* Seek file buffer to position <newpos> beginning of file. */
    bool (*seek_buffer)(off_t newpos);
    /* Calculate mp3 seek position from given time data in ms. */
    off_t (*mp3_get_filepos)(int newtime);
    /* Request file change from file buffer. Returns true is next
       track is available and changed. If return value is false,
       codec should exit immediately with PLUGIN_OK status. */
    bool (*request_next_track)(void);
    
    /* Configure different codec buffer parameters. */
    void (*configure)(int setting, void *value);
};

#endif


