/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Parsing ADTS and ADIF headers
 *
 * Written by Igor B. Poretsky
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

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "platform.h"

#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"

static const int sample_rates[] =
{
    96000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0
};

static bool check_adts_syncword(int fd)
{
    uint16_t syncword;

    read_uint16be(fd, &syncword);
    return (syncword & 0xFFF6) == 0xFFF0;
}

bool get_aac_metadata(int fd, struct mp3entry *entry)
{
    unsigned char buf[5];

    entry->title = NULL;
    entry->tracknum = 0;
    entry->discnum = 0;
    entry->id3v1len = 0;
    entry->id3v2len = getid3v2len(fd);
    entry->first_frame_offset = entry->id3v2len;
    entry->filesize = filesize(fd) - entry->first_frame_offset;
    entry->needs_upsampling_correction = false;

    if (entry->id3v2len)
        setid3v2title(fd, entry);

    if (-1 == lseek(fd, entry->first_frame_offset, SEEK_SET))
        return false;

    if (check_adts_syncword(fd))
    {
        int frames;
        int stat_length;
        uint64_t total;
        if (read(fd, buf, 5) != 5)
            return false;
        entry->frequency = sample_rates[(buf[0] >> 2) & 0x0F];
        entry->vbr = ((buf[3] & 0x1F) == 0x1F)
          && ((buf[4] & 0xFC) == 0xFC);
        stat_length = entry->frequency >> ((entry->vbr) ? 5 : 7);
        for (frames = 1, total = 0; frames < stat_length; frames++)
        {
            unsigned int frame_length = (((unsigned int)buf[1] & 0x3) << 11)
              | ((unsigned int)buf[2] << 3)
              | ((unsigned int)buf[3] >> 5);
            total += frame_length;
            if (frame_length < 7)
                break;
            if (-1 == lseek(fd, frame_length - 7, SEEK_CUR))
                break;
            if (!check_adts_syncword(fd))
                break;
            if (read(fd, buf, 5) != 5)
                break;
        }
        entry->bitrate = (unsigned int)((total * entry->frequency / frames + 64000) / 128000);
        if (entry->frequency <= 24000)
        {
            entry->frequency <<= 1;
            entry->needs_upsampling_correction = true;
        }
    }
    else
    {
        uint32_t bitrate;
        if (-1 == lseek(fd, entry->first_frame_offset, SEEK_SET))
            return false;
        if (read(fd, buf, 5) != 5)
            return false;
        if (memcmp(buf, "ADIF", 4))
            return false;
        if (-1 == lseek(fd, (buf[4] & 0x80) ? (entry->first_frame_offset + 9) : entry->first_frame_offset, SEEK_SET))
            return false;
        read_uint32be(fd, &bitrate);
        entry->vbr = (bitrate & 0x10000000) != 0;
        entry->bitrate = ((bitrate & 0xFFFFFE0) + 16000) / 32000;
        read_uint32be(fd, (uint32_t*)(&(entry->frequency)));
        entry->frequency = sample_rates[(entry->frequency >> (entry->vbr ? 23 : 3)) & 0x0F];
    }
    entry->length = (unsigned long)((entry->filesize * 8LL + (entry->bitrate >> 1)) / entry->bitrate);

    return true;
}
