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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "config.h"
#include "file.h"
#include "logf.h"
#include "atoi.h"

#include "id3.h"
#include "mp3data.h"
#include "system.h"
#include "replaygain.h"
#include "rbunicode.h"

/** Database of audio formats **/
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
        AFMT_ENTRY("Ogg",  "vorbis",  NULL,          "ogg\0"      ),
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
/****/

unsigned long unsync(unsigned long b0,
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

char* id3_get_num_genre(const unsigned int genre_num)
{
    if (genre_num < sizeof(genres)/sizeof(char*))
        return (char*)genres[genre_num];
    return NULL;
}

/* True if the string is from the "genres" array */
static bool id3_is_genre_string(const char *string)
{
    return ( string >= genres[0] &&
             string <= genres[sizeof(genres)/sizeof(char*) - 1] );
}

char* id3_get_codec(const struct mp3entry* id3)
{
    if (id3->codectype < AFMT_NUM_CODECS) {
        return (char*)audio_formats[id3->codectype].label;
    } else {
        return NULL;
    }
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
    return (long)wp - (long)tag;
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
    if(entry->id3version >= ID3_VER_2_4) {
        /* In version 2.4 and up, there are no parentheses, and the genre frame
           is a list of strings, either numbers or text. */

        /* Is it a number? */
        if(isdigit(tag[0])) {
            entry->genre_string = id3_get_num_genre(atoi( tag ));
            return tag - entry->id3v2buf;
        } else {
            entry->genre_string = tag;
            return bufferpos;
        }
    } else {
        if( tag[0] == '(' && tag[1] != '(' ) {
            entry->genre_string = id3_get_num_genre(atoi( tag + 1 ));
            return tag - entry->id3v2buf;
        }
        else {
            entry->genre_string = tag;
            return bufferpos;
        }
    }
}

#if CONFIG_CODEC == SWCODEC
/* parse user defined text, looking for replaygain information. */
static int parseuser( struct mp3entry* entry, char* tag, int bufferpos )
{
    char* value = NULL;
    int desc_len = strlen(tag);
    int value_len = 0;

    if ((tag - entry->id3v2buf + desc_len + 2) < bufferpos) {
        /* At least part of the value was read, so we can safely try to
         * parse it
         */
        value = tag + desc_len + 1;
        value_len = parse_replaygain(tag, value, entry, tag,
            bufferpos - (tag - entry->id3v2buf));
    }

    return tag - entry->id3v2buf + value_len;
}

/* parse RVA2 binary data and convert to replaygain information. */
static int parserva2( struct mp3entry* entry, char* tag, int bufferpos )
{
    int desc_len = strlen(tag);
    int end_pos = tag - entry->id3v2buf + desc_len + 5;
    int value_len = 0;
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
            gain = 0;
        }
            
        value_len = parse_replaygain_int(album, gain, peak * 2, entry,
            tag, sizeof(entry->id3v2buf) - (tag - entry->id3v2buf));
    }

    return tag - entry->id3v2buf + value_len;
}
#endif

static const struct tag_resolver taglist[] = {
    { "TPE1", 4, offsetof(struct mp3entry, artist), NULL, false },
    { "TP1",  3, offsetof(struct mp3entry, artist), NULL, false },
    { "TIT2", 4, offsetof(struct mp3entry, title), NULL, false },
    { "TT2",  3, offsetof(struct mp3entry, title), NULL, false },
    { "TALB", 4, offsetof(struct mp3entry, album), NULL, false },
    { "TAL",  3, offsetof(struct mp3entry, album), NULL, false },
    { "TRK",  3, offsetof(struct mp3entry, track_string), &parsetracknum, false },
    { "TPOS", 4, offsetof(struct mp3entry, disc_string), &parsediscnum, false },
    { "TRCK", 4, offsetof(struct mp3entry, track_string), &parsetracknum, false },
    { "TDRC", 4, offsetof(struct mp3entry, year_string), &parseyearnum, false },
    { "TYER", 4, offsetof(struct mp3entry, year_string), &parseyearnum, false },
    { "TYE",  3, offsetof(struct mp3entry, year_string), &parseyearnum, false },
    { "TCOM", 4, offsetof(struct mp3entry, composer), NULL, false },
    { "TPE2", 4, offsetof(struct mp3entry, albumartist), NULL, false },
    { "TP2",  3, offsetof(struct mp3entry, albumartist), NULL, false },
    { "TIT1", 4, offsetof(struct mp3entry, grouping), NULL, false },
    { "TT1",  3, offsetof(struct mp3entry, grouping), NULL, false },
    { "COMM", 4, offsetof(struct mp3entry, comment), NULL, false }, 
    { "TCON", 4, offsetof(struct mp3entry, genre_string), &parsegenre, false },
    { "TCO",  3, offsetof(struct mp3entry, genre_string), &parsegenre, false },
#if CONFIG_CODEC == SWCODEC
    { "TXXX", 4, 0, &parseuser, false },
    { "RVA2", 4, 0, &parserva2, true },
#endif
};

