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
#include "debug.h"
#include "rbunicode.h"

bool get_spc_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char * buf = (unsigned char *)id3->path;
    int read_bytes;
    char * p;
    
    unsigned long length;
    unsigned long fade;
    bool isbinary = true;
    int i;
    
    /* try to get the ID666 tag */
    if ((lseek(fd, 0x2e, SEEK_SET) < 0)
        || ((read_bytes = read(fd, buf, 0xD2)) < 0xD2))
    {
        DEBUGF("lseek or read failed\n");
        return false;
    }

    p = id3->id3v2buf;
    
    id3->title = p;
    buf[31] = 0;
    p = iso_decode(buf, p, 0, 32);
    buf += 32;

    id3->album = p;
    buf[31] = 0;
    p = iso_decode(buf, p, 0, 32);
    buf += 48;
    
    id3->comment = p;
    buf[31] = 0;
    p = iso_decode(buf, p, 0, 32);
    buf += 32;
    
    /* Date check */
    if(buf[2] == '/' && buf[5] == '/')
        isbinary = false;
    
    /* Reserved bytes check */
    if(buf[0xD2 - 0x2E - 112] >= '0' &&
       buf[0xD2 - 0x2E - 112] <= '9' &&
       buf[0xD3 - 0x2E - 112] == 0x00)
        isbinary = false;
    
    /* is length & fade only digits? */
    for (i=0;i<8 && ( 
        (buf[0xA9 - 0x2E - 112+i]>='0'&&buf[0xA9 - 0x2E - 112+i]<='9') ||
        buf[0xA9 - 0x2E - 112+i]=='\0');
        i++);
    if (i==8) isbinary = false;
    
    if(isbinary) {
        id3->year = buf[0] | (buf[1]<<8);
        buf += 11;
        
        length = (buf[0] | (buf[1]<<8) | (buf[2]<<16)) * 1000;
        buf += 3;
        
        fade = (buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24));
        buf += 4;
    } else {
        char tbuf[6];
        
        buf += 6;
        buf[4] = 0;
        id3->year = atoi(buf);
        buf += 5;
        
        memcpy(tbuf, buf, 3);
        tbuf[3] = 0;
        length = atoi(tbuf) * 1000;
        buf += 3;
        
        memcpy(tbuf, buf, 5);
        tbuf[5] = 0;
        fade = atoi(tbuf);
        buf += 5;
    }
    
    id3->artist = p;
    buf[31] = 0;
    p = iso_decode(buf, p, 0, 32);
    
    if (length==0) {
        length=3*60*1000; /* 3 minutes */
        fade=5*1000; /* 5 seconds */
    }
    
    id3->length = length+fade;

    return true;
}
