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
#include "structec.h"
#include "logf.h"

/* Read the items in a Vorbis comment packet. Returns true the items were
 * fully read, false otherwise.
 */
bool read_vorbis_tags(int fd, struct mp3entry *id3, 
    long tag_remaining)
{
    char *buf = id3->id3v2buf;
    int32_t comment_count;
    int32_t len;
    int buf_remaining = sizeof(id3->id3v2buf) + sizeof(id3->id3v1buf);
    int i;

    if (ecread(fd, &len, 1, "l", IS_BIG_ENDIAN) < (long) sizeof(len)) 
    {
        return false;
    }
    
    if ((lseek(fd, len, SEEK_CUR) < 0)
        || (ecread(fd, &comment_count, 1, "l", IS_BIG_ENDIAN) 
            < (long) sizeof(comment_count)))
    {
        return false;
    }
    
    tag_remaining -= len + sizeof(len) + sizeof(comment_count);

    if (tag_remaining <= 0)
    {
        return true;
    }

    for (i = 0; i < comment_count; i++)
    {
        char name[TAG_NAME_LENGTH];
        char value[TAG_VALUE_LENGTH];
        int32_t read_len;

        if (tag_remaining < 4) 
        {
            break;
        }

        if (ecread(fd, &len, 1, "l", IS_BIG_ENDIAN) < (long) sizeof(len))
        {
            return false;
        }

        tag_remaining -= 4;

        /* Quit if we've passed the end of the page */
        if (tag_remaining < len)
        {
            break;
        }

        tag_remaining -= len;
        read_len = read_string(fd, name, sizeof(name), '=', len);
        
        if (read_len < 0)
        {
            return false;
        }

        len -= read_len;

        if (read_string(fd, value, sizeof(value), -1, len) < 0)
        {
            return false;
        }

        len = parse_tag(name, value, id3, buf, buf_remaining, 
            TAGTYPE_VORBIS);
        buf += len;
        buf_remaining -= len;
    }

    /* Skip to the end of the block */
    if (tag_remaining) 
    {
        if (lseek(fd, tag_remaining, SEEK_CUR) < 0)
        {
            return false;
        }
    }

    return true;
}
