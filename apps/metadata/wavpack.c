/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2007 David Bryant
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
#include "id3.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "logf.h"

#define ID_UNIQUE               0x3f
#define ID_LARGE                0x80
#define ID_SAMPLE_RATE          0x27

#define MONO_FLAG       4
#define HYBRID_FLAG     8

static const long wavpack_sample_rates [] = 
{
     6000,  8000,  9600, 11025, 12000, 16000,  22050, 24000, 
    32000, 44100, 48000, 64000, 88200, 96000, 192000 
};

/* A simple parser to read basic information from a WavPack file. This
 * now works with self-extrating WavPack files and also will scan the
 * metadata for non-standard sampling rates. This no longer fails on
 * WavPack files containing floating-point audio data because these are
 * now converted to standard Rockbox format in the decoder.
 */

bool get_wavpack_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    uint32_t totalsamples, blocksamples, flags;
    int i;

    for (i = 0; i < 256; ++i) {

        /* at every 256 bytes into file, try to read a WavPack header */

        if ((lseek(fd, i * 256, SEEK_SET) < 0) || (read(fd, buf, 32) < 32))
            return false;

        /* if valid WavPack 4 header version, break */

        if (memcmp (buf, "wvpk", 4) == 0 && buf [9] == 4 &&
            (buf [8] >= 2 && buf [8] <= 0x10))
                break;
    }

    if (i == 256) {
        logf ("Not a WavPack file");
        return false;
    }

    id3->vbr = true;   /* All WavPack files are VBR */
    id3->filesize = filesize (fd);
    totalsamples = get_long_le(&buf[12]);
    blocksamples = get_long_le(&buf[20]);
    flags = get_long_le(&buf[24]);

    if (blocksamples) {
        int srindx = ((buf [26] >> 7) & 1) + ((buf [27] << 1) & 14);

        if (srindx == 15) {
            uint32_t meta_bytes = buf [4] + (buf [5] << 8) + (buf [6] << 16) - 24;
            uint32_t meta_size;

            id3->frequency = 44100;

            while (meta_bytes >= 6) {
                if (read(fd, buf, 2) < 2)
                    break;

                if (buf [0] & ID_LARGE) {
                    if (read(fd, buf + 2, 2) < 2)
                        break;

                    meta_size = (buf [1] << 1) + (buf [2] << 9) + (buf [3] << 17);
                    meta_bytes -= meta_size + 4;
                }
                else {
                    meta_size = buf [1] << 1;
                    meta_bytes -= meta_size + 2;

                    if ((buf [0] & ID_UNIQUE) == ID_SAMPLE_RATE) {
                        if (meta_size == 4 && read(fd, buf + 2, 4) == 4)
                            id3->frequency = buf [2] + (buf [3] << 8) + (buf [4] << 16);

                        break;
                    }
                }

                if (meta_size > 0 && lseek(fd, meta_size, SEEK_CUR) < 0)
                    break;
            }
        }
        else
            id3->frequency = wavpack_sample_rates[srindx];

        /* if the total number of samples is unknown, make a guess on the high side (for now) */

        if (totalsamples == (uint32_t) -1) {
            totalsamples = filesize (fd) * 3;

            if (!(flags & HYBRID_FLAG))
                totalsamples /= 2;

            if (!(flags & MONO_FLAG))
                totalsamples /= 2;
        }

        id3->length = ((int64_t) totalsamples * 1000) / id3->frequency;
        id3->bitrate = filesize (fd) / (id3->length / 8);

        return true;
    }

    return false;
}
