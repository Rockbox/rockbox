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

/* For trailing tag stripping */
#include "buffering.h"

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
bool get_metadata(struct mp3entry* id3, int fd, const char* trackname)
{
#if CONFIG_CODEC == SWCODEC
    unsigned char* buf;
    unsigned long totalsamples;
    int i;
#endif

    /* Clear the mp3entry to avoid having bogus pointers appear */
    memset(id3, 0, sizeof(struct mp3entry));

    /* Take our best guess at the codec type based on file extension */
    id3->codectype = probe_file_format(trackname);

    /* Load codec specific track tag information and confirm the codec type. */
    switch (id3->codectype)
    {
    case AFMT_MPA_L1:
    case AFMT_MPA_L2:
    case AFMT_MPA_L3:
        if (!get_mp3_metadata(fd, id3, trackname))
        {
            return false;
        }

        break;

#if CONFIG_CODEC == SWCODEC
    case AFMT_FLAC:
        if (!get_flac_metadata(fd, id3))
        {
            return false;
        }

        break;

    case AFMT_WMA:
        if (!get_asf_metadata(fd, id3))
        {
            return false;
        }
        break;

    case AFMT_APE:
        if (!get_monkeys_metadata(fd, id3))
        {
            return false;
        }
        read_ape_tags(fd, id3);
        break;

    case AFMT_MPC:
        if (!get_musepack_metadata(fd, id3))
            return false;
        read_ape_tags(fd, id3);
        break;
    
    case AFMT_OGG_VORBIS:
        if (!get_vorbis_metadata(fd, id3))/*detects and handles Ogg/Speex files*/
        {
            return false;
        }

        break;

    case AFMT_SPEEX:
        if (!get_speex_metadata(fd, id3))
        {
            return false;
        }

        break;

    case AFMT_PCM_WAV:
        if (!get_wave_metadata(fd, id3))
        {
            return false;
        }

        break;

    case AFMT_WAVPACK:
        if (!get_wavpack_metadata(fd, id3))
            return false;

        read_ape_tags(fd, id3);     /* use any apetag info we find */
        break;

    case AFMT_A52:
        /* Use the trackname part of the id3 structure as a temporary buffer */
        buf = (unsigned char *)id3->path;
        
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
      
        id3->bitrate = a52_bitrates[i >> 1];
        id3->vbr = false;
        id3->filesize = filesize(fd);

        switch (buf[4] & 0xc0) 
        {
        case 0x00: 
            id3->frequency = 48000;
            id3->bytesperframe=id3->bitrate * 2 * 2;
            break;
            
        case 0x40: 
            id3->frequency = 44100;
            id3->bytesperframe = a52_441framesizes[i];
            break;
        
        case 0x80: 
            id3->frequency = 32000;
            id3->bytesperframe = id3->bitrate * 3 * 2;
            break;
        
        default: 
            logf("A52: Invalid samplerate code: 0x%02x\n", buf[4] & 0xc0);
            return false;
            break;
        }

        /* One A52 frame contains 6 blocks, each containing 256 samples */
        totalsamples = id3->filesize / id3->bytesperframe * 6 * 256;
        id3->length = totalsamples / id3->frequency * 1000;
        break;

    case AFMT_ALAC:
    case AFMT_AAC:
        if (!get_mp4_metadata(fd, id3))
        {
            return false;
        }

        break;

    case AFMT_SHN:
        id3->vbr = true;
        id3->filesize = filesize(fd);
        if (!skip_id3v2(fd, id3))
        {
            return false;
        }
        /* TODO: read the id3v2 header if it exists */
        break;

    case AFMT_SID:
        if (!get_sid_metadata(fd, id3))
        {
            return false;
        }
        break;
    case AFMT_SPC:
        if(!get_spc_metadata(fd, id3))
        {
            DEBUGF("get_spc_metadata error\n");
        }

        id3->filesize = filesize(fd);
        id3->genre_string = id3_get_num_genre(36);
        break;
    case AFMT_ADX:
        if (!get_adx_metadata(fd, id3))
        {
            DEBUGF("get_adx_metadata error\n");
            return false;
        }
        
        break;
    case AFMT_NSF:
        buf = (unsigned char *)id3->path;
        if ((lseek(fd, 0, SEEK_SET) < 0) || ((read(fd, buf, 8)) < 8))
        {
            DEBUGF("lseek or read failed\n");
            return false;
        }
        id3->vbr = false;
        id3->filesize = filesize(fd);
        if (memcmp(buf,"NESM",4) && memcmp(buf,"NSFE",4)) return false;
        break;

    case AFMT_AIFF:
        if (!get_aiff_metadata(fd, id3))
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
        id3->cuesheet_type = 1;
    }
#endif
    
    lseek(fd, 0, SEEK_SET);
    strncpy(id3->path, trackname, sizeof(id3->path));

    return true;
}

#if CONFIG_CODEC == SWCODEC
void strip_tags(int handle_id)
{
    int i;
    static const unsigned char tag[] = "TAG";
    static const unsigned char apetag[] = "APETAGEX";    
    size_t len, version;
    unsigned char *tail;

    if (bufgettail(handle_id, 128, (void **)&tail) != 128)
        return;

    for(i = 0;i < 3;i++)
        if(tail[i] != tag[i])
            goto strip_ape_tag;

    /* Skip id3v1 tag */
    logf("Cutting off ID3v1 tag");
    bufcuttail(handle_id, 128);

strip_ape_tag:
    /* Check for APE tag (look for the APE tag footer) */

    if (bufgettail(handle_id, 32, (void **)&tail) != 32)
        return;

    for(i = 0;i < 8;i++)
        if(tail[i] != apetag[i])
            return;

    /* Read the version and length from the footer */
    version = tail[8] | (tail[9] << 8) | (tail[10] << 16) | (tail[11] << 24);
    len = tail[12] | (tail[13] << 8) | (tail[14] << 16) | (tail[15] << 24);
    if (version == 2000)
        len += 32; /* APEv2 has a 32 byte header */

    /* Skip APE tag */
    logf("Cutting off APE tag (%ldB)", len);
    bufcuttail(handle_id, len);
}
#endif /* CONFIG_CODEC == SWCODEC */
