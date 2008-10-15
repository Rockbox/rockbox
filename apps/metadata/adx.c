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
#include "debug.h"

bool get_adx_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char * buf = (unsigned char *)id3->path;
    int chanstart, channels, read_bytes;
    int looping = 0, start_adr = 0, end_adr = 0;
    
    /* try to get the basic header */
    if ((lseek(fd, 0, SEEK_SET) < 0)
        || ((read_bytes = read(fd, buf, 0x38)) < 0x38))
    {
        DEBUGF("lseek or read failed\n");
        return false;
    }
    
    /* ADX starts with 0x80 */
    if (buf[0] != 0x80) {
        DEBUGF("get_adx_metadata: wrong first byte %c\n",buf[0]);
        return false;
    }
    
    /* check for a reasonable offset */
    chanstart = ((buf[2] << 8) | buf[3]) + 4;
    if (chanstart > 4096) {
        DEBUGF("get_adx_metadata: bad chanstart %i\n", chanstart);
        return false;
    }

    /* check for a workable number of channels */
    channels = buf[7];
    if (channels != 1 && channels != 2) {
        DEBUGF("get_adx_metadata: bad channel count %i\n",channels);
        return false;
    }
    
    id3->frequency = get_long_be(&buf[8]);
    /* 32 samples per 18 bytes */
    id3->bitrate = id3->frequency * channels * 18 * 8 / 32 / 1000;
    id3->length = get_long_be(&buf[12]) / id3->frequency * 1000;
    id3->vbr = false;
    id3->filesize = filesize(fd);
    
    /* get loop info */
    if (!memcmp(buf+0x10,"\x01\xF4\x03\x00",4)) {
        /* Soul Calibur 2 style (type 03) */
        DEBUGF("get_adx_metadata: type 03 found\n");
        /* check if header is too small for loop data */
        if (chanstart-6 < 0x2c) looping=0;
        else {
            looping = get_long_be(&buf[0x18]);
            end_adr = get_long_be(&buf[0x28]);
            start_adr = get_long_be(&buf[0x1c])/32*channels*18+chanstart;
        }
    } else if (!memcmp(buf+0x10,"\x01\xF4\x04\x00",4)) {
        /* Standard (type 04) */
        DEBUGF("get_adx_metadata: type 04 found\n");
        /* check if header is too small for loop data */
        if (chanstart-6 < 0x38) looping=0;
        else {
            looping = get_long_be(&buf[0x24]);
            end_adr = get_long_be(&buf[0x34]);
            start_adr = get_long_be(&buf[0x28])/32*channels*18+chanstart;
        }
    } else {
        DEBUGF("get_adx_metadata: error, couldn't determine ADX type\n");
        return false;
    }
    
    if (looping) {
        /* 2 loops, 10 second fade */
        id3->length = (start_adr-chanstart + 2*(end_adr-start_adr))
                      *8 / id3->bitrate + 10000;
    }
        
    /* try to get the channel header */
    if ((lseek(fd, chanstart-6, SEEK_SET) < 0)
        || ((read_bytes = read(fd, buf, 6)) < 6))
    {
        return false;
    }
    
    /* check channel header */
    if (memcmp(buf, "(c)CRI", 6) != 0) return false;
    
    return true;    
}
