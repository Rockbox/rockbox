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
 * Copyright (C) 2010 Yoshihisa Uchida
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
#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "logf.h"

#   define AV_WL32(p, d) do {                   \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
        ((uint8_t*)(p))[3] = (d)>>24;           \
    } while(0)
#   define AV_WL16(p, d) do {                   \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
    } while(0)

/* Wave(RIFF)/Wave64 format */

/* Wave64 GUIDs */
#define WAVE64_GUID_RIFF "riff\x2e\x91\xcf\x11\xa5\xd6\x28\xdb\x04\xc1\x00\x00"
#define WAVE64_GUID_WAVE "wave\xf3\xac\xd3\x11\x8c\xd1\x00\xc0\x4f\x8e\xdb\x8a"
#define WAVE64_GUID_FMT  "fmt \xf3\xac\xd3\x11\x8c\xd1\x00\xc0\x4f\x8e\xdb\x8a"
#define WAVE64_GUID_FACT "fact\xf3\xac\xd3\x11\x8c\xd1\x00\xc0\x4f\x8e\xdb\x8a"
#define WAVE64_GUID_DATA "data\xf3\xac\xd3\x11\x8c\xd1\x00\xc0\x4f\x8e\xdb\x8a"

enum
{
    WAVE_FORMAT_PCM = 0x0001,   /* Microsoft PCM Format */
    WAVE_FORMAT_ADPCM = 0x0002, /* Microsoft ADPCM Format */
    WAVE_FORMAT_IEEE_FLOAT = 0x0003, /* IEEE Float */
    WAVE_FORMAT_ALAW = 0x0006,  /* Microsoft ALAW */
    WAVE_FORMAT_MULAW = 0x0007, /* Microsoft MULAW */
    WAVE_FORMAT_DVI_ADPCM = 0x0011, /* Intel's DVI ADPCM */
    WAVE_FORMAT_DIALOGIC_OKI_ADPCM = 0x0017, /* Dialogic OKI ADPCM */
    WAVE_FORMAT_YAMAHA_ADPCM = 0x0020, /* Yamaha ADPCM */
    WAVE_FORMAT_XBOX_ADPCM = 0x0069, /* XBOX ADPCM */
    IBM_FORMAT_MULAW = 0x0101,  /* same as WAVE_FORMAT_MULAW */
    IBM_FORMAT_ALAW = 0x0102,   /* same as WAVE_FORMAT_ALAW */
    WAVE_FORMAT_ATRAC3 = 0x0270, /* Atrac3 stream */
    WAVE_FORMAT_SWF_ADPCM = 0x5346, /* Adobe SWF ADPCM */
};

struct wave_fmt {
    unsigned int formattag;
    unsigned long channels;
    unsigned int blockalign;
    unsigned long bitspersample;
    unsigned int samplesperblock;
    uint64_t numbytes;
};

static unsigned long get_totalsamples(struct wave_fmt *fmt, struct mp3entry* id3)
{
    unsigned long totalsamples = 0;

    switch (fmt->formattag)
    {
        case WAVE_FORMAT_PCM:
        case WAVE_FORMAT_IEEE_FLOAT:
        case WAVE_FORMAT_ALAW:
        case WAVE_FORMAT_MULAW:
        case IBM_FORMAT_ALAW:
        case IBM_FORMAT_MULAW:
            totalsamples =
                 fmt->numbytes / ((((fmt->bitspersample - 1) / 8) + 1) * fmt->channels);
            break;
        case WAVE_FORMAT_ADPCM:
        case WAVE_FORMAT_DVI_ADPCM:
        case WAVE_FORMAT_XBOX_ADPCM:
            totalsamples = (fmt->numbytes / fmt->blockalign) * fmt->samplesperblock;
            break;
        case WAVE_FORMAT_YAMAHA_ADPCM:
            if (fmt->samplesperblock == 0)
            {
                if (fmt->blockalign == ((id3->frequency / 60) + 4) * fmt->channels)
                    fmt->samplesperblock = id3->frequency / 30;
                else
                    fmt->samplesperblock = fmt->blockalign * 2 / fmt->channels;
            }
            totalsamples = (fmt->numbytes / fmt->blockalign) * fmt->samplesperblock;
            break;
        case WAVE_FORMAT_DIALOGIC_OKI_ADPCM:
            totalsamples = 2 * fmt->numbytes;
            break;
        case WAVE_FORMAT_SWF_ADPCM:
            if (fmt->samplesperblock == 0)
                fmt->samplesperblock = (((fmt->blockalign << 3) - 2) / fmt->channels - 22)
                                                                     / fmt->bitspersample;

            totalsamples = (fmt->numbytes / fmt->blockalign) * fmt->samplesperblock;
            break;
        default:
            totalsamples = 0;
            break;
    }
    return totalsamples;
}

