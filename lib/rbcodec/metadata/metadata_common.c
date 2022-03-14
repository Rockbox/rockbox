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
#include "string-extra.h"
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

#include "platform.h"
#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "replaygain.h"

/* Read a string from the file. Read up to size bytes, or, if eos != -1,
 * until the eos character is found (eos is not stored in buf, unless it is
 * nil). Writes up to buf_size chars to buf, always terminating with a nil.
 * Returns number of chars read or -1 on read error.
 */
long read_string(int fd, char* buf, long buf_size, int eos, long size)
{
    long read_bytes = 0;
    char c;

    while (size != 0)
    {
        if (read(fd, &c, 1) != 1)
        {
            read_bytes = -1;
            break;
        }

        read_bytes++;
        size--;

        if ((eos != -1) && (eos == (unsigned char) c))
        {
            break;
        }

        if (buf_size > 1)
        {
            *buf++ = c;
            buf_size--;
        }
    }

    *buf = 0;
    return read_bytes;
}
/* Read an unsigned 8-bit integer from a file. */
int read_uint8(int fd, uint8_t* buf)
{
  size_t n;

  n = read(fd, (char*) buf, 1);
  return n;
}

#ifdef ROCKBOX_LITTLE_ENDIAN
/* Read an unsigned 16-bit integer from a big-endian file. */
int read_uint16be(int fd, uint16_t* buf)
{
  size_t n;

  n = read(fd, (char*) buf, 2);
  *buf = betoh16(*buf);
  return n;
}
/* Read an unsigned 32-bit integer from a big-endian file. */
int read_uint32be(int fd, uint32_t* buf)
{
  size_t n;

  n = read(fd, (char*) buf, 4);
  *buf = betoh32(*buf);
  return n;
}
/* Read an unsigned 64-bit integer from a big-endian file. */
int read_uint64be(int fd, uint64_t* buf)
{
  size_t n;
  uint8_t data[8];
  int i;

  n = read(fd, data, 8);

  for (i=0, *buf=0; i<=7; i++) {
       *buf <<= 8;
       *buf |= data[i];
  }
  return n;
}
#else
/* Read unsigned integers from a little-endian file. */
int read_uint16le(int fd, uint16_t* buf)
{
  size_t n;

  n = read(fd, (char*) buf, 2);
  *buf = letoh16(*buf);
  return n;
}
int read_uint32le(int fd, uint32_t* buf)
{
  size_t n;

  n = read(fd, (char*) buf, 4);
  *buf = letoh32(*buf);
  return n;
}
int read_uint64le(int fd, uint64_t* buf)
{
  size_t n;
  uint8_t data[8];
  int i;

  n = read(fd, data, 8);

  for (i=7, *buf=0; i>=0; i--) {
       *buf <<= 8;
       *buf |= data[i];
  }

  return n;
}
#endif

/* Read an unaligned 64-bit little endian unsigned integer from buffer. */
uint64_t get_uint64_le(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return ((uint64_t)p[0]) | ((uint64_t)p[1] << 8)  | ((uint64_t)p[2] << 16) | ((uint64_t)p[3] << 24) | ((uint64_t)p[4] << 32) |
                  ((uint64_t)p[5] << 40) | ((uint64_t)p[6] << 48) | ((uint64_t)p[7] << 56);
}

