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
#ifndef CODEC_LIBPCM_PCM_COMMON_H
#define CODEC_LIBPCM_PCM_COMMON_H

#include <sys/types.h>
#include <stdbool.h>
#include <inttypes.h>

/* Macro that sign extends an unsigned byte */
#define SE(x) ((int32_t)((int8_t)(x)))

/* Macro that shift to -0x80. (0 .. 127 to -128 .. -1, 128 .. 255 to 0 .. 127) */
#define SFT(x) ((int32_t)x-0x80)

/* Macro that clipping data */
#define CLIP(data, min, max) \
if ((data) > (max)) data = max; \
else if ((data) < (min)) data = min;

/* nums of msadpcm coeffs
 * In many case, nNumCoef is 7.
 * Depending upon the encoder, as for this value there is a possibility
 * of increasing more.
 * If you found the file where this value exceeds 7, please report.
 */
#define MSADPCM_NUM_COEFF 7

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

    /* the following values are format speciffic parameters */

    /* microsoft adpcm: aCoeff */
    int16_t coeffs[MSADPCM_NUM_COEFF][2];
};

struct pcm_pos {
    uint32_t pos;
    uint32_t samples;
};

struct pcm_codec {
    /*
     * sets the format speciffic RIFF/AIFF header information and checks the pcm_format.
     *
     * [In/Out] format
     *         the structure which supplies RIFF/AIFF header information.
     *
     * return
     *     true:  RIFF/AIFF header check OK
     *     false: RIFF/AIFF header check NG
     */
    bool (*set_format)(struct pcm_format *format);

    /*
     * get seek position
     *
     * [In] seek_time
     *         seek time [ms]
     *
     * [In] read_buffer
     *         the function which reads the data from the file (chunksize bytes read).
     *
     * return
     *     position after the seeking.
     */
    struct pcm_pos *(*get_seek_pos)(long seek_time,
                                    uint8_t *(*read_buffer)(size_t *realsize));

    /*
     * decode wave data.
     *
     * [In] inbuf
     *         the start pointer of wave data buffer.
     *
     * [In] inbufsize
     *         wave data buffer size (bytes).
     *
     * [Out] outbuf
     *         the start pointer of the buffer which supplies decoded pcm data.
     *
     * [Out] outbufcount
     *         decoded pcm data count.
     *
     * return
     *     CODEC_OK:    decode succeed.
     *     CODEC_ERROR: decode failure.
     */
    int (*decode)(const uint8_t *inbuf, size_t inbufsize,
                  int32_t *outbuf, int *outbufcount);
};

struct pcm_entry {
    uint32_t format_tag;
    const struct pcm_codec *(*get_codec)(void);
};

#endif
