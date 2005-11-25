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
 * by David Härdeman. It has since been extended and enhanced pretty much by
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
#include "debug.h"
#include "atoi.h"

#include "id3.h"
#include "mp3data.h"
#include "system.h"
#include "replaygain.h"

#define UNSYNC(b0,b1,b2,b3) (((long)(b0 & 0x7F) << (3*7)) | \
                             ((long)(b1 & 0x7F) << (2*7)) | \
                             ((long)(b2 & 0x7F) << (1*7)) | \
                             ((long)(b3 & 0x7F) << (0*7)))

#define BYTES2INT(b0,b1,b2,b3) (((long)(b0 & 0xFF) << (3*8)) |  \
                                ((long)(b1 & 0xFF) << (2*8)) |  \
                                ((long)(b2 & 0xFF) << (1*8)) |  \
                                ((long)(b3 & 0xFF) << (0*8)))

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
    "Christian Rock", "Merengue", "Salsa", "Trash Metal", "Anime", "Jpop",
    "Synthpop"
};

static const char* const codec_labels[] = {
  "???",  /* Unknown file format */

  "MP1",  /* MPEG Audio layer 1 */
  "MP2",  /* MPEG Audio layer 2 */
  "MP3",  /* MPEG Audio layer 3 */

#if CONFIG_CODEC == SWCODEC
  "WAV",  /* Uncompressed PCM in a WAV file */
  "OGG",  /* Ogg Vorbis */
  "FLAC", /* FLAC */
  "MPC",  /* Musepack */
  "AC3",  /* A/52 (aka AC3) audio */
  "WV",   /* WavPack */
  "ALAC", /* Apple Lossless Audio Codec */
  "AAC",  /* Advanced Audio Coding in M4A container */
  "SHN",  /* Shorten */
#endif
};

char* id3_get_genre(const struct mp3entry* id3)
{
    if( id3->genre_string )
        return id3->genre_string ;

    if (id3->genre < sizeof(genres)/sizeof(char*))
        return (char*)genres[id3->genre];
    return NULL;
}

