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
#include <string.h>
#include <inttypes.h>

#include "system.h"
#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "rbunicode.h"
#include "logf.h"

static const int basebits[4] = { 4, 8, 12, 16 };

static const int frequency[5] = { 4000, 8000, 11025, 22050, 44100 };

static const int support_codepages[5] = {
#ifdef HAVE_LCD_BITMAP
    SJIS, ISO_8859_1, -1, GB_2312, BIG_5,
#else
    -1, ISO_8859_1, -1, -1, -1,
#endif
};

/* extra codepage */
#define UCS2  (NUM_CODEPAGES + 1)

/* support id3 tag */
#define TAG_TITLE    (('S'<<8)|'T')
#define TAG_ARTIST   (('A'<<8)|'N')
#define TAG_COMPOSER (('S'<<8)|'W')

/* convert functions */
#define CONVERT_SMAF_CHANNELS(c) (((c) >> 7) + 1)


static inline int convert_smaf_audio_basebit(unsigned int basebit)
{
    if (basebit > 3)
        return 0;
    return basebits[basebit];
}

static inline int convert_smaf_audio_frequency(unsigned int freq)
{
    if (freq > 4)
        return 0;
    return frequency[freq];
}

static int convert_smaf_codetype(unsigned int codetype)
{
    if (codetype < 5)
        return support_codepages[codetype];
    else if (codetype == 0x20 || codetype == 0x24) /* In Rockbox, UCS2 and UTF-16 are same. */
        return UCS2;
    else if (codetype == 0x23)
        return UTF_8;
    else if (codetype == 0xff)
        return ISO_8859_1;
    return -1;
}

static void set_length(struct mp3entry *id3, unsigned int ch, unsigned int basebit,
                       unsigned int numbytes)
{
    int bitspersample = convert_smaf_audio_basebit(basebit);

    if (bitspersample != 0 && id3->frequency != 0)
    {
        /* Calculate track length [ms] and bitrate [kbit/s] */
        id3->length  = (uint64_t)numbytes * 8000LL
                       / (bitspersample * CONVERT_SMAF_CHANNELS(ch) * id3->frequency);
        id3->bitrate = bitspersample * id3->frequency / 1000;
    }

    /* output contents/wave data/id3 info (for debug) */
    DEBUGF("contents info ----\n");
    DEBUGF("  TITLE:         %s\n", (id3->title)? id3->title : "(NULL)");
    DEBUGF("  ARTIST:        %s\n", (id3->artist)? id3->artist : "(NULL)");
    DEBUGF("  COMPOSER:      %s\n", (id3->composer)? id3->composer : "(NULL)");
    DEBUGF("wave data info ----\n");
    DEBUGF("  channels:      %u\n", CONVERT_SMAF_CHANNELS(ch));
    DEBUGF("  bitspersample: %d\n", bitspersample);
    DEBUGF("  numbytes;      %u\n", numbytes);
    DEBUGF("id3 info ----\n");
    DEBUGF("  frquency:      %u\n", (unsigned int)id3->frequency);
    DEBUGF("  bitrate:       %d\n", id3->bitrate);
    DEBUGF("  length:        %u\n", (unsigned int)id3->length);
}

/* contents parse functions */

/* Note: 
 *  1) When the codepage is UTF-8 or UCS2, contents data do not start BOM.
 *  2) The byte order of contents data is big endian.
 */

static void decode2utf8(const unsigned char *src, unsigned char **dst,
                        int srcsize, int *dstsize, int codepage)
{
    unsigned char tmpbuf[srcsize * 3 + 1];
    unsigned char *p;
    int utf8size;

    if (codepage < NUM_CODEPAGES)
        p = iso_decode(src, tmpbuf, codepage, srcsize);
    else /* codepage == UCS2 */
        p = utf16BEdecode(src, tmpbuf, srcsize);

    *p = '\0';

    strlcpy(*dst, tmpbuf, *dstsize);
    utf8size = (p - tmpbuf) + 1;
    if (utf8size > *dstsize)
    {
        DEBUGF("metadata warning: data length: %d > contents store buffer size: %d\n",
                    utf8size, *dstsize);
        utf8size = *dstsize;
    }
    *dst     += utf8size;
    *dstsize -= utf8size;
}

