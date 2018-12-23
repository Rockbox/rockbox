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
#include <stdlib.h>
#include <ctype.h>
#include "string-extra.h"
#include "platform.h"
#include "debug.h"
#include "logf.h"
#include "metadata.h"

#include "metadata_parsers.h"

#if CONFIG_CODEC == SWCODEC

/* For trailing tag stripping and base audio data types */
#include "buffering.h"

#include "metadata/metadata_common.h"

static bool get_shn_metadata(int fd, struct mp3entry *id3)
{
    /* TODO: read the id3v2 header if it exists */
    id3->vbr = true;
    id3->filesize = filesize(fd);
    return skip_id3v2(fd, id3);
}

static bool get_other_asap_metadata(int fd, struct mp3entry *id3)
{    
    id3->bitrate = 706;
    id3->frequency = 44100;
    id3->vbr = false;
    id3->filesize = filesize(fd);
    id3->genre_string = id3_get_num_genre(36);
    return true;
}
#endif /* CONFIG_CODEC == SWCODEC */
bool write_metadata_log = false;

const struct afmt_entry audio_formats[AFMT_NUM_CODECS] =
{
    /* Unknown file format */
    [0 ... AFMT_NUM_CODECS-1] =
        AFMT_ENTRY("???", NULL,    NULL,        NULL, "\0"  ),

    /* MPEG Audio layer 2 */
    [AFMT_MPA_L2] =
        AFMT_ENTRY("MP2",   "mpa",  NULL,       get_mp3_metadata,   "mpa\0mp2\0"),

#if CONFIG_CODEC != SWCODEC
    /* MPEG Audio layer 3 on HWCODEC: .talk clips, no encoder  */
    [AFMT_MPA_L3] =
        AFMT_ENTRY("MP3",   "mpa",  NULL,       get_mp3_metadata,   "mp3\0talk\0"),

#else /* CONFIG_CODEC == SWCODEC */
    /* MPEG Audio layer 3 on SWCODEC */
    [AFMT_MPA_L3] =
        AFMT_ENTRY("MP3",   "mpa",  "mp3_enc",  get_mp3_metadata,   "mp3\0"),

    /* MPEG Audio layer 1 */
    [AFMT_MPA_L1] =
        AFMT_ENTRY("MP1",   "mpa",  NULL,       get_mp3_metadata,   "mp1\0"),
    /* Audio Interchange File Format */
    [AFMT_AIFF] =
        AFMT_ENTRY("AIFF",  "aiff", "aiff_enc", get_aiff_metadata,  "aiff\0aif\0"),
    /* Uncompressed PCM in a WAV file OR ATRAC3 stream in WAV file (.at3) */
    [AFMT_PCM_WAV] =
        AFMT_ENTRY("WAV",   "wav",  "wav_enc",  get_wave_metadata,  "wav\0at3\0"),
    /* Ogg Vorbis */
    [AFMT_OGG_VORBIS] =
        AFMT_ENTRY("Ogg", "vorbis", NULL,       get_ogg_metadata,   "ogg\0oga\0"),
    /* FLAC */
    [AFMT_FLAC] =
        AFMT_ENTRY("FLAC",  "flac", NULL,       get_flac_metadata,  "flac\0"),
    /* Musepack SV7 */
    [AFMT_MPC_SV7] =
        AFMT_ENTRY("MPCv7", "mpc",  NULL,       get_musepack_metadata,"mpc\0"),
    /* A/52 (aka AC3) audio */                  
    [AFMT_A52] =
        AFMT_ENTRY("AC3",   "a52",  NULL,       get_a52_metadata,   "a52\0ac3\0"),
    /* WavPack */
    [AFMT_WAVPACK] =
        AFMT_ENTRY("WV","wavpack","wavpack_enc",get_wavpack_metadata,"wv\0"),
    /* Apple Lossless Audio Codec */
    [AFMT_MP4_ALAC] =
        AFMT_ENTRY("ALAC",  "alac", NULL,       get_mp4_metadata,   "m4a\0m4b\0"),
    /* Advanced Audio Coding in M4A container */
    [AFMT_MP4_AAC] =
        AFMT_ENTRY("AAC",   "aac",  NULL,       get_mp4_metadata,   "mp4\0"),
    /* Shorten */
    [AFMT_SHN] =
        AFMT_ENTRY("SHN","shorten", NULL,       get_shn_metadata,   "shn\0"),
    /* SID File Format */
    [AFMT_SID] =
        AFMT_ENTRY("SID",   "sid",  NULL,       get_sid_metadata,   "sid\0"),
    /* ADX File Format */
    [AFMT_ADX] =
        AFMT_ENTRY("ADX",   "adx",  NULL,       get_adx_metadata,   "adx\0"),
    /* NESM (NES Sound Format) */
    [AFMT_NSF] =
        AFMT_ENTRY("NSF",   "nsf",  NULL,       get_nsf_metadata,   "nsf\0nsfe\0"),
    /* Speex File Format */                     
    [AFMT_SPEEX] =
        AFMT_ENTRY("Speex", "speex",NULL,       get_ogg_metadata,   "spx\0"),
    /* SPC700 Save State */
    [AFMT_SPC] =
        AFMT_ENTRY("SPC",   "spc",  NULL,       get_spc_metadata,   "spc\0"),
    /* APE (Monkey's Audio) */
    [AFMT_APE] =
        AFMT_ENTRY("APE",   "ape",  NULL,       get_monkeys_metadata,"ape\0mac\0"),
    /* WMA (WMAV1/V2 in ASF) */
    [AFMT_WMA] =
        AFMT_ENTRY("WMA",   "wma",  NULL,       get_asf_metadata,"wma\0wmv\0asf\0"),
    /* WMA Professional in ASF */
    [AFMT_WMAPRO] =
        AFMT_ENTRY("WMAPro","wmapro",NULL,      NULL,            "wma\0wmv\0asf\0"),
    /* Amiga MOD File */
    [AFMT_MOD] =
        AFMT_ENTRY("MOD",   "mod",  NULL,       get_mod_metadata,   "mod\0"),
    /* Atari SAP File */
    [AFMT_SAP] =
        AFMT_ENTRY("SAP",   "asap", NULL,       get_asap_metadata,  "sap\0"),
    /* Cook in RM/RA */
    [AFMT_RM_COOK] =
        AFMT_ENTRY("Cook",  "cook", NULL,       get_rm_metadata,"rm\0ra\0rmvb\0"),
    /* AAC in RM/RA */
    [AFMT_RM_AAC] =
        AFMT_ENTRY("RAAC",  "raac", NULL,       NULL,           "rm\0ra\0rmvb\0"),
    /* AC3 in RM/RA */
    [AFMT_RM_AC3] =
        AFMT_ENTRY("AC3", "a52_rm", NULL,       NULL,           "rm\0ra\0rmvb\0"),
    /* ATRAC3 in RM/RA */
    [AFMT_RM_ATRAC3] =
        AFMT_ENTRY("ATRAC3","atrac3_rm",NULL,   NULL,           "rm\0ra\0rmvb\0"),
    /* Atari CMC File */
    [AFMT_CMC] =
        AFMT_ENTRY("CMC",   "asap", NULL,       get_other_asap_metadata,"cmc\0"),
    /* Atari CM3 File */
    [AFMT_CM3] =
        AFMT_ENTRY("CM3",   "asap", NULL,       get_other_asap_metadata,"cm3\0"),
    /* Atari CMR File */
    [AFMT_CMR] =
        AFMT_ENTRY("CMR",   "asap", NULL,       get_other_asap_metadata,"cmr\0"),
    /* Atari CMS File */
    [AFMT_CMS] =
        AFMT_ENTRY("CMS",   "asap", NULL,       get_other_asap_metadata,"cms\0"),
    /* Atari DMC File */
    [AFMT_DMC] =
        AFMT_ENTRY("DMC",   "asap", NULL,       get_other_asap_metadata,"dmc\0"),
    /* Atari DLT File */
    [AFMT_DLT] =
        AFMT_ENTRY("DLT",   "asap", NULL,       get_other_asap_metadata,"dlt\0"),
    /* Atari MPT File */
    [AFMT_MPT] =
        AFMT_ENTRY("MPT",   "asap", NULL,       get_other_asap_metadata,"mpt\0"), 
    /* Atari MPD File */
    [AFMT_MPD] =
        AFMT_ENTRY("MPD",   "asap", NULL,       get_other_asap_metadata,"mpd\0"),
    /* Atari RMT File */
    [AFMT_RMT] =                   
        AFMT_ENTRY("RMT",   "asap", NULL,       get_other_asap_metadata,"rmt\0"),
    /* Atari TMC File */
    [AFMT_TMC] =
        AFMT_ENTRY("TMC",   "asap", NULL,       get_other_asap_metadata,"tmc\0"),
    /* Atari TM8 File */
    [AFMT_TM8] =
        AFMT_ENTRY("TM8",   "asap", NULL,       get_other_asap_metadata,"tm8\0"),
    /* Atari TM2 File */
    [AFMT_TM2] =
        AFMT_ENTRY("TM2",   "asap", NULL,       get_other_asap_metadata,"tm2\0"),        
    /* Atrac3 in Sony OMA Container */
    [AFMT_OMA_ATRAC3] =
        AFMT_ENTRY("ATRAC3","atrac3_oma",NULL,  get_oma_metadata,   "oma\0aa3\0"), 
    /* SMAF (Synthetic music Mobile Application Format) */
    [AFMT_SMAF] =
        AFMT_ENTRY("SMAF",  "smaf", NULL,       get_smaf_metadata,  "mmf\0"),
    /* Sun Audio file */
    [AFMT_AU] =
        AFMT_ENTRY("AU",    "au",   NULL,       get_au_metadata,    "au\0snd\0"),
    /* VOX (Dialogic telephony file formats) */
    [AFMT_VOX] =
        AFMT_ENTRY("VOX",   "vox",  NULL,       get_vox_metadata,   "vox\0"),
    /* Wave64 */
    [AFMT_WAVE64] =
        AFMT_ENTRY("WAVE64","wav64",NULL,       get_wave64_metadata,"w64\0"),
    /* True Audio */
    [AFMT_TTA] =                                
        AFMT_ENTRY("TTA",   "tta",  NULL,       get_tta_metadata,   "tta\0"),
    /* WMA Voice in ASF */
    [AFMT_WMAVOICE] =
        AFMT_ENTRY("WMAVoice","wmavoice",NULL,  NULL,               "wma\0wmv\0"),
    /* Musepack SV8 */
    [AFMT_MPC_SV8] =
        AFMT_ENTRY("MPCv8", "mpc",  NULL,       get_musepack_metadata,"mpc\0"),
    /* Advanced Audio Coding High Efficiency in M4A container */
    [AFMT_MP4_AAC_HE] =
        AFMT_ENTRY("AAC-HE","aac",  NULL,       get_mp4_metadata,   "mp4\0"),
    /* AY (ZX Spectrum, Amstrad CPC Sound Format) */
    [AFMT_AY] = 
        AFMT_ENTRY("AY",    "ay",  NULL, get_ay_metadata,           "ay\0"),
    /* GBS (Game Boy Sound Format) */
    [AFMT_GBS] =
        AFMT_ENTRY("GBS",   "gbs",  NULL,       get_gbs_metadata,   "gbs\0"),
    /* HES (Hudson Entertainment System Sound Format) */
    [AFMT_HES] =
        AFMT_ENTRY("HES",   "hes",  NULL,       get_hes_metadata,   "hes\0"),
    /* SGC (Sega Master System, Game Gear, Coleco Vision Sound Format) */
    [AFMT_SGC] =
        AFMT_ENTRY("SGC", "sgc", NULL, get_sgc_metadata,     "sgc\0"),
    /* VGM (Video Game Music Format) */
    [AFMT_VGM] =
        AFMT_ENTRY("VGM", "vgm", NULL, get_vgm_metadata,   "vgm\0vgz\0"),
    /* KSS (MSX computer KSS Music File) */
    [AFMT_KSS] =
        AFMT_ENTRY("KSS", "kss", NULL, get_kss_metadata,   "kss\0"),
    /* Opus */
    [AFMT_OPUS] =
        AFMT_ENTRY("Opus", "opus", NULL, get_ogg_metadata,   "opus\0"),
    /* AAC bitstream format */
    [AFMT_AAC_BSF] =
        AFMT_ENTRY("AAC", "aac_bsf", NULL, get_aac_metadata,   "aac\0"),
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

#if 0 /* Currently unused, left for reference and future use */
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
#endif
#endif /* CONFIG_CODEC == SWCODEC && defined (HAVE_RECORDING) */

#if CONFIG_CODEC == SWCODEC
/* Get the canonical AFMT type */
int get_audio_base_codec_type(int type)
{
    int base_type = type;
    switch (type) {
        case AFMT_MPA_L1:
        case AFMT_MPA_L2:
        case AFMT_MPA_L3:
            base_type = AFMT_MPA_L3;
            break;
        case AFMT_MPC_SV7:
        case AFMT_MPC_SV8:
            base_type = AFMT_MPC_SV7;
            break;
        case AFMT_MP4_AAC:
        case AFMT_MP4_AAC_HE:
            base_type = AFMT_MP4_AAC;
            break;
        case AFMT_SAP:
        case AFMT_CMC:
        case AFMT_CM3:
        case AFMT_CMR:
        case AFMT_CMS:
        case AFMT_DMC:
        case AFMT_DLT:
        case AFMT_MPT:
        case AFMT_MPD:
        case AFMT_RMT:
        case AFMT_TMC:
        case AFMT_TM8:
        case AFMT_TM2:
            base_type = AFMT_SAP;
            break;
        default:
            break;
    }

    return base_type;
}

/* Get the basic audio type */
bool rbcodec_format_is_atomic(int afmt)
{
    if ((unsigned)afmt >= AFMT_NUM_CODECS)
        return false;

    switch (get_audio_base_codec_type(afmt))
    {
    case AFMT_NSF:
    case AFMT_SPC:
    case AFMT_SID:
    case AFMT_MOD:
    case AFMT_SAP:
    case AFMT_AY:
    case AFMT_GBS:
    case AFMT_HES:
    case AFMT_SGC:
    case AFMT_VGM:
    case AFMT_KSS:
        /* Type must be allocated and loaded in its entirety onto
           the buffer */
        return true;

    default:
        /* Assume type may be loaded and discarded incrementally */
        return false;
    }
}

/* Is the format allowed to buffer starting at some offset other than 0
   or first frame only for resume purposes? */
bool format_buffers_with_offset(int afmt)
{
    switch (afmt)
    {
    case AFMT_MPA_L1:
    case AFMT_MPA_L2:
    case AFMT_MPA_L3:
    case AFMT_WAVPACK:
        /* Format may be loaded at the first needed frame */
        return true;
    default:
        /* Format must be loaded from the beginning of the file
           (does not imply 'atomic', while 'atomic' implies 'no offset') */
        return false;
    }
}
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

/* Note, that this returns false for successful, true for error! */
bool mp3info(struct mp3entry *entry, const char *filename)
{
    int fd;
    bool result;

    fd = open(filename, O_RDONLY);
    if (fd < 0)
        return true;

    result = !get_metadata(entry, fd, filename);

    close(fd);

    return result;
}

/* Get metadata for track - return false if parsing showed problems with the
 * file that would prevent playback.
 */
bool get_metadata(struct mp3entry* id3, int fd, const char* trackname)
{
    const struct afmt_entry *entry;
    int logfd = 0;
    DEBUGF("Read metadata for %s\n", trackname);
    if (write_metadata_log)
    {
        logfd = open("/metadata.log", O_WRONLY | O_APPEND | O_CREAT, 0666);
        if (logfd >= 0)
        {
            write(logfd, trackname, strlen(trackname));
            write(logfd, "\n", 1);
            close(logfd);
        }
    }
    
    /* Clear the mp3entry to avoid having bogus pointers appear */
    wipe_mp3entry(id3);

    /* Take our best guess at the codec type based on file extension */
    id3->codectype = probe_file_format(trackname);

    /* default values for embedded cuesheets */
    id3->has_embedded_cuesheet = false;
    id3->embedded_cuesheet.pos = 0;

    entry = &audio_formats[id3->codectype];

    /* Load codec specific track tag information and confirm the codec type. */
    if (!entry->parse_func)
    {
        DEBUGF("nothing to parse for %s (format %s)\n", trackname, entry->label);
        return false;
    }

    if (!entry->parse_func(fd, id3))
    {
        DEBUGF("parsing %s failed (format: %s)\n", trackname, entry->label);
        return false;
    }

    lseek(fd, 0, SEEK_SET);
    strlcpy(id3->path, trackname, sizeof(id3->path));
    /* We have successfully read the metadata from the file */
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

#define MOVE_ENTRY(x) if (x) x += offset;

void adjust_mp3entry(struct mp3entry *entry, void *dest, const void *orig)
{
    long offset;
    if (orig > dest)
        offset = -((size_t)orig - (size_t)dest);
    else
        offset =  ((size_t)dest - (size_t)orig);

    MOVE_ENTRY(entry->title)
    MOVE_ENTRY(entry->artist)
    MOVE_ENTRY(entry->album)

    if (entry->genre_string > (char*)orig && 
        entry->genre_string < (char*)orig + sizeof(struct mp3entry))
        /* Don't adjust that if it points to an entry of the "genres" array */
        entry->genre_string += offset;

    MOVE_ENTRY(entry->track_string)
    MOVE_ENTRY(entry->disc_string)
    MOVE_ENTRY(entry->year_string)
    MOVE_ENTRY(entry->composer)
    MOVE_ENTRY(entry->comment)
    MOVE_ENTRY(entry->albumartist)
    MOVE_ENTRY(entry->grouping)
    MOVE_ENTRY(entry->mb_track_id)
}

void copy_mp3entry(struct mp3entry *dest, const struct mp3entry *orig)
{
    memcpy(dest, orig, sizeof(struct mp3entry));
    adjust_mp3entry(dest, dest, orig);
}

/* A shortcut to simplify the common task of clearing the struct */
void wipe_mp3entry(struct mp3entry *id3)
{
    memset(id3, 0, sizeof (struct mp3entry));
}

#if CONFIG_CODEC == SWCODEC
/* Glean what is possible from the filename alone - does not parse metadata */
void fill_metadata_from_path(struct mp3entry *id3, const char *trackname)
{
    char *p;

    /* Clear the mp3entry to avoid having bogus pointers appear */
    wipe_mp3entry(id3);

    /* Find the filename portion of the path */
    p = strrchr(trackname, '/');
    strlcpy(id3->id3v2buf, p ? ++p : id3->path, ID3V2_BUF_SIZE);

    /* Get the format from the extension and trim it off */
    p = strrchr(id3->id3v2buf, '.');
    if (p)
    {
        /* Might be wrong for container formats - should we bother? */
        id3->codectype = probe_file_format(p);

        if (id3->codectype != AFMT_UNKNOWN)
            *p = '\0';
    }

    /* Set the filename as the title */
    id3->title = id3->id3v2buf;

    /* Copy the path info */
    strlcpy(id3->path, trackname, sizeof (id3->path));
}
#endif /* CONFIG_CODEC == SWCODEC */