static void parse_riff_format(unsigned char* buf, int fmtsize, struct wave_fmt *fmt,
                              struct mp3entry* id3)
{
    /* wFormatTag */
    fmt->formattag = buf[0] | (buf[1] << 8);
    /* wChannels */
    fmt->channels = buf[2] | (buf[3] << 8);
    /* dwSamplesPerSec */
    id3->frequency = get_long_le(&buf[4]);
    /* dwAvgBytesPerSec */
    id3->bitrate = (get_long_le(&buf[8]) * 8) / 1000;
    /* wBlockAlign */
    fmt->blockalign = buf[12] | (buf[13] << 8);
    /* wBitsPerSample */
    fmt->bitspersample = buf[14] | (buf[15] << 8);
    if (fmtsize > 19)
    {
        /* wSamplesPerBlock */
        fmt->samplesperblock = buf[18] | (buf[19] << 8);
    }
}

bool get_wave_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    struct wave_fmt fmt;
    unsigned long totalsamples = 0;
    unsigned long offset = 0;
    int read_bytes;
    int i;

    memset(&fmt, 0, sizeof(struct wave_fmt));

    /* get RIFF chunk header */
    if ((lseek(fd, 0, SEEK_SET) < 0) || (read(fd, buf, 12) < 12))
    {
        return false;
    }
    offset += 12;

    if ((memcmp(buf, "RIFF", 4) != 0) || (memcmp(&buf[8], "WAVE", 4) != 0))
    {
        DEBUGF("metadata error: missing riff header.\n");
        return false;
    }

    /* iterate over WAVE chunks until 'data' chunk */
    while (true)
    {
        /* get chunk header */
        if (read(fd, buf, 8) < 8)
            return false;
        offset += 8;

        /* chunkSize */
        i = get_long_le(&buf[4]);

        if (memcmp(buf, "fmt ", 4) == 0)
        {
            /* get rest of chunk */
            if (i < 16)
                return false;

            read_bytes = 16;
            if (i > 19)
                read_bytes = 20;

            if (read(fd, buf, read_bytes) != read_bytes)
                return false;

            offset += read_bytes;
            i -= read_bytes;

            parse_riff_format(buf, i, &fmt, id3);

            /* Check for ATRAC3 stream */
            if (fmt.formattag == WAVE_FORMAT_ATRAC3)
            {
                int jsflag = 0;
                if(id3->bitrate == 66 || id3->bitrate == 94)
                    jsflag = 1;

                id3->extradata_size = 14;
                id3->channels = 2;
                id3->codectype = AFMT_OMA_ATRAC3;
                /* Store the extradata for the codec */
                AV_WL16(&id3->id3v2buf[0],  1);             // always 1
                AV_WL32(&id3->id3v2buf[2],  id3->frequency);    // samples rate
                AV_WL16(&id3->id3v2buf[6],  jsflag);        // coding mode
                AV_WL16(&id3->id3v2buf[8],  jsflag);        // coding mode
                AV_WL16(&id3->id3v2buf[10], 1);             // always 1
                AV_WL16(&id3->id3v2buf[12], 0);             // always 0
            }
        }
        else if (memcmp(buf, "data", 4) == 0)
        {
            fmt.numbytes = i;
            if (fmt.formattag == WAVE_FORMAT_ATRAC3)
                id3->first_frame_offset = offset;
            break;
        }
        else if (memcmp(buf, "fact", 4) == 0)
        {
            /* dwSampleLength */
            if (i >= 4)
            {
                /* get rest of chunk */
                if (read(fd, buf, 4) < 4)
                    return false;
                offset += 4;
                i -= 4;
                totalsamples = get_long_le(buf);
            }
        }

        /* seek to next chunk (even chunk sizes must be padded) */
        if (i & 0x01)
            i++;

        if(lseek(fd, i, SEEK_CUR) < 0)
            return false;
        offset += i;
    }

    if ((fmt.numbytes == 0) || (fmt.channels == 0) || (fmt.blockalign == 0))
    {
        DEBUGF("metadata error: numbytes, channels, or blockalign is 0.\n");
        return false;
    }

    if (totalsamples == 0)
    {
        totalsamples = get_totalsamples(&fmt, id3);
    }

    id3->vbr = false;   /* All WAV files are CBR */
    id3->filesize = filesize(fd);

    /* Calculate track length (in ms) and estimate the bitrate (in kbit/s) */
    if(id3->codectype != AFMT_OMA_ATRAC3)
        id3->length = ((int64_t) totalsamples * 1000) / id3->frequency;
    else
        id3->length   = ((id3->filesize - id3->first_frame_offset) * 8) / id3->bitrate;

    return true;
}

