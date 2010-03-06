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
 * Dialogic OKI ADPCM
 *
 * References
 * [1] Dialogic Corporation, Dialogic ADPCM Algorithm, 1988
 * [2] MultimediaWiki, Dialogic IMA ADPCM, URL:http://wiki.multimedia.cx/index.php?title=Dialogic_IMA_ADPCM
 * [3] sox source code, src/adpcms.c
 * [4] Tetsuya Isaki, NetBSD:/sys/dev/audio.c, http://www.tri-tree.gr.jp/~isaki/NetBSD/src/sys/dev/ic/msm6258.c.html
 */

static const uint16_t step_table[] ICONST_ATTR = {
    16,  17,   19,   21,   23,   25,   28,  31,  34,  37,  41,  45,  50,  55,
    60,  66,   73,   80,   88,   97,  107, 118, 130, 143, 157, 173, 190, 209,
   230, 253,  279,  307,  337,  371,  408, 449, 494, 544, 598, 658, 724, 796,
   876, 963, 1060, 1166, 1282, 1411, 1552,
};

static const int index_table[] ICONST_ATTR = {
    -1, -1, -1, -1, 2, 4, 6, 8
};

static struct adpcm_data cur_data;

static struct pcm_format *fmt;

static bool set_format(struct pcm_format *format)
{
    uint32_t max_chunk_count;

    fmt = format;

    if (fmt->bitspersample != 4)
    {
        DEBUGF("CODEC_ERROR: dialogic oki adpcm must be 4 bitspersample: %d\n",
                             fmt->bitspersample);
        return false;
    }

    if (fmt->channels != 1)
    {
        DEBUGF("CODEC_ERROR: dialogic oki adpcm must be monaural\n");
        return false;
    }

    /* blockalign = 2 samples */
    fmt->blockalign = 1;
    fmt->samplesperblock = 2;

    /* chunksize = about 1/32[sec] data */
    fmt->chunksize = ci->id3->frequency >> 6;

    max_chunk_count = (uint64_t)ci->id3->length * ci->id3->frequency
                                         / (2000LL * fmt->chunksize);

    /* initialize seek table */
    init_seek_table(max_chunk_count);
    /* add first data */
    add_adpcm_data(&cur_data);

    return true;
}

static int16_t create_pcmdata(uint8_t nibble)
{
    int16_t delta;
    int16_t index = cur_data.step[0];
    int16_t step = step_table[index];

    delta = (step >> 3);
    if (nibble & 4) delta += step;
    if (nibble & 2) delta += (step >> 1);
    if (nibble & 1) delta += (step >> 2);

    if (nibble & 0x08)
        cur_data.pcmdata[0] -= delta;
    else
        cur_data.pcmdata[0] += delta;

    CLIP(cur_data.pcmdata[0], -2048, 2047);

    index += index_table[nibble & 0x07];
    CLIP(index, 0, 48);
    cur_data.step[0] = index;

    return cur_data.pcmdata[0];
}

static int decode(const uint8_t *inbuf, size_t inbufsize,
                  int32_t *outbuf, int *outbufcount)
{
    size_t nsamples = 0;

    while (inbufsize)
    {
        *outbuf++ = create_pcmdata(*inbuf >> 4) << (PCM_OUTPUT_DEPTH - 12);
        *outbuf++ = create_pcmdata(*inbuf     ) << (PCM_OUTPUT_DEPTH - 12);
        nsamples += 2;

        inbuf++;
        inbufsize--;
    }

    *outbufcount = nsamples;
    add_adpcm_data(&cur_data);

    return CODEC_OK;
}

static int decode_for_seek(const uint8_t *inbuf, size_t inbufsize)
{
    while (inbufsize)
    {
        create_pcmdata(*inbuf >> 4);
        create_pcmdata(*inbuf     );

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
    uint32_t seek_count = 0;
    uint32_t new_count;

    if (seek_time > 0)
        seek_count = (uint64_t)seek_time * ci->id3->frequency
                                         / (2000LL * fmt->chunksize);

    new_count = seek(seek_count, &cur_data, read_buffer, &decode_for_seek);
    newpos.pos     = new_count * fmt->chunksize;
    newpos.samples = (newpos.pos / fmt->blockalign) * fmt->samplesperblock;
    return &newpos;
}

static const struct pcm_codec codec = {
                                          set_format,
                                          get_seek_pos,
                                          decode,
                                      };

const struct pcm_codec *get_dialogic_oki_adpcm_codec(void)
{
    /*
     * initialize first pcm data, step index
     * because the dialogic oki adpcm is always monaural,
     * pcmdata[1], step[1] do not use.
     */
    cur_data.pcmdata[0] = 0;
    cur_data.step[0]    = 0;

    return &codec;
}
