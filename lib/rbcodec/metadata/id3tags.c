/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
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
/*
 * Parts of this code has been stolen from the Ample project and was written
 * by David H�deman. It has since been extended and enhanced pretty much by
 * all sorts of friendly Rockbox people.
 *
 */

 /* tagResolver and associated code copyright 2003 Thomas Paul Diffenbach
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <ctype.h>
#include "platform.h"
#include "string-extra.h"
#include "logf.h"
#include "replaygain.h"
#include "rbunicode.h"

#include "metadata.h"
#include "mp3data.h"
#if CONFIG_CODEC == SWCODEC
#include "metadata_common.h"
#endif
#include "metadata_parsers.h"
#include "misc.h"

static unsigned long unsync(unsigned long b0,
                            unsigned long b1,
                            unsigned long b2,
                            unsigned long b3)
{
   return (((long)(b0 & 0x7F) << (3*7)) |
           ((long)(b1 & 0x7F) << (2*7)) |
           ((long)(b2 & 0x7F) << (1*7)) |
           ((long)(b3 & 0x7F) << (0*7)));
}

static const char* const genres[] = {
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge",
    "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B",
    "Rap", "Reggae", "Rock", "Techno", "Industrial", "Alternative", "Ska",
    "Death Metal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
    "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical", "Instrumental",
    "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise", "AlternRock",
    "Bass", "Soul", "Punk", "Space", "Meditative", "Instrumental Pop",
    "Instrumental Rock", "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
    "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy",
    "Cult", "Gangsta", "Top 40", "Christian Rap", "Pop/Funk", "Jungle",
    "Native American", "Cabaret", "New Wave", "Psychadelic", "Rave",
    "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz",
    "Polka", "Retro", "Musical", "Rock & Roll", "Hard Rock",

    /* winamp extensions */
    "Folk", "Folk-Rock", "National Folk", "Swing", "Fast Fusion", "Bebob",
    "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock",
    "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
    "Big Band", "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech",
    "Chanson", "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass",
    "Primus", "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
    "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle",
    "Duet", "Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall",
    "Goa", "Drum & Bass", "Club-House", "Hardcore", "Terror", "Indie",
    "BritPop", "Negerpunk", "Polsk Punk", "Beat", "Christian Gangsta Rap",
    "Heavy Metal", "Black Metal", "Crossover", "Contemporary Christian",
    "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "Jpop",
    "Synthpop"
};

#if CONFIG_CODEC != SWCODEC
static
#endif
char* id3_get_num_genre(unsigned int genre_num)
{
    if (genre_num < ARRAYLEN(genres))
        return (char*)genres[genre_num];
    return NULL;
}

/*
    HOW TO ADD ADDITIONAL ID3 VERSION 2 TAGS
    Code and comments by Thomas Paul Diffenbach

    To add another ID3v2 Tag, do the following:
    1.  add a char* named for the tag to struct mp3entry in id3.h,
        (I (tpd) prefer to use char* rather than ints, even for what seems like
        numerical values, for cases where a number won't do, e.g.,
        YEAR: "circa 1765", "1790/1977" (composed/performed), "28 Feb 1969"
        TRACK: "1/12", "1 of 12", GENRE: "Freeform genre name"
        Text is more flexible, and as the main use of id3 data is to
        display it, converting it to an int just means reconverting to
        display it, at a runtime cost.)

    2. If any special processing beyond copying the tag value from the Id3
       block to the struct mp3entry is rrequired (such as converting to an
       int), write a function to perform this special processing.

       This function's prototype must match that of
       typedef tagPostProcessFunc, that is it must be:
         int func( struct mp3entry*, char* tag, int bufferpos )
       the first argument is a pointer to the current mp3entry structure the
       second argument is a pointer to the null terminated string value of the
       tag found the third argument is the offset of the next free byte in the
       mp3entry's buffer your function should return the corrected offset; if
       you don't lengthen or shorten the tag string, you can return the third
       argument unchanged.

       Unless you have a good reason no to, make the function static.
       TO JUST COPY THE TAG NO SPECIAL PROCESSING FUNCTION IS NEEDED.

    3. add one or more entries to the tagList array, using the format:
            char* ID3 Tag symbolic name -- see the ID3 specification for these,
            sizeof() that name minus 1,
            offsetof( struct mp3entry, variable_name_in_struct_mp3entry ),
            pointer to your special processing function or NULL
                if you need no special processing
            flag indicating if this tag is binary or textual
        Many ID3 symbolic names come in more than one form. You can add both
        forms, each referencing the same variable in struct mp3entry.
        If both forms are present, the last found will be used.
        Note that the offset can be zero, in which case no entry will be set
        in the mp3entry struct; the frame is still read into the buffer and
        the special processing function is called (several times, if there
        are several frames with the same name).

    4. Alternately, use the TAG_LIST_ENTRY macro with
         ID3 tag symbolic name,
         variable in struct mp3entry,
         special processing function address

    5.  Add code to wps-display.c function get_tag to assign a printf-like
        format specifier for the tag */