char* id3_get_codec(const struct mp3entry* id3)
{
    if (id3->codectype < sizeof(codec_labels)/sizeof(char*)) {
        return (char*)codec_labels[id3->codectype];
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
        if(rc < 0)
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
        if(rc < 0)
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
            entry->genre = atoi( tag );
            entry->genre_string = 0;
            return tag - entry->id3v2buf;
        } else {
            entry->genre_string = tag;
            entry->genre = 0xff;
            return bufferpos;
        }
    } else {
        if( tag[0] == '(' && tag[1] != '(' ) {
            entry->genre = atoi( tag + 1 );
            entry->genre_string = 0;
            return tag - entry->id3v2buf;
        }
        else {
            entry->genre_string = tag;
            entry->genre = 0xff;
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
    
    if (value_len) {
        bufferpos = tag - entry->id3v2buf + value_len;
    } else {
        bufferpos = tag - entry->id3v2buf;
    }

    return bufferpos;
}
#endif

static const struct tag_resolver taglist[] = {
    { "TPE1", 4, offsetof(struct mp3entry, artist), NULL },
    { "TP1",  3, offsetof(struct mp3entry, artist), NULL },
    { "TIT2", 4, offsetof(struct mp3entry, title), NULL },
    { "TT2",  3, offsetof(struct mp3entry, title), NULL },
    { "TALB", 4, offsetof(struct mp3entry, album), NULL },
    { "TAL",  3, offsetof(struct mp3entry, album), NULL },
    { "TRK",  3, offsetof(struct mp3entry, track_string), &parsetracknum },
    { "TRCK", 4, offsetof(struct mp3entry, track_string), &parsetracknum },
    { "TYER", 4, offsetof(struct mp3entry, year_string), &parseyearnum },
    { "TYE",  3, offsetof(struct mp3entry, year_string), &parseyearnum },
    { "TCOM", 4, offsetof(struct mp3entry, composer), NULL },
    { "TCON", 4, offsetof(struct mp3entry, genre_string), &parsegenre },
    { "TCO",  3, offsetof(struct mp3entry, genre_string), &parsegenre },
#if CONFIG_CODEC == SWCODEC
    { "TXXX", 4, 0, &parseuser },
#endif
};

#define TAGLIST_SIZE ((int)(sizeof(taglist) / sizeof(taglist[0])))

/* Checks to see if the passed in string is a 16-bit wide Unicode v2
   string.  If it is, we attempt to convert it to a 8-bit ASCII string
   (for valid 8-bit ASCII characters).  If it's not unicode, we leave
   it alone.  At some point we should fully support unicode strings */
static int unicode_munge(char* string, int *len) {
   long tmp;
   bool le = false;
   int i;
   char *str = string;
   char *outstr = string;
   bool bom = false;
   int outlen;

   if(str[0] > 0x03) {
      /* Plain old string */
      return 0;
   }
   
   /* Type 0x00 is ordinary ISO 8859-1 */
   if(str[0] == 0x00) {
      int i = --(*len);

      /* We must move the string to the left */
      while (i--) {
         string[0] = string[1];
         string++;
      }

      return 0;
   }

   /* Unicode with or without BOM */
   if(str[0] == 0x01 || str[0] == 0x02) {
      (*len)--;
      str++;
      i = 0;
   
      /* Handle frames with more than one string (needed for TXXX frames).
       */
      do {
         tmp = BYTES2INT(0, 0, str[0], str[1]);
    
         /* Now check if there is a BOM (zero-width non-breaking space, 0xfeff)
            and if it is in little or big endian format */
         if(tmp == 0xfffe) { /* Little endian? */
            bom = true;
            le = true;
            str += 2;
            (*len)-=2;
         }
    
         if(tmp == 0xfeff) { /* Big endian? */
            bom = true;
            str += 2;
            (*len)-=2;
         }
    
         /* If there is no BOM (which is a specification violation),
    	    let's try to guess it. If one of the bytes is 0x00, it is
    	    probably the most significant one. */
         if(!bom) {
            if(str[1] == 0)
               le = true;
         }
      
         outlen = *len / 2;

         do {
            if(le) {
               if(str[1])
                  outstr[i++] = '.';
               else
                  outstr[i++] = str[0];
            } else {
               if(str[0])
                  outstr[i++] = '.';
               else
                  outstr[i++] = str[1];
            }
            str += 2;
         } while((str[0] || str[1]) && (i < outlen));

         str += 2;
         outstr[i++] = 0; /* Terminate the string */
      } while(i < outlen);

      *len = i - 1;

      return 0;
   }

   /* If we come here, the string was of an unsupported type */
   *len = 1;
   outstr[0] = 0;
   return -1;
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
    static const char offsets[] = {3, 33, 63, 93, 125, 127};
    int i, j;

    if (-1 == lseek(fd, -128, SEEK_END))
        return false;

    if (read(fd, buffer, sizeof buffer) != sizeof buffer)
        return false;

    if (strncmp((char *)buffer, "TAG", 3))
        return false;

    entry->id3v1len = 128;
    entry->id3version = ID3_VER_1_0;

    for (i=0; i < (int)sizeof offsets; i++) {
        char* ptr = (char *)buffer + offsets[i];
        
        if (i<3) {
            /* kill trailing space in strings */
            for (j=29; j && ptr[j]==' '; j--)
                ptr[j] = 0;
        }

        switch(i) {
            case 0:
                strncpy(entry->id3v1buf[2], ptr, 30);
                entry->title = entry->id3v1buf[2];
                break;

            case 1:
                strncpy(entry->id3v1buf[0], ptr, 30);
                entry->artist = entry->id3v1buf[0];
                break;

            case 2:
                strncpy(entry->id3v1buf[1], ptr, 30);
                entry->album = entry->id3v1buf[1];
                break;

            case 3:
                ptr[4] = 0;
                entry->year = atoi(ptr);
                break;

            case 4:
                /* id3v1.1 uses last two bytes of comment field for track
                   number: first must be 0 and second is track num */
                if (!ptr[0] && ptr[1]) {
                    entry->tracknum = ptr[1];
                    entry->id3version = ID3_VER_1_1;
                }
                break;

            case 5:
                /* genre */
                entry->genre = ptr[0];
                break;
        }
    }

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
    int i;
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
    entry->tracknum = entry->year = 0;
    entry->genre = 0xff;
    entry->title = entry->artist = entry->album = NULL;

    global_flags = header[5];
    
    /* Skip the extended header if it is present */
    if(global_flags & 0x40) {
        if(version == ID3_VER_2_3) {
            if(10 != read(fd, header, 10))
                return;
            /* The 2.3 extended header size doesn't include the following
               data, so we have to find out the size by checking the flags.
               Also, it is not unsynched. */
            framelen = BYTES2INT(header[0], header[1], header[2], header[3]) +
                BYTES2INT(header[6], header[7], header[8], header[9]);
            flags = BYTES2INT(0, 0, header[4], header[5]);
            if(flags & 0x8000)
                framelen += 4;   /* CRC */

            lseek(fd, framelen - 10, SEEK_CUR);
        }
        
        if(version >= ID3_VER_2_4) {
            if(4 != read(fd, header, 4))
                return;

            /* The 2.4 extended header size does include the entire header,
               so here we can just skip it. This header is unsynched. */
            framelen = UNSYNC(header[0], header[1], 
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

            flags = BYTES2INT(0, 0, header[8], header[9]);
            
            if (version >= ID3_VER_2_4) {
                framelen = UNSYNC(header[4], header[5], 
                                  header[6], header[7]);
            } else {
                /* version .3 files don't use synchsafe ints for
                 * size */
                framelen = BYTES2INT(header[4], header[5], 
                                     header[6], header[7]);
            }
        } else {
            if(6 != read(fd, header, 6))
                return;
            /* Adjust for the 6 bytes we read */
            size -= 6;
            
            framelen = BYTES2INT(0, header[3], header[4], header[5]);
        }

        /* Keep track of the total size */
        totframelen = framelen;
        
        DEBUGF("framelen = %d\n", framelen);
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
                /* Skip it using the total size in case
                   it was truncated */
                size -= totframelen;
                lseek(fd, totframelen, SEEK_CUR);
                continue;
            }

            if(flags & 0x0002) /* Unsynchronization */
                unsynch = true;

            if (version >= ID3_VER_2_4) {
                if(flags & 0x0001) { /* Data length indicator */
                    if(4 != read(fd, tmp, 4))
                        return;

                    data_length_ind = UNSYNC(tmp[0], tmp[1], tmp[2], tmp[3]);
                    framelen -= 4;
                }
            }
        }
        
        /* If the frame is larger than the remaining buffer space we try
           to read as much as would fit in the buffer */
        if(framelen >= buffersize - bufferpos)
            framelen = buffersize - bufferpos - 1;

        DEBUGF("id3v2 frame: %.4s\n", header);

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
                
                unicode_munge( tag, &bytesread );

                if (ptag)
                    *ptag = tag;

                /* remove trailing spaces */
                while ( bytesread > 0 && isspace(tag[bytesread-1]))
                    bytesread--;
                tag[bytesread] = 0;
                bufferpos += bytesread + 1;

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
            offset = UNSYNC(buf[0], buf[1], buf[2], buf[3]) + 10;

    DEBUGF("ID3V2 Length: 0x%x\n", offset);
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

    DEBUGF("Space between ID3V2 tag and first audio frame: 0x%x bytes\n",
           bytecount);

    if(bytecount < 0)
        return -1;
    
    bytecount += entry->id3v2len;

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
        filetime = (entry->filesize - bytecount) / (info.bitrate / 8);
        /* bitrate is in kbps so this delivers milliseconds. Doing bitrate / 8
         * instead of filesize * 8 is exact, because mpeg audio bitrates are
         * always multiples of 8, and it avoids overflows. */
    }

    entry->frame_count = info.frame_count;

    entry->vbr = info.is_vbr;
    entry->has_toc = info.has_toc;

    entry->lead_trim = info.enc_delay;
    entry->tail_trim = info.enc_padding;
    
    memcpy(entry->toc, info.toc, sizeof(info.toc));

    entry->vbr_header_pos = info.vbr_header_pos;
    
    /* Update the seek point for the first playable frame */
    entry->first_frame_offset = bytecount;
    DEBUGF("First frame is at %x\n", entry->first_frame_offset);

    return filetime;
}

