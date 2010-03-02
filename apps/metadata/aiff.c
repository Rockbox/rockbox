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

#include "system.h"
#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"

/* compressionType: AIFC QuickTime IMA ADPCM */
#define AIFC_FORMAT_QT_IMA_ADPCM "ima4"

bool get_aiff_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    unsigned long numChannels = 0;
    unsigned long numSampleFrames = 0;
    unsigned long numbytes = 0;
    int read_bytes;
    int i;
    bool is_aifc = false;

    if ((lseek(fd, 0, SEEK_SET) < 0) ||
        ((read_bytes = read(fd, buf, sizeof(id3->path))) < 54) ||
        (memcmp(buf, "FORM", 4) != 0) || (memcmp(buf + 8, "AIF", 3) != 0) ||
        (!(is_aifc = (buf[11] == 'C')) && buf[11] != 'F'))
    {
        return false;
    }

    i = 12;
    while ((numbytes == 0) && (read_bytes >= 8)) 
    {
        buf        += i;
        read_bytes -= i;

        /* chunkSize */
        i = get_long_be(&buf[4]);
        
        if (memcmp(buf, "COMM", 4) == 0)
        {
            /* numChannels */
            numChannels = ((buf[8]<<8)|buf[9]);
            /* numSampleFrames */
            numSampleFrames = get_long_be(&buf[10]);
            /* sampleRate */
            id3->frequency = get_long_be(&buf[18]);
            id3->frequency >>= (16+14-buf[17]);
            /* save format infos */
            id3->bitrate = (((buf[14]<<8)|buf[15]) * numChannels * id3->frequency) / 1000;
            if (!is_aifc || memcmp(&buf[26], AIFC_FORMAT_QT_IMA_ADPCM, 4) != 0)
                id3->length = ((int64_t) numSampleFrames * 1000) / id3->frequency;
            else
            {
                /* QuickTime IMA ADPCM is 1block = 64 data for each channel */
                id3->length = ((int64_t) numSampleFrames * 64000LL) / id3->frequency;
            }

            id3->vbr = false;   /* AIFF files are CBR */
            id3->filesize = filesize(fd);
        }
        else if (memcmp(buf, "SSND", 4) == 0) 
        {
            numbytes = i - 8;
        }

        /* odd chunk sizes must be padded */
        i += 8 + (i & 0x01);
    }

    return ((numbytes != 0) && (numChannels != 0));
}
