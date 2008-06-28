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
#include "id3.h"
#include "metadata_common.h"
#include "metadata_parsers.h"

bool get_aiff_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    unsigned long numChannels = 0;
    unsigned long numSampleFrames = 0;
    unsigned long sampleSize = 0;
    unsigned long sampleRate = 0;
    unsigned long numbytes = 0;
    int read_bytes;
    int i;

    if ((lseek(fd, 0, SEEK_SET) < 0) 
        || ((read_bytes = read(fd, buf, sizeof(id3->path))) < 54))
    {
        return false;
    }
    
    if ((memcmp(buf, "FORM",4) != 0)
        || (memcmp(&buf[8], "AIFF", 4) !=0 ))
    {
        return false;
    }

    buf += 12;
    read_bytes -= 12;

    while ((numbytes == 0) && (read_bytes >= 8)) 
    {
        /* chunkSize */
        i = get_long_be(&buf[4]);
        
        if (memcmp(buf, "COMM", 4) == 0)
        {
            /* numChannels */
            numChannels = ((buf[8]<<8)|buf[9]);
            /* numSampleFrames */
            numSampleFrames = get_long_be(&buf[10]);
            /* sampleSize */
            sampleSize = ((buf[14]<<8)|buf[15]);
            /* sampleRate */
            sampleRate = get_long_be(&buf[18]);
            sampleRate = sampleRate >> (16+14-buf[17]);
            /* save format infos */
            id3->bitrate = (sampleSize * numChannels * sampleRate) / 1000;
            id3->frequency = sampleRate;
            id3->length = ((int64_t) numSampleFrames * 1000) / id3->frequency;

            id3->vbr = false;   /* AIFF files are CBR */
            id3->filesize = filesize(fd);
        }
        else if (memcmp(buf, "SSND", 4) == 0) 
        {
            numbytes = i - 8;
        }

        if (i & 0x01)
        {
            i++;  /* odd chunk sizes must be padded */
        }
        buf += i + 8;
        read_bytes -= i + 8;
    }

    if ((numbytes == 0) || (numChannels == 0)) 
    {
        return false;
    }
    return true;
}