/* Structure for ID3 Tag extraction information */
struct tag_resolver {
    const char* tag;
    int tag_length;
    size_t offset;
    int (*ppFunc)(struct mp3entry*, char* tag, int bufferpos);
    bool binary;
};

static bool global_ff_found;

static int unsynchronize(char* tag, int len, bool *ff_found)
{
    int i;
    unsigned char c;
    unsigned char *rp, *wp;

    wp = rp = (unsigned char *)tag;

    rp = (unsigned char *)tag;
    for(i = 0;i < len;i++) {
        /* Read the next byte and write it back, but don't increment the
           write pointer */
        c = *rp++;
        *wp = c;
        if(*ff_found) {
            /* Increment the write pointer if it isn't an unsynch pattern */
            if(c != 0)
                wp++;
            *ff_found = false;
        } else {
            if(c == 0xff)
                *ff_found = true;
            wp++;
        }
    }
    return (intptr_t)wp - (intptr_t)tag;
}

static int unsynchronize_frame(char* tag, int len)
{
    bool ff_found = false;

    return unsynchronize(tag, len, &ff_found);
}

static int read_unsynched(int fd, void *buf, int len)
{
    int i;
    int rc;
    int remaining = len;
    char *wp;
    char *rp;

    wp = buf;

    while(remaining) {
        rp = wp;
        rc = read(fd, rp, remaining);
        if(rc <= 0)
            return rc;

        i = unsynchronize(wp, remaining, &global_ff_found);
        remaining -= i;
        wp += i;
    }

    return len;
}

static int skip_unsynched(int fd, int len)
{
    int rc;
    int remaining = len;
    int rlen;
    char buf[32];

    while(remaining) {
        rlen = MIN(sizeof(buf), (unsigned int)remaining);
        rc = read(fd, buf, rlen);
        if(rc <= 0)
            return rc;

        remaining -= unsynchronize(buf, rlen, &global_ff_found);
    }

    return len;
}

/* parse numeric value from string */
static int parsetracknum( struct mp3entry* entry, char* tag, int bufferpos )
{
    entry->tracknum = atoi( tag );
    return bufferpos;
}

/* parse numeric value from string */
static int parsediscnum( struct mp3entry* entry, char* tag, int bufferpos )
{
    entry->discnum = atoi( tag );
    return bufferpos;
}

/* parse numeric value from string */
static int parseyearnum( struct mp3entry* entry, char* tag, int bufferpos )
{
    entry->year = atoi( tag );
    return bufferpos;
}

/* parse numeric genre from string, version 2.2 and 2.3 */
static int parsegenre( struct mp3entry* entry, char* tag, int bufferpos )
{
    /* Use bufferpos to hold current position in entry->id3v2buf. */
    bufferpos = tag - entry->id3v2buf;

    if(entry->id3version >= ID3_VER_2_4) {
        /* In version 2.4 and up, there are no parentheses, and the genre frame
           is a list of strings, either numbers or text. */

        /* Is it a number? */
        if(isdigit(tag[0])) {
            entry->genre_string = id3_get_num_genre(atoi( tag ));
            return bufferpos;
        } else {
            entry->genre_string = tag;
            return bufferpos + strlen(tag) + 1;
        }
    } else {
        if( tag[0] == '(' && tag[1] != '(' ) {
            entry->genre_string = id3_get_num_genre(atoi( tag + 1 ));
            return bufferpos;
        }
        else {
            entry->genre_string = tag;
            return bufferpos + strlen(tag) + 1;
        }
    }
}

#ifdef HAVE_ALBUMART
/* parse embed albumart */
static int parsealbumart( struct mp3entry* entry, char* tag, int bufferpos )
{
    /* don't parse albumart if already one found. This callback function is
     * called unconditionally. */
    if(entry->has_embedded_albumart)
        return bufferpos;

    /* we currently don't support unsynchronizing albumart */
    if (entry->albumart.type == AA_TYPE_UNSYNC)
        return bufferpos;

    entry->albumart.type = AA_TYPE_UNKNOWN;

    char *start = tag;
    /* skip text encoding */
    tag += 1;

    if (memcmp(tag, "image/", 6) == 0)
    {
        /* ID3 v2.3+ */
        tag += 6;
        if (strcmp(tag, "jpeg") == 0)
        {
            entry->albumart.type = AA_TYPE_JPG;
            tag += 5;
        }
        else if (strcmp(tag, "jpg") == 0)
        {
            /* image/jpg is technically invalid, but it does occur in
             * the wild */
            entry->albumart.type = AA_TYPE_JPG;
            tag += 4;
        }
        else if (strcmp(tag, "png") == 0)
        {
            entry->albumart.type = AA_TYPE_PNG;
            tag += 4;
        }
    }
    else
    {
        /* ID3 v2.2 */
        if (memcmp(tag, "JPG", 3) == 0)
            entry->albumart.type = AA_TYPE_JPG;
        else if (memcmp(tag, "PNG", 3) == 0)
            entry->albumart.type = AA_TYPE_PNG;
        tag += 3;
    }

    if (entry->albumart.type != AA_TYPE_UNKNOWN)
    {
        /* skip picture type */
        tag += 1;
        /* skip description */
        tag = strchr(tag, '\0') + 1;
        /* fixup offset&size for image data */
        entry->albumart.pos  += tag - start;
        entry->albumart.size -= tag - start;
        /* check for malformed tag with no picture data */
        entry->has_embedded_albumart = (entry->albumart.size != 0);
    }
    /* return bufferpos as we didn't store anything in id3v2buf */
    return bufferpos;
}
#endif

