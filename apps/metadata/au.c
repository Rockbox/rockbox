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
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

#include "system.h"
#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "logf.h"

/* table of bits per sample / 8 */
static const unsigned char bitspersamples[28] = {
    0,
    1,  /* G.711 MULAW */
    1,  /* 8bit  */
    2,  /* 16bit */
    3,  /* 24bit */
    4,  /* 32bit */
    4,  /* 32bit */
    8,  /* 64bit */
    0,  /* Fragmented sample data */
    0,  /* DSP program */
    0,  /* 8bit fixed point */
    0,  /* 16bit fixed point */
    0,  /* 24bit fixed point */
    0,  /* 32bit fixed point */
    0,
    0,
    0,
    0,
    0,  /* 16bit linear with emphasis */
    0,  /* 16bit linear compressed */
    0,  /* 16bit linear with emphasis and compression */
    0,  /* Music kit DSP commands */
    0,
    0,  /* G.721 MULAW */
    0,  /* G.722 */
    0,  /* G.723 3bit */
    0,  /* G.723 5bit */
    1,  /* G.711 ALAW */
};

static inline unsigned char get_au_bitspersample(unsigned int encoding)
{
    if (encoding > 27)
        return 0;
    return bitspersamples[encoding];
}

bool get_au_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    unsigned long numbytes = 0;
    int read_bytes;
    int offset;
    unsigned char bits_ch; /* bitspersample * channels */

    id3->vbr      = false;   /* All Sun audio files are CBR */
    id3->filesize = filesize(fd);
    id3->length   = 0;

    if ((lseek(fd, 0, SEEK_SET) < 0) || ((read_bytes = read(fd, buf, 24)) < 0))
        return false;

    if (read_bytes < 24 || (memcmp(buf, ".snd", 4) != 0))
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
        bits_ch = 1;
    }
    else
    {
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

        bits_ch = get_au_bitspersample(get_long_be(buf + 12)) * get_long_be(buf + 20);
        id3->frequency = get_long_be(buf + 16);
    }

    /* Calculate track length [ms] */
    if (bits_ch)
        id3->length = ((int64_t)numbytes * 1000LL) / (bits_ch * id3->frequency);

    return true;
}
