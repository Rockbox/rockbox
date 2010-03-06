/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
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
#include "codeclib.h"
#include "support_formats.h"

/*
 * Linear PCM
 */

#define INC_DEPTH_8   (PCM_OUTPUT_DEPTH - 8)
#define INC_DEPTH_16  (PCM_OUTPUT_DEPTH - 16)
#define INC_DEPTH_24  (PCM_OUTPUT_DEPTH - 24)
#define DEC_DEPTH_32  (32 - PCM_OUTPUT_DEPTH)


static struct pcm_format *fmt;

static bool set_format(struct pcm_format *format)
{
    fmt = format;

    if (fmt->bitspersample > 32)
    {
        DEBUGF("CODEC_ERROR: pcm with more than 32 bitspersample "
               "is unsupported\n");
        return false;
    }

    fmt->bytespersample = fmt->bitspersample >> 3;

    if (fmt->totalsamples == 0)
        fmt->totalsamples = fmt->numbytes/fmt->bytespersample;

    fmt->samplesperblock = fmt->blockalign / (fmt->bytespersample * fmt->channels);

    /* chunksize = about 1/50[sec] data */
    fmt->chunksize = (ci->id3->frequency / (50 * fmt->samplesperblock))
                                         * fmt->blockalign;

    return true;
}

static struct pcm_pos *get_seek_pos(long seek_time,
                                    uint8_t *(*read_buffer)(size_t *realsize))
{
    static struct pcm_pos newpos;
    uint32_t newblock = ((uint64_t)seek_time * ci->id3->frequency)
                                             / (1000LL * fmt->samplesperblock);

    (void)read_buffer;
    newpos.pos     = newblock * fmt->blockalign;
    newpos.samples = newblock * fmt->samplesperblock;
    return &newpos;
}

/* 8bit decode functions */
static inline void decode_s8(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i++)
        outbuf[i] = SE(inbuf[i]) << INC_DEPTH_8;
}

static inline void decode_u8(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i++)
        outbuf[i] = SFT(inbuf[i]) << INC_DEPTH_8;
}

/* 16bit decode functions */
static inline void decode_s16le(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i += 2)
        outbuf[i/2] = (inbuf[i] << INC_DEPTH_16)|(SE(inbuf[i+1]) << INC_DEPTH_16);
}

static inline void decode_u16le(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i += 2)
        outbuf[i/2] = (inbuf[i] << INC_DEPTH_16)|(SFT(inbuf[i+1]) << INC_DEPTH_8);
}

static inline void decode_s16be(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i += 2)
        outbuf[i/2] = (inbuf[i+1] << INC_DEPTH_16)|(SE(inbuf[i]) << INC_DEPTH_8);
}

static inline void decode_u16be(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i += 2)
        outbuf[i/2] = (inbuf[i+1] << INC_DEPTH_16)|(SFT(inbuf[i]) << INC_DEPTH_8);
}

/* 24bit decode functions */
static inline void decode_s24le(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i += 3)
        outbuf[i/3] = (inbuf[i] << INC_DEPTH_24)|(inbuf[i+1] << INC_DEPTH_16)|
                      (SE(inbuf[i+2]) << INC_DEPTH_8);
}

static inline void decode_u24le(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i += 3)
        outbuf[i/3] = (inbuf[i] << INC_DEPTH_24)|(inbuf[i+1] << INC_DEPTH_16)|
                      (SFT(inbuf[i+2]) << INC_DEPTH_8);
}

static inline void decode_s24be(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i += 3)
        outbuf[i/3] = (inbuf[i+2] << INC_DEPTH_24)|(inbuf[i+1] << INC_DEPTH_16)|
                      (SE(inbuf[i]) << INC_DEPTH_8);
}

static inline void decode_u24be(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i += 3)
        outbuf[i/3] = (inbuf[i+2] << INC_DEPTH_24)|(inbuf[i+1] << INC_DEPTH_8)|
                      (SFT(inbuf[i]) << INC_DEPTH_8);
}

/* 32bit decode functions */
static inline void decode_s32le(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i += 4)
        outbuf[i/4] = (inbuf[i]   >> DEC_DEPTH_32)|(inbuf[i+1] << INC_DEPTH_24)|
                      (inbuf[i+2] << INC_DEPTH_16)|(SE(inbuf[i+3]) << INC_DEPTH_8);
}

static inline void decode_u32le(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i += 4)
        outbuf[i/4] = (inbuf[i]   >> DEC_DEPTH_32)|(inbuf[i+1] << INC_DEPTH_24)|
                      (inbuf[i+2] << INC_DEPTH_16)|(SFT(inbuf[i+3]) << INC_DEPTH_8);
}

static inline void decode_s32be(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i += 4)
        outbuf[i/4] = (inbuf[i+3] >> DEC_DEPTH_32)|(inbuf[i+2] << INC_DEPTH_24)|
                      (inbuf[i+1] << INC_DEPTH_16)|(SE(inbuf[i]) << INC_DEPTH_8);
}

static inline void decode_u32be(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf)
{
    size_t i = 0;

    for ( ; i < inbufsize; i += 4)
        outbuf[i/4] = (inbuf[i+3] >> DEC_DEPTH_32)|(inbuf[i+2] << INC_DEPTH_24)|
                      (inbuf[i+1] << INC_DEPTH_16)|(SFT(inbuf[i]) << INC_DEPTH_8);
}

static int decode(const uint8_t *inbuf, size_t inbufsize, int32_t *outbuf, int *outbufcount)
{
    if (fmt->bitspersample > 24)
    {
        if (fmt->is_little_endian)
        {
            if (fmt->is_signed)
                decode_s32le(inbuf, inbufsize, outbuf);
            else
                decode_u32le(inbuf, inbufsize, outbuf);
        }
        else
        {
            if (fmt->is_signed)
                decode_s32be(inbuf, inbufsize, outbuf);
            else
                decode_u32be(inbuf, inbufsize, outbuf);
        }
        *outbufcount = inbufsize >> 2;
    }
    else if (fmt->bitspersample > 16)
    {
        if (fmt->is_little_endian)
        {
            if (fmt->is_signed)
                decode_s24le(inbuf, inbufsize, outbuf);
            else
                decode_u24le(inbuf, inbufsize, outbuf);
        }
        else
        {
            if (fmt->is_signed)
                decode_s24be(inbuf, inbufsize, outbuf);
            else
                decode_u24be(inbuf, inbufsize, outbuf);
        }
        *outbufcount = inbufsize / 3;
    }
    else if (fmt->bitspersample > 8)
    {
        if (fmt->is_little_endian)
        {
            if (fmt->is_signed)
                decode_s16le(inbuf, inbufsize, outbuf);
            else
                decode_u16le(inbuf, inbufsize, outbuf);
        }
        else
        {
            if (fmt->is_signed)
                decode_s16be(inbuf, inbufsize, outbuf);
            else
                decode_u16be(inbuf, inbufsize, outbuf);
        }
        *outbufcount = inbufsize >> 1;
    }
    else
    {
        if (fmt->is_signed)
            decode_s8(inbuf, inbufsize, outbuf);
        else
            decode_u8(inbuf, inbufsize, outbuf);

        *outbufcount = inbufsize;
    }

    if (fmt->channels == 2)
        *outbufcount >>= 1;

    return CODEC_OK;
}

static const struct pcm_codec codec = {
                                          set_format,
                                          get_seek_pos,
                                          decode,
                                      };

const struct pcm_codec *get_linear_pcm_codec(void)
{
    return &codec;
}