/* parse user defined text, looking for album artist and replaygain 
 * information.
 */
static int parseuser( struct mp3entry* entry, char* tag, int bufferpos )
{
    char* value = NULL;
    int desc_len = strlen(tag);
    int length = 0;

    if ((tag - entry->id3v2buf + desc_len + 2) < bufferpos) {
        /* At least part of the value was read, so we can safely try to
         * parse it */
        value = tag + desc_len + 1;
        
        if (!strcasecmp(tag, "ALBUM ARTIST")) {
            length = strlen(value) + 1;
            strlcpy(tag, value, length);
            entry->albumartist = tag;
#if CONFIG_CODEC == SWCODEC
        } else {
            /* Call parse_replaygain(). */
            parse_replaygain(tag, value, entry);
#endif
        }
    }

    return tag - entry->id3v2buf + length;
}

#if CONFIG_CODEC == SWCODEC
/* parse RVA2 binary data and convert to replaygain information. */
static int parserva2( struct mp3entry* entry, char* tag, int bufferpos)
{
    int desc_len = strlen(tag);
    int start_pos = tag - entry->id3v2buf;
    int end_pos = start_pos + desc_len + 5;
    unsigned char* value = tag + desc_len + 1;

    /* Only parse RVA2 replaygain tags if tag version == 2.4 and channel
     * type is master volume.
     */
    if (entry->id3version == ID3_VER_2_4 && end_pos < bufferpos 
            && *value++ == 1) {
        long gain = 0;
        long peak = 0;
        long peakbits;
        long peakbytes;
        bool album = false;

        /* The RVA2 specification is unclear on some things (id string and
         * peak volume), but this matches how Quod Libet use them.
         */
            
        gain = (int16_t) ((value[0] << 8) | value[1]);
        value += 2;
        peakbits = *value++;
        peakbytes = (peakbits + 7) / 8;
    
        /* Only use the topmost 24 bits for peak volume */
        if (peakbytes > 3) {
            peakbytes = 3;
        }

        /* Make sure the peak bits were read */
        if (end_pos + peakbytes < bufferpos) {
            long shift = ((8 - (peakbits & 7)) & 7) + (3 - peakbytes) * 8;

            for ( ; peakbytes; peakbytes--) {
                peak <<= 8;
                peak += *value++;
            }
    
            peak <<= shift;
    
            if (peakbits > 24) {
                peak += *value >> (8 - shift);
            }
        }
    
        if (strcasecmp(tag, "album") == 0) {
            album = true;
        } else if (strcasecmp(tag, "track") != 0) {
            /* Only accept non-track values if we don't have any previous
             * value.
             */
            if (entry->track_gain != 0) {
                return start_pos;
            }
        }
            
        parse_replaygain_int(album, gain, peak * 2, entry);
    }

    return start_pos;
}
#endif

static int parsembtid( struct mp3entry* entry, char* tag, int bufferpos )
{
    char* value = NULL;
    int desc_len = strlen(tag);
    /*DEBUGF("MBID len: %d\n", desc_len);*/
    /* Musicbrainz track IDs are always 36 chars long */
    const size_t mbtid_len = 36; 

    if ((tag - entry->id3v2buf + desc_len + 2) < bufferpos)
    {
        value = tag + desc_len + 1;

        if (strcasecmp(tag, "http://musicbrainz.org") == 0)
        {
            if (mbtid_len == strlen(value))
            {
                entry->mb_track_id = value;
                return bufferpos + mbtid_len + 1;
            }
        }
    }

    return bufferpos;
}