/* Read an unaligned 32-bit little endian long from buffer. */
uint32_t get_long_le(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

/* Read an unaligned 16-bit little endian short from buffer. */
uint16_t get_short_le(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return p[0] | (p[1] << 8);
}

/* Read an unaligned 32-bit big endian long from buffer. */
uint32_t get_long_be(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

/* Read an unaligned 16-bit little endian short from buffer. */
uint16_t get_short_be(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return (p[0] << 8) | p[1];
}

/* Read an unaligned 32-bit little endian long from buffer. */
int32_t get_slong(void* buf)
{
    unsigned char* p = (unsigned char*) buf;

    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

uint32_t get_itunes_int32(char* value, int count)
{
    static const char hexdigits[] = "0123456789ABCDEF";
    const char* c;
    int r = 0;

    while (count-- > 0)
    {
        while (isspace(*value))
        {
            value++;
        }

        while (*value && !isspace(*value))
        {
            value++;
        }
    }

    while (isspace(*value))
    {
        value++;
    }

    while (*value && ((c = strchr(hexdigits, toupper(*value))) != NULL))
    {
        r = (r << 4) | (c - hexdigits);
        value++;
    }

    return r;
}

/* Skip an ID3v2 tag if it can be found. We assume the tag is located at the
 * start of the file, which should be true in all cases where we need to skip it.
 * Returns true if successfully skipped or not skipped, and false if
 * something went wrong while skipping.
 */
bool skip_id3v2(int fd, struct mp3entry *id3)
{
    #define ID3TAGSZ 4
    char buf[ID3TAGSZ];
    bool success = (read(fd, buf, ID3TAGSZ) == ID3TAGSZ);
    if (success && memcmp(buf, "ID3", 3) == 0)
    {
        /* We have found an ID3v2 tag at the start of the file - find its
           length and then skip it. */
        if ((id3->first_frame_offset = getid3v2len(fd)) == 0)
            success = false;

        if (success && (lseek(fd, id3->first_frame_offset, SEEK_SET) < 0))
            success = false;
    } else {
        lseek(fd, 0, SEEK_SET);
        id3->first_frame_offset = 0;
    }
    return success;
}

#if !defined(ROCKBOX) || defined(WARBLE) /*codecs can be built without rockbox */
/* returns match index from option list
 * returns -1 if option was not found
 * option list is array of char pointers with the final item set to null
 * ex - const char *option[] = { "op_a", "op_b", "op_c", NULL}
 */
int string_option(const char *option, const char *const oplist[], bool ignore_case)
{
    const char *op;
    int (*cmp_fn)(const char*, const char*) = &strcasecmp;
    if (!ignore_case)
        cmp_fn = strcmp;
    for (int i=0; (op=oplist[i]) != NULL; i++)
    {
        if (cmp_fn(op, option) == 0)
            return i;
    }
    return -1;
}
#endif
/* Parse the tag (the name-value pair) and fill id3 and buffer accordingly.
 * String values to keep are written to buf. Returns number of bytes written
 * to buf (including end nil).
 */
long parse_tag(const char* name, char* value, struct mp3entry* id3,
    char* buf, long buf_remaining, enum tagtype type)
{
    long len = 0;
    char** p;

    enum
    {
        eTRACK = 0, eTRACKNUMBER, eDISCNUMBER, eDISC,
        eYEAR, eDATE, eTITLE, eARTIST, eALBUM, eGENRE,
        eCOMPOSER, eCOMMENT, eALBUMARTIST, eALBUM_ARTIST,
        eENSEMBLE, eGROUPING, eCONTENTGROUP, eCONTENT_GROUP,
        eMUSICBRAINZ1, eMUSICBRAINZ2, e_COUNT_TAG_COUNT
    };

    static const char *tagops[e_COUNT_TAG_COUNT + 1] =
    { [eTRACK] = "track", [eTRACKNUMBER] = "tracknumber",
      [eDISCNUMBER] = "discnumber", [eDISC] = "disc",
      [eYEAR] = "year", [eDATE] = "date", [eTITLE] = "title",
      [eARTIST] = "artist", [eALBUM] = "album", [eGENRE] = "genre",
      [eCOMPOSER] = "composer", [eCOMMENT] = "comment",
      [eALBUMARTIST] = "albumartist", [eALBUM_ARTIST] ="album artist",
      [eENSEMBLE] = "ensemble", [eGROUPING] = "grouping",
      [eCONTENTGROUP] = "contentgroup", [eCONTENT_GROUP] = "content group",
      [eMUSICBRAINZ1] = "musicbrainz_trackid",
      [eMUSICBRAINZ2] = "http://musicbrainz.org",
      [e_COUNT_TAG_COUNT] = NULL
    };

    int item = string_option(name, tagops, true);

    if (((item == eTRACK && (type == TAGTYPE_APE)))
        || (item == eTRACKNUMBER && (type == TAGTYPE_VORBIS)))
    {
        id3->tracknum = atoi(value);
        p = &(id3->track_string);
    }
    else if (item == eDISCNUMBER || item == eDISC)
    {
        id3->discnum = atoi(value);
        p = &(id3->disc_string);
    }
    else if ((item == eYEAR && (type == TAGTYPE_APE))
        || (item == eDATE && (type == TAGTYPE_VORBIS)))
    {
        /* Date's can be in any format in Vorbis. However most of them
         * are in ISO8601 format so if we try and parse the first part
         * of the tag as a number, we should get the year. If we get crap,
         * then act like we never parsed it.
         */
        id3->year = atoi(value);
        if (id3->year < 1900)
        { /* yeah, not likely */
            id3->year = 0;
        }
        p = &(id3->year_string);
    }
    else if (item == eTITLE)
    {
        p = &(id3->title);
    }
    else if (item == eARTIST)
    {
        p = &(id3->artist);
    }
    else if (item == eALBUM)
    {
        p = &(id3->album);
    }
    else if (item == eGENRE)
    {
        p = &(id3->genre_string);
    }
    else if (item == eCOMPOSER)
    {
        p = &(id3->composer);
    }
    else if (item == eCOMMENT)
    {
        p = &(id3->comment);
    }
    else if (item == eALBUMARTIST || item == eALBUM_ARTIST || item == eENSEMBLE)
    {
        p = &(id3->albumartist);
    }
    else if (item == eGROUPING || item == eCONTENTGROUP || item == eCONTENT_GROUP)
    {
        p = &(id3->grouping);
    }
    else if (item == eMUSICBRAINZ1 || item == eMUSICBRAINZ2)
    {
        p = &(id3->mb_track_id);
    }
    else
    {
        parse_replaygain(name, value, id3);
        p = NULL;
    }

    /* Do not overwrite already available metadata. Especially when reading
     * tags with e.g. multiple genres / artists. This way only the first
     * of multiple entries is used, all following are dropped. */
    if (p!=NULL && *p==NULL)
    {
        len = strlen(value);
        len = MIN(len, buf_remaining - 1);
        len = MIN(len, ID3V2_MAX_ITEM_SIZE); /* Limit max. item size. */

        if (len > 0)
        {
            len++;
            strlcpy(buf, value, len);
            *p = buf;
        }
        else
        {
            len = 0;
        }
    }

    return len;
}
