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
#include <inttypes.h>
#include "platform.h"

#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "rbunicode.h"
#include "logf.h"

#ifdef DEBUGF
#undef DEBUGF
#define DEBUGF(...)
#endif

/* Wave(RIFF)/Wave64 format */


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

enum {
   RIFF_CHUNK = 0,
   WAVE_CHUNK,
   FMT_CHUNK,
   FACT_CHUNK,
   DATA_CHUNK,
   LIST_CHUNK,
};

/* Wave chunk names */
#define WAVE_CHUNKNAME_LENGTH 4
#define WAVE_CHUNKSIZE_LENGTH 4

static const unsigned char * const wave_chunklist 
                                           = "RIFF"
                                             "WAVE"
                                             "fmt "
                                             "fact"
                                             "data"
                                             "LIST";

/* Wave64 GUIDs */
#define WAVE64_CHUNKNAME_LENGTH 16
#define WAVE64_CHUNKSIZE_LENGTH 8

static const unsigned char * const wave64_chunklist
                 = "riff\x2e\x91\xcf\x11\xa5\xd6\x28\xdb\x04\xc1\x00\x00"
                   "wave\xf3\xac\xd3\x11\x8c\xd1\x00\xc0\x4f\x8e\xdb\x8a"
                   "fmt \xf3\xac\xd3\x11\x8c\xd1\x00\xc0\x4f\x8e\xdb\x8a"
                   "fact\xf3\xac\xd3\x11\x8c\xd1\x00\xc0\x4f\x8e\xdb\x8a"
                   "data\xf3\xac\xd3\x11\x8c\xd1\x00\xc0\x4f\x8e\xdb\x8a"
                   "\xbc\x94\x5f\x92\x5a\x52\xd2\x11\x86\xdc\x00\xc0\x4f\x8e\xdb\x8a";

/* list/info chunk */

struct info_chunk {
    const unsigned char* tag;
    size_t offset;
};

/* info chunk names are common wave/wave64 */
static const struct info_chunk info_chunks[] = {
    { "INAM", offsetof(struct mp3entry, title),        }, /* title */
    { "IART", offsetof(struct mp3entry, artist),       }, /* artist */
    { "ISBJ", offsetof(struct mp3entry, albumartist),  }, /* albumartist */
    { "IPRD", offsetof(struct mp3entry, album),        }, /* album */
    { "IWRI", offsetof(struct mp3entry, composer),     }, /* composer */
    { "ICMT", offsetof(struct mp3entry, comment),      }, /* comment */
    { "ISRF", offsetof(struct mp3entry, grouping),     }, /* grouping */
    { "IGNR", offsetof(struct mp3entry, genre_string), }, /* genre */
    { "ICRD", offsetof(struct mp3entry, year_string),  }, /* date */
    { "IPRT", offsetof(struct mp3entry, track_string), }, /* track/trackcount */
    { "IFRM", offsetof(struct mp3entry, disc_string),  }, /* disc/disccount */
};

#define INFO_CHUNK_COUNT ((int)ARRAYLEN(info_chunks))

/* support formats */
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
    WAVE_FORMAT_EXTENSIBLE = 0xFFFE,
};

struct wave_fmt {
    unsigned int formattag;
    unsigned int channels;
    unsigned int blockalign;
    unsigned int bitspersample;
    unsigned int samplesperblock;
    uint32_t totalsamples;
    uint64_t numbytes;
};

static unsigned char *convert_utf8(const unsigned char *src, unsigned char *dst,
                                   int size, bool is_64)
{
    if (is_64)
    {
        /* Note: wave64: metadata codepage is UTF-16 only */
        return utf16LEdecode(src, dst, size);
    }
    return iso_decode(src, dst, -1, size);
}

