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
#include "rbunicode.h"

bool get_mod_metadata(int fd, struct mp3entry* id3)
{    
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char buf[1084];
    int read_bytes;
    char *p;
      

    if ((lseek(fd, 0, SEEK_SET) < 0) 
         || ((read_bytes = read(fd, buf, sizeof(buf))) < 1084))
    {
        return false;
    }
    
    /* We don't do file format checking here
     * There can be .mod files without any signatures out there */

    p = id3->id3v2buf;
    
    /* Copy Title as artist */
    strcpy(p, &buf[0x00]);
    id3->artist = p;
    p += strlen(p)+1;

    id3->bitrate = filesize(fd)/1024; /* size in kb */
    id3->frequency = 44100;
    id3->length = 120*1000;
    id3->vbr = false;
    id3->filesize = filesize(fd);
        
    return true;
}