static const struct tag_resolver taglist[] = {
    { "TPE1", 4, offsetof(struct mp3entry, artist), NULL, false },
    { "TP1",  3, offsetof(struct mp3entry, artist), NULL, false },
    { "TIT2", 4, offsetof(struct mp3entry, title), NULL, false },
    { "TT2",  3, offsetof(struct mp3entry, title), NULL, false },
    { "TALB", 4, offsetof(struct mp3entry, album), NULL, false },
    { "TAL",  3, offsetof(struct mp3entry, album), NULL, false },
    { "TRK",  3, offsetof(struct mp3entry, track_string), &parsetracknum, false },
    { "TPOS", 4, offsetof(struct mp3entry, disc_string), &parsediscnum, false },
    { "TPA",  3, offsetof(struct mp3entry, disc_string), &parsediscnum, false },
    { "TRCK", 4, offsetof(struct mp3entry, track_string), &parsetracknum, false },
    { "TDRC", 4, offsetof(struct mp3entry, year_string), &parseyearnum, false },
    { "TYER", 4, offsetof(struct mp3entry, year_string), &parseyearnum, false },
    { "TYE",  3, offsetof(struct mp3entry, year_string), &parseyearnum, false },
    { "TCOM", 4, offsetof(struct mp3entry, composer), NULL, false },
    { "TCM",  3, offsetof(struct mp3entry, composer), NULL, false },
    { "TPE2", 4, offsetof(struct mp3entry, albumartist), NULL, false },
    { "TP2",  3, offsetof(struct mp3entry, albumartist), NULL, false },
    { "TIT1", 4, offsetof(struct mp3entry, grouping), NULL, false },
    { "TT1",  3, offsetof(struct mp3entry, grouping), NULL, false },
    { "COMM", 4, offsetof(struct mp3entry, comment), NULL, false }, 
    { "COM",  3, offsetof(struct mp3entry, comment), NULL, false }, 
    { "TCON", 4, offsetof(struct mp3entry, genre_string), &parsegenre, false },
    { "TCO",  3, offsetof(struct mp3entry, genre_string), &parsegenre, false },
#ifdef HAVE_ALBUMART
    { "APIC", 4, 0, &parsealbumart, true },
    { "PIC",  3, 0, &parsealbumart, true },
#endif
    { "TXXX", 4, 0, &parseuser, false },
#if CONFIG_CODEC == SWCODEC
    { "RVA2", 4, 0, &parserva2, true },
#endif
    { "UFID", 4, 0, &parsembtid, false },
};

#define TAGLIST_SIZE ((int)ARRAYLEN(taglist))

/* Get the length of an ID3 string in the given encoding. Returns the length
 * in bytes, including end nil, or -1 if the encoding is unknown.
 */
static int unicode_len(char encoding, const void* string)
{
    int len = 0;

    if (encoding == 0x01 || encoding == 0x02) {
        char first;
        const char *s = string;
        /* string might be unaligned, so using short* can crash on ARM and SH1 */
        do {
            first = *s++;
        } while ((first | *s++) != 0);

        len = s - (const char*) string;
    } else {
        len = strlen((char*) string) + 1;
    }

    return len;
}

/* Checks to see if the passed in string is a 16-bit wide Unicode v2
   string.  If it is, we convert it to a UTF-8 string.  If it's not unicode,
   we convert from the default codepage */
static int unicode_munge(char* string, char* utf8buf, int *len) {
    long tmp;
    bool le = false;
    int i = 0;
    unsigned char *str = (unsigned char *)string;
    int templen = 0;
    unsigned char* utf8 = (unsigned char *)utf8buf;

    switch (str[0]) {
        case 0x00: /* Type 0x00 is ordinary ISO 8859-1 */
            str++;
            (*len)--;
            utf8 = iso_decode(str, utf8, -1, *len);
            *utf8 = 0;
            *len = (intptr_t)utf8 - (intptr_t)utf8buf;
            break;

        case 0x01: /* Unicode with or without BOM */
        case 0x02:
            (*len)--;
            str++;

            /* Handle frames with more than one string
               (needed for TXXX frames).*/
            do {
                tmp = bytes2int(0, 0, str[0], str[1]);

                /* Now check if there is a BOM
                   (zero-width non-breaking space, 0xfeff)
                   and if it is in little or big endian format */
                if(tmp == 0xfffe) { /* Little endian? */
                    le = true;
                    str += 2;
                    (*len)-=2;
                } else if(tmp == 0xfeff) { /* Big endian? */
                    str += 2;
                    (*len)-=2;
                } else
                /* If there is no BOM (which is a specification violation),
                   let's try to guess it. If one of the bytes is 0x00, it is
                   probably the most significant one. */
                    if(str[1] == 0)
                        le = true;

                while ((i < *len) && (str[0] || str[1])) {
                    if(le)
                        utf8 = utf16LEdecode(str, utf8, 1);
                    else
                        utf8 = utf16BEdecode(str, utf8, 1);

                    str+=2;
                    i += 2;
                }

                *utf8++ = 0; /* Terminate the string */
                templen += (strlen(&utf8buf[templen]) + 1);
                str += 2;
                i+=2;
            } while(i < *len);
            *len = templen - 1;
            break;

        case 0x03: /* UTF-8 encoded string */
            for(i=0; i < *len; i++)
                utf8[i] = str[i+1];
            (*len)--;
            break;

        default: /* Plain old string */
            utf8 = iso_decode(str, utf8, -1, *len);
            *utf8 = 0;
            *len = (intptr_t)utf8 - (intptr_t)utf8buf;
            break;
    }
    return 0;
}

