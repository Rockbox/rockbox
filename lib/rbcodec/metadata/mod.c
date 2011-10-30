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
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include "platform.h"

#include "metadata.h"
#include <string-extra.h>
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "rbunicode.h"

#define MODULEHEADERSIZE 0x438

bool get_mod_metadata(int fd, struct mp3entry* id3)
{    
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char *buf = id3->id3v2buf;
    unsigned char id[4];
    bool is_mod_file = false;

    /* Seek to file begin */
    if (lseek(fd, 0, SEEK_SET) < 0)
        return false;
    /* Use id3v2buf as buffer for the track name */
    if (read(fd, buf, sizeof(id3->id3v2buf)) < (ssize_t)sizeof(id3->id3v2buf))
        return false;
    /* Seek to MOD ID position */
    if (lseek(fd, MODULEHEADERSIZE, SEEK_SET) < 0)
        return false;
    /* Read MOD ID */
    if (read(fd, id, sizeof(id)) < (ssize_t)sizeof(id))
        return false;

    /* Mod type checking based on MikMod */
    /* Protracker and variants */
    if ((!memcmp(id, "M.K.", 4)) || (!memcmp(id, "M!K!", 4))) {
        is_mod_file = true;
    }
    
    /* Star Tracker */
    if (((!memcmp(id, "FLT", 3)) || (!memcmp(id, "EXO", 3))) &&
        (isdigit(id[3]))) {
        char numchn = id[3] - '0';
        if (numchn == 4 || numchn == 8)
            is_mod_file = true;
    }

    /* Oktalyzer (Amiga) */
    if (!memcmp(id, "OKTA", 4)) {
        is_mod_file = true;
    }

    /* Oktalyser (Atari) */
    if (!memcmp(id, "CD81", 4)) {
        is_mod_file = true;
    }

    /* Fasttracker */
    if ((!memcmp(id + 1, "CHN", 3)) && (isdigit(id[0]))) {
        is_mod_file = true;
    }
    /* Fasttracker or Taketracker */
    if (((!memcmp(id + 2, "CH", 2)) || (!memcmp(id + 2, "CN", 2)))
        && (isdigit(id[0])) && (isdigit(id[1]))) {
        is_mod_file = true;
    }

    /* Don't try to play if we can't find a known mod type
     * (there are mod files which have nothing to do with music) */
    if (!is_mod_file)
        return false;

    id3->title = id3->id3v2buf; /* Point title to previous read ID3 buffer. */
    id3->bitrate = filesize(fd)/1024; /* size in kb */
    id3->frequency = 44100;
    id3->length = 120*1000;
    id3->vbr = false;
    id3->filesize = filesize(fd);
        
    return true;
}

