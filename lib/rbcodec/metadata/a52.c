/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 Dave Chapman
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
#include "metadata.h"
#include "logf.h"
#include "metadata_parsers.h"
#include "platform.h"

static const unsigned short a52_bitrates[] =
{
     32,  40,  48,  56,  64,  80,  96, 112, 128, 160, 
    192, 224, 256, 320, 384, 448, 512, 576, 640
};

/* Only store frame sizes for 44.1KHz - others are simply multiples 
   of the bitrate */
static const unsigned short a52_441framesizes[] =
{
      69 * 2,   70 * 2,   87 * 2,   88 * 2,  104 * 2,  105 * 2,  121 * 2, 
     122 * 2,  139 * 2,  140 * 2,  174 * 2,  175 * 2,  208 * 2,  209 * 2,
     243 * 2,  244 * 2,  278 * 2,  279 * 2,  348 * 2,  349 * 2,  417 * 2,
     418 * 2,  487 * 2,  488 * 2,  557 * 2,  558 * 2,  696 * 2,  697 * 2,  
     835 * 2,  836 * 2,  975 * 2,  976 * 2, 1114 * 2, 1115 * 2, 1253 * 2, 
    1254 * 2, 1393 * 2, 1394 * 2
};

/* How far into the file to look for the first frame */
#define A52_MAX_SCAN (64 * 1024)
#define A52_HEADER_SIZE 6

/* Return the size in bytes of the frame starting with hdr, or 0 if hdr is
   not a valid frame header. */
static int a52_frame_size(const unsigned char *hdr)
{
    int frmsizecod = hdr[4] & 0x3f;
    int bitrate;

    if ((hdr[0] != 0x0b) || (hdr[1] != 0x77) || (frmsizecod > 37)
        || (hdr[5] >= 0x60))   /* bsid >= 12 is not A52 */
    {
        return 0;
    }

    bitrate = a52_bitrates[frmsizecod >> 1];

    switch (hdr[4] & 0xc0)
    {
    case 0x00:
        return bitrate * 2 * 2;
    case 0x40:
        return a52_441framesizes[frmsizecod];
    case 0x80:
        return bitrate * 3 * 2;
    default:
        return 0;
    }
}

/* Find the first frame in the file, which is not at the start if the file
   was cut from a longer stream. A match must be followed by a second valid
   header so that sync words inside the audio data are skipped. Returns the
   offset of the frame and leaves its header in hdr, or -1 if none found. */
static off_t a52_find_first_frame(int fd, unsigned char *buf, size_t bufsize,
                                  unsigned char *hdr)
{
    unsigned char next[A52_HEADER_SIZE];
    off_t pos = 0;
    ssize_t n;
    int i, size;

    while (pos < A52_MAX_SCAN)
    {
        if ((lseek(fd, pos, SEEK_SET) < 0)
            || ((n = read(fd, buf, bufsize)) < A52_HEADER_SIZE))
        {
            return -1;
        }

        for (i = 0; i <= n - A52_HEADER_SIZE; i++)
        {
            size = a52_frame_size(&buf[i]);
            if (!size)
                continue;

            /* A frame at the very start is accepted as before, even if the
               file holds only one frame */
            if ((pos + i == 0)
                || ((lseek(fd, pos + i + size, SEEK_SET) >= 0)
                    && (read(fd, next, sizeof(next)) == sizeof(next))
                    && a52_frame_size(next)))
            {
                memcpy(hdr, &buf[i], A52_HEADER_SIZE);
                return pos + i;
            }
        }

        /* Overlap so a header split across reads is still found */
        pos += n - (A52_HEADER_SIZE - 1);
    }

    return -1;
}

bool get_a52_metadata(int fd, struct mp3entry *id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    unsigned char hdr[A52_HEADER_SIZE];
    unsigned long totalsamples;
    off_t offset;
    int i;

    offset = a52_find_first_frame(fd, buf, sizeof(id3->path), hdr);
    if (offset < 0)
    {
        logf("not an A52/AC3 file\n");
        return false;
    }

    i = hdr[4] & 0x3e;

    id3->bitrate = a52_bitrates[i >> 1];
    id3->vbr = false;
    id3->filesize = ffilesize(fd);
    id3->first_frame_offset = offset;

    switch (hdr[4] & 0xc0)
    {
    case 0x00:
        id3->frequency = 48000;
        id3->bytesperframe=id3->bitrate * 2 * 2;
        break;

    case 0x40:
        id3->frequency = 44100;
        id3->bytesperframe = a52_441framesizes[i];
        break;
    
    case 0x80: 
        id3->frequency = 32000;
        id3->bytesperframe = id3->bitrate * 3 * 2;
        break;
    }

    /* One A52 frame contains 6 blocks, each containing 256 samples */
    totalsamples = (id3->filesize - offset) / id3->bytesperframe * 6 * 256;
    id3->length = totalsamples / id3->frequency * 1000;
    return true;
}
