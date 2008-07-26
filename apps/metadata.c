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

#include "system.h"
#include "playback.h"
#include "debug.h"
#include "logf.h"
#include "cuesheet.h"
#include "metadata.h"

#if CONFIG_CODEC == SWCODEC

/* For trailing tag stripping */
#include "buffering.h"

#include "metadata/metadata_common.h"
#include "metadata/metadata_parsers.h"

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
    case AFMT_SPEEX:
        if (!get_ogg_metadata(fd, id3))/*detects and handles Ogg/Speex files*/
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
        {
            return false;
        }

        read_ape_tags(fd, id3);     /* use any apetag info we find */
        break;

    case AFMT_A52:
        if (!get_a52_metadata(fd, id3))
        {
            return false;
        }

        break;

    case AFMT_ALAC:
    case AFMT_AAC:
        if (!get_mp4_metadata(fd, id3))
        {
            return false;
        }

        break;

    case AFMT_MOD:
        if (!get_mod_metadata(fd, id3))
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
        if (!get_spc_metadata(fd, id3))
        {
            DEBUGF("get_spc_metadata error\n");
            return false;
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
        
    case AFMT_SAP:
        if (!get_asap_metadata(fd, id3))
        {
            DEBUGF("get_sap_metadata error\n");
            return false;
        }
        id3->filesize = filesize(fd);
        id3->genre_string = id3_get_num_genre(36);
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
    static const unsigned char tag[] = "TAG";
    static const unsigned char apetag[] = "APETAGEX";    
    size_t len, version;
    void *tail;

    if (bufgettail(handle_id, 128, &tail) != 128)
        return;

    if (memcmp(tail, tag, 3) == 0)
    {
        /* Skip id3v1 tag */
        logf("Cutting off ID3v1 tag");
        bufcuttail(handle_id, 128);
    }

    /* Get a new tail, as the old one may have been cut */
    if (bufgettail(handle_id, 32, &tail) != 32)
        return;

    /* Check for APE tag (look for the APE tag footer) */
    if (memcmp(tail, apetag, 8) != 0)
        return;

    /* Read the version and length from the footer */
    version = get_long_le(&((unsigned char *)tail)[8]);
    len = get_long_le(&((unsigned char *)tail)[12]);
    if (version == 2000)
        len += 32; /* APEv2 has a 32 byte header */

    /* Skip APE tag */
    logf("Cutting off APE tag (%ldB)", len);
    bufcuttail(handle_id, len);
}
#endif /* CONFIG_CODEC == SWCODEC */