#define TAGLIST_SIZE ((int)(sizeof(taglist) / sizeof(taglist[0])))

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
            *len = (unsigned long)utf8 - (unsigned long)utf8buf;
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

                do {
                    if(le)
                        utf8 = utf16LEdecode(str, utf8, 1);
                    else
                        utf8 = utf16BEdecode(str, utf8, 1);

                    str+=2;
                    i += 2;
                } while((str[0] || str[1]) && (i < *len));

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
            *len = (unsigned long)utf8 - (unsigned long)utf8buf;
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
static bool setid3v1title(int fd, struct mp3entry *entry)
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
static void setid3v2title(int fd, struct mp3entry *entry)
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
    int skip;
    bool global_unsynch = false;
    bool unsynch = false;
    int data_length_ind;
    int i, j;
    int rc;

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

        logf("framelen = %ld", framelen);
        if(framelen == 0){
            if (header[0] == 0 && header[1] == 0 && header[2] == 0)
                return;
            else
                continue;
        }

        unsynch = false;
        data_length_ind = 0;

        if(flags)
        {
            skip = 0;

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

                    data_length_ind = unsync(tmp[0], tmp[1], tmp[2], tmp[3]);
                    framelen -= 4;
                }
            }
        }

        /* Keep track of the remaining frame size */
        totframelen = framelen;

        /* If the frame is larger than the remaining buffer space we try
           to read as much as would fit in the buffer */
        if(framelen >= buffersize - bufferpos)
            framelen = buffersize - bufferpos - 1;

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

            /* Note that parser functions sometimes set *ptag to NULL, so
             * the "!*ptag" check here doesn't always have the desired
             * effect. Should the parser functions (parsegenre in
             * particular) be updated to handle the case of being called
             * multiple times, or should the "*ptag" check be removed?
             */
            if( (!ptag || !*ptag) && !memcmp( header, tr->tag, tr->tag_length ) ) {

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
                 
                if(!memcmp( header, "COMM", 4 )) {
                    int offset;
                    /* ignore comments with iTunes 7 soundcheck/gapless data */
                    if(!strncmp(tag+4, "iTun", 4))
                        break;
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

                    for (j = 0; j < bytesread; j++)
                        tag[j] = utf8buf[j];

                    /* remove trailing spaces */
                    while ( bytesread > 0 && isspace(tag[bytesread-1]))
                        bytesread--;
                }

                tag[bytesread] = 0;
                bufferpos += bytesread + 1;

                if (ptag)
                    *ptag = tag;

                if( tr->ppFunc )
                    bufferpos = tr->ppFunc(entry, tag, bufferpos);

                /* Seek to the next frame */
                if(framelen < totframelen)
                    lseek(fd, totframelen - framelen, SEEK_CUR);
                break;
            }
        }

        if( i == TAGLIST_SIZE ) {
            /* no tag in tagList was found, or it was a repeat.
               skip it using the total size */

            if(global_unsynch && version <= ID3_VER_2_3) {
                size -= skip_unsynched(fd, totframelen);
            } else {
                if(data_length_ind)
                    totframelen = data_length_ind;

                size -= totframelen;
                if( lseek(fd, totframelen, SEEK_CUR) == -1 )
                    return;
            }
        }
    }
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

/*
 * Calculates the length (in milliseconds) of an MP3 file.
 *
 * Modified to only use integers.
 *
 * Arguments: file - the file to calculate the length upon
 *            entry - the entry to update with the length
 *
 * Returns: the song length in milliseconds,
 *          0 means that it couldn't be calculated
 */
