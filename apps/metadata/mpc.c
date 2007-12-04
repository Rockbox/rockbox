/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Thom Johansen 
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <string.h>
#include <inttypes.h>
#include "system.h"
#include "id3.h"
#include "metadata_common.h"
#include "logf.h"
#include "replaygain.h"

static int set_replaygain(struct mp3entry* id3, bool album, long value, 
    long used)
{
    long gain = (int16_t) ((value >> 16) & 0xffff);
    long peak = (uint16_t) (value & 0xffff);
    
    /* We use a peak value of 0 to indicate a given gain type isn't used. */
    if (peak != 0)
    {
        /* Use the Xing TOC field to store ReplayGain strings for use in the
         * ID3 screen, since Musepack files shouldn't need to use it in any
         * other way.
         */
        used += parse_replaygain_int(album, gain * 512 / 100, peak << 9,
            id3, id3->toc + used, sizeof(id3->toc) - used);
    }
    
    return used;
}

bool get_musepack_metadata(int fd, struct mp3entry *id3)
{
    const int32_t sfreqs_sv7[4] = { 44100, 48000, 37800, 32000 };
    uint32_t header[8];
    uint64_t samples = 0;
    int i;
    
    if (!skip_id3v2(fd, id3))
        return false;
    if (read(fd, header, 4*8) != 4*8) return false;
    /* Musepack files are little endian, might need swapping */
    for (i = 1; i < 8; i++) 
       header[i] = letoh32(header[i]); 
    if (!memcmp(header, "MP+", 3)) { /* Compare to sig "MP+" */
        unsigned int streamversion;
        
        header[0] = letoh32(header[0]);
        streamversion = (header[0] >> 24) & 15;
        if (streamversion >= 8) {
            return false; /* SV8 or higher don't exist yet, so no support */
        } else if (streamversion == 7) {
            unsigned int gapless = (header[5] >> 31) & 0x0001;
            unsigned int last_frame_samples = (header[5] >> 20) & 0x07ff;
            unsigned int bufused = 0;
            
            id3->frequency = sfreqs_sv7[(header[2] >> 16) & 0x0003];
            samples = (uint64_t)header[1]*1152; /* 1152 is mpc frame size */
            if (gapless)
                samples -= 1152 - last_frame_samples;
            else
                samples -= 481; /* Musepack subband synth filter delay */
           
            bufused = set_replaygain(id3, false, header[3], bufused);
            bufused = set_replaygain(id3, true, header[4], bufused);
        }
    } else {
        header[0] = letoh32(header[0]);
        unsigned int streamversion = (header[0] >> 11) & 0x03FF;
        if (streamversion != 4 && streamversion != 5 && streamversion != 6)
            return false;
        id3->frequency = 44100;
        id3->track_gain = 0;
        id3->track_peak = 0;
        id3->album_gain = 0;
        id3->album_peak = 0;

        if (streamversion >= 5)
            samples = (uint64_t)header[1]*1152; // 32 bit
        else
            samples = (uint64_t)(header[1] >> 16)*1152; // 16 bit

        samples -= 576;
        if (streamversion < 6)
            samples -= 1152;
    }

    id3->vbr = true;
    /* Estimate bitrate, we should probably subtract the various header sizes
       here for super-accurate results */
    id3->length = ((int64_t) samples * 1000) / id3->frequency;

    if (id3->length <= 0)
    {
        logf("mpc length invalid!");
        return false;
    }

    id3->filesize = filesize(fd);
    id3->bitrate = id3->filesize * 8 / id3->length;
    return true;
}
