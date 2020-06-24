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

#include "debug.h"

/* compressionType: AIFC QuickTime IMA ADPCM */
#define AIFC_FORMAT_QT_IMA_ADPCM "ima4"

static void read_id3_tags(int fd, struct mp3entry* id3)
{
    id3->tracknum = 0;
    id3->discnum  = 0;
    setid3v2title(fd, id3);
}

bool get_aiff_metadata(int fd, struct mp3entry* id3)
{
    unsigned char buf[512];
    unsigned long numChannels = 0;
    unsigned long numSampleFrames = 0;
    unsigned long numbytes = 0;
    unsigned long offset = 0;
    bool is_aifc = false;
    char *p=id3->id3v2buf;


    if ((lseek(fd, 0, SEEK_SET) < 0) || (read(fd, &buf[0], 12) < 12) ||
        (memcmp(&buf[0], "FORM", 4) != 0) || (memcmp(&buf[8], "AIF", 3) != 0) ||
        (!(is_aifc = (buf[11] == 'C')) && buf[11] != 'F'))
    {
        return false;
    }


    while (read(fd, &buf[0], 8) == 8)
    {
        size_t size = get_long_be(&buf[4]); /* chunkSize */

        if (memcmp(&buf[0], "SSND", 4) == 0)
        {
            numbytes = size - 8;

            /* check for ID3 tag */
            offset=lseek(fd, 0, SEEK_CUR);
            lseek(fd, size, SEEK_CUR);
            if ((read(fd, &buf[0], 8) == 8) &&  (memcmp(&buf[0], "ID3", 3) == 0))
            {
                id3->id3v2len = get_long_be(&buf[4]);
                read_id3_tags(fd, id3);
            }
            else
                DEBUGF("ID3 tag not present immediately after sound data");
            lseek(fd, offset, SEEK_SET);
            break;  /* assume COMM was already read */
        }

        /* odd chunk sizes must be padded */
        size += size & 1;

        if (size > sizeof(buf))
        {
            DEBUGF("AIFF \"%4.4s\" chunk too large (%zd > %zd)",
                    (char*) &buf[0], size, sizeof(buf));
        }


        if (memcmp(&buf[0], "NAME", 4) == 0)
        {
            read_string(fd, p, 512, 0, size);
            id3->title=p;
            p+=size;
        }

        else if (memcmp(&buf[0], "AUTH", 4) == 0)
        {
            read_string(fd, p, 512, 0, size);
            id3->artist=p;
            p+=size;
        }

        else if (memcmp(&buf[0], "ANNO", 4) == 0)
        {
            read_string(fd, p, 512, 0, size);
            id3->comment=p;
            p+=size;
        }

        else if (memcmp(&buf[0], "ID3", 3) == 0)
        {
            read_id3_tags(fd, id3);
        }


        else if (memcmp(&buf[0], "COMM", 4) == 0)
        {
            if (size > sizeof(buf) || read(fd, &buf[0], size) != (ssize_t)size)
                return false;

            numChannels = ((buf[0]<<8)|buf[1]);

            numSampleFrames = get_long_be(&buf[2]);

            /* sampleRate */
            id3->frequency = get_long_be(&buf[10]);
            id3->frequency >>= (16+14-buf[9]);

            /* save format infos */
            id3->bitrate = ((buf[6]<<8)|buf[7]) * numChannels * id3->frequency;
            id3->bitrate /= 1000;

            if (!is_aifc || memcmp(&buf[18], AIFC_FORMAT_QT_IMA_ADPCM, 4) != 0)
                id3->length = ((int64_t) numSampleFrames * 1000) / id3->frequency;
            else
            {
                /* QuickTime IMA ADPCM is 1block = 64 data for each channel */
                id3->length = ((int64_t) numSampleFrames * 64000LL) / id3->frequency;
            }

            id3->vbr = false;   /* AIFF files are CBR */
            id3->filesize = filesize(fd);
        }
        else
        {
            /* skip chunk */
            if (lseek(fd, size, SEEK_CUR) < 0)
                return false;
        }
    }

    return numbytes && numChannels;
}
