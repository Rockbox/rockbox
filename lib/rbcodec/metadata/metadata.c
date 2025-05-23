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
bool write_metadata_log = false;

const struct afmt_entry audio_formats[AFMT_NUM_CODECS] =
{
 /* NOTE enc_root_fname is ignored here but kept for consistency see get_codec_enc_root_fn() */
    /* Unknown file format */
    [0 ... AFMT_NUM_CODECS-1] =
        AFMT_ENTRY("???", NULL,    "N/A",        NULL, "\0"  ),

    /* MPEG Audio layer 2 */
    [AFMT_MPA_L2] =
        AFMT_ENTRY("MP2",   "mpa",  "N/A",       get_mp3_metadata,   "mpa\0mp2\0"),

    /* MPEG Audio layer 3 */
    [AFMT_MPA_L3] =
        AFMT_ENTRY("MP3",   "mpa",  "mp3_enc",  get_mp3_metadata,   "mp3\0"),

    /* MPEG Audio layer 1 */
    [AFMT_MPA_L1] =
        AFMT_ENTRY("MP1",   "mpa",  "N/A",       get_mp3_metadata,   "mp1\0"),
    /* Audio Interchange File Format */
    [AFMT_AIFF] =
        AFMT_ENTRY("AIFF",  "aiff", "aiff_enc", get_aiff_metadata,  "aiff\0aif\0"),
    /* Uncompressed PCM in a WAV file OR ATRAC3 stream in WAV file (.at3) */
    [AFMT_PCM_WAV] =
        AFMT_ENTRY("WAV",   "wav",  "wav_enc",  get_wave_metadata,  "wav\0at3\0"),
    /* Ogg Vorbis */
    [AFMT_OGG_VORBIS] =
        AFMT_ENTRY("Ogg", "vorbis", "N/A",       get_ogg_metadata,   "ogg\0oga\0"),
    /* FLAC */
    [AFMT_FLAC] =
        AFMT_ENTRY("FLAC",  "flac", "N/A",       get_flac_metadata,  "flac\0"),
    /* Musepack SV7 */
    [AFMT_MPC_SV7] =
        AFMT_ENTRY("MPCv7", "mpc",  "N/A",       get_musepack_metadata,"mpc\0"),
    /* A/52 (aka AC3) audio */
    [AFMT_A52] =
        AFMT_ENTRY("AC3",   "a52",  "N/A",       get_a52_metadata,   "a52\0ac3\0"),
    /* WavPack */
    [AFMT_WAVPACK] =
        AFMT_ENTRY("WV","wavpack","wavpack_enc",get_wavpack_metadata,"wv\0"),
    /* Apple Lossless Audio Codec */
    [AFMT_MP4_ALAC] =
        AFMT_ENTRY("ALAC",  "alac", "N/A",       get_mp4_metadata,   "m4a\0m4b\0"),
    /* Advanced Audio Coding in M4A container */
    [AFMT_MP4_AAC] =
        AFMT_ENTRY("AAC",   "aac",  "N/A",       get_mp4_metadata,   "mp4\0"),
    /* Shorten */
    [AFMT_SHN] =
        AFMT_ENTRY("SHN","shorten", "N/A",       get_shn_metadata,   "shn\0"),
    /* SID File Format */
    [AFMT_SID] =
        AFMT_ENTRY("SID",   "sid",  "N/A",       get_sid_metadata,   "sid\0"),
    /* ADX File Format */
    [AFMT_ADX] =
        AFMT_ENTRY("ADX",   "adx",  "N/A",       get_adx_metadata,   "adx\0"),
    /* NESM (NES Sound Format) */
    [AFMT_NSF] =
        AFMT_ENTRY("NSF",   "nsf",  "N/A",       get_nsf_metadata,   "nsf\0nsfe\0"),
    /* Speex File Format */
    [AFMT_SPEEX] =
        AFMT_ENTRY("Speex", "speex","N/A",       get_ogg_metadata,   "spx\0"),
    /* SPC700 Save State */
    [AFMT_SPC] =
        AFMT_ENTRY("SPC",   "spc",  "N/A",       get_spc_metadata,   "spc\0"),
    /* APE (Monkey's Audio) */
    [AFMT_APE] =
        AFMT_ENTRY("APE",   "ape",  "N/A",       get_monkeys_metadata,"ape\0mac\0"),
    /* WMA (WMAV1/V2 in ASF) */
    [AFMT_WMA] =
        AFMT_ENTRY("WMA",   "wma",  "N/A",       get_asf_metadata,"wma\0wmv\0asf\0"),
    /* WMA Professional in ASF */
    [AFMT_WMAPRO] =
        AFMT_ENTRY("WMAPro","wmapro","N/A",      NULL,            "wma\0wmv\0asf\0"),
    /* Amiga MOD File */
    [AFMT_MOD] =
        AFMT_ENTRY("MOD",   "mod",  "N/A",       get_mod_metadata,   "mod\0"),
    /* Atari SAP File */
    [AFMT_SAP] =
        AFMT_ENTRY("SAP",   "asap", "N/A",       get_asap_metadata,  "sap\0"),
    /* Cook in RM/RA */
    [AFMT_RM_COOK] =
        AFMT_ENTRY("Cook",  "cook", "N/A",       get_rm_metadata,"rm\0ra\0rmvb\0"),
    /* AAC in RM/RA */
    [AFMT_RM_AAC] =
        AFMT_ENTRY("RAAC",  "raac", "N/A",       NULL,           "rm\0ra\0rmvb\0"),
    /* AC3 in RM/RA */
    [AFMT_RM_AC3] =
        AFMT_ENTRY("AC3", "a52_rm", "N/A",       NULL,           "rm\0ra\0rmvb\0"),
    /* ATRAC3 in RM/RA */
    [AFMT_RM_ATRAC3] =
        AFMT_ENTRY("ATRAC3","atrac3_rm","N/A",   NULL,           "rm\0ra\0rmvb\0"),
    /* Atari CMC File */
    [AFMT_CMC] =
        AFMT_ENTRY("CMC",   "asap", "N/A",       get_other_asap_metadata,"cmc\0"),
    /* Atari CM3 File */
    [AFMT_CM3] =
        AFMT_ENTRY("CM3",   "asap", "N/A",       get_other_asap_metadata,"cm3\0"),
    /* Atari CMR File */
    [AFMT_CMR] =
        AFMT_ENTRY("CMR",   "asap", "N/A",       get_other_asap_metadata,"cmr\0"),
    /* Atari CMS File */
    [AFMT_CMS] =
        AFMT_ENTRY("CMS",   "asap", "N/A",       get_other_asap_metadata,"cms\0"),
    /* Atari DMC File */
    [AFMT_DMC] =
        AFMT_ENTRY("DMC",   "asap", "N/A",       get_other_asap_metadata,"dmc\0"),
    /* Atari DLT File */
    [AFMT_DLT] =
        AFMT_ENTRY("DLT",   "asap", "N/A",       get_other_asap_metadata,"dlt\0"),
    /* Atari MPT File */
    [AFMT_MPT] =
        AFMT_ENTRY("MPT",   "asap", "N/A",       get_other_asap_metadata,"mpt\0"),
    /* Atari MPD File */
    [AFMT_MPD] =
        AFMT_ENTRY("MPD",   "asap", "N/A",       get_other_asap_metadata,"mpd\0"),
    /* Atari RMT File */
    [AFMT_RMT] =
        AFMT_ENTRY("RMT",   "asap", "N/A",       get_other_asap_metadata,"rmt\0"),
    /* Atari TMC File */
    [AFMT_TMC] =
        AFMT_ENTRY("TMC",   "asap", "N/A",       get_other_asap_metadata,"tmc\0"),
    /* Atari TM8 File */
    [AFMT_TM8] =
        AFMT_ENTRY("TM8",   "asap", "N/A",       get_other_asap_metadata,"tm8\0"),
    /* Atari TM2 File */
    [AFMT_TM2] =
        AFMT_ENTRY("TM2",   "asap", "N/A",       get_other_asap_metadata,"tm2\0"),
    /* Atrac3 in Sony OMA Container */
    [AFMT_OMA_ATRAC3] =
        AFMT_ENTRY("ATRAC3","atrac3_oma","N/A",  get_oma_metadata,   "oma\0aa3\0"),
    /* SMAF (Synthetic music Mobile Application Format) */
    [AFMT_SMAF] =
        AFMT_ENTRY("SMAF",  "smaf", "N/A",       get_smaf_metadata,  "mmf\0"),
    /* Sun Audio file */
    [AFMT_AU] =
        AFMT_ENTRY("AU",    "au",   "N/A",       get_au_metadata,    "au\0snd\0"),
    /* VOX (Dialogic telephony file formats) */
    [AFMT_VOX] =
        AFMT_ENTRY("VOX",   "vox",  "N/A",       get_vox_metadata,   "vox\0"),
    /* Wave64 */
    [AFMT_WAVE64] =
        AFMT_ENTRY("WAVE64","wav64","N/A",       get_wave64_metadata,"w64\0"),
    /* True Audio */
    [AFMT_TTA] =
        AFMT_ENTRY("TTA",   "tta",  "N/A",       get_tta_metadata,   "tta\0"),
    /* WMA Voice in ASF */
    [AFMT_WMAVOICE] =
        AFMT_ENTRY("WMAVoice","wmavoice","N/A",  NULL,               "wma\0wmv\0"),
    /* Musepack SV8 */
    [AFMT_MPC_SV8] =
        AFMT_ENTRY("MPCv8", "mpc",  "N/A",       get_musepack_metadata,"mpc\0"),
    /* Advanced Audio Coding High Efficiency in M4A container */
    [AFMT_MP4_AAC_HE] =
        AFMT_ENTRY("AAC-HE","aac",  "N/A",       get_mp4_metadata,   "mp4\0"),
    /* AY (ZX Spectrum, Amstrad CPC Sound Format) */
    [AFMT_AY] =
        AFMT_ENTRY("AY",    "ay",  "N/A", get_ay_metadata,           "ay\0"),
    /* AY (ZX Spectrum Sound Format) */
#ifdef HAVE_FPU
    [AFMT_VTX] =
        AFMT_ENTRY("VTX",   "vtx", "N/A", get_vtx_metadata,          "vtx\0"),
#endif
    /* GBS (Game Boy Sound Format) */
    [AFMT_GBS] =
        AFMT_ENTRY("GBS",   "gbs",  "N/A",       get_gbs_metadata,   "gbs\0"),
    /* HES (Hudson Entertainment System Sound Format) */
    [AFMT_HES] =
        AFMT_ENTRY("HES",   "hes",  "N/A",       get_hes_metadata,   "hes\0"),
    /* SGC (Sega Master System, Game Gear, Coleco Vision Sound Format) */
    [AFMT_SGC] =
        AFMT_ENTRY("SGC", "sgc", "N/A", get_sgc_metadata,     "sgc\0"),
    /* VGM (Video Game Music Format) */
    [AFMT_VGM] =
        AFMT_ENTRY("VGM", "vgm", "N/A", get_vgm_metadata,   "vgm\0vgz\0"),
    /* KSS (MSX computer KSS Music File) */
    [AFMT_KSS] =
        AFMT_ENTRY("KSS", "kss", "N/A", get_kss_metadata,   "kss\0"),
    /* Opus */
    [AFMT_OPUS] =
        AFMT_ENTRY("Opus", "opus", "N/A", get_ogg_metadata,   "opus\0"),
    /* AAC bitstream format */
    [AFMT_AAC_BSF] =
        AFMT_ENTRY("AAC", "aac_bsf", "N/A", get_aac_metadata,   "aac\0"),
};