bool get_wave64_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    struct wave_fmt fmt;
    unsigned long totalsamples = 0;
    int read_bytes;
    uint64_t i;

    memset(&fmt, 0, sizeof(struct wave_fmt));

    /* get RIFF chunk header */
    if ((lseek(fd, 0, SEEK_SET) < 0) || (read(fd, buf, 40) < 40))
        return false;

    if ((memcmp(buf   , WAVE64_GUID_RIFF, 16) != 0)||
        (memcmp(buf+24, WAVE64_GUID_WAVE, 16) != 0))
    {
        DEBUGF("metada error: does not wave64 file\n");
        return false;
    }

    /* iterate over WAVE chunks until 'data' chunk */
    while (true)
    {
        /* get chunk header */
        if (read(fd, buf, 24) < 24)
            return false;

        /* chunkSize (excludes GUID and size length) */
        i = get_uint64_le(&buf[16]) - 24;

        if (memcmp(buf, WAVE64_GUID_FMT, 16) == 0)
        {
            DEBUGF("find 'fmt ' chunk\n");
            if (i < 16)
                return false;

            read_bytes = 16;
            if (i > 16)
            {
                read_bytes = 24;
                if (i < 24)
                    i = 24;
            }

            /* get rest of chunk */
            if (read(fd, buf, read_bytes) < read_bytes)
                return false;

            i -= read_bytes;
            parse_riff_format(buf, read_bytes, &fmt, id3);
        }
        else if (memcmp(buf, WAVE64_GUID_DATA, 16) == 0)
        {
            DEBUGF("find 'data' chunk\n");
            fmt.numbytes = i;
            break;
        }
        else if (memcmp(buf, WAVE64_GUID_FACT, 16) == 0)
        {
            /* Skip 'fact' chunk */
            DEBUGF("find 'fact' chunk\n");
        }

        /* seek to next chunk (8byte bound) */
        if (i & 0x07)
            i += 8 - (i & 0x7);

        if(lseek(fd, i, SEEK_CUR) < 0)
            return false;
    }

    if ((fmt.numbytes == 0) || (fmt.channels == 0) || (fmt.blockalign == 0))
    {
        DEBUGF("metadata error: numbytes, channels, or blockalign is 0\n");
        return false;
    }

    if (totalsamples == 0)
    {
        totalsamples = get_totalsamples(&fmt, id3);
    }

    id3->vbr = false;   /* All Wave64 files are CBR */
    id3->filesize = filesize(fd);

    /* Calculate track length (in ms) and estimate the bitrate (in kbit/s) */
    id3->length = ((int64_t) totalsamples * 1000) / id3->frequency;

    return true;
}
