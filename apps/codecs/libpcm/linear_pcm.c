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
#include "pcm_common.h"
#include "support_formats.h"

/*
 * Linear PCM
 */

static struct pcm_format *fmt;

static bool set_format(struct pcm_format *format, const unsigned char *fmtpos)
{
    fmt = format;

    (void)fmtpos;

    if (fmt->bitspersample > 32)
    {
        DEBUGF("CODEC_ERROR: pcm with more than 32 bitspersample "
               "is unsupported\n");
        return false;
    }

    if (fmt->totalsamples == 0)
    {
        fmt->bytespersample = (((fmt->bitspersample - 1)/8 + 1)*fmt->channels);
        fmt->totalsamples = fmt->numbytes/fmt->bytespersample;
    }

    /* chunksize is computed so that one chunk is about 1/50s.
     * this make 4096 for 44.1kHz 16bits stereo.
     * It also has to be a multiple of blockalign */
    fmt->chunksize = (1 + fmt->avgbytespersec / (50*fmt->blockalign))*fmt->blockalign;

    return true;
}

static uint32_t get_seek_pos(long seek_time)
{
    uint32_t newpos;

    /* use avgbytespersec to round to the closest blockalign multiple,
       add firstblockposn. 64-bit casts to avoid overflows. */
    newpos = (((uint64_t)fmt->avgbytespersec*(seek_time - 1))
             / (1000LL*fmt->blockalign))*fmt->blockalign;
    return newpos;
}

static int decode(const uint8_t *inbuf, size_t inbufsize,
                  int32_t *outbuf, int *outbufsize)
{
    uint32_t i;

    if (fmt->bitspersample > 24)
    {
        for (i = 0; i < inbufsize; i += 4)
        {
            if (fmt->is_little_endian)
            {
                if (fmt->is_signed)
                    outbuf[i/4] = (inbuf[i]>>3)|(inbuf[i+1]<<5)|(inbuf[i+2]<<13)|(SE(inbuf[i+3])<<21);
                else
                    outbuf[i/4] = (inbuf[i]>>3)|(inbuf[i+1]<<5)|(inbuf[i+2]<<13)|(SFT(inbuf[i+3])<<21);
            }
            else
            {
                if (fmt->is_signed)
                    outbuf[i/4] = (inbuf[i+3]>>3)|(inbuf[i+2]<<5)|(inbuf[i+1]<<13)|(SE(inbuf[i])<<21);
                else
                    outbuf[i/4] = (inbuf[i+3]>>3)|(inbuf[i+2]<<5)|(inbuf[i+1]<<13)|(SFT(inbuf[i])<<21);
            }
        }
        *outbufsize = inbufsize >> 2;
    }
    else if (fmt->bitspersample > 16)
    {
        for (i = 0; i < inbufsize; i += 3)
        {
            if (fmt->is_little_endian)
            {
                if (fmt->is_signed)
                    outbuf[i/3] = (inbuf[i]<<5)|(inbuf[i+1]<<13)|(SE(inbuf[i+2])<<21);
                else
                    outbuf[i/3] = (inbuf[i]<<5)|(inbuf[i+1]<<13)|(SFT(inbuf[i+2])<<21);
            }
            else
            {
                if (fmt->is_signed)
                    outbuf[i/3] = (inbuf[i+2]<<5)|(inbuf[i+1]<<13)|(SE(inbuf[i])<<21);
                else
                    outbuf[i/3] = (inbuf[i+2]<<5)|(inbuf[i+1]<<13)|(SFT(inbuf[i])<<21);
            }
        }
        *outbufsize = inbufsize/3;
    }
    else if (fmt->bitspersample > 8)
    {
        for (i = 0; i < inbufsize; i += 2)
        {
            if (fmt->is_little_endian)
            {
                if (fmt->is_signed)
                    outbuf[i/2] = (inbuf[i]<<13)|(SE(inbuf[i+1])<<21);
                else
                    outbuf[i/2] = (inbuf[i]<<13)|(SFT(inbuf[i+1])<<21);
            }
            else
            {
                if (fmt->is_signed)
                    outbuf[i/2] = (inbuf[i+1]<<13)|(SE(inbuf[i])<<21);
                else
                    outbuf[i/2] = (inbuf[i+1]<<13)|(SFT(inbuf[i])<<21);
            }
        }
        *outbufsize = inbufsize >> 1;
    }
    else
    {
        for (i = 0; i < inbufsize; i++) {
            if (fmt->is_signed)
                outbuf[i] = SE(inbuf[i])<<21;
            else
                outbuf[i] = SFT(inbuf[i])<<21;
        }
        *outbufsize = inbufsize;
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

const struct pcm_codec *get_linear_pcm_codec(void)
{
    return &codec;
}