#if defined (HAVE_RECORDING)
inline const char *get_codec_enc_root_fn(int type) /* filename of encoder codec */
{
    /* NOTE these are the same as above audio_formats but only enc_root_fname is used.. */
    if (type == AFMT_MPA_L3)
        return AFMT_ENTRY_ENC("MP3",   "mpa",  "mp3_enc",  get_mp3_metadata,   "mp3\0");
    if (type == AFMT_AIFF)
        return AFMT_ENTRY_ENC("AIFF",  "aiff", "aiff_enc", get_aiff_metadata,  "aiff\0aif\0");
    if (type == AFMT_PCM_WAV)
        return AFMT_ENTRY_ENC("WAV",   "wav",  "wav_enc",  get_wave_metadata,  "wav\0at3\0");
    if (AFMT_WAVPACK)
        return AFMT_ENTRY_ENC("WV","wavpack","wavpack_enc",get_wavpack_metadata,"wv\0");
    return NULL;
}

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
#endif /* defined (HAVE_RECORDING) */

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

const char * get_codec_string(int type)
{
    if (type < 0 || type >= AFMT_NUM_CODECS)
        type = AFMT_UNKNOWN;

    return audio_formats[type].label;
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
#ifdef HAVE_FPU
    case AFMT_VTX:
#endif
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
 * file that would prevent playback. supply a filedescriptor <0 and the file will be opened
 * and closed automatically within the get_metadata call
 * get_metadata_ex allows flags to change the way get_metadata behaves
 * METADATA_EXCLUDE_ID3_PATH  won't copy filename path to the id3 path buffer 
 * METADATA_CLOSE_FD_ON_EXIT closes the open filedescriptor on exit
 */
bool get_metadata_ex(struct mp3entry* id3, int fd, const char* trackname, int flags)
{
    bool success = true;
    const struct afmt_entry *entry;
    int logfd = 0;
    DEBUGF("Read metadata for %s\n", trackname);
    const char *res_str = "\n";

    /* Clear the mp3entry to avoid having bogus pointers appear */
    wipe_mp3entry(id3);

    if (fd < 0)
    {
        fd = open(trackname, O_RDONLY);
        flags |= METADATA_CLOSE_FD_ON_EXIT;
        if (fd < 0)
        {
            DEBUGF("Error opening %s\n", trackname);
            res_str = " - [Error opening]\n";
            success = false;
            goto log_on_exit;
        }
    }

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
        res_str = " - [No parser]\n";
        success = false;
    }
    else if (!entry->parse_func(fd, id3))
    {
        DEBUGF("parsing %s failed (format: %s)\n", trackname, entry->label);
        res_str = " - [Parser failed]\n";
        success = false;
        wipe_mp3entry(id3); /* ensure the mp3entry is clear */
    }

    if ((flags & METADATA_CLOSE_FD_ON_EXIT))
        close(fd);
    else
        lseek(fd, 0, SEEK_SET);

    if (success && (flags & METADATA_EXCLUDE_ID3_PATH) == 0)
    {
        strlcpy(id3->path, trackname, sizeof(id3->path));
    }
    /* have we successfully read the metadata from the file? */
log_on_exit:
    if (write_metadata_log)
    {
        logfd = open("/metadata.log", O_WRONLY | O_APPEND | O_CREAT, 0666);
        if (logfd >= 0)
        {
            write(logfd, trackname, strlen(trackname));
            write(logfd, res_str, strlen(res_str));
            close(logfd);
        }
    }

    return success;
}

bool get_metadata(struct mp3entry* id3, int fd, const char* trackname)
{
    return get_metadata_ex(id3, fd, trackname, 0);
}

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
    if (dest != orig)
    {
        memcpy(dest, orig, sizeof(struct mp3entry));
        adjust_mp3entry(dest, dest, orig);
    }
}

/* A shortcut to simplify the common task of clearing the struct */
void wipe_mp3entry(struct mp3entry *id3)
{
    memset(id3, 0, sizeof (struct mp3entry));
}

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