/*
 * Checks all relevant information (such as ID3v1 tag, ID3v2 tag, length etc)
 * about an MP3 file and updates it's entry accordingly.
 *
 */
bool mp3info(struct mp3entry *entry, const char *filename, bool v1first)
{
    int fd;
    int v1found = false;
    
    fd = open(filename, O_RDONLY);
    if(-1 == fd)
        return true;

#if CONFIG_CODEC != SWCODEC
    memset(entry, 0, sizeof(struct mp3entry));
#endif
	
    strncpy(entry->path, filename, sizeof(entry->path));
 
    entry->title = NULL;
    entry->filesize = filesize(fd);
    entry->id3v2len = getid3v2len(fd);
    entry->tracknum = 0;
    entry->genre = 0xff;

    if(v1first)
        v1found = setid3v1title(fd, entry);
    
    if (!v1found && entry->id3v2len)
        setid3v2title(fd, entry);
    entry->length = getsonglength(fd, entry);

    /* Subtract the meta information from the file size to get
       the true size of the MP3 stream */
    entry->filesize -= entry->first_frame_offset;
    
    /* only seek to end of file if no id3v2 tags were found,
       and we already haven't looked for a v1 tag */
    if (!v1first && !entry->id3v2len) {
        setid3v1title(fd, entry);
    }

    close(fd);

    if(!entry->length || (entry->filesize < 8 ))
        /* no song length or less than 8 bytes is hereby considered to be an
           invalid mp3 and won't be played by us! */
        return true;

    return false;
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
