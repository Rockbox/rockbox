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
#include "adpcm_seek.h"
#include "support_formats.h"

/*
 * YAMAHA ADPCM
 *
 * References
 * [1] YAMAHA, YAMAHA ADPCM ACM Driver Version 1.0.0.0, 2005
 * [2] BlendWorks, YM2608 ADPCM,
 *     http://web.archive.org/web/20050208190547/www.memb.jp/~dearna/ma/ym2608/adpcm.html
 * [3] Naoyuki Sawa, ADPCM no shikumi #1,
 *     http://www.piece-me.org/piece-lab/adpcm/adpcm1.html
 * [4] ffmpeg source code, libavcodec/adpcm.c
 */

/* ADPCM data block layout
 *
 * when the block header exists. (for example, encoding by YAMAHA ADPCM ACM Driver)
 *    blockAlign = (frequency / 60 + 4) * channels.
 *
 *    block
 *       <Mono> (channels = 1)
 *          int16_t   first value (Little endian)
 *          uint16_t  first predictor (Little endian)
 *          uint8_t   ADPCM data (1st data: 0-3 bit, 2nd data: 4-7 bit)
 *          ....
 *
 *       <Stereo> (channels = 2)
 *          int16_t   Left channel first value (Little endian)
 *          uint16_t  Left channel first predictor (Little endian)
 *          int16_t   Right channel first value (Little endian)
 *          uint16_t  Right channel first predictor (Little endian)
 *          uint8_t   ADPCM data (Left channel: 0-3 bit, Right channel: 4-7 bit)
 *          ....
 *
 * when the block header does not exist. (for example, encoding by ffmpeg)
 *    blockAlign = 8000
 *
 *    block
 *       <Mono> (channels = 1)
 *          uint8_t   ADPCM data (1st data: 0-3 bit, 2nd data: 4-7 bit)
 *          ....
 *
 *       <Stereo> (channels = 2)
 *          uint8_t   ADPCM data (Left channel: 0-3 bit, Right channel: 4-7 bit)
 *          ....
 */

static const int32_t amplification_table[] ICONST_ATTR = {
    230, 230, 230, 230, 307, 409, 512, 614, 230, 230, 230, 230, 307, 409, 512, 614
};

static bool has_block_header = false;

static struct adpcm_data cur_data;
static int blocksperchunk;

static struct pcm_format *fmt;

static bool set_format(struct pcm_format *format)
{
    fmt = format;

    if (fmt->channels == 0)
    {
        DEBUGF("CODEC_ERROR: channels is 0\n");
        return false;
    }

    if (fmt->bitspersample != 4)
    {
        DEBUGF("CODEC_ERROR: yamaha adpcm must be 4 bitspersample: %d\n",
                             fmt->bitspersample);
        return false;
    }

    /* check exists block header */
    if (fmt->blockalign == ((ci->id3->frequency / 60) + 4) * fmt->channels)
    {
        has_block_header = true;

        /* chunksize = about 1/30 [sec] data */
        fmt->chunksize = fmt->blockalign;
        blocksperchunk = 1;
    }
    else
    {
        uint32_t max_chunk_count;

        has_block_header = false;

        /* blockalign = 2 * channels samples */
        fmt->blockalign = fmt->channels;
        fmt->samplesperblock = 2;

        /* chunksize = about 1/32[sec] data */
        blocksperchunk = ci->id3->frequency >> 6;
        fmt->chunksize = blocksperchunk * fmt->blockalign;

        max_chunk_count = (uint64_t)ci->id3->length * ci->id3->frequency
                                      / (2000LL * fmt->chunksize / fmt->channels);

        /* initialize seek table */
        init_seek_table(max_chunk_count);
        /* add first data */
        add_adpcm_data(&cur_data);
    }

    return true;
}

static int16_t create_pcmdata(int ch, uint8_t nibble)
{
    int32_t tmp_pcmdata = cur_data.pcmdata[ch];
    int32_t step        = cur_data.step[ch];
    int32_t delta       = step >> 3;

    if (nibble & 4) delta += step;
    if (nibble & 2) delta += (step >> 1);
    if (nibble & 1) delta += (step >> 2);

    if (nibble & 0x08)
        tmp_pcmdata -= delta;
    else
        tmp_pcmdata += delta;

    CLIP(tmp_pcmdata, -32768, 32767);
    cur_data.pcmdata[ch] = tmp_pcmdata;

    step = (step * amplification_table[nibble & 0x07]) >> 8;
    CLIP(step, 127, 24576);
    cur_data.step[ch] = step;

    return cur_data.pcmdata[ch];
}

static int decode(const uint8_t *inbuf, size_t inbufsize,
                  int32_t *outbuf, int *outbufcount)
{
    int ch;
    size_t nsamples = 0;

    /* read block header */
    if (has_block_header)
    {
        for (ch = 0; ch < fmt->channels; ch++)
        {
            cur_data.pcmdata[ch] = inbuf[0] | (SE(inbuf[1]) << 8);
            cur_data.step[ch]    = inbuf[2] | (inbuf[3] << 8);

            inbuf += 4;
            inbufsize -= 4;
        }
    }

    /* read block data */
    ch = fmt->channels - 1;
    while (inbufsize)
    {
        *outbuf++ = create_pcmdata(0,  *inbuf     ) << (PCM_OUTPUT_DEPTH - 16);
        *outbuf++ = create_pcmdata(ch, *inbuf >> 4) << (PCM_OUTPUT_DEPTH - 16);
        nsamples += 2;

        inbuf++;
        inbufsize--;
    }

    if (fmt->channels == 2)
        nsamples >>= 1;
    *outbufcount = nsamples;

    if (!has_block_header)
        add_adpcm_data(&cur_data);

    return CODEC_OK;
}

static int decode_for_seek(const uint8_t *inbuf, size_t inbufsize)
{
    int ch = fmt->channels - 1;

    while (inbufsize)
    {
        create_pcmdata(0,  *inbuf     );
        create_pcmdata(ch, *inbuf >> 4);

        inbuf++;
        inbufsize--;
    }

    add_adpcm_data(&cur_data);

    return CODEC_OK;
}

static struct pcm_pos *get_seek_pos(long seek_time,
                                    uint8_t *(*read_buffer)(size_t *realsize))
{
    static struct pcm_pos newpos;
    uint32_t new_count= 0;

    if (seek_time > 0)
        new_count = ((uint64_t)seek_time * ci->id3->frequency
                                  / (1000LL * fmt->samplesperblock)) / blocksperchunk;

    if (!has_block_header)
    {
        new_count = seek(new_count, &cur_data, read_buffer, &decode_for_seek);
    }
    newpos.pos     = new_count * fmt->chunksize;
    newpos.samples = new_count * blocksperchunk * fmt->samplesperblock;
    return &newpos;
}

static struct pcm_codec codec = {
                                    set_format,
                                    get_seek_pos,
                                    decode,
                                };

const struct pcm_codec *get_yamaha_adpcm_codec(void)
{
    /* initialize first step, pcm data */
    cur_data.pcmdata[0] = 0;
    cur_data.pcmdata[1] = 0;
    cur_data.step[0]    = 127;
    cur_data.step[1]    = 127;

    return &codec;
}
