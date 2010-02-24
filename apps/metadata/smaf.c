/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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
#include "rbunicode.h"
#include "logf.h"

static int basebits[4] = { 4, 8, 12, 16 };

static int frequency[5] = { 4000, 8000, 11025, 22050, 44100 };

static int support_codepages[7] = {
    SJIS, ISO_8859_1, -1, GB_2312, BIG_5, -1, -1,
};

/* extra codepage */
#define UCS_2  (NUM_CODEPAGES + 1)
#define UTF_16 (NUM_CODEPAGES + 2)

/* support id3 tag */
#define TAG_TITLE    (('S'<<8)|'T')
#define TAG_ARTIST   (('A'<<8)|'N')
#define TAG_COMPOSER (('S'<<8)|'W')

static unsigned char smafbuf[1024];

static bool read_datachunk(unsigned char *src, int size, unsigned short tag,
                           int codepage, unsigned char *dst)
{
    int datasize = 0;
    unsigned char *utf8;

    while(size > datasize + 4)
    {
        datasize = (src[2] << 8) | src[3];
        if (tag == ((src[0] << 8) | src[1]))
        {
            src += 4;
            if (codepage < NUM_CODEPAGES)
                utf8 = iso_decode(src, dst, codepage, datasize);
            else /* codepage == UTF_16, UCS_2 */
                utf8 = utf16BEdecode(src, dst, datasize);
            *utf8 = '\0';

            return true;
        }
        src += (datasize + 4);
    }
    return false;
}

static bool read_option(unsigned char *src, int size, unsigned short tag,
                        int codepage, unsigned char *dst)
{
    int datasize = 0;
    unsigned char *endsrc = src + size;
    unsigned char *utf8;

    while(src < endsrc)
    {
        utf8 = src;
        src += 3;
        datasize = 0;
        while (*src != ',' || *(src-1) == '\\')
        {
            datasize++;
            src++;
        }
        if (tag == ((utf8[0] << 8) | utf8[1]) && utf8[2] == ':')
        {
            utf8 += 3;
            if (codepage < NUM_CODEPAGES)
                utf8 = iso_decode(utf8, dst, codepage, datasize);
            else /* codepage == UTF_16, UCS_2 */
                utf8 = utf16BEdecode(utf8, dst, datasize);
            *utf8 = '\0';

            return true;
        }
        src++;
    }
    return false;
}

static int convert_smaf_audio_frequency(unsigned int freq)
{
    if (freq > 4)
        return 0;
    return frequency[freq];
}

static int convert_smaf_audio_basebit(unsigned int basebit)
{
    if (basebit > 4)
        return 0;
    return basebits[basebit];
}

static int convert_smaf_codetype(unsigned int codetype)
{
    if (codetype < 7)
        return support_codepages[codetype];
    else if (codetype < 0x20)
        return -1;
    else if (codetype == 0x20)
        return UCS_2;
    else if (codetype == 0x23)
        return UTF_8;
    else if (codetype == 0x24)
        return UTF_16;
    else if (codetype == 0xff)
        return ISO_8859_1;
    return -1;
}

static bool get_smaf_metadata_audio_track(struct mp3entry *id3,
                                          unsigned char* buf, unsigned char *endbuf)
{
    int bitspersample;
    int channels;
    int chunksize;
    long numbytes;
    unsigned long totalsamples;

    channels = ((buf[10] & 0x80) >> 7) + 1;
    bitspersample = convert_smaf_audio_basebit(buf[11] >> 4);
    if (bitspersample == 0)
    {
        DEBUGF("metada error: smaf unsupport basebit %d\n", buf[11] >> 4);
        return false;
    }
    id3->frequency = convert_smaf_audio_frequency(buf[10] & 0x0f);

    buf += 14;
    while (buf < endbuf)
    {
        chunksize = get_long_be(buf + 4) + 8;
        if (memcmp(buf, "Awa", 3) == 0)
        {
            numbytes = get_long_be(buf + 4) - 3;
            totalsamples = (numbytes << 3) / (bitspersample * channels);

            /* Calculate track length (in ms) and estimate the bitrate (in kbit/s) */
            id3->length = ((int64_t)totalsamples * 1000LL) / id3->frequency;

            return true;
        }
        buf += chunksize;
    }
    DEBUGF("metada error: smaf does not include pcm audio data\n");
    return false;
}

