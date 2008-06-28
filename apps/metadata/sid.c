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
#include "rbunicode.h"

/* PSID metadata info is available here: 
   http://www.unusedino.de/ec64/technical/formats/sidplay.html */
bool get_sid_metadata(int fd, struct mp3entry* id3)
{    
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;    
    int read_bytes;
    char *p;
    

    if ((lseek(fd, 0, SEEK_SET) < 0) 
         || ((read_bytes = read(fd, buf, 0x80)) < 0x80))
    {
        return false;
    }
    
    if ((memcmp(buf, "PSID",4) != 0))
    {
        return false;
    }

    p = id3->id3v2buf;

    /* Copy Title (assumed max 0x1f letters + 1 zero byte) */
    id3->title = p;
    buf[0x16+0x1f] = 0;
    p = iso_decode(&buf[0x16], p, 0, strlen(&buf[0x16])+1);

    /* Copy Artist (assumed max 0x1f letters + 1 zero byte) */
    id3->artist = p;
    buf[0x36+0x1f] = 0;
    p = iso_decode(&buf[0x36], p, 0, strlen(&buf[0x36])+1);

    /* Copy Year (assumed max 4 letters + 1 zero byte) */
    buf[0x56+0x4] = 0;
    id3->year = atoi(&buf[0x56]);

    /* Copy Album (assumed max 0x1f-0x05 letters + 1 zero byte) */
    id3->album = p;
    buf[0x56+0x1f] = 0;
    p = iso_decode(&buf[0x5b], p, 0, strlen(&buf[0x5b])+1);

    id3->bitrate = 706;
    id3->frequency = 44100;
    /* New idea as posted by Marco Alanen (ravon):
     * Set the songlength in seconds to the number of subsongs
     * so every second represents a subsong.
     * Users can then skip the current subsong by seeking
     *
     * Note: the number of songs is a 16bit value at 0xE, so this code only
     * uses the lower 8 bits of the counter.
    */
    id3->length = (buf[0xf]-1)*1000;
    id3->vbr = false;
    id3->filesize = filesize(fd);

    return true;
}
