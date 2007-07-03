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
#include "playback.h"
#include "debug.h"
#include "logf.h"
#include "cuesheet.h"

#if CONFIG_CODEC == SWCODEC

#include "metadata/metadata_common.h"
#include "metadata/metadata_parsers.h"

static const unsigned short a52_bitrates[] =
{
     32,  40,  48,  56,  64,  80,  96, 112, 128, 160, 
    192, 224, 256, 320, 384, 448, 512, 576, 640
};

/* Only store frame sizes for 44.1KHz - others are simply multiples 
   of the bitrate */
static const unsigned short a52_441framesizes[] =
{
      69 * 2,   70 * 2,   87 * 2,   88 * 2,  104 * 2,  105 * 2,  121 * 2, 
     122 * 2,  139 * 2,  140 * 2,  174 * 2,  175 * 2,  208 * 2,  209 * 2,
     243 * 2,  244 * 2,  278 * 2,  279 * 2,  348 * 2,  349 * 2,  417 * 2,
     418 * 2,  487 * 2,  488 * 2,  557 * 2,  558 * 2,  696 * 2,  697 * 2,  
     835 * 2,  836 * 2,  975 * 2,  976 * 2, 1114 * 2, 1115 * 2, 1253 * 2, 
    1254 * 2, 1393 * 2, 1394 * 2
};

static const long wavpack_sample_rates [] = 
{
     6000,  8000,  9600, 11025, 12000, 16000,  22050, 24000, 
    32000, 44100, 48000, 64000, 88200, 96000, 192000 
};

#endif /* CONFIG_CODEC == SWCODEC */


/* Simple file type probing by looking at the filename extension. */
unsigned int probe_file_format(const char *filename)
{
    char *suffix;
    unsigned int i;
    
    suffix = strrchr(filename, '.');

    if (suffix == NULL)
    {
        return AFMT_UNKNOWN;
    }
    
    /* skip '.' */
    suffix++;
    
    for (i = 1; i < AFMT_NUM_CODECS; i++)
    {
        /* search extension list for type */
        const char *ext = audio_formats[i].ext_list;

        do
        {
            if (strcasecmp(suffix, ext) == 0)
            {
                return i;
            }

            ext += strlen(ext) + 1;
        }
        while (*ext != '\0');
    }
    
    return AFMT_UNKNOWN;
}

/* Get metadata for track - return false if parsing showed problems with the
 * file that would prevent playback.
 */