static bool get_smaf_metadata_score_track(struct mp3entry *id3,
                                          unsigned char* buf, unsigned char *endbuf)
{
    int bitspersample;
    int channels;
    int chunksize;
    long numbytes;
    unsigned long totalsamples;

    /*
     * skip to the next chunk.
     *    MA-2/MA-3/MA-5: padding 16 bytes
     *    MA-7:           padding 32 bytes
     */
    if (buf[3] < 7)
        buf += 28;
    else
        buf += 44;

    while (buf + 10 < endbuf)
    {
        chunksize = get_long_be(buf + 4) + 8;
        if (memcmp(buf, "Mtsp", 4) == 0)
        {
            buf += 8;
            if (memcmp(buf, "Mwa", 3) != 0)
            {
                DEBUGF("metada error: smaf unsupport format: %c%c%c%c\n",
                       buf[0], buf[1], buf[2], buf[3]);
                return false;
            }

            channels = ((buf[8] & 0x80) >> 7) + 1;
            bitspersample = convert_smaf_audio_basebit(buf[8] & 0x0f);
            if (bitspersample == 0)
            {
                DEBUGF("metada error: smaf unsupport basebit %d\n", buf[8] & 0x0f);
                return false;
            }

            numbytes = get_long_be(buf + 4) - 3;
            totalsamples = numbytes * 8 / (bitspersample * channels);

            id3->frequency = (buf[9] << 8) | buf[10];

            /* Calculate track length (in ms) and estimate the bitrate (in kbit/s) */
            id3->length = ((int64_t) totalsamples * 1000) / id3->frequency;

            return true;
        }
        buf += chunksize;
    }
    DEBUGF("metada error: smaf does not include pcm audio data\n");
    return false;
}

bool get_smaf_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    unsigned char *endbuf = smafbuf + 1024;
    int i;
    int contents_size;
    int codepage = ISO_8859_1;

    id3->title = NULL;
    id3->artist = NULL;
    id3->composer = NULL;

    id3->vbr = false;   /* All SMAF files are CBR */
    id3->filesize = filesize(fd);

    /* get RIFF chunk header */
    if ((lseek(fd, 0, SEEK_SET) < 0) || (read(fd, buf, 21) < 21))
    {
        return false;
    }

    if ((memcmp(buf, "MMMD", 4) != 0) || (memcmp(&buf[8], "CNTI", 4) != 0))
    {
        DEBUGF("metada error: does not smaf format\n");
        return false;
    }

    contents_size = get_long_be(buf + 12);
    if (contents_size < 5)
    {
        DEBUGF("metada error: CNTI chunk size is small %d\n", contents_size);
        return false;
    }

    contents_size -= 5;
    i = contents_size;
    if (i == 0)
    {
        read(fd, buf, 16);
        if (memcmp(buf, "OPDA", 4) != 0)
        {
            DEBUGF("metada error: smaf does not include OPDA chunk\n");
            return false;
        }
        contents_size = get_long_be(buf + 4) - 8;

        if (memcmp(buf + 8, "Dch", 3) != 0)
        {
            DEBUGF("metada error: smaf does not include Dch chunk\n");
            return false;
        }
        codepage = convert_smaf_codetype(buf[11]);
        if (codepage < 0)
        {
            DEBUGF("metada error: smaf unsupport codetype: %d\n", buf[11]);
            return false;
        }

        i = get_long_be(buf + 12);

        if (i > MAX_PATH)
        {
            DEBUGF("metada warning: smaf contents size is big %d\n", i);
            i = MAX_PATH;
        }
        if (read(fd, buf, i) < i)
            return false;

        /* title */
        if (read_datachunk(buf, i, TAG_TITLE, codepage, id3->id3v1buf[0]))
            id3->title = id3->id3v1buf[0];

        /* artist */
        if (read_datachunk(buf, i, TAG_ARTIST, codepage, id3->id3v1buf[1]))
            id3->artist = id3->id3v1buf[1];

        /* composer */
        if (read_datachunk(buf, i, TAG_COMPOSER, codepage, id3->id3v1buf[2]))
            id3->composer = id3->id3v1buf[2];
    }
    else
    {
        codepage = convert_smaf_codetype(buf[14]);
        if (codepage < 0)
        {
            DEBUGF("metada error: smaf unsupport codetype: %d\n", buf[11]);
            return false;
        }

        if (i > MAX_PATH)
        {
            DEBUGF("metada warning: smaf contents size is big %d\n", i);
            i = MAX_PATH;
        }
        if (read(fd, buf, i) < i)
            return false;

        /* title */
        if (read_option(buf, i, TAG_TITLE, codepage, id3->id3v1buf[0]))
            id3->title = id3->id3v1buf[0];

        /* artist */
        if (read_option(buf, i, TAG_ARTIST, codepage, id3->id3v1buf[1]))
            id3->artist = id3->id3v1buf[1];

        /* composer */
        if (read_option(buf, i, TAG_COMPOSER, codepage, id3->id3v1buf[2]))
            id3->composer = id3->id3v1buf[2];
    }

    if (contents_size > i)
        lseek(fd, contents_size - i, SEEK_CUR);

    /* assume the SMAF pcm data position is less than 1024 bytes */
    if (read(fd, smafbuf, 1024) < 1024)
        return false;

    buf = smafbuf;
    while (buf + 8 < endbuf)
    {
        i = get_long_be(buf + 4) + 8;

        if (memcmp(buf, "ATR", 3) == 0)
            return get_smaf_metadata_audio_track(id3, buf, endbuf);
        else if (memcmp(buf, "MTR", 3) == 0)
            return get_smaf_metadata_score_track(id3, buf, endbuf);

        buf += i;
    }

    DEBUGF("metada error: smaf does not include track chunk\n");
    return false;
}
