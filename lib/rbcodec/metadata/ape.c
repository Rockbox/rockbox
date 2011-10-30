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

#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "structec.h"

#define APETAG_HEADER_LENGTH        32
#define APETAG_HEADER_FORMAT        "8llll8"
#define APETAG_ITEM_HEADER_FORMAT   "ll"
#define APETAG_ITEM_TYPE_MASK       3

#ifdef HAVE_ALBUMART
/* The AA header consists of the pseudo filename "Album Cover (Front).ext"
 * whereas ".ext" is the file extension. For now ".jpg" and ".png" are
 * supported by this APE metadata parser. Therefore the length is 22. */
#define APETAG_AA_HEADER_LENGTH     22
#endif

struct apetag_header 
{
    char id[8];
    long version;
    long length;
    long item_count;
    long flags;
    char reserved[8];
};

struct apetag_item_header 
{
    long length;
    long flags;
};

/* Read the items in an APEV2 tag. Only looks for a tag at the end of a 
 * file. Returns true if a tag was found and fully read, false otherwise.
 */
bool read_ape_tags(int fd, struct mp3entry* id3)
{
    struct apetag_header header;

    if ((lseek(fd, -APETAG_HEADER_LENGTH, SEEK_END) < 0)
        || (ecread(fd, &header, 1, APETAG_HEADER_FORMAT, IS_BIG_ENDIAN)
            != APETAG_HEADER_LENGTH)
        || (memcmp(header.id, "APETAGEX", sizeof(header.id))))
    {
        return false;
    }

    if ((header.version == 2000) && (header.item_count > 0)
        && (header.length > APETAG_HEADER_LENGTH)) 
    {
        char *buf = id3->id3v2buf;
        unsigned int buf_remaining = sizeof(id3->id3v2buf) 
            + sizeof(id3->id3v1buf);
        unsigned int tag_remaining = header.length - APETAG_HEADER_LENGTH;
        int i;
        
        if (lseek(fd, -header.length, SEEK_END) < 0)
        {
            return false;
        }

        for (i = 0; i < header.item_count; i++)
        {
            struct apetag_item_header item;
            char name[TAG_NAME_LENGTH];
            char value[TAG_VALUE_LENGTH];
            long r;

            if (tag_remaining < sizeof(item))
            {
                break;
            }
            
            if (ecread(fd, &item, 1, APETAG_ITEM_HEADER_FORMAT, IS_BIG_ENDIAN)
                < (long) sizeof(item))
            {
                return false;
            }
            
            tag_remaining -= sizeof(item);
            r = read_string(fd, name, sizeof(name), 0, tag_remaining);
            
            if (r == -1)
            {
                return false;
            }

            tag_remaining -= r + item.length;

            if ((item.flags & APETAG_ITEM_TYPE_MASK) == 0) 
            {
                long len;
                
                if (read_string(fd, value, sizeof(value), -1, item.length) 
                    != item.length)
                {
                    return false;
                }

                len = parse_tag(name, value, id3, buf, buf_remaining, 
                    TAGTYPE_APE);
                buf += len;
                buf_remaining -= len;
            }
            else
            {
#ifdef HAVE_ALBUMART
                if (strcasecmp(name, "cover art (front)") == 0)
                {
                    /* Allow to read at least APETAG_AA_HEADER_LENGTH bytes. */
                    r = read_string(fd, name, sizeof(name), 0, APETAG_AA_HEADER_LENGTH);
                    if (r == -1)
                    {
                        return false;
                    }

                    /* Gather the album art format from the pseudo file name's ending. */
                    strcpy(name, name + strlen(name) - 4);
                    id3->albumart.type = AA_TYPE_UNKNOWN;
                    if      (strcasecmp(name, ".jpg") == 0)
                    {
                        id3->albumart.type = AA_TYPE_JPG;
                    }
                    else if (strcasecmp(name, ".png") == 0)
                    {
                        id3->albumart.type = AA_TYPE_PNG;
                    }

                    /* Set the album art size and position. */
                    if (id3->albumart.type != AA_TYPE_UNKNOWN)
                    {
                        id3->albumart.pos  = lseek(fd, 0, SEEK_CUR);
                        id3->albumart.size = item.length - r;
                        id3->has_embedded_albumart = true;
                    }
                    
                    /* Seek back to this APE items begin. */
                    if (lseek(fd, -r, SEEK_CUR) < 0)
                    {
                        return false;
                    }
                }
#endif
                /* Seek to the next APE item. */
                if (lseek(fd, item.length, SEEK_CUR) < 0)
                {
                    return false;
                }
            }
        }
    }

    return true;
}
