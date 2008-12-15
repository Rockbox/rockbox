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

#include "playback.h"
#include "debug.h"
#include "logf.h"
#include "cuesheet.h"
#include "metadata.h"

#include "metadata/metadata_parsers.h"

#if CONFIG_CODEC == SWCODEC

/* For trailing tag stripping */
#include "buffering.h"

#include "metadata/metadata_common.h"

#endif /* CONFIG_CODEC == SWCODEC */

const struct afmt_entry audio_formats[AFMT_NUM_CODECS] =
{
    /* Unknown file format */
    [AFMT_UNKNOWN] =
        AFMT_ENTRY("???", NULL,       NULL,          NULL         ),

    /* MPEG Audio layer 1 */
    [AFMT_MPA_L1] =
        AFMT_ENTRY("MP1", "mpa",      NULL,          "mp1\0"      ),
    /* MPEG Audio layer 2 */
    [AFMT_MPA_L2] =
        AFMT_ENTRY("MP2", "mpa",      NULL,          "mpa\0mp2\0" ),
    /* MPEG Audio layer 3 */
    [AFMT_MPA_L3] =
        AFMT_ENTRY("MP3", "mpa",      "mp3_enc",     "mp3\0"      ),

#if CONFIG_CODEC == SWCODEC
    /* Audio Interchange File Format */
    [AFMT_AIFF] =
        AFMT_ENTRY("AIFF", "aiff",    "aiff_enc",    "aiff\0aif\0"),
    /* Uncompressed PCM in a WAV file */
    [AFMT_PCM_WAV] =
        AFMT_ENTRY("WAV",  "wav",     "wav_enc",     "wav\0"      ),
    /* Ogg Vorbis */
    [AFMT_OGG_VORBIS] =
        AFMT_ENTRY("Ogg",  "vorbis",  NULL,          "ogg\0oga\0" ),
    /* FLAC */
    [AFMT_FLAC] =
        AFMT_ENTRY("FLAC", "flac",    NULL,          "flac\0"     ),
    /* Musepack */
    [AFMT_MPC] =
        AFMT_ENTRY("MPC",  "mpc",     NULL,          "mpc\0"      ),
    /* A/52 (aka AC3) audio */
    [AFMT_A52] =
        AFMT_ENTRY("AC3",  "a52",     NULL,          "a52\0ac3\0" ),
    /* WavPack */
    [AFMT_WAVPACK] =
        AFMT_ENTRY("WV",   "wavpack", "wavpack_enc", "wv\0"       ),
    /* Apple Lossless Audio Codec */
    [AFMT_ALAC] =
        AFMT_ENTRY("ALAC", "alac",    NULL,          "m4a\0m4b\0" ),
    /* Advanced Audio Coding in M4A container */
    [AFMT_AAC] =
        AFMT_ENTRY("AAC",  "aac",     NULL,          "mp4\0"      ),
    /* Shorten */
    [AFMT_SHN] =
        AFMT_ENTRY("SHN",  "shorten", NULL,          "shn\0"      ),
    /* SID File Format */
    [AFMT_SID] =
        AFMT_ENTRY("SID",  "sid",     NULL,          "sid\0"      ),
    /* ADX File Format */
    [AFMT_ADX] =
        AFMT_ENTRY("ADX",  "adx",     NULL,          "adx\0"      ),
    /* NESM (NES Sound Format) */
    [AFMT_NSF] =
        AFMT_ENTRY("NSF",  "nsf",     NULL,          "nsf\0nsfe\0"      ),
    /* Speex File Format */
    [AFMT_SPEEX] =
        AFMT_ENTRY("Speex","speex",   NULL,          "spx\0"      ),
    /* SPC700 Save State */
    [AFMT_SPC] =
        AFMT_ENTRY("SPC",  "spc",     NULL,          "spc\0"      ),
    /* APE (Monkey's Audio) */
    [AFMT_APE] =
        AFMT_ENTRY("APE",  "ape",     NULL,          "ape\0mac\0"      ),
    /* WMA (WMAV1/V2 in ASF) */
    [AFMT_WMA] =
        AFMT_ENTRY("WMA",  "wma",     NULL,          "wma\0wmv\0asf\0"   ),
    /* Amiga MOD File */
    [AFMT_MOD] =
        AFMT_ENTRY("MOD",  "mod",     NULL,          "mod\0"      ),
    /* Amiga SAP File */
    [AFMT_SAP] =
        AFMT_ENTRY("SAP",  "asap",     NULL,          "sap\0"      ),
#endif
};

