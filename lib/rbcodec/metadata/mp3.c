/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
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
/*
 * Parts of this code has been stolen from the Ample project and was written
 * by David H�deman. It has since been extended and enhanced pretty much by
 * all sorts of friendly Rockbox people.
 *
 */

 /* tagResolver and associated code copyright 2003 Thomas Paul Diffenbach
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "string-extra.h"
#include "config.h"
#include "logf.h"
#include "platform.h"

#include "metadata.h"
#include "mp3data.h"
#include "metadata_common.h"
#include "metadata_parsers.h"

/*
 * Calculates the length (in milliseconds) of an MP3 file.
 *
 * Modified to only use integers.
 *
 * Arguments: file - the file to calculate the length upon
 *            entry - the entry to update with the length
 *
 * Returns: the song length in milliseconds,
 *          0 means that it couldn't be calculated
 */
static int getsonglength(int fd, struct mp3entry *entry)
{
    unsigned long filetime = 0;
    struct mp3info info;
    long bytecount;

    /* Start searching after ID3v2 header */
    if(-1 == lseek(fd, entry->id3v2len, SEEK_SET))
        return 0;

    bytecount = get_mp3file_info(fd, &info);

    logf("Space between ID3V2 tag and first audio frame: 0x%lx bytes",
           bytecount);

    if(bytecount < 0)
        return -1;

    /* Subtract the meta information from the file size to get
       the true size of the MP3 stream */
    entry->filesize -= entry->id3v1len + entry->id3v2len;

    /* Validate byte count, in case the file has been edited without
     * updating the header.
     */
    if (info.byte_count)
    {
        const unsigned long expected = entry->filesize - entry->id3v1len
            - entry->id3v2len;
        const unsigned long diff = MAX(10240, info.byte_count / 20);

        if ((info.byte_count > expected + diff)
            || (info.byte_count < expected - diff))
        {
            logf("Note: info.byte_count differs from expected value by "
                 "%ld bytes", labs((long) (expected - info.byte_count)));
            info.byte_count = 0;
            info.frame_count = 0;
            info.file_time = 0;
            info.enc_padding = 0;

            /* Even if the bitrate was based on "known bad" values, it
             * should still be better for VBR files than using the bitrate
             * of the first audio frame.
             */
        }
    }

    entry->filesize -= bytecount;
    bytecount += entry->id3v2len;

    entry->bitrate   = info.bitrate;
    entry->frequency = info.frequency;
    entry->layer     = info.layer;
    switch(entry->layer) {
#if CONFIG_CODEC==SWCODEC
        case 0:
            entry->codectype=AFMT_MPA_L1;
            break;
#endif
        case 1:
            entry->codectype=AFMT_MPA_L2;
            break;
        case 2:
            entry->codectype=AFMT_MPA_L3;
            break;
    }

    /* If the file time hasn't been established, this may be a fixed
       rate MP3, so just use the default formula */

    filetime = info.file_time;

    if(filetime == 0)
    {
        /* Prevent a division by zero */
        if (info.bitrate < 8)
            filetime = 0;
        else
            filetime = entry->filesize / (info.bitrate >> 3);
        /* bitrate is in kbps so this delivers milliseconds. Doing bitrate / 8
         * instead of filesize * 8 is exact, because mpeg audio bitrates are
         * always multiples of 8, and it avoids overflows. */
    }

    entry->frame_count = info.frame_count;

    entry->vbr = info.is_vbr;
    entry->has_toc = info.has_toc;

#if CONFIG_CODEC==SWCODEC
    if (!entry->lead_trim)
        entry->lead_trim = info.enc_delay;
    if (!entry->tail_trim)
        entry->tail_trim = info.enc_padding;
#endif

    memcpy(entry->toc, info.toc, sizeof(info.toc));

    /* Update the seek point for the first playable frame */
    entry->first_frame_offset = bytecount;
    logf("First frame is at %lx", entry->first_frame_offset);

    return filetime;
}

/*
 * Checks all relevant information (such as ID3v1 tag, ID3v2 tag, length etc)
 * about an MP3 file and updates it's entry accordingly.
 *
  Note, that this returns true for successful, false for error! */
bool get_mp3_metadata(int fd, struct mp3entry *entry)
{
    entry->title = NULL;
    entry->filesize = filesize(fd);
    entry->id3v1len = getid3v1len(fd);
    entry->id3v2len = getid3v2len(fd);
    entry->tracknum = 0;
    entry->discnum = 0;

    if (entry->id3v2len)
        setid3v2title(fd, entry);
    int len = getsonglength(fd, entry);
    if (len < 0)
        return false;
    entry->length = len;

    /* only seek to end of file if no id3v2 tags were found */
    if (!entry->id3v2len) {
        setid3v1title(fd, entry);
    }

    if(!entry->length || (entry->filesize < 8 ))
        /* no song length or less than 8 bytes is hereby considered to be an
           invalid mp3 and won't be played by us! */
        return false;

    return true;
}
