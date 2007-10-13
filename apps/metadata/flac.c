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

bool get_flac_metadata(int fd, struct mp3entry* id3)
{
    /* A simple parser to read vital metadata from a FLAC file - length,
     * frequency, bitrate etc. This code should either be moved to a 
     * seperate file, or discarded in favour of the libFLAC code.
     * The FLAC stream specification can be found at 
     * http://flac.sourceforge.net/format.html#stream
     */

    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    bool last_metadata = false;
    bool rc = false;

    if (!skip_id3v2(fd, id3) || (read(fd, buf, 4) < 4))
    {
        return rc;
    }
    
    if (memcmp(buf, "fLaC", 4) != 0) 
    {
        return rc;
    }

    while (!last_metadata) 
    {
        unsigned long i;
        int type;
        
        if (read(fd, buf, 4) < 0)
        {
            return rc;
        }
        
        last_metadata = buf[0] & 0x80;
        type = buf[0] & 0x7f;
        /* The length of the block */
        i = (buf[1] << 16) | (buf[2] << 8) | buf[3];

        if (type == 0)       /* 0 is the STREAMINFO block */
        {
            unsigned long totalsamples;
            
            if (i >= sizeof(id3->path) || read(fd, buf, i) < 0)
            {
                return rc;
            }
          
            id3->vbr = true;   /* All FLAC files are VBR */
            id3->filesize = filesize(fd);
            id3->frequency = (buf[10] << 12) | (buf[11] << 4) 
                | ((buf[12] & 0xf0) >> 4);
            rc = true;  /* Got vital metadata */

            /* totalsamples is a 36-bit field, but we assume <= 32 bits are used */
            totalsamples = get_long_be(&buf[14]);

            /* Calculate track length (in ms) and estimate the bitrate (in kbit/s) */
            id3->length = ((int64_t) totalsamples * 1000) / id3->frequency;
        
            if (id3->length <= 0)
            {
                logf("flac length invalid!");
                return false;
            }

            id3->bitrate = (id3->filesize * 8) / id3->length;
        } 
        else if (type == 4)  /* 4 is the VORBIS_COMMENT block */
        {
            /* The next i bytes of the file contain the VORBIS COMMENTS. */
            if (!read_vorbis_tags(fd, id3, i))
            {
                return rc;
            }
        } 
        else if (!last_metadata)
        {
            /* Skip to next metadata block */
            if (lseek(fd, i, SEEK_CUR) < 0)
            {
                return rc;
            }
        }
    }
    
    return true;
}