#if CONFIG_CODEC == SWCODEC && defined (HAVE_RECORDING)
/* get REC_FORMAT_* corresponding AFMT_* */
const int rec_format_afmt[REC_NUM_FORMATS] =
{
    /* give AFMT_UNKNOWN by default */
    [0 ... REC_NUM_FORMATS-1] = AFMT_UNKNOWN,
    /* add new entries below this line */
    [REC_FORMAT_AIFF]    = AFMT_AIFF,
    [REC_FORMAT_MPA_L3]  = AFMT_MPA_L3,
    [REC_FORMAT_WAVPACK] = AFMT_WAVPACK,
    [REC_FORMAT_PCM_WAV] = AFMT_PCM_WAV,
};

/* get AFMT_* corresponding REC_FORMAT_* */
const int afmt_rec_format[AFMT_NUM_CODECS] =
{
    /* give -1 by default */
    [0 ... AFMT_NUM_CODECS-1] = -1,
    /* add new entries below this line */
    [AFMT_AIFF]    = REC_FORMAT_AIFF,
    [AFMT_MPA_L3]  = REC_FORMAT_MPA_L3,
    [AFMT_WAVPACK] = REC_FORMAT_WAVPACK,
    [AFMT_PCM_WAV] = REC_FORMAT_PCM_WAV,
};
#endif /* CONFIG_CODEC == SWCODEC && defined (HAVE_RECORDING) */


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

/* Note, that this returns false for successful, true for error! */
bool mp3info(struct mp3entry *entry, const char *filename)
{
    int fd;
    bool result;

    fd = open(filename, O_RDONLY);
    if (fd < 0)
        return true;

    result = !get_mp3_metadata(fd, entry, filename);

    close(fd);

    return result;
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

#ifndef __PCTOOL__
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
#endif /* ! __PCTOOL__ */

void adjust_mp3entry(struct mp3entry *entry, void *dest, const void *orig)
{
    long offset;
    if (orig > dest)
        offset = - ((size_t)orig - (size_t)dest);
    else
        offset = (size_t)dest - (size_t)orig;

    if (entry->title)
        entry->title += offset;
    if (entry->artist)
        entry->artist += offset;
    if (entry->album)
        entry->album += offset;
    if (entry->genre_string && !id3_is_genre_string(entry->genre_string))
        /* Don't adjust that if it points to an entry of the "genres" array */
        entry->genre_string += offset;
    if (entry->track_string)
        entry->track_string += offset;
    if (entry->disc_string)
        entry->disc_string += offset;
    if (entry->year_string)
        entry->year_string += offset;
    if (entry->composer)
        entry->composer += offset;
    if (entry->comment)
        entry->comment += offset;
    if (entry->albumartist)
        entry->albumartist += offset;
    if (entry->grouping)
        entry->grouping += offset;
#if CONFIG_CODEC == SWCODEC
    if (entry->track_gain_string)
        entry->track_gain_string += offset;
    if (entry->album_gain_string)
        entry->album_gain_string += offset;
#endif
    if (entry->mb_track_id)
        entry->mb_track_id += offset;
}

void copy_mp3entry(struct mp3entry *dest, const struct mp3entry *orig)
{
    memcpy(dest, orig, sizeof(struct mp3entry));
    adjust_mp3entry(dest, dest, orig);
}
