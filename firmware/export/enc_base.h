/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Base declarations for working with software encoders
 *
 * Copyright (C) 2006-2013 Michael Sevakis
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef ENC_BASE_H
#define ENC_BASE_H

#include <sys/types.h>

/** Encoder config structures **/

/** aiff_enc.codec **/
struct aiff_enc_config
{
#if 0
    unsigned long sample_depth;
#endif
};

/** mp3_enc.codec **/
#define MP3_BITR_CAP_8      (1 << 0)
#define MP3_BITR_CAP_16     (1 << 1)
#define MP3_BITR_CAP_24     (1 << 2)
#define MP3_BITR_CAP_32     (1 << 3)
#define MP3_BITR_CAP_40     (1 << 4)
#define MP3_BITR_CAP_48     (1 << 5)
#define MP3_BITR_CAP_56     (1 << 6)
#define MP3_BITR_CAP_64     (1 << 7)
#define MP3_BITR_CAP_80     (1 << 8)
#define MP3_BITR_CAP_96     (1 << 9)
#define MP3_BITR_CAP_112    (1 << 10)
#define MP3_BITR_CAP_128    (1 << 11)
#define MP3_BITR_CAP_144    (1 << 12)
#define MP3_BITR_CAP_160    (1 << 13)
#define MP3_BITR_CAP_192    (1 << 14)
#define MP3_BITR_CAP_224    (1 << 15)
#define MP3_BITR_CAP_256    (1 << 16)
#define MP3_BITR_CAP_320    (1 << 17)
#define MP3_ENC_NUM_BITR    18

/* MPEG 1 */
#define MPEG1_SAMPR_CAPS    (SAMPR_CAP_32 | SAMPR_CAP_48 | SAMPR_CAP_44)
#define MPEG1_BITR_CAPS     (MP3_BITR_CAP_32  | MP3_BITR_CAP_40  | \
                             MP3_BITR_CAP_48  | MP3_BITR_CAP_56  | \
                             MP3_BITR_CAP_64  | MP3_BITR_CAP_80  | \
                             MP3_BITR_CAP_96  | MP3_BITR_CAP_112 | \
                             MP3_BITR_CAP_128 | MP3_BITR_CAP_160 | \
                             MP3_BITR_CAP_192 | MP3_BITR_CAP_224 | \
                             MP3_BITR_CAP_256 | MP3_BITR_CAP_320)

/* MPEG 2 */
#define MPEG2_SAMPR_CAPS    (SAMPR_CAP_22 | SAMPR_CAP_24 | SAMPR_CAP_16)
#define MPEG2_BITR_CAPS     (MP3_BITR_CAP_8   | MP3_BITR_CAP_16  | \
                             MP3_BITR_CAP_24  | MP3_BITR_CAP_32  | \
                             MP3_BITR_CAP_40  | MP3_BITR_CAP_48  | \
                             MP3_BITR_CAP_56  | MP3_BITR_CAP_64  | \
                             MP3_BITR_CAP_80  | MP3_BITR_CAP_96  | \
                             MP3_BITR_CAP_112 | MP3_BITR_CAP_128 | \
                             MP3_BITR_CAP_144 | MP3_BITR_CAP_160)

#if 0
/* MPEG 2.5 */
#define MPEG2_5_SAMPR_CAPS  (SAMPR_CAP_8  | SAMPR_CAP_12 | SAMPR_CAP_11)
#define MPEG2_5_BITR_CAPS   MPEG2_BITR_CAPS
#endif

/* HAVE_MPEG* defines mainly apply to the bitrate menu */
#if (REC_SAMPR_CAPS & MPEG1_SAMPR_CAPS) || defined (HAVE_SPDIF_REC)
#define HAVE_MPEG1_SAMPR
#endif

#if (REC_SAMPR_CAPS & MPEG2_SAMPR_CAPS) || defined (HAVE_SPDIF_REC)
#define HAVE_MPEG2_SAMPR
#endif

#if 0
#if (REC_SAMPR_CAPS & MPEG2_5_SAMPR_CAPS) || defined (HAVE_SPDIF_REC)
#define HAVE_MPEG2_5_SAMPR
#endif
#endif /* 0 */

#define MP3_ENC_SAMPR_CAPS      (MPEG1_SAMPR_CAPS | MPEG2_SAMPR_CAPS)

/* This number is count of full encoder set */
#define MP3_ENC_NUM_SAMPR       6