bool get_metadata(struct track_info* track, int fd, const char* trackname,
                  bool v1first) 
{
#if CONFIG_CODEC == SWCODEC
    unsigned char* buf;
    unsigned long totalsamples;
    int i;
#endif

    /* Take our best guess at the codec type based on file extension */
    track->id3.codectype = probe_file_format(trackname);

    /* Load codec specific track tag information and confirm the codec type. */
    switch (track->id3.codectype) 
    {
    case AFMT_MPA_L1:
    case AFMT_MPA_L2:
    case AFMT_MPA_L3:
        if (!get_mp3_metadata(fd, &track->id3, trackname, v1first))
        {
            return false;
        }

        break;

#if CONFIG_CODEC == SWCODEC
    case AFMT_FLAC:
        if (!get_flac_metadata(fd, &(track->id3)))
        {
            return false;
        }

        break;

    case AFMT_WMA:
        if (!get_asf_metadata(fd, &(track->id3)))
        {
            return false;
        }
        break;

    case AFMT_APE:
        if (!get_monkeys_metadata(fd, &(track->id3)))
        {
            return false;
        }
        read_ape_tags(fd, &(track->id3));
        break;

    case AFMT_MPC:
        if (!get_musepack_metadata(fd, &(track->id3)))
            return false;
        read_ape_tags(fd, &(track->id3));
        break;
    
    case AFMT_OGG_VORBIS:
        if (!get_vorbis_metadata(fd, &(track->id3)))/*detects and handles Ogg/Speex files*/
        {
            return false;
        }

        break;

    case AFMT_SPEEX:
        if (!get_speex_metadata(fd, &(track->id3)))
        {
            return false;
        }

        break;

    case AFMT_PCM_WAV:
        if (!get_wave_metadata(fd, &(track->id3)))
        {
            return false;
        }

        break;

    case AFMT_WAVPACK:
        /* A simple parser to read basic information from a WavPack file. This
         * now works with self-extrating WavPack files. This no longer fails on
         * WavPack files containing floating-point audio data because these are
         * now converted to standard Rockbox format in the decoder.
         */

        /* Use the trackname part of the id3 structure as a temporary buffer */
        buf = (unsigned char *)track->id3.path;
      
        for (i = 0; i < 256; ++i) {

            /* at every 256 bytes into file, try to read a WavPack header */

            if ((lseek(fd, i * 256, SEEK_SET) < 0) || (read(fd, buf, 32) < 32))
            {
                return false;
            }

            /* if valid WavPack 4 header version & not floating data, break */

            if (memcmp (buf, "wvpk", 4) == 0 && buf [9] == 4 &&
                (buf [8] >= 2 && buf [8] <= 0x10))
            {          
                break;
            }
        }

        if (i == 256) {
            logf ("%s is not a WavPack file\n", trackname);
            return false;
        }

        track->id3.vbr = true;   /* All WavPack files are VBR */
        track->id3.filesize = filesize (fd);

        if ((buf [20] | buf [21] | buf [22] | buf [23]) &&
            (buf [12] & buf [13] & buf [14] & buf [15]) != 0xff) 
        {
            int srindx = ((buf [26] >> 7) & 1) + ((buf [27] << 1) & 14);

            if (srindx == 15)
            {
                track->id3.frequency = 44100;
            }
            else
            {
                track->id3.frequency = wavpack_sample_rates[srindx];
            }

            totalsamples = get_long_le(&buf[12]);
            track->id3.length = totalsamples / (track->id3.frequency / 100) * 10;
            track->id3.bitrate = filesize (fd) / (track->id3.length / 8);
        }

        read_ape_tags(fd, &track->id3);     /* use any apetag info we find */
        break;

    case AFMT_A52:
        /* Use the trackname part of the id3 structure as a temporary buffer */
        buf = (unsigned char *)track->id3.path;
        
        if ((lseek(fd, 0, SEEK_SET) < 0) || (read(fd, buf, 5) < 5))
        {
            return false;
        }

        if ((buf[0] != 0x0b) || (buf[1] != 0x77)) 
        { 
            logf("%s is not an A52/AC3 file\n",trackname);
            return false;
        }

        i = buf[4] & 0x3e;
      
        if (i > 36) 
        {
            logf("A52: Invalid frmsizecod: %d\n",i);
            return false;
        }
      
        track->id3.bitrate = a52_bitrates[i >> 1];
        track->id3.vbr = false;
        track->id3.filesize = filesize(fd);

        switch (buf[4] & 0xc0) 
        {
        case 0x00: 
            track->id3.frequency = 48000; 
            track->id3.bytesperframe=track->id3.bitrate * 2 * 2;
            break;
            
        case 0x40: 
            track->id3.frequency = 44100;
            track->id3.bytesperframe = a52_441framesizes[i];
            break;
        
        case 0x80: 
            track->id3.frequency = 32000; 
            track->id3.bytesperframe = track->id3.bitrate * 3 * 2;
            break;
        
        default: 
            logf("A52: Invalid samplerate code: 0x%02x\n", buf[4] & 0xc0);
            return false;
            break;
        }

        /* One A52 frame contains 6 blocks, each containing 256 samples */
        totalsamples = track->id3.filesize / track->id3.bytesperframe * 6 * 256;
        track->id3.length = totalsamples / track->id3.frequency * 1000;
        break;

    case AFMT_ALAC:
    case AFMT_AAC:
        if (!get_mp4_metadata(fd, &(track->id3)))
        {
            return false;
        }

        break;

    case AFMT_SHN:
        track->id3.vbr = true;
        track->id3.filesize = filesize(fd);
        if (!skip_id3v2(fd, &(track->id3)))
        {
            return false;
        }
        /* TODO: read the id3v2 header if it exists */
        break;

    case AFMT_SID:
        if (!get_sid_metadata(fd, &(track->id3)))
        {
            return false;
        }
        break;
    case AFMT_SPC:
        if(!get_spc_metadata(fd, &(track->id3)))
        {
            DEBUGF("get_spc_metadata error\n");
        }

        track->id3.filesize = filesize(fd);
        track->id3.genre_string = id3_get_num_genre(36);
        break;
    case AFMT_ADX:
        if (!get_adx_metadata(fd, &(track->id3)))
        {
            DEBUGF("get_adx_metadata error\n");
            return false;
        }
        
        break;
    case AFMT_NSF:
        buf = (unsigned char *)track->id3.path;
        if ((lseek(fd, 0, SEEK_SET) < 0) || ((read(fd, buf, 8)) < 8))
        {
            DEBUGF("lseek or read failed\n");
            return false;
        }
        track->id3.vbr = false;
        track->id3.filesize = filesize(fd);
        if (memcmp(buf,"NESM",4) && memcmp(buf,"NSFE",4)) return false;
        break;

    case AFMT_AIFF:
        if (!get_aiff_metadata(fd, &(track->id3)))
        {
            return false;
        }

        break;

#endif /* CONFIG_CODEC == SWCODEC */
        
    default:
        /* If we don't know how to read the metadata, assume we can't play 
           the file */
        return false;
        break;
    }

    /* We have successfully read the metadata from the file */

#ifndef __PCTOOL__
    if (cuesheet_is_enabled() && look_for_cuesheet_file(trackname, NULL))
    {
        track->id3.cuesheet_type = 1;
    }
#endif
    
    lseek(fd, 0, SEEK_SET);
    strncpy(track->id3.path, trackname, sizeof(track->id3.path));
    track->taginfo_ready = true;

    return true;
}

