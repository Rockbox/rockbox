/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Yoshihisa Uchida
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
#ifndef CODEC_LIBPCMS_PCM_COMMON_H
#define CODEC_LIBPCMS_PCM_COMMON_H

#include <sys/types.h>
#include <stdbool.h>
#include <inttypes.h>

/*
 * PCM_CHUNK_SIZE has the size only of storing the sample at 1/50 seconds.
 * But it might not be 1/50 seconds according to the format.
 * Please confirm the source file of each format.
 */
#define PCM_CHUNK_SIZE (4096*2)

/* Macro that sign extends an unsigned byte */
#define SE(x) ((int32_t)((int8_t)(x)))

/* Macro that shift to -0x80. (0 .. 127 to -128 .. -1, 128 .. 255 to 0 .. 127) */
#define SFT(x) ((int32_t)x-0x80)

struct pcm_format {
    /*
     * RIFF: wFormatTag (in 'fmt ' chunk)
     * AIFF: compressionType (in 'COMM' chunk)
     */
    uint32_t formattag;

    /*
     * RIFF: wChannels (in 'fmt ' chunk)
     * AIFF: numChannels (in 'COMM' chunk)
     */
    uint16_t channels;

    /*
     * RIFF: dwSamplesPerSec (in 'fmt ' chunk)
     * AIFF: sampleRate (in 'COMM' chunk)
     */
    uint32_t samplespersec;

    /* RIFF: dwAvgBytesPerSec (in 'fmt ' chunk) */
    uint32_t avgbytespersec;

    /*
     * RIFF: wBlockAlign (in 'fmt ' chunk)
     * AIFF: blockSize (in 'SSND' chunk)
     */
    uint16_t blockalign;

    /*
     * RIFF: wBitsPerSample (in 'fmt ' chunk)
     * AIFF: sampleSize (in 'COMM' chunk)
     */
    uint16_t bitspersample;

    /* RIFF: wSize (in 'fmt ' chunk) */
    uint16_t size;

    /* RIFF: dSamplesPerBlock (in 'fmt ' chunk) */
    uint16_t samplesperblock;

    /* RIFF: wTotalSamples (in 'fact' chunk) */
    uint16_t totalsamples;

    /* the following values are not RIFF/AIFF chunk values */

    /* bytes per sample */
    int bytespersample;

    /* chunk size */
    long chunksize;

    /* data size */
    uint32_t numbytes;

    /*
     * data endian
     * true: little endian, false: big endian
     */
    bool is_little_endian;

    /*
     * data signess
     * true: signed, false: unsigned
     */
    bool is_signed;
};

struct pcm_codec {
    bool (*set_format)(struct pcm_format *format, const unsigned char *fmtpos);
    uint32_t (*get_seek_pos)(long seek_time);
    int (*decode)(const uint8_t *inbuf, size_t inbufsize,
                  int32_t *outbuf, int *outbufsize);
};

struct pcm_entry {
    uint32_t format_tag;
    const struct pcm_codec *(*get_codec)(void);
};

#endif