static int read_audio_track_contets(int fd, int codepage, unsigned char **dst,
                                    int *dstsize)
{
    /* value length <= 256 bytes */
    unsigned char buf[256];
    unsigned char *p = buf;
    unsigned char *q = buf;
    int datasize;

    read(fd, buf, 256);

    while (p - buf < 256 && *p != ',')
    {
        /* skip yen mark */
        if (codepage != UCS2)
        {
            if (*p == '\\')
                p++;
        }
        else if (*p == '\0' && *(p+1) == '\\')
            p += 2;

        if (*p > 0x7f)
        {
            if (codepage == UTF_8)
            {
                while ((*p & MASK) != COMP)
                    *q++ = *p++;
            }
#ifdef HAVE_LCD_BITMAP
            else if (codepage == SJIS)
            {
                if (*p <= 0xa0 || *p >= 0xe0)
                    *q++ = *p++;
            }
#endif
        }

        *q++ = *p++;
        if (codepage == UCS2)
            *q++ = *p++;
    }
    datasize =  p - buf + 1;
    lseek(fd, datasize - 256, SEEK_CUR);

    if (dst != NULL)
        decode2utf8(buf, dst, q - buf, dstsize, codepage);

    return datasize;
}

static void read_score_track_contets(int fd, int codepage, int datasize,
                                     unsigned char **dst, int *dstsize)
{
    unsigned char buf[datasize];

    read(fd, buf, datasize);
    decode2utf8(buf, dst, datasize, dstsize, codepage);
}

/* traverse chunk functions */

static unsigned int search_chunk(int fd, const unsigned char *name, int nlen)
{
    unsigned char buf[8];
    unsigned int chunksize;

    while (read(fd, buf, 8) > 0)
    {
        chunksize = get_long_be(buf + 4);
        if (memcmp(buf, name, nlen) == 0)
            return chunksize;

        lseek(fd, chunksize, SEEK_CUR);
    }
    DEBUGF("metadata error: missing '%s' chunk\n", name);
    return 0;
}

static bool parse_smaf_audio_track(int fd, struct mp3entry *id3, unsigned int datasize)
{
    /* temporary buffer */
    unsigned char *tmp = (unsigned char*)id3->path;
    /* contents stored buffer */
    unsigned char *buf = id3->id3v2buf;
    int bufsize = sizeof(id3->id3v2buf);

    unsigned int chunksize = datasize;
    int valsize;

    int codepage;

    /* parse contents info */
    read(fd, tmp, 5);
    codepage = convert_smaf_codetype(tmp[2]);
    if (codepage < 0)
    {
        DEBUGF("metadata error: smaf unsupport codetype: %d\n", tmp[2]);
        return false;
    }

    datasize -= 5;
    while ((id3->title == NULL || id3->artist == NULL || id3->composer == NULL)
           && (datasize > 0 && bufsize > 0))
    {
        if (read(fd, tmp, 3) <= 0)
            return false;

        if (tmp[2] != ':')
        {
            DEBUGF("metadata error: illegal tag: %c%c%c\n", tmp[0], tmp[1], tmp[2]);
            return false;
        }
        switch ((tmp[0]<<8)|tmp[1])
        {
            case TAG_TITLE:
                id3->title = buf;
                valsize = read_audio_track_contets(fd, codepage, &buf, &bufsize);
                break;
            case TAG_ARTIST:
                id3->artist = buf;
                valsize = read_audio_track_contets(fd, codepage, &buf, &bufsize);
                break;
            case TAG_COMPOSER:
                id3->composer = buf;
                valsize = read_audio_track_contets(fd, codepage, &buf, &bufsize);
                break;
            default:
                valsize = read_audio_track_contets(fd, codepage, NULL, &bufsize);
                break;
        }
        datasize -= (valsize + 3);
    }

    /* search PCM Audio Track Chunk */
    lseek(fd, 16 + chunksize, SEEK_SET);

    chunksize = search_chunk(fd, "ATR", 3);
    if (chunksize == 0)
    {
        DEBUGF("metadata error: missing PCM Audio Track Chunk\n");
        return false;
    }

    /*
     * get format
     *  tmp
     *    +0: Format Type
     *    +1: Sequence Type
     *    +2: bit 7 0:mono/1:stereo, bit 4-6 format, bit 0-3: frequency
     *    +3: bit 4-7: base bit
     *    +4: TimeBase_D
     *    +5: TimeBase_G
     *
     * Note: If PCM Audio Track does not include Sequence Data Chunk,
     *       tmp+6 is the start position of Wave Data Chunk.
     */
    read(fd, tmp, 6);

    /* search Wave Data Chunk */
    chunksize = search_chunk(fd, "Awa", 3);
    if (chunksize == 0)
    {
        DEBUGF("metadata error: missing Wave Data Chunk\n");
        return false;
    }

    /* set track length and bitrate */
    id3->frequency = convert_smaf_audio_frequency(tmp[2] & 0x0f);
    set_length(id3, tmp[2], tmp[3] >> 4, chunksize);
    return true;
}

