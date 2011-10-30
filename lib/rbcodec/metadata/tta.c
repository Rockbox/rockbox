/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Yoshihisa Uchida
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
#include "logf.h"

#define TTA1_SIGN    0x31415454

#define TTA_HEADER_ID              0
#define TTA_HEADER_AUDIO_FORMAT    (TTA_HEADER_ID              + sizeof(unsigned int))
#define TTA_HEADER_NUM_CHANNELS    (TTA_HEADER_AUDIO_FORMAT    + sizeof(unsigned short))
#define TTA_HEADER_BITS_PER_SAMPLE (TTA_HEADER_NUM_CHANNELS    + sizeof(unsigned short))
#define TTA_HEADER_SAMPLE_RATE     (TTA_HEADER_BITS_PER_SAMPLE + sizeof(unsigned short))
#define TTA_HEADER_DATA_LENGTH     (TTA_HEADER_SAMPLE_RATE     + sizeof(unsigned int))
#define TTA_HEADER_CRC32           (TTA_HEADER_DATA_LENGTH     + sizeof(unsigned int))
#define TTA_HEADER_SIZE            (TTA_HEADER_CRC32           + sizeof(unsigned int))

#define TTA_HEADER_GETTER_ID(x)              get_long_le(x)
#define TTA_HEADER_GETTER_AUDIO_FORMAT(x)    get_short_le(x)
#define TTA_HEADER_GETTER_NUM_CHANNELS(x)    get_short_le(x)
#define TTA_HEADER_GETTER_BITS_PER_SAMPLE(x) get_short_le(x)
#define TTA_HEADER_GETTER_SAMPLE_RATE(x)     get_long_le(x)
#define TTA_HEADER_GETTER_DATA_LENGTH(x)     get_long_le(x)
#define TTA_HEADER_GETTER_CRC32(x)           get_long_le(x)

#define GET_HEADER(x, tag) TTA_HEADER_GETTER_ ## tag((x) + TTA_HEADER_ ## tag)

static void read_id3_tags(int fd, struct mp3entry* id3)
{
    id3->title    = NULL;
    id3->filesize = filesize(fd);
    id3->id3v2len = getid3v2len(fd);
    id3->tracknum = 0;
    id3->discnum  = 0;
    id3->vbr      = false;   /* All TTA files are CBR */

    /* first get id3v2 tags. if no id3v2 tags ware found, get id3v1 tags */
    if (id3->id3v2len)
    {
        setid3v2title(fd, id3);
        id3->first_frame_offset = id3->id3v2len;
        return;
    }
    setid3v1title(fd, id3);
}

bool get_tta_metadata(int fd, struct mp3entry* id3)
{
    unsigned char ttahdr[TTA_HEADER_SIZE];
    unsigned int datasize;
    unsigned int origsize;
    int bps;

    lseek(fd, 0, SEEK_SET);

    /* read id3 tags */
    read_id3_tags(fd, id3);
    lseek(fd, id3->id3v2len, SEEK_SET);

    /* read TTA header */
    if (read(fd, ttahdr, TTA_HEADER_SIZE) < 0)
        return false;

    /* check for TTA3 signature */
    if ((GET_HEADER(ttahdr, ID)) != TTA1_SIGN)
        return false;

    /* skip check CRC */

    id3->channels  = (GET_HEADER(ttahdr, NUM_CHANNELS));
    id3->frequency = (GET_HEADER(ttahdr, SAMPLE_RATE));
    id3->length    = ((GET_HEADER(ttahdr, DATA_LENGTH)) / id3->frequency) * 1000LL;
    bps            = (GET_HEADER(ttahdr, BITS_PER_SAMPLE));

    datasize = id3->filesize - id3->first_frame_offset;
    origsize = (GET_HEADER(ttahdr, DATA_LENGTH)) * ((bps + 7) / 8) * id3->channels;

    id3->bitrate = (int) ((uint64_t) datasize * id3->frequency * id3->channels * bps
                                               / (origsize * 1000LL));

    /* output header info (for debug) */
    DEBUGF("TTA header info ----\n");
    DEBUGF("id:        %x\n",  (unsigned int)(GET_HEADER(ttahdr, ID)));
    DEBUGF("channels:  %d\n",  id3->channels);
    DEBUGF("frequency: %ld\n", id3->frequency);
    DEBUGF("length:    %ld\n", id3->length);
    DEBUGF("bitrate:   %d\n",  id3->bitrate);
    DEBUGF("bits per sample: %d\n", bps);
    DEBUGF("compressed size: %d\n", datasize);
    DEBUGF("original size:   %d\n", origsize);
    DEBUGF("id3----\n");
    DEBUGF("artist:  %s\n",  id3->artist);
    DEBUGF("title:   %s\n",  id3->title);
    DEBUGF("genre:   %s\n",  id3->genre_string);

    return true;
}