static int getsonglength(int fd, struct mp3entry *entry)
{
    unsigned long filetime = 0;
    struct mp3info info;
    long bytecount;

    /* Start searching after ID3v2 header */
    if(-1 == lseek(fd, entry->id3v2len, SEEK_SET))
        return 0;

    bytecount = get_mp3file_info(fd, &info);

    logf("Space between ID3V2 tag and first audio frame: 0x%lx bytes",
           bytecount);

    if(bytecount < 0)
        return -1;

    bytecount += entry->id3v2len;

    /* Validate byte count, in case the file has been edited without
     * updating the header.
     */
    if (info.byte_count)
    {
        const unsigned long expected = entry->filesize - entry->id3v1len
            - entry->id3v2len;
        const unsigned long diff = MAX(10240, info.byte_count / 20);

        if ((info.byte_count > expected + diff)
            || (info.byte_count < expected - diff))
        {
            logf("Note: info.byte_count differs from expected value by "
                 "%ld bytes", labs((long) (expected - info.byte_count)));
            info.byte_count = 0;
            info.frame_count = 0;
            info.file_time = 0;
            info.enc_padding = 0;

            /* Even if the bitrate was based on "known bad" values, it
             * should still be better for VBR files than using the bitrate
             * of the first audio frame.
             */
        }
    }

    entry->bitrate = info.bitrate;
    entry->frequency = info.frequency;
    entry->version = info.version;
    entry->layer = info.layer;
    switch(entry->layer) {
#if CONFIG_CODEC==SWCODEC
        case 0:
            entry->codectype=AFMT_MPA_L1;
            break;
#endif
        case 1:
            entry->codectype=AFMT_MPA_L2;
            break;
        case 2:
            entry->codectype=AFMT_MPA_L3;
            break;
    }

    /* If the file time hasn't been established, this may be a fixed
       rate MP3, so just use the default formula */

    filetime = info.file_time;

    if(filetime == 0)
    {
        /* Prevent a division by zero */
        if (info.bitrate < 8)
            filetime = 0;
        else
            filetime = (entry->filesize - bytecount) / (info.bitrate / 8);
        /* bitrate is in kbps so this delivers milliseconds. Doing bitrate / 8
         * instead of filesize * 8 is exact, because mpeg audio bitrates are
         * always multiples of 8, and it avoids overflows. */
    }

    entry->frame_count = info.frame_count;

    entry->vbr = info.is_vbr;
    entry->has_toc = info.has_toc;

#if CONFIG_CODEC==SWCODEC
    entry->lead_trim = info.enc_delay;
    entry->tail_trim = info.enc_padding;
#endif

    memcpy(entry->toc, info.toc, sizeof(info.toc));

    entry->vbr_header_pos = info.vbr_header_pos;

    /* Update the seek point for the first playable frame */
    entry->first_frame_offset = bytecount;
    logf("First frame is at %lx", entry->first_frame_offset);

    return filetime;
}

/*
 * Checks all relevant information (such as ID3v1 tag, ID3v2 tag, length etc)
 * about an MP3 file and updates it's entry accordingly.
 *
  Note, that this returns true for successful, false for error! */
bool get_mp3_metadata(int fd, struct mp3entry *entry, const char *filename)
{
#if CONFIG_CODEC != SWCODEC
    memset(entry, 0, sizeof(struct mp3entry));
#endif

    strncpy(entry->path, filename, sizeof(entry->path));

    entry->title = NULL;
    entry->filesize = filesize(fd);
    entry->id3v2len = getid3v2len(fd);
    entry->tracknum = 0;
    entry->discnum = 0;

    if (entry->id3v2len)
        setid3v2title(fd, entry);
    int len = getsonglength(fd, entry);
    if (len < 0)
        return false;
    entry->length = len;

    /* Subtract the meta information from the file size to get
       the true size of the MP3 stream */
    entry->filesize -= entry->first_frame_offset;

    /* only seek to end of file if no id3v2 tags were found */
    if (!entry->id3v2len) {
        setid3v1title(fd, entry);
    }

    if(!entry->length || (entry->filesize < 8 ))
        /* no song length or less than 8 bytes is hereby considered to be an
           invalid mp3 and won't be played by us! */
        return false;

    return true;
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

void adjust_mp3entry(struct mp3entry *entry, void *dest, void *orig)
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
}

void copy_mp3entry(struct mp3entry *dest, struct mp3entry *orig)
{
    memcpy(dest, orig, sizeof(struct mp3entry));
    adjust_mp3entry(dest, dest, orig);
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
