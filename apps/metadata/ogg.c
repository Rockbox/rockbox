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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "logf.h"

/* A simple parser to read vital metadata from an Ogg Vorbis file. 
 * Can also handle parsing Ogg Speex files for metadata. Returns
 * false if metadata needed by the codec couldn't be read.
 */
bool get_ogg_metadata(int fd, struct mp3entry* id3)
{
    /* An Ogg File is split into pages, each starting with the string 
     * "OggS". Each page has a timestamp (in PCM samples) referred to as
     * the "granule position".
     *
     * An Ogg Vorbis has the following structure:
     * 1) Identification header (containing samplerate, numchannels, etc)
     * 2) Comment header - containing the Vorbis Comments
     * 3) Setup header - containing codec setup information
     * 4) Many audio packets...
     *
     * An Ogg Speex has the following structure:
     * 1) Identification header (containing samplerate, numchannels, etc)
     *    Described in this page: (http://www.speex.org/manual2/node7.html)
     * 2) Comment header - containing the Vorbis Comments
     * 3) Many audio packets.
     */

    /* Use the path name of the id3 structure as a temporary buffer. */
    unsigned char* buf = (unsigned char *)id3->path;
    long comment_size;
    long remaining = 0;
    long last_serial = 0;
    long serial, r;
    int segments, header_size;
    int i;
    bool eof = false;

    /* 92 bytes is enough for both Vorbis and Speex headers */
    if ((lseek(fd, 0, SEEK_SET) < 0) || (read(fd, buf, 92) < 92))
    {
        return false;
    }
    
    /* All Ogg streams start with OggS */
    if (memcmp(buf, "OggS", 4) != 0)
    {
        return false;
    }

    /* Check for format magic and then get metadata */
    if (memcmp(&buf[29], "vorbis", 6) == 0)
    {
        id3->codectype = AFMT_OGG_VORBIS;
        id3->frequency = get_long_le(&buf[40]);
        id3->vbr = true;

        /* Comments are in second Ogg page (byte 58 onwards for Vorbis) */
        if (lseek(fd, 58, SEEK_SET) < 0)
        {
            return false;
        }
    }
    else if (memcmp(&buf[28], "Speex   ", 8) == 0)
    {
        id3->codectype = AFMT_SPEEX;
        id3->frequency = get_slong(&buf[64]);
        id3->vbr = get_long_le(&buf[88]);

        header_size = get_long_le(&buf[60]);

        /* Comments are in second Ogg page (byte 108 onwards for Speex) */
        if (lseek(fd, 28 + header_size, SEEK_SET) < 0)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    id3->filesize = filesize(fd);
    
    /* We need to ensure the serial number from this page is the same as the
     * one from the last page (since we only support a single bitstream).
     */
    serial = get_long_le(&buf[14]);

    /* Minimum header length for Ogg pages is 27. */
    if (read(fd, buf, 27) < 27) 
    {
        return false;
    }

    if (memcmp(buf, "OggS", 4) !=0 )
    {
        return false;
    }

    segments = buf[26];

    /* read in segment table */
    if (read(fd, buf, segments) < segments) 
    {
        return false;
    }

    /* The second packet in a vorbis stream is the comment packet. It *may*
     * extend beyond the second page, but usually does not. Here we find the
     * length of the comment packet (or the rest of the page if the comment
     * packet extends to the third page).
     */
    for (i = 0; i < segments; i++) 
    {
        remaining += buf[i];
        
        /* The last segment of a packet is always < 255 bytes */
        if (buf[i] < 255) 
        {
              break;
        }
    }

    comment_size = remaining;
    
    if (id3->codectype == AFMT_OGG_VORBIS) {
        /* Now read in packet header (type and id string) */
        if (read(fd, buf, 7) < 7) 
        {
            return false;
        }

        remaining -= 7;

        /* The first byte of a packet is the packet type; comment packets are
         * type 3.
         */
        if (buf[0] != 3)
        {
            return false;
        }
    }

    /* Failure to read the tags isn't fatal. */
    read_vorbis_tags(fd, id3, remaining);

    /* We now need to search for the last page in the file - identified by 
     * by ('O','g','g','S',0) and retrieve totalsamples.
     */

    /* A page is always < 64 kB */
    if (lseek(fd, -(MIN(64 * 1024, id3->filesize)), SEEK_END) < 0)
    {
        return false;
    }

    remaining = 0;

    while (!eof) 
    {
        r = read(fd, &buf[remaining], MAX_PATH - remaining);
        
        if (r <= 0) 
        {
            eof = true;
        } 
        else 
        {
            remaining += r;
        }
        
        /* Inefficient (but simple) search */
        i = 0;
        
        while (i < (remaining - 3)) 
        {
            if ((buf[i] == 'O') && (memcmp(&buf[i], "OggS", 4) == 0))
            {
                if (i < (remaining - 17)) 
                {
                    /* Note that this only reads the low 32 bits of a
                     * 64 bit value.
                     */
                     id3->samples = get_long_le(&buf[i + 6]);
                     last_serial = get_long_le(&buf[i + 14]);

                    /* If this page is very small the beginning of the next
                     * header could be in buffer. Jump near end of this header
                     * and continue */
                    i += 27;
                } 
                else 
                {
                    break;
                }
            } 
            else 
            {
                i++;
            }
        }

        if (i < remaining) 
        {
            /* Move the remaining bytes to start of buffer.
             * Reuse var 'segments' as it is no longer needed */
            segments = 0;
            while (i < remaining)
            {
                buf[segments++] = buf[i++];
            }
            remaining = segments;
        }
        else
        {
            /* Discard the rest of the buffer */
            remaining = 0;
        }
    }

    /* This file has mutiple vorbis bitstreams (or is corrupt). */
    /* FIXME we should display an error here. */
    if (serial != last_serial)
    {
        logf("serialno mismatch");
        logf("%ld", serial);
        logf("%ld", last_serial);
        return false;
    }

    id3->length = ((int64_t) id3->samples * 1000) / id3->frequency;
    if (id3->length <= 0)
    {
        logf("ogg length invalid!");
        return false;
    }
    
    id3->bitrate = (((int64_t) id3->filesize - comment_size) * 8) / id3->length;
    
    return true;
}