static void set_totalsamples(struct wave_fmt *fmt, struct mp3entry* id3)
{
    switch (fmt->formattag)
    {
        case WAVE_FORMAT_PCM:
        case WAVE_FORMAT_IEEE_FLOAT:
        case WAVE_FORMAT_ALAW:
        case WAVE_FORMAT_MULAW:
        case IBM_FORMAT_ALAW:
        case IBM_FORMAT_MULAW:
            fmt->blockalign      = fmt->bitspersample * fmt->channels >> 3;
            fmt->samplesperblock = 1;
            break;
        case WAVE_FORMAT_YAMAHA_ADPCM:
            if (id3->channels != 0)
            {
                fmt->samplesperblock =
                    (fmt->blockalign == ((id3->frequency / 60) + 4) * fmt->channels)?
                        id3->frequency / 30 : (fmt->blockalign << 1) / fmt->channels;
            }
            break;
        case WAVE_FORMAT_DIALOGIC_OKI_ADPCM:
            fmt->blockalign      = 1;
            fmt->samplesperblock = 2;
            break;
        case WAVE_FORMAT_SWF_ADPCM:
            if (fmt->bitspersample != 0 && id3->channels != 0)
            {
                fmt->samplesperblock
                     = (((fmt->blockalign << 3) - 2) / fmt->channels - 22)
                                                     / fmt->bitspersample + 1;
            }
            break;
        default:
            break;
    }

    if (fmt->blockalign != 0)
        fmt->totalsamples = (fmt->numbytes / fmt->blockalign) * fmt->samplesperblock;
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

    if (fmt->formattag != WAVE_FORMAT_EXTENSIBLE)
    {
        if (fmtsize > 19)
        {
            /* wSamplesPerBlock */
            fmt->samplesperblock = buf[18] | (buf[19] << 8);
        }
    }
    else if (fmtsize > 25)
    {
        /* wValidBitsPerSample */
        fmt->bitspersample = buf[18] | (buf[19] << 8);
        /* SubFormat */
        fmt->formattag = buf[24] | (buf[25] << 8);
    }

    /* Check for ATRAC3 stream */
    if (fmt->formattag == WAVE_FORMAT_ATRAC3)
    {
        int jsflag = 0;
        if(id3->bitrate == 66 || id3->bitrate == 94)
            jsflag = 1;

        id3->extradata_size = 14;
        id3->channels = 2;
        id3->codectype = AFMT_OMA_ATRAC3;
        id3->bytesperframe = fmt->blockalign;
        
        /* Store the extradata for the codec */
        AV_WL16(&id3->id3v2buf[0],  1);             // always 1
        AV_WL32(&id3->id3v2buf[2],  id3->frequency);// samples rate
        AV_WL16(&id3->id3v2buf[6],  jsflag);        // coding mode
        AV_WL16(&id3->id3v2buf[8],  jsflag);        // coding mode
        AV_WL16(&id3->id3v2buf[10], 1);             // always 1
        AV_WL16(&id3->id3v2buf[12], 0);             // always 0
    }
}

static void parse_list_chunk(int fd, struct mp3entry* id3, int chunksize, bool is_64)
{
    unsigned char tmpbuf[ID3V2_BUF_SIZE];
    unsigned char *bp = tmpbuf;
    unsigned char *endp;
    unsigned char *data_pos;
    unsigned char *tag_pos  = id3->id3v2buf;
    int datasize;
    int infosize;
    int remain;
    int i;

    if (is_64)
        lseek(fd, 4, SEEK_CUR);
    else if (read(fd, bp, 4) < 4 || memcmp(bp, "INFO", 4))
        return;

    /* decrease skip bytes */
    chunksize -= 4;

    infosize = read(fd, bp, (ID3V2_BUF_SIZE > chunksize)? chunksize : ID3V2_BUF_SIZE);
    if (infosize <= 8)
        return;

    endp = bp + infosize;
    while (bp < endp)
    {
        datasize = get_long_le(bp + 4);
        data_pos = bp + 8;
        remain = ID3V2_BUF_SIZE - (tag_pos - (unsigned char*)id3->id3v2buf);
        if (remain < 1)
            break;

        for (i = 0; i < INFO_CHUNK_COUNT; i++)
        {
            if (memcmp(bp, info_chunks[i].tag, 4) == 0)
            {
                *((char **)(((char*)id3) + info_chunks[i].offset)) = tag_pos;
                tag_pos = convert_utf8(data_pos, tag_pos,
                                       (datasize + 1 >= remain )? remain - 1 : datasize,
                                       is_64);
                *tag_pos++ = 0;
                break;
            }
        }
        bp = data_pos + datasize + (datasize & 1);
    };
}

