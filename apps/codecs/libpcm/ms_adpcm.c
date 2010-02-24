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
#include "pcm_common.h"
#include "support_formats.h"

/*
 * Microsoft ADPCM
 *
 * References
 * [1] Microsoft, New Multimedia Data Types and Data Techniques Revision 3.0, 1994
 * [2] MulitimediaWiki, Microsoft ADPCM, 2006
 * (http://wiki.multimedia.cx/index.php?title=Microsoft_ADPCM)
 * [3] ffmpeg source code, libavcodec/adpcm.c
 */

#define ADPCM_NUM_COEFF 7

static int16_t dec_coeff[2][2];
static uint16_t delta[2];
static int16_t sample[2][2];

static struct pcm_format *fmt;

static const int16_t adaptation_table[] ICONST_ATTR = {
    230, 230, 230, 230, 307, 409, 512, 614,
    768, 614, 512, 409, 307, 230, 230, 230
};

static bool set_format(struct pcm_format *format)
{
    fmt = format;

    if (fmt->bitspersample != 4)
    {
        DEBUGF("CODEC_ERROR: microsoft adpcm must be 4 bitspersample: %d\n",
                             fmt->bitspersample);
        return false;
    }

    fmt->chunksize = fmt->blockalign;

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

static int16_t create_pcmdata(int ch, uint8_t nibble)
{
    int32_t pcmdata;

    pcmdata = (sample[ch][0] * dec_coeff[ch][0] +
               sample[ch][1] * dec_coeff[ch][1]) / 256;
    pcmdata += (delta[ch] * (nibble - ((nibble & 0x8) << 1)));

    CLIP(pcmdata, -32768, 32767);
  
    sample[ch][1] = sample[ch][0];
    sample[ch][0] = pcmdata;

    delta[ch] = (adaptation_table[nibble] * delta[ch]) >> 8;
    if (delta[ch] < 16)
        delta[ch] = 16;

    return (int16_t)pcmdata;
}

static int decode(const uint8_t *inbuf, size_t inbufsize,
                  int32_t *outbuf, int *outbufcount)
{
    int ch;
    size_t nsamples = 0;
    int size = fmt->samplesperblock;

    /* read block header */
    for (ch = 0; ch < fmt->channels; ch++)
    {
        if (*inbuf >= ADPCM_NUM_COEFF)
        {
            DEBUGF("CODEC_ERROR: microsoft adpcm illegal initial coeff=%d > 7\n",
                                 *inbuf);
            return CODEC_ERROR;
        }
        dec_coeff[ch][0] = fmt->coeffs[*inbuf][0];
        dec_coeff[ch][1] = fmt->coeffs[*inbuf][1];
        inbuf++;
    }

    for (ch = 0; ch < fmt->channels; ch++)
    {
        delta[ch] = inbuf[0] | (SE(inbuf[1]) << 8);
        inbuf += 2;
    }

    for (ch = 0; ch < fmt->channels; ch++)
    {
        sample[ch][0] = inbuf[0] | (SE(inbuf[1]) << 8);
        inbuf += 2;
    }

    for (ch = 0; ch < fmt->channels; ch++)
    {
        sample[ch][1] = inbuf[0] | (SE(inbuf[1]) << 8);
        inbuf += 2;
    }

    inbufsize -= 7 * fmt->channels;
    ch = fmt->channels - 1;

    while (size-- > 0)
    {
        *outbuf++ = create_pcmdata(0,  *inbuf >> 4 ) << 13;
        *outbuf++ = create_pcmdata(ch, *inbuf & 0xf) << 13;
        nsamples += 2;

        inbuf++;
        inbufsize--;
        if (inbufsize <= 0)
            break;
    }

    if (fmt->channels == 2)
        nsamples >>= 1;
    *outbufcount = nsamples;

    return CODEC_OK;
}

static const struct pcm_codec codec = {
                                          set_format,
                                          get_seek_pos,
                                          decode,
                                      };

const struct pcm_codec *get_ms_adpcm_codec(void)
{
    return &codec;
}