#if 0
extern const unsigned long mp3_enc_sampr[MP3_ENC_NUM_SAMPR];
#endif
extern const unsigned long mp3_enc_bitr[MP3_ENC_NUM_BITR];

struct mp3_enc_config
{
    unsigned long bitrate;
};

#define MP3_ENC_BITRATE_CFG_DEFAULT     11 /* 128 */
#define MP3_ENC_BITRATE_CFG_VALUE_LIST  "8,16,24,32,40,48,56,64,80,96," \
                                        "112,128,144,160,192,224,256,320"

/** wav_enc.codec **/
#define WAV_ENC_SAMPR_CAPS      SAMPR_CAP_ALL

struct wav_enc_config
{
#if 0
    unsigned long sample_depth;
#endif
};

/** wavpack_enc.codec **/
#define WAVPACK_ENC_SAMPR_CAPS  SAMPR_CAP_ALL

struct wavpack_enc_config
{
#if 0
    unsigned long sample_depth;
#endif
};

/* General config information about any encoder */
struct encoder_config
{
    union
    {
        /* states which *_enc_config member is valid */
        int rec_format; /* REC_FORMAT_* value */
        int afmt;       /* AFMT_* value       */
    };

    union
    {
        struct mp3_enc_config     mp3_enc;
        struct wavpack_enc_config wavpack_enc;
        struct wav_enc_config     wav_enc;
    };
};

/** Encoder chunk macros and definitions **/

/* What sort of data does the header describe? */
enum CHUNK_T
{
    CHUNK_T_DATA         = 0x0, /* Encoded audio data */
    CHUNK_T_STREAM_START = 0x1, /* Stream start marker */
    CHUNK_T_STREAM_END   = 0x2, /* Stream end marker */
    CHUNK_T_WRAP         = 0x3  /* Buffer early wrap marker */
};

/* Header for every buffer slot and chunk */
union enc_chunk_hdr
{
    struct
    {
        uint32_t type   : 2;    /* Chunk type (CHUNK_T_*) */
        uint32_t err    : 1;    /* Encoder error */
        uint32_t pre    : 1;    /* Chunk is prerecorded data */
        uint32_t aux0   : 1;    /* Aux flag 0 - for encoder */
        uint32_t unused : 3;    /* */
        uint32_t size   : 24;   /* size of data */
    };
    uint32_t zero;              /* Zero-out struct access */
    intptr_t reserved1;         /* Want it at least pointer-sized */
} __attribute__((__may_alias__));

#define ENC_HDR_SIZE    (sizeof (union enc_chunk_hdr))

/* When hdr.type is CHUNK_T_STREAM_START */
struct enc_chunk_file
{
    union enc_chunk_hdr hdr;    /* This chunk's header */
                                /* hdr.size = slot count of chunk */
    char path[];                /* NULL-terminated path of file */
} __attribute__((__may_alias__));

/* If flags = CHUNK_T_STREAM_END, just the header exists */

/* When hdr.type is CHUNK_T_DATA */
struct enc_chunk_data
{
    union enc_chunk_hdr hdr;    /* IN,OUT: This chunk's header */
                                /* hdr.size = total size of data[] */
    uint32_t pcm_count;         /* OUT: number of PCM samples encoded */
    uint8_t data[];             /* OUT: encoded audio data */
} __attribute__((__may_alias__));

/* CHUNK_T_STREAM_END and CHUNK_T_WRAP consist of only the header */

#define ENC_FILE_HDR(hdr) ((struct enc_chunk_file *)(hdr))
#define ENC_DATA_HDR(hdr) ((struct enc_chunk_data *)(hdr))

/* Audio and encoder stream parameters */
struct enc_inputs
{
    /* IN parameters */
    unsigned long sample_rate;     /* PCM samplerate setting */
    int           num_channels;    /* Number of audio channels */
    struct encoder_config *config; /* Encoder settings */

    /* IN,OUT parameters */
    unsigned long enc_sample_rate; /* Actual sample rate accepted by encoder
                                      (for recorded time calculation) */
};

enum enc_callback_reason
{
    ENC_CB_INPUTS, /* 'params' is struct enc_inputs * */
    ENC_CB_STREAM, /* 'params' is union enc_chunk_hdr * */
};

typedef int (* enc_callback_t)(enum enc_callback_reason reason, void *params);

#endif /* ENC_BASE_H */
