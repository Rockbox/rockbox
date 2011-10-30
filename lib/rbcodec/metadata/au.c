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
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "platform.h"

#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "logf.h"

static const unsigned char bitspersamples[9] = {
    0,  /* encoding */
    8,  /* 1:  G.711 MULAW      */
    8,  /* 2:  Linear PCM 8bit  */
    16, /* 3:  Linear PCM 16bit */
    24, /* 4:  Linear PCM 24bit */
    32, /* 5:  Linear PCM 32bit */
    32, /* 6:  IEEE float 32bit */
    64, /* 7:  IEEE float 64bit */
        /* encoding 8 - 26 unsupported. */
    8,  /* 27: G.711 ALAW       */
};

static inline unsigned char get_au_bitspersample(unsigned int encoding)
{
    if (encoding < 8)
        return bitspersamples[encoding];
    else if (encoding == 27)
        return bitspersamples[8];

    return 0;
}

bool get_au_metadata(int fd, struct mp3entry* id3)
{
    /* temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    unsigned long numbytes = 0;
    int offset;

    id3->vbr      = false;   /* All Sun audio files are CBR */
    id3->filesize = filesize(fd);
    id3->length   = 0;

    lseek(fd, 0, SEEK_SET);
    if ((read(fd, buf, 24) < 24) || (memcmp(buf, ".snd", 4) != 0))
    {
        /*
         * no header
         *
         * frequency:       8000 Hz
         * bits per sample: 8 bit
         * channel:         mono
         */
        numbytes = id3->filesize;
        id3->frequency = 8000;
        id3->bitrate   = 8;
    }
    else
    {
        /* parse header */

        /* data offset */
        offset = get_long_be(buf + 4);
        if (offset < 24)
        {
            DEBUGF("CODEC_ERROR: sun audio offset size is small: %d\n", offset);
            return false;
        }
        /* data size */
        numbytes = get_long_be(buf + 8);
        if (numbytes == (uint32_t)0xffffffff)
            numbytes = id3->filesize - offset;

        id3->frequency = get_long_be(buf + 16);
        id3->bitrate = get_au_bitspersample(get_long_be(buf + 12)) * get_long_be(buf + 20)
                                                                   * id3->frequency / 1000;
    }

    /* Calculate track length [ms] */
    if (id3->bitrate)
        id3->length = (numbytes << 3) / id3->bitrate;

    return true;
}
