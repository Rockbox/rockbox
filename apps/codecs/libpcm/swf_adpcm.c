/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Yoshihisa Uchida
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
#include "ima_adpcm_common.h"
#include "support_formats.h"

/*
 * Adobe SWF ADPCM
 *
 * References
 * [1] Adobe, SWF File Format Specification Version 10, 2008
 * [2] Jack Jansen, adpcm.c in adpcm.zip
 * [3] ffmpeg source code, libavcodec/adpcm.c
 */

/* step index table when bitspersample is 3.
 * (when bitspersample is 2, 4, 5, step index table uses the table
 * which is defined ima_adpcm_common.c.)
 */
static const int index_table[4] ICONST_ATTR = {
    -1, -1, 2, 4,
};

static int validity_bits = 8;
static bool first_block = true;
static int blockbits = 0;
static int lastbytebits = 0;
static bool after_seek = false;

static struct pcm_format *fmt;

static bool set_format(struct pcm_format *format)
{
    fmt = format;

    if (fmt->bitspersample < 2 || fmt->bitspersample > 5)
    {
        DEBUGF("CODEC_ERROR: swf adpcm must be 2, 3, 4 or 5 bitspersample: %d\n",
                             fmt->bitspersample);
        return false;
    }

    if (fmt->samplesperblock == 0)
        fmt->samplesperblock = (((fmt->blockalign << 3) - 2) / fmt->channels - 22)
                                                             / fmt->bitspersample + 1;

    blockbits = ((fmt->samplesperblock - 1) * fmt->bitspersample + 22) * fmt->channels;

    /*
     * chunksize = about 93 [ms] data (frequency:44.1kHz, 4096 [sample/block])
     * chunksize changes depending upon the position of block.
     */
    fmt->chunksize = (blockbits + 9) >> 3;

    /* initialize for ima adpcm common functions */
    if (fmt->bitspersample == 3)
        init_ima_adpcm_decoder(fmt->bitspersample, index_table);
    else
        init_ima_adpcm_decoder(fmt->bitspersample, NULL);

   return true;
}

static struct pcm_pos *get_seek_pos(long seek_time,
                                    uint8_t *(*read_buffer)(size_t *realsize))
{
    static struct pcm_pos newpos;
    uint32_t chunkbits = blockbits;
    uint32_t seekbits = (((uint64_t)seek_time * ci->id3->frequency)
                                       / (1000LL * fmt->samplesperblock)) * blockbits + 2;

    (void)read_buffer;

    newpos.pos     = seekbits >> 3;
    newpos.samples = (((uint64_t)seek_time * ci->id3->frequency)
                                           / (1000LL * fmt->samplesperblock))
                                           * fmt->samplesperblock;

    if (newpos.pos == 0)
    {
        first_block = true;
        lastbytebits = 0;
    }
    else
    {
        first_block = false;
        lastbytebits = seekbits & 0x07;
        if (lastbytebits != 0)
            chunkbits -= (8 - lastbytebits);
    }

    /* calculates next read bytes */
    fmt->chunksize = (chunkbits >> 3) + (((chunkbits & 0x07) > 0)?1:0)
                                      + ((lastbytebits > 0)?1:0);

    after_seek = true;
    return &newpos;
}

static uint8_t get_data(const uint8_t **buf, int bit)
{
    uint8_t res = 0;
    uint8_t mask = (1 << bit) - 1;

    if (validity_bits >= bit)
    {
        validity_bits -= bit;
        return (**buf >> validity_bits) & mask;
    }

    if (validity_bits > 0)
        res = **buf << (bit - validity_bits);

    validity_bits += 8 - bit;
    res = (res | (*(++(*buf)) >> validity_bits)) & mask;
    return res;
}

static int decode(const uint8_t *inbuf, size_t inbufsize,
                  int32_t *outbuf, int *outbufcount)
{
    int ch;
    int adpcm_code_size;
    int count = fmt->samplesperblock;
    int32_t init_pcmdata[2];
    int8_t init_index[2];
    static uint8_t lastbyte = 0;

    (void)inbufsize;

    validity_bits = 8;

    /* read block header */
    ch = fmt->channels - 1;
    if (first_block)
    {
        adpcm_code_size = get_data(&inbuf, 2) + 2;
        if (fmt->bitspersample != adpcm_code_size)
        {
            DEBUGF("CODEC_ERROR: swf adpcm different adpcm code size=%d != %d\n",
                                 adpcm_code_size, fmt->bitspersample);
            return CODEC_ERROR;
        }
        init_pcmdata[0] = (get_data(&inbuf, 8) << 8) | get_data(&inbuf, 8);

        lastbytebits = 0;
        first_block = false;
    }
    else
    {
        if (after_seek && lastbytebits > 0)
        {
            lastbyte = *inbuf++;
            after_seek = false;
        }
        if (lastbytebits > 0)
            init_pcmdata[0] = ((lastbyte << (8 + lastbytebits)) |
                              (get_data(&inbuf, 8) << lastbytebits) |
                              get_data(&inbuf, lastbytebits)) & 65535;
        else
            init_pcmdata[0] = (get_data(&inbuf, 8) << 8) | get_data(&inbuf, 8);
    }
    after_seek = false;

    init_index[0] = get_data(&inbuf, 6);
    if (init_pcmdata[0] > 32767)
        init_pcmdata[0] -= 65536;

    if (ch > 0)
    {
        init_pcmdata[1] = (get_data(&inbuf, 8) << 8) | get_data(&inbuf, 8);
        init_index[1] = get_data(&inbuf, 6);
        if (init_pcmdata[1] > 32767)
            init_pcmdata[1] -= 65536;
    }

    *outbuf++ = init_pcmdata[0] << IMA_ADPCM_INC_DEPTH;
    if (ch > 0)
        *outbuf++ = init_pcmdata[1] << IMA_ADPCM_INC_DEPTH;

    set_decode_parameters(fmt->channels, init_pcmdata, init_index);

    /* read block data */
    while (--count > 0)
    {
        *outbuf++ = create_pcmdata(0, get_data(&inbuf, fmt->bitspersample))
                        << IMA_ADPCM_INC_DEPTH;
        if (ch > 0)
            *outbuf++ = create_pcmdata(ch, get_data(&inbuf, fmt->bitspersample))
                            << IMA_ADPCM_INC_DEPTH;
    }

    *outbufcount = fmt->samplesperblock;

    lastbyte = *inbuf;
    lastbytebits = (8 - validity_bits) & 0x07;

    /* calculates next read bytes */
    fmt->chunksize = (blockbits - validity_bits + 7) >> 3;

    return CODEC_OK;
}

static const struct pcm_codec codec = {
                                          set_format,
                                          get_seek_pos,
                                          decode,
                                      };

const struct pcm_codec *get_swf_adpcm_codec(void)
{
    first_block = true;
    lastbytebits = 0;
    after_seek = false;

    return &codec;
}