/*
 * Sets the title of an MP3 entry based on its ID3v1 tag.
 *
 * Arguments: file - the MP3 file to scen for a ID3v1 tag
 *            entry - the entry to set the title in
 *
 * Returns: true if a title was found and created, else false
 */
bool setid3v1title(int fd, struct mp3entry *entry)
{
    unsigned char buffer[128];
    static const char offsets[] = {3, 33, 63, 97, 93, 125, 127};
    int i, j;
    unsigned char* utf8;

    if (-1 == lseek(fd, -128, SEEK_END))
        return false;

    if (read(fd, buffer, sizeof buffer) != sizeof buffer)
        return false;

    if (strncmp((char *)buffer, "TAG", 3))
        return false;

    entry->id3v1len = 128;
    entry->id3version = ID3_VER_1_0;

    for (i=0; i < (int)sizeof offsets; i++) {
        unsigned char* ptr = (unsigned char *)buffer + offsets[i];

        switch(i) {
            case 0:
            case 1:
            case 2:
                /* kill trailing space in strings */
                for (j=29; j && (ptr[j]==0 || ptr[j]==' '); j--)
                    ptr[j] = 0;
                /* convert string to utf8 */
                utf8 = (unsigned char *)entry->id3v1buf[i];
                utf8 = iso_decode(ptr, utf8, -1, 30);
                /* make sure string is terminated */
                *utf8 = 0;
                break;

            case 3:
                /* kill trailing space in strings */
                for (j=27; j && (ptr[j]==0 || ptr[j]==' '); j--)
                    ptr[j] = 0;
                /* convert string to utf8 */
                utf8 = (unsigned char *)entry->id3v1buf[3];
                utf8 = iso_decode(ptr, utf8, -1, 28);
                /* make sure string is terminated */
                *utf8 = 0;
                break;

            case 4:
                ptr[4] = 0;
                entry->year = atoi((char *)ptr);
                break;

            case 5:
                /* id3v1.1 uses last two bytes of comment field for track
                   number: first must be 0 and second is track num */
                if (!ptr[0] && ptr[1]) {
                    entry->tracknum = ptr[1];
                    entry->id3version = ID3_VER_1_1;
                }
                break;

            case 6:
                /* genre */
                entry->genre_string = id3_get_num_genre(ptr[0]);
                break;
        }
    }

    entry->title = entry->id3v1buf[0];
    entry->artist = entry->id3v1buf[1];
    entry->album = entry->id3v1buf[2];
    entry->comment = entry->id3v1buf[3];

    return true;
}


/*
 * Sets the title of an MP3 entry based on its ID3v2 tag.
 *
 * Arguments: file - the MP3 file to scan for a ID3v2 tag
 *            entry - the entry to set the title in
 *
 * Returns: true if a title was found and created, else false
 */
