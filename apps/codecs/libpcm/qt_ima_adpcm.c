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
#include "ima_adpcm_common.h"
#include "support_formats.h"

/*
 * Apple QuickTime IMA ADPCM
 *
 * References
 * [1] Multimedia Wiki, Apple QuickTime IMA ADPCM
 *     URL:http://wiki.multimedia.cx/index.php?title=Apple_QuickTime_IMA_ADPCM
 * [2] Apple Inc., Technical Note TN1081 Understanding the Differences Between
 *     Apple and Windows IMA-ADPCM Compressed Sound Files, 1996
 * [3] ffmpeg source code, libavcodec/adpcm.c
 */

static struct pcm_format *fmt;

static bool set_format(struct pcm_format *format)
{
    fmt = format;

    if (fmt->bitspersample != 4)
    {
        DEBUGF("CODEC_ERROR: quicktime ima adpcm must be 4 bitspersample: %d\n",
                             fmt->bitspersample);
        return false;
    }

    fmt->blockalign = 34 * fmt->channels;
    fmt->samplesperblock = 64;

    /* chunksize = about 1/50[s] data */
    fmt->chunksize = (ci->id3->frequency / (50 * fmt->samplesperblock))
                                         * fmt->blockalign;

    init_ima_adpcm_decoder(4, NULL);
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

static int decode(const uint8_t *inbuf, size_t inbufsize,
                  int32_t *outbuf, int *outbufcount)
{
    int ch;
    size_t nsamples = 0;
    int block_size;
    int32_t *pcmbuf;
    int32_t init_pcmdata;
    int8_t init_index;

    while (inbufsize > 0)
    {
        for (ch = 0; ch < fmt->channels; ch++)
        {
            /* read block header */
            init_pcmdata = (inbuf[0] << 8)|(inbuf[1] & 0x80);
            if (init_pcmdata > 32767)
                init_pcmdata -= 65536;

            init_index = inbuf[1] & 0x7f;
            if (init_index > 88)
            {
                DEBUGF("CODEC_ERROR: quicktime ima adpcm illegal step index=%d > 88\n",
                                     init_index);
                return CODEC_ERROR;
            }

            inbuf += 2;
            inbufsize -= 2;

            set_decode_parameters(1, &init_pcmdata, &init_index);

            /* read block data */
            pcmbuf = outbuf + ch;
            for (block_size = 32; block_size > 0 && inbufsize > 0; block_size--, inbufsize--)
            {
                *pcmbuf = create_pcmdata_size4(ch, *inbuf     ) << 13;
                pcmbuf += fmt->channels;
                *pcmbuf = create_pcmdata_size4(ch, *inbuf >> 4) << 13;
                pcmbuf += fmt->channels;
                nsamples += 2;
                inbuf++;
            }
        }
        outbuf += 64 * fmt->channels;
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

const struct pcm_codec *get_qt_ima_adpcm_codec(void)
{
    return &codec;
}