static bool parse_smaf_score_track(int fd, struct mp3entry *id3)
{
    /* temporary buffer */
    unsigned char *tmp = (unsigned char*)id3->path;
    unsigned char *p = tmp;
    /* contents stored buffer */
    unsigned char *buf = id3->id3v2buf;
    int bufsize = sizeof(id3->id3v2buf);

    unsigned int chunksize;
    unsigned int datasize;
    int valsize;

    int codepage;

    /* parse Optional Data Chunk */
    read(fd, tmp, 21);
    if (memcmp(tmp + 5, "OPDA", 4) != 0)
    {
        DEBUGF("metadata error: missing Optional Data Chunk\n");
        return false;
    }

    /* Optional Data Chunk size */
    chunksize = get_long_be(tmp + 9);

    /* parse Data Chunk */
    if (memcmp(tmp + 13, "Dch", 3) != 0)
    {
        DEBUGF("metadata error: missing Data Chunk\n");
        return false;
    }

    codepage = convert_smaf_codetype(tmp[16]);
    if (codepage < 0)
    {
        DEBUGF("metadata error: smaf unsupport codetype: %d\n", tmp[16]);
        return false;
    }

    /* Data Chunk size */
    datasize = get_long_be(tmp + 17);
    while ((id3->title == NULL || id3->artist == NULL || id3->composer == NULL)
           && (datasize > 0 && bufsize > 0))
    {
        if (read(fd, tmp, 4) <= 0)
            return false;

        valsize = (tmp[2] << 8) | tmp[3];
        datasize -= (valsize + 4);
        switch ((tmp[0]<<8)|tmp[1])
        {
            case TAG_TITLE:
                id3->title = buf;
                read_score_track_contets(fd, codepage, valsize, &buf, &bufsize);
                break;
            case TAG_ARTIST:
                id3->artist = buf;
                read_score_track_contets(fd, codepage, valsize, &buf, &bufsize);
                break;
            case TAG_COMPOSER:
                id3->composer = buf;
                read_score_track_contets(fd, codepage, valsize, &buf, &bufsize);
                break;
            default:
                lseek(fd, valsize, SEEK_CUR);
                break;
        }
    }

    /* search Score Track Chunk */
    lseek(fd, 29 + chunksize, SEEK_SET);

    if (search_chunk(fd, "MTR", 3) == 0)
    {
        DEBUGF("metadata error: missing Score Track Chunk\n");
        return false;
    }

    /*
     * search next chunk
     * usually, next chunk ('M***') found within 40 bytes.
     */
    chunksize = 40;
    read(fd, tmp, chunksize);

    tmp[chunksize] = 'M'; /* stopper */
    while (*p != 'M')
        p++;

    chunksize -= (p - tmp);
    if (chunksize == 0)
    {
        DEBUGF("metadata error: missing Score Track Stream PCM Data Chunk");
        return false;
    }

    /* search Score Track Stream PCM Data Chunk */
    lseek(fd, -chunksize, SEEK_CUR);
    if (search_chunk(fd, "Mtsp", 4) == 0)
    {
        DEBUGF("metadata error: missing Score Track Stream PCM Data Chunk\n");
        return false;
    }

    /*
     * parse Score Track Stream Wave Data Chunk
     *  tmp
     *    +4-7: chunk size (WaveType(3bytes) + wave data count)
     *    +8:   bit 7 0:mono/1:stereo, bit 4-6 format, bit 0-3: base bit
     *    +9:   frequency (MSB)
     *    +10:  frequency (LSB)
     */
    read(fd, tmp, 11);
    if (memcmp(tmp, "Mwa", 3) != 0)
    {
        DEBUGF("metadata error: missing Score Track Stream Wave Data Chunk\n");
        return false;
    }

    /* set track length and bitrate */
    id3->frequency = (tmp[9] << 8) | tmp[10];
    set_length(id3, tmp[8], tmp[8] & 0x0f, get_long_be(tmp + 4) - 3);
    return true;
}

bool get_smaf_metadata(int fd, struct mp3entry* id3)
{
    /* temporary buffer */
    unsigned char *tmp = (unsigned char *)id3->path;
    unsigned int chunksize;

    id3->title    = NULL;
    id3->artist   = NULL;
    id3->composer = NULL;

    id3->vbr      = false;   /* All SMAF files are CBR */
    id3->filesize = filesize(fd);

    /* check File Chunk and Contents Info Chunk */
    lseek(fd, 0, SEEK_SET);
    read(fd, tmp, 16);
    if ((memcmp(tmp, "MMMD", 4) != 0) || (memcmp(tmp + 8, "CNTI", 4) != 0))
    {
        DEBUGF("metadata error: does not smaf format\n");
        return false;
    }

    chunksize = get_long_be(tmp + 12);
    if (chunksize > 5)
        return parse_smaf_audio_track(fd, id3, chunksize);

    return parse_smaf_score_track(fd, id3);
}