void setid3v2title(int fd, struct mp3entry *entry)
{
    int minframesize;
    int size;
    long bufferpos = 0, totframelen, framelen;
    char header[10];
    char tmp[4];
    unsigned char version;
    char *buffer = entry->id3v2buf;
    int bytesread = 0;
    int buffersize = sizeof(entry->id3v2buf);
    unsigned char global_flags;
    int flags;
    bool global_unsynch = false;
    bool unsynch = false;
    int i, j;
    int rc;
#if CONFIG_CODEC == SWCODEC
    bool itunes_gapless = false;
#endif

#ifdef HAVE_ALBUMART
    entry->has_embedded_albumart = false;
#endif

    global_ff_found = false;

    /* Bail out if the tag is shorter than 10 bytes */
    if(entry->id3v2len < 10)
        return;

    /* Read the ID3 tag version from the header */
    lseek(fd, 0, SEEK_SET);
    if(10 != read(fd, header, 10))
        return;

    /* Get the total ID3 tag size */
    size = entry->id3v2len - 10;

    version = header[3];
    switch ( version ) {
        case 2:
            version = ID3_VER_2_2;
            minframesize = 8;
            break;

        case 3:
            version = ID3_VER_2_3;
            minframesize = 12;
            break;

        case 4:
            version = ID3_VER_2_4;
            minframesize = 12;
            break;

        default:
            /* unsupported id3 version */
            return;
    }
    entry->id3version = version;
    entry->tracknum = entry->year = entry->discnum = 0;
    entry->title = entry->artist = entry->album = NULL; /* FIXME incomplete */

    global_flags = header[5];

    /* Skip the extended header if it is present */
    if(global_flags & 0x40) {
        if(version == ID3_VER_2_3) {
            if(10 != read(fd, header, 10))
                return;
            /* The 2.3 extended header size doesn't include the header size
               field itself. Also, it is not unsynched. */
            framelen =
                bytes2int(header[0], header[1], header[2], header[3]) + 4;

            /* Skip the rest of the header */
            lseek(fd, framelen - 10, SEEK_CUR);
        }

        if(version >= ID3_VER_2_4) {
            if(4 != read(fd, header, 4))
                return;

            /* The 2.4 extended header size does include the entire header,
               so here we can just skip it. This header is unsynched. */
            framelen = unsync(header[0], header[1],
                              header[2], header[3]);

            lseek(fd, framelen - 4, SEEK_CUR);
        }
    }

    /* Is unsynchronization applied? */
    if(global_flags & 0x80) {
        global_unsynch = true;
    }

    /*
     * We must have at least minframesize bytes left for the
     * remaining frames to be interesting
     */
    while (size >= minframesize && bufferpos < buffersize - 1) {
        flags = 0;

        /* Read frame header and check length */
        if(version >= ID3_VER_2_3) {
            if(global_unsynch && version <= ID3_VER_2_3)
                rc = read_unsynched(fd, header, 10);
            else
                rc = read(fd, header, 10);
            if(rc != 10)
                return;
            /* Adjust for the 10 bytes we read */
            size -= 10;

            flags = bytes2int(0, 0, header[8], header[9]);

            if (version >= ID3_VER_2_4) {
                framelen = unsync(header[4], header[5],
                                  header[6], header[7]);
            } else {
                /* version .3 files don't use synchsafe ints for
                 * size */
                framelen = bytes2int(header[4], header[5],
                                     header[6], header[7]);
            }
        } else {
            if(6 != read(fd, header, 6))
                return;
            /* Adjust for the 6 bytes we read */
            size -= 6;

            framelen = bytes2int(0, header[3], header[4], header[5]);
        }

        logf("framelen = %ld, flags = 0x%04x", framelen, flags);
        if(framelen == 0){
            if (header[0] == 0 && header[1] == 0 && header[2] == 0)
                return;
            else
                continue;
        }

        unsynch = false;

        if(flags)
        {
            if (version >= ID3_VER_2_4) {
                if(flags & 0x0040) { /* Grouping identity */
                    lseek(fd, 1, SEEK_CUR); /* Skip 1 byte */
                    framelen--;
                }
            } else {
                if(flags & 0x0020) { /* Grouping identity */
                    lseek(fd, 1, SEEK_CUR); /* Skip 1 byte */
                    framelen--;
                }
            }

            if(flags & 0x000c) /* Compression or encryption */
            {
                /* Skip it */
                size -= framelen;
                lseek(fd, framelen, SEEK_CUR);
                continue;
            }

            if(flags & 0x0002) /* Unsynchronization */
                unsynch = true;

            if (version >= ID3_VER_2_4) {
                if(flags & 0x0001) { /* Data length indicator */
                    if(4 != read(fd, tmp, 4))
                        return;

                    /* We don't need the data length */
                    framelen -= 4;
                }
            }
        }
        
        if (framelen == 0)
            continue;

        if (framelen < 0)
            return;
        
        /* Keep track of the remaining frame size */
        totframelen = framelen;

        /* If the frame is larger than the remaining buffer space we try
           to read as much as would fit in the buffer */
        if(framelen >= buffersize - bufferpos)
            framelen = buffersize - bufferpos - 1;

        /* Limit the maximum length of an id3 data item to ID3V2_MAX_ITEM_SIZE
           bytes. This reduces the chance that the available buffer is filled
           by single metadata items like large comments. */
        if (ID3V2_MAX_ITEM_SIZE < framelen)
            framelen = ID3V2_MAX_ITEM_SIZE;

        logf("id3v2 frame: %.4s", header);

        /* Check for certain frame headers

           'size' is the amount of frame bytes remaining.  We decrement it by
           the amount of bytes we read.  If we fail to read as many bytes as
           we expect, we assume that we can't read from this file, and bail
           out.

           For each frame. we will iterate over the list of supported tags,
           and read the tag into entry's buffer. All tags will be kept as
           strings, for cases where a number won't do, e.g., YEAR: "circa
           1765", "1790/1977" (composed/performed), "28 Feb 1969" TRACK:
           "1/12", "1 of 12", GENRE: "Freeform genre name" Text is more
           flexible, and as the main use of id3 data is to display it,
           converting it to an int just means reconverting to display it, at a
           runtime cost.

           For tags that the current code does convert to ints, a post
           processing function will be called via a pointer to function. */

        for (i=0; i<TAGLIST_SIZE; i++) {
            const struct tag_resolver* tr = &taglist[i];
            char** ptag = tr->offset ? (char**) (((char*)entry) + tr->offset)
                : NULL;
            char* tag;

            /* Only ID3_VER_2_2 uses frames with three-character names. */
            if (((version == ID3_VER_2_2) && (tr->tag_length != 3))
                || ((version > ID3_VER_2_2) && (tr->tag_length != 4))) {
                continue;
            }

            if( !memcmp( header, tr->tag, tr->tag_length ) ) {

                /* found a tag matching one in tagList, and not yet filled */
                tag = buffer + bufferpos;

                if(global_unsynch && version <= ID3_VER_2_3)
                    bytesread = read_unsynched(fd, tag, framelen);
                else
                    bytesread = read(fd, tag, framelen);

                if( bytesread != framelen )
                    return;

                size -= bytesread;

                if(unsynch || (global_unsynch && version >= ID3_VER_2_4))
                    bytesread = unsynchronize_frame(tag, bytesread);

                /* the COMM frame has a 3 char field to hold an ISO-639-1 
                 * language string and an optional short description;
                 * remove them so unicode_munge can work correctly
                 */

                if((tr->tag_length == 4 && !memcmp( header, "COMM", 4)) ||
                   (tr->tag_length == 3 && !memcmp( header, "COM", 3))) {
                    int offset;
                    if(bytesread >= 8 && !strncmp(tag+4, "iTun", 4)) {
#if CONFIG_CODEC == SWCODEC
                        /* check for iTunes gapless information */
                        if(bytesread >= 12 && !strncmp(tag+4, "iTunSMPB", 8))
                            itunes_gapless = true;
                        else
#endif
                        /* ignore other with iTunes tags */
                        break;
                    }

                    offset = 3 + unicode_len(*tag, tag + 4);
                    if(bytesread > offset) {
                        bytesread -= offset;
                        memmove(tag + 1, tag + 1 + offset, bytesread - 1);
                    }
                }

                /* Attempt to parse Unicode string only if the tag contents
                   aren't binary */
                if(!tr->binary) {
                    /* UTF-8 could potentially be 3 times larger */
                    /* so we need to create a new buffer         */
                    char utf8buf[(3 * bytesread) + 1];

                    unicode_munge( tag, utf8buf, &bytesread );

                    if(bytesread >= buffersize - bufferpos)
                        bytesread = buffersize - bufferpos - 1;

                    if ( /* Is it an embedded cuesheet? */
                       (tr->tag_length == 4 && !memcmp(header, "TXXX", 4)) &&
                       (bytesread >= 14 && !strncmp(utf8buf, "CUESHEET", 8))
                    ) {
                        unsigned char char_enc = 0;
                        /* [enc type]+"CUESHEET\0" = 10 */
                        unsigned char cuesheet_offset = 10;
                        switch (tag[0]) {
                            case 0x00:
                                char_enc = CHAR_ENC_ISO_8859_1;
                                break;
                            case 0x01:
                                tag++;
                                if (!memcmp(tag,
                                   BOM_UTF_16_BE, BOM_UTF_16_SIZE)) {
                                    char_enc = CHAR_ENC_UTF_16_BE;
                                } else if (!memcmp(tag,
                                          BOM_UTF_16_LE, BOM_UTF_16_SIZE)) {
                                    char_enc = CHAR_ENC_UTF_16_LE;
                                }
                                /* \1 + BOM(2) + C0U0E0S0H0E0E0T000 = 21 */
                                cuesheet_offset = 21;
                                break;
                            case 0x02:
                                char_enc = CHAR_ENC_UTF_16_BE;
                                /* \2 + 0C0U0E0S0H0E0E0T00 = 19 */
                                cuesheet_offset = 19;
                                break;
                            case 0x03:
                                char_enc = CHAR_ENC_UTF_8;
                                break;
                        }
                        if (char_enc > 0) {
                            entry->has_embedded_cuesheet = true;
                            entry->embedded_cuesheet.pos = lseek(fd, 0, SEEK_CUR)
                                - framelen + cuesheet_offset;
                            entry->embedded_cuesheet.size = totframelen
                                - cuesheet_offset;
                            entry->embedded_cuesheet.encoding = char_enc;
                        }
                        break;
                    }

                    for (j = 0; j < bytesread; j++)
                        tag[j] = utf8buf[j];

                    /* remove trailing spaces */
                    while ( bytesread > 0 && isspace(tag[bytesread-1]))
                        bytesread--;
                }

                if(bytesread == 0)
                    /* Skip empty frames */
                    break;

                tag[bytesread] = 0;
                bufferpos += bytesread + 1;

#if CONFIG_CODEC == SWCODEC
                /* parse the tag if it contains iTunes gapless info */
                if (itunes_gapless)
                {
                    itunes_gapless = false;
                    entry->lead_trim = get_itunes_int32(tag, 1);
                    entry->tail_trim = get_itunes_int32(tag, 2);
                }
#endif

                /* Note that parser functions sometimes set *ptag to NULL, so
                 * the "!*ptag" check here doesn't always have the desired
                 * effect. Should the parser functions (parsegenre in
                 * particular) be updated to handle the case of being called
                 * multiple times, or should the "*ptag" check be removed?
                 */
                if (ptag && !*ptag)
                    *ptag = tag;

#ifdef HAVE_ALBUMART
                /* albumart */
                if ((!entry->has_embedded_albumart) &&
                    ((tr->tag_length == 4 && !memcmp( header, "APIC", 4)) ||
                     (tr->tag_length == 3 && !memcmp( header, "PIC" , 3))))
                {
                    if (unsynch || (global_unsynch && version <= ID3_VER_2_3))
                        entry->albumart.type = AA_TYPE_UNSYNC;
                    else
                    {
                        entry->albumart.pos = lseek(fd, 0, SEEK_CUR) - framelen;
                        entry->albumart.size = totframelen;
                        entry->albumart.type = AA_TYPE_UNKNOWN;
                    }
                }
#endif
                if( tr->ppFunc )
                    bufferpos = tr->ppFunc(entry, tag, bufferpos);
                break;
            }
        }

        if( i == TAGLIST_SIZE ) {
            /* no tag in tagList was found, or it was a repeat.
               skip it using the total size */

            if(global_unsynch && version <= ID3_VER_2_3) {
                size -= skip_unsynched(fd, totframelen);
            } else {
                size -= totframelen;
                if( lseek(fd, totframelen, SEEK_CUR) == -1 )
                    return;
            }
        } else {
            /* Seek to the next frame */
            if(framelen < totframelen) {
                if(global_unsynch && version <= ID3_VER_2_3) {
                    size -= skip_unsynched(fd, totframelen - framelen);
                }
                else {
                    lseek(fd, totframelen - framelen, SEEK_CUR);
                    size -= totframelen - framelen;
                }
            }
        }
    }
}

