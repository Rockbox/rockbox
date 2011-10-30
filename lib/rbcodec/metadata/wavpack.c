/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "platform.h"

#include "metadata.h"
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
 * now converted to standard Rockbox format in the decoder, and also
 * handles the case where up to 15 non-audio blocks might occur at the
 * beginning of the file.
 */

bool get_wavpack_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    uint32_t totalsamples = (uint32_t) -1;
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

    /* check up to 16 headers before we give up finding one with audio */

    for (i = 0; i < 16; ++i) {
        uint32_t meta_bytes = get_long_le(&buf [4]) - 24;
        uint32_t trial_totalsamples = get_long_le(&buf[12]);
        uint32_t blockindex = get_long_le(&buf[16]);
        uint32_t blocksamples = get_long_le(&buf[20]);
        uint32_t flags = get_long_le(&buf[24]);

        if (totalsamples == (uint32_t) -1 && blockindex == 0)
            totalsamples = trial_totalsamples;

        if (blocksamples) {
            int srindx = ((buf [26] >> 7) & 1) + ((buf [27] << 1) & 14);

            if (srindx == 15) {
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

            /* if the total number of samples is still unknown, make a guess on the high side (for now) */

            if (totalsamples == (uint32_t) -1) {
                totalsamples = id3->filesize * 3;

                if (!(flags & HYBRID_FLAG))
                    totalsamples /= 2;

                if (!(flags & MONO_FLAG))
                    totalsamples /= 2;
            }

            id3->length = ((int64_t) totalsamples * 1000) / id3->frequency;
            id3->bitrate = id3->filesize / (id3->length / 8);

            read_ape_tags(fd, id3);
            return true;
        }
        else {   /* block did not contain audio, so seek to the end and see if there's another */
            if ((meta_bytes > 0 && lseek(fd, meta_bytes, SEEK_CUR) < 0) ||
                read(fd, buf, 32) < 32 || memcmp (buf, "wvpk", 4) != 0)
                    break;
        }
    }

    return false;
}
