/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef ID3_H
#define ID3_H

#include <stdbool.h>
#include "config.h"
#include "file.h"

#define ID3V2_BUF_SIZE 300

/* Audio file types. */
/* NOTE: The values of the AFMT_* items are used for the %fc tag in the WPS
         - so new entries MUST be added to the end to maintain compatibility.
 */
enum
{
    AFMT_UNKNOWN = 0,  /* Unknown file format */

    /* start formats */

    AFMT_MPA_L1,       /* MPEG Audio layer 1 */
    AFMT_MPA_L2,       /* MPEG Audio layer 2 */
    AFMT_MPA_L3,       /* MPEG Audio layer 3 */

#if CONFIG_CODEC == SWCODEC
    AFMT_AIFF,         /* Audio Interchange File Format */
    AFMT_PCM_WAV,      /* Uncompressed PCM in a WAV file */
    AFMT_OGG_VORBIS,   /* Ogg Vorbis */
    AFMT_FLAC,         /* FLAC */
    AFMT_MPC,          /* Musepack */
    AFMT_A52,          /* A/52 (aka AC3) audio */
    AFMT_WAVPACK,      /* WavPack */
    AFMT_ALAC,         /* Apple Lossless Audio Codec */
    AFMT_AAC,          /* Advanced Audio Coding (AAC) in M4A container */
    AFMT_MOD,          /* MOD File Format */
    AFMT_SHN,          /* Shorten */
    AFMT_SID,          /* SID File Format */
    AFMT_ADX,          /* ADX File Format */
    AFMT_NSF,          /* NESM (NES Sound Format) */
    AFMT_SPEEX,        /* Ogg Speex speech */
    AFMT_SPC,          /* SPC700 save state */
    AFMT_APE,          /* Monkey's Audio (APE) */
    AFMT_WMA,          /* WMAV1/V2 in ASF */
#endif

    /* add new formats at any index above this line to have a sensible order -
       specified array index inits are used */
    /* format arrays defined in id3.c */

    AFMT_NUM_CODECS,

#if CONFIG_CODEC == SWCODEC && defined(HAVE_RECORDING)
    /* masks to decompose parts */
    CODEC_AFMT_MASK    = 0x0fff,
    CODEC_TYPE_MASK    = 0x7000,

    /* switch for specifying codec type when requesting a filename */
    CODEC_TYPE_DECODER = (0 << 12), /* default */
    CODEC_TYPE_ENCODER = (1 << 12),
#endif /* CONFIG_CODEC == SWCODEC && defined(HAVE_RECORDING) */
};

#if CONFIG_CODEC == SWCODEC
#define CODEC_EXTENSION "codec"

#ifdef HAVE_RECORDING
#define ENCODER_SUFFIX  "_enc"
enum rec_format_indexes
{
    __REC_FORMAT_START_INDEX = -1,

    /* start formats */

    REC_FORMAT_PCM_WAV,
    REC_FORMAT_AIFF,
    REC_FORMAT_WAVPACK,
    REC_FORMAT_MPA_L3,

    /* add new formats at any index above this line to have a sensible order -
       specified array index inits are used
       REC_FORMAT_CFG_NUM_BITS should allocate enough bits to hold the range
       REC_FORMAT_CFG_VALUE_LIST should be in same order as indexes
    */

    REC_NUM_FORMATS,

    REC_FORMAT_DEFAULT = REC_FORMAT_PCM_WAV,
    REC_FORMAT_CFG_NUM_BITS = 2
};

#define REC_FORMAT_CFG_VAL_LIST "wave,aiff,wvpk,mpa3" 

/* get REC_FORMAT_* corresponding AFMT_* */
extern const int rec_format_afmt[REC_NUM_FORMATS];
/* get AFMT_* corresponding REC_FORMAT_* */
extern const int afmt_rec_format[AFMT_NUM_CODECS];

#define AFMT_ENTRY(label, root_fname, enc_root_fname, ext_list) \
    { label, root_fname, enc_root_fname, ext_list }