/*
 * Calculates the size of the ID3v1 tag if any.
 *
 * Arguments: file - the file to search for a tag.
 *
 * Returns: the size of the tag or 0 if none was found
 */
int getid3v1len(int fd)
{
    char buf[4];

    if (-1 == lseek(fd, -128, SEEK_END))
        return 0;

    if (read(fd, buf, 3) != 3)
        return 0;

    if (strncmp(buf, "TAG", 3))
        return 0;

    return 128;
}

/*
 * Calculates the size of the ID3v2 tag.
 *
 * Arguments: file - the file to search for a tag.
 *
 * Returns: the size of the tag or 0 if none was found
 */
int getid3v2len(int fd)
{
    char buf[6];
    int offset;

    /* Make sure file has a ID3 tag */
    if((-1 == lseek(fd, 0, SEEK_SET)) ||
       (read(fd, buf, 6) != 6) ||
       (strncmp(buf, "ID3", strlen("ID3")) != 0))
        offset = 0;

    /* Now check what the ID3v2 size field says */
    else
        if(read(fd, buf, 4) != 4)
            offset = 0;
        else
            offset = unsync(buf[0], buf[1], buf[2], buf[3]) + 10;

    logf("ID3V2 Length: 0x%x", offset);
    return offset;
}

#ifdef DEBUG_STANDALONE

