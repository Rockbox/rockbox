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

#ifndef _AUDIO_H
#define _AUDIO_H

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

#define CODEC_SET_FILEBUF_WATERMARK     1
#define CODEC_SET_FILEBUF_CHUNKSIZE     2
#define CODEC_SET_FILEBUF_LIMIT         3

/* Not yet implemented. */
#define CODEC_SET_AUDIOBUF_WATERMARK    4

struct codec_api {
    off_t  filesize;
    off_t  curpos;
    size_t bitspersampe;
    
    /* For gapless mp3 */
    struct mp3entry *id3;
    struct mp3info *mp3data;
    bool *taginfo_ready;
    
    bool stop_codec;
    bool reload_codec;
    int seek_time;
    
    void* (*get_codec_memory)(size_t *size);
    bool (*audiobuffer_insert)(char *data, size_t length);
    void (*set_elapsed)(unsigned int value);
    
    size_t (*read_filebuf)(void *ptr, size_t size);
    void* (*request_buffer)(size_t *realsize, size_t reqsize);
    void (*advance_buffer)(size_t amount);
    void (*advance_buffer_loc)(void *ptr);
    bool (*seek_buffer)(off_t newpos);
    off_t (*mp3_get_filepos)(int newtime);
    bool (*request_next_track)(void);
    
    void (*configure)(int setting, void *value);
};

#endif


