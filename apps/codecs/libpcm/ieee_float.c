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
#include "codeclib.h"
#include "pcm_common.h"
#include "support_formats.h"

/*
 * IEEE float
 */

static struct pcm_format *fmt;

static bool set_format(struct pcm_format *format)
{
    fmt = format;

    if (fmt->channels == 0)
    {
        DEBUGF("CODEC_ERROR: channels is 0\n");
        return false;
    }

    if (fmt->bitspersample != 32 && fmt->bitspersample != 64)
    {
        DEBUGF("CODEC_ERROR: ieee float must be 32 or 64 bitspersample: %d\n",
                             fmt->bitspersample);
        return false;
    }

    fmt->bytespersample = fmt->bitspersample >> 3;

    if (fmt->blockalign == 0)
        fmt->blockalign = fmt->bytespersample * fmt->channels;

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

static int decode(const uint8_t *inbuf, size_t inbufsize,
                  int32_t *outbuf, int *outbufsize)
{
    uint32_t i;
    int32_t pcm;
    int16_t exp;
    int sgn;

    if (fmt->bitspersample == 32)
    {
        for (i = 0; i < inbufsize; i += 4)
        {
            if (fmt->is_little_endian)
            {
                pcm = (inbuf[0]<<5)|(inbuf[1]<<13)|((inbuf[2]|0x80)<<21);
                exp = ((inbuf[2]>>7)|((inbuf[3]&0x7f)<<1)) - 127;
                sgn = inbuf[3] & 0x80;
            }
            else
            {
                pcm = (inbuf[3]<<5)|(inbuf[2]<<13)|((inbuf[1]|0x80)<<21);
                exp = ((inbuf[1]>>7)|((inbuf[0]&0x7f)<<1)) - 127;
                sgn = inbuf[0] & 0x80;
            }
            if (exp > -29 && exp < 0)
            {
                pcm >>= -exp;
                if (sgn)
                    pcm = -pcm;
            }
            else if (exp < -28)
                pcm = 0;
            else
                pcm = (sgn)?-(1<<28):(1<<28)-1;

            outbuf[i/4] = pcm;
            inbuf += 4;
        }
        *outbufsize = inbufsize >> 2;
    }
    else
    {
        for (i = 0; i < inbufsize; i += 8)
        {
            if (fmt->is_little_endian)
            {
                pcm = inbuf[3]|(inbuf[4]<<8)|(inbuf[5]<<16)|(((inbuf[6]&0x0f)|0x10)<<24);
                exp = (((inbuf[6]&0xf0)>>4)|((inbuf[7]&0x7f)<<4)) - 1023;
                sgn = inbuf[7] & 0x80;
            }
            else
            {
                pcm = inbuf[4]|(inbuf[3]<<8)|(inbuf[2]<<16)|(((inbuf[1]&0x0f)|0x10)<<24);
                exp = (((inbuf[1]&0xf0)>>4)|((inbuf[0]&0x7f)<<4)) - 1023;
                sgn = inbuf[0] & 0x80;
            }
            if (exp > -29 && exp < 0)
            {
                pcm >>= -exp;
                if (sgn)
                    pcm = -pcm;
            }
            else if (exp < -28)
                pcm = 0;
            else
                pcm = (sgn)?-(1<<28):(1<<28)-1;

            outbuf[i/8] = pcm;
            inbuf += 8;
        }
        *outbufsize = inbufsize >> 3;
    }

    if (fmt->channels == 2)
        *outbufsize >>= 1;

    return CODEC_OK;
}

static const struct pcm_codec codec = {
                                          set_format,
                                          get_seek_pos,
                                          decode,
                                      };

const struct pcm_codec *get_ieee_float_codec(void)
{
    return &codec;
}