char *secs2str(int ms)
{
    static char buffer[32];
    int secs = ms/1000;
    ms %= 1000;
    snprintf(buffer, sizeof(buffer), "%d:%02d.%d", secs/60, secs%60, ms/100);
    return buffer;
}

int main(int argc, char **argv)
{
    int i;
    for(i=1; i<argc; i++) {
        struct mp3entry mp3;
        mp3.album = "Bogus";
        if(mp3info(&mp3, argv[i], false)) {
            printf("Failed to get %s\n", argv[i]);
            return 0;
        }

        printf("****** File: %s\n"
               "      Title: %s\n"
               "     Artist: %s\n"
               "      Album: %s\n"
               "      Genre: %s (%d) \n"
               "   Composer: %s\n"
               "       Year: %s (%d)\n"
               "      Track: %s (%d)\n"
               "     Length: %s / %d s\n"
               "    Bitrate: %d\n"
               "  Frequency: %d\n",
               argv[i],
               mp3.title?mp3.title:"<blank>",
               mp3.artist?mp3.artist:"<blank>",
               mp3.album?mp3.album:"<blank>",
               mp3.genre_string?mp3.genre_string:"<blank>",
                    mp3.genre,
               mp3.composer?mp3.composer:"<blank>",
               mp3.year_string?mp3.year_string:"<blank>",
                    mp3.year,
               mp3.track_string?mp3.track_string:"<blank>",
                    mp3.tracknum,
               secs2str(mp3.length),
               mp3.length/1000,
               mp3.bitrate,
               mp3.frequency);
    }

    return 0;
}

#endif