#else /* !HAVE_RECORDING */
#define AFMT_ENTRY(label, root_fname, enc_root_fname, ext_list) \
    { label, root_fname, ext_list }
#endif /* HAVE_RECORDING */
#else /* !SWCODEC */

#define AFMT_ENTRY(label, root_fname, enc_root_fname, ext_list) \
    { label, ext_list }
#endif /* CONFIG_CODEC == SWCODEC */

/* record describing the audio format */
struct afmt_entry
{
    char label[8];      /* format label */
#if CONFIG_CODEC == SWCODEC
    char *codec_root_fn; /* root codec filename (sans _enc and .codec) */
#ifdef HAVE_RECORDING
    char *codec_enc_root_fn; /* filename of encoder codec */
#endif
#endif
    char *ext_list;     /* double NULL terminated extension
                           list for type with the first as
                           the default for recording */
};

/* database of labels and codecs. add formats per above enum */
extern const struct afmt_entry audio_formats[AFMT_NUM_CODECS];

struct mp3entry {
    char path[MAX_PATH];
    char* title;
    char* artist;
    char* album;
    char* genre_string;
    char* disc_string;
    char* track_string;
    char* year_string;
    char* composer;
    char* comment;
    char* albumartist;
    char* grouping;
    int discnum;    
    int tracknum;
    int version;
    int layer;
    int year;
    unsigned char id3version;
    unsigned int codectype;
    unsigned int bitrate;
    unsigned long frequency;
    unsigned long id3v2len;
    unsigned long id3v1len;
    unsigned long first_frame_offset; /* Byte offset to first real MP3 frame.
                                         Used for skipping leading garbage to
                                         avoid gaps between tracks. */
    unsigned long vbr_header_pos;
    unsigned long filesize; /* without headers; in bytes */
    unsigned long length;   /* song length in ms */
    unsigned long elapsed;  /* ms played */

    int lead_trim;          /* Number of samples to skip at the beginning */
    int tail_trim;          /* Number of samples to remove from the end */

    /* Added for Vorbis */
    unsigned long samples;  /* number of samples in track */

    /* MP3 stream specific info */
    unsigned long frame_count; /* number of frames in the file (if VBR) */

    /* Used for A52/AC3 */
    unsigned long bytesperframe; /* number of bytes per frame (if CBR) */

    /* Xing VBR fields */
    bool vbr;
    bool has_toc;           /* True if there is a VBR header in the file */
    unsigned char toc[100]; /* table of contents */

    /* these following two fields are used for local buffering */
    char id3v2buf[ID3V2_BUF_SIZE];
    char id3v1buf[4][92];

    /* resume related */
    unsigned long offset;  /* bytes played */
    int index;             /* playlist index */

    /* runtime database fields */
    long tagcache_idx;     /* 0=invalid, otherwise idx+1 */
    int rating;
    int score;
    long playcount;
    long lastplayed;
    long playtime;
    
    /* replaygain support */
    
#if CONFIG_CODEC == SWCODEC
    char* track_gain_string;
    char* album_gain_string;
    long track_gain;    /* 7.24 signed fixed point. 0 for no gain. */
    long album_gain;
    long track_peak;    /* 7.24 signed fixed point. 0 for no peak. */
    long album_peak;
#endif

    /* Cuesheet support */
    int cuesheet_type;      /* 0: none, 1: external, 2: embedded */
};

enum {
    ID3_VER_1_0 = 1,
    ID3_VER_1_1,
    ID3_VER_2_2,
    ID3_VER_2_3,
    ID3_VER_2_4
};

bool get_mp3_metadata(int fd, struct mp3entry *entry, const char *filename);
bool mp3info(struct mp3entry *entry, const char *filename);
char* id3_get_num_genre(unsigned int genre_num);
char* id3_get_codec(const struct mp3entry* id3);
int getid3v2len(int fd);
void adjust_mp3entry(struct mp3entry *entry, void *dest, const void *orig);
void copy_mp3entry(struct mp3entry *dest, const struct mp3entry *orig);

#endif