static bool read_header(int fd, struct mp3entry* id3, const unsigned char *chunknames,
                        bool is_64)
{
    /* Use the temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;

    struct wave_fmt fmt;

    const unsigned int namelen = (is_64)? WAVE64_CHUNKNAME_LENGTH : WAVE_CHUNKNAME_LENGTH;
    const unsigned int sizelen = (is_64)? WAVE64_CHUNKSIZE_LENGTH : WAVE_CHUNKSIZE_LENGTH;
    const unsigned int len = namelen + sizelen;
    uint64_t chunksize;
    uint64_t offset = len + namelen;
    int read_data;

    memset(&fmt, 0, sizeof(struct wave_fmt));
 
    id3->vbr = false;   /* All Wave/Wave64 files are CBR */
    id3->filesize = filesize(fd);

    /* get RIFF chunk header */
    lseek(fd, 0, SEEK_SET);
    read(fd, buf, offset);

    if ((memcmp(buf,       chunknames + RIFF_CHUNK * namelen, namelen) != 0) ||
        (memcmp(buf + len, chunknames + WAVE_CHUNK * namelen, namelen) != 0))
    {
        DEBUGF("metadata error: missing riff header.\n");
        return false;
    }

    /* iterate over WAVE chunks until 'data' chunk */
    while (read(fd, buf, len) > 0)
    {
        offset += len;

        /* get chunk size (when the header is wave64, chunksize includes GUID and data length) */
        chunksize = (is_64) ? get_uint64_le(buf + namelen) - len :
                              get_long_le(buf + namelen);

        read_data = 0;
        if (memcmp(buf, chunknames + FMT_CHUNK * namelen, namelen) == 0)
        {
            DEBUGF("find 'fmt ' chunk\n");

            if (chunksize < 16)
            {
                DEBUGF("metadata error: 'fmt ' chunk is too small: %d\n", (int)chunksize);
                return false;
            }

            /* get and parse format */
            read_data = (chunksize > 25)? 26 : chunksize;

            read(fd, buf, read_data);
            parse_riff_format(buf, read_data, &fmt, id3);
        }
        else if (memcmp(buf, chunknames + FACT_CHUNK * namelen, namelen) == 0)
        {
            DEBUGF("find 'fact' chunk\n");

            /* dwSampleLength */
            if (chunksize >= sizelen)
            {
                /* get totalsamples */
                read_data = sizelen;
                read(fd, buf, read_data);
                fmt.totalsamples = (is_64)? get_uint64_le(buf) : get_long_le(buf);
            }
        }
        else if (memcmp(buf, chunknames + DATA_CHUNK * namelen, namelen) == 0)
        {
            DEBUGF("find 'data' chunk\n");
            fmt.numbytes = chunksize;
            if (fmt.formattag == WAVE_FORMAT_ATRAC3)
                id3->first_frame_offset = offset;
        }
        else if (memcmp(buf, chunknames + LIST_CHUNK * namelen, namelen) == 0)
        {
            DEBUGF("find 'LIST' chunk\n");
            parse_list_chunk(fd, id3, chunksize, is_64);
            lseek(fd, offset, SEEK_SET);
        }

        /* padded to next chunk */
        chunksize += ((is_64)? ((1 + ~chunksize) & 0x07) : (chunksize & 1));

        offset += chunksize;
        if (offset >= id3->filesize)
            break;

        lseek(fd, chunksize - read_data, SEEK_CUR);
    }

    if (fmt.numbytes == 0)
    {
        DEBUGF("metadata error: read error or missing 'data' chunk.\n");
        return false;
    }

    if (fmt.totalsamples == 0)
        set_totalsamples(&fmt, id3);

    if (id3->frequency == 0 || id3->bitrate == 0)
    {
        DEBUGF("metadata error: frequency or bitrate is 0\n");
        return false;
    }

    /* Calculate track length (in ms) and estimate the bitrate (in kbit/s) */
    id3->length = (fmt.formattag != WAVE_FORMAT_ATRAC3)?
                      (uint64_t)fmt.totalsamples * 1000 / id3->frequency :
                      ((id3->filesize - id3->first_frame_offset) * 8) / id3->bitrate;

    /* output header/id3 info (for debug) */
    DEBUGF("%s header info ----\n", (is_64)? "wave64" : "wave");
    DEBUGF("  format:          %04x\n", (int)fmt.formattag);
    DEBUGF("  channels:        %u\n", fmt.channels);
    DEBUGF("  blockalign:      %u\n", fmt.blockalign);
    DEBUGF("  bitspersample:   %u\n", fmt.bitspersample);
    DEBUGF("  samplesperblock: %u\n", fmt.samplesperblock);
    DEBUGF("  totalsamples:    %u\n", (unsigned int)fmt.totalsamples);
    DEBUGF("  numbytes:        %u\n", (unsigned int)fmt.numbytes);
    DEBUGF("id3 info ----\n");
    DEBUGF("  frequency:       %u\n", (unsigned int)id3->frequency);
    DEBUGF("  bitrate:         %d\n", id3->bitrate);
    DEBUGF("  length:          %u\n", (unsigned int)id3->length);

    return true;
}

bool get_wave_metadata(int fd, struct mp3entry* id3)
{
    return read_header(fd, id3, wave_chunklist, false);
}

bool get_wave64_metadata(int fd, struct mp3entry* id3)
{
    return read_header(fd, id3, wave64_chunklist, true);
}
