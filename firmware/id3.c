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
#include "file.h"
#include "debug.h"
#include "atoi.h"

#include "id3.h"
#include "mp3data.h"

#define UNSYNC(b0,b1,b2,b3) (((b0 & 0x7F) << (3*7)) | \
                             ((b1 & 0x7F) << (2*7)) | \
                             ((b2 & 0x7F) << (1*7)) | \
                             ((b3 & 0x7F) << (0*7)))

#define BYTES2INT(b0,b1,b2,b3) (((b0 & 0xFF) << (3*8)) | \
                                ((b1 & 0xFF) << (2*8)) | \
                                ((b2 & 0xFF) << (1*8)) | \
                                ((b3 & 0xFF) << (0*8)))

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

/* parse numeric value from string */
static int parsenum( struct mp3entry* entry, char* tag, int bufferpos )
{
    entry->tracknum = atoi( tag );
    return bufferpos;
}

/* parse numeric genre from string */
static int parsegenre( struct mp3entry* entry, char* tag, int bufferpos )
{
    if( tag[ 1 ] == '(' && tag[ 2 ] != '(' ) {
        entry->genre = atoi( tag + 2 );
        entry->genre_string = 0;
        return tag - entry->id3v2buf;
    }
    else {
        entry->genre = 0xFF;
        return bufferpos;
    }
}

static struct tag_resolver taglist[] = {
    { "TPE1", 4, offsetof(struct mp3entry, artist), NULL },
    { "TP1",  3, offsetof(struct mp3entry, artist), NULL },
    { "TIT2", 4, offsetof(struct mp3entry, title), NULL },
    { "TT2",  3, offsetof(struct mp3entry, title), NULL },
    { "TALB", 4, offsetof(struct mp3entry, album), NULL },
    { "TRCK", 4, offsetof(struct mp3entry, track_string), &parsenum },
    { "TYER", 4, offsetof(struct mp3entry, year_string), &parsenum },
    { "TYR",  3, offsetof(struct mp3entry, year_string), &parsenum },
    { "TCON", 4, offsetof(struct mp3entry, genre_string), &parsegenre },
    { "TCOM", 5, offsetof(struct mp3entry, composer), NULL }
};

#define TAGLIST_SIZE ((int)(sizeof(taglist) / sizeof(taglist[0])))

/* Checks to see if the passed in string is a 16-bit wide Unicode v2
   string.  If it is, we attempt to convert it to a 8-bit ASCII string
   (for valid 8-bit ASCII characters).  If it's not unicode, we leave
   it alone.  At some point we should fully support unicode strings */
static int unicode_munge(char** string, int *len) {
   int tmp;
   bool le = false;
   int i;
   char *str = *string;
   char *outstr = *string;

   if(str[0] > 0x03) {
      /* Plain old string */
      return 0;
   }
   
   /* Type 0x00 is ordinary ISO 8859-1 */
   if(str[0] == 0x00) {
      (*len)--;
      (*string)++; /* Skip the encoding type byte */
      return 0;
   }

   /* Unicode with or without BOM */
   if(str[0] == 0x01 || str[0] == 0x02) {
      str++;
      tmp = BYTES2INT(0, 0, str[0], str[1]);

      /* Now check if there is a BOM (zero-width non-breaking space, 0xfeff)
         and if it is in little or big endian format */
      if(tmp == 0xfffe) { /* Little endian? */
         le = true;
         str += 2;
      }

      if(tmp == 0xfeff) /* Big endian? */
         str += 2;

      i = 0;

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
      } while(str[0] || str[1]);

      *len = i;
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
    static char offsets[] = {3, 33, 63, 93, 125, 127};
    int i, j;

    if (-1 == lseek(fd, -128, SEEK_END))
        return false;

    if (read(fd, buffer, sizeof buffer) != sizeof buffer)
        return false;

    if (strncmp(buffer, "TAG", 3))
        return false;

    entry->id3v1len = 128;
    entry->id3version = ID3_VER_1_0;

    for (i=0; i < (int)sizeof offsets; i++) {
        char* ptr = buffer + offsets[i];
        
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
    int bufferpos = 0, totframelen, framelen;
    char header[10];
    unsigned char version;
    char *buffer = entry->id3v2buf;
    int bytesread = 0;
    int buffersize = sizeof(entry->id3v2buf);
    int flags;
    int skip;
    int i;

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
    entry->tracknum = entry->year = entry->genre = 0;
    entry->title = entry->artist = entry->album = NULL;

    /* Skip the extended header if it is present */
    if(version >= ID3_VER_2_4) {
        if(header[5] & 0x40) {
            if(4 != read(fd, header, 4))
                return;
            
            framelen = UNSYNC(header[0], header[1], 
                              header[2], header[3]);

            lseek(fd, framelen - 4, SEEK_CUR);
        }
    }
    
    /* 
     * We must have at least minframesize bytes left for the 
     * remaining frames to be interesting 
     */
    while(size > minframesize ) {
        flags = 0;
        
        /* Read frame header and check length */
        if(version >= ID3_VER_2_3) {
            if(10 != read(fd, header, 10))
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

        if(framelen == 0)
            return;

        if(flags)
        {
            skip = 0;
            
            if(flags & 0x0040) /* Grouping identity */
                skip++;

            if(flags & 0x000c) /* Compression or encryption */
            {
                /* Skip it using the total size in case
                   it was truncated */
                size -= totframelen;
                lseek(fd, totframelen, SEEK_CUR);
                continue;
            }

            /* The Unsynchronization flag can't be trusted, so we
               don't check it for now... */
            
            if(flags & 0x0001) /* Data length indicator */
                skip += 4;

            if(skip)
            {
                lseek(fd, skip, SEEK_CUR);
                framelen -= skip;
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
            struct tag_resolver* tr = &taglist[i];
            char** ptag =  (char**) (((char*)entry) + tr->offset);
            char* tag;
            
            if( !*ptag && !memcmp( header, tr->tag, tr->tag_length ) ) { 

                /* found a tag matching one in tagList, and not yet filled */
                bytesread = read(fd, buffer + bufferpos, framelen);
                if( bytesread != framelen )
                    return;
                
                size -= bytesread;
                *ptag = buffer + bufferpos;
                unicode_munge( ptag, &bytesread );
                tag = *ptag; 
                tag[bytesread + 1] = 0;
                bufferpos += bytesread + 2;
                if( tr->ppFunc )
                    bufferpos = tr->ppFunc(entry, tag, bufferpos);
                break;
            }
        }
        
        if( i == TAGLIST_SIZE ) {
            /* no tag in tagList was found, or it was a repeat. 
               skip it using the total size */

            size -= totframelen;
            if( lseek(fd, totframelen, SEEK_CUR) == -1 )
                return;
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
static int getid3v2len(int fd) 
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
    unsigned int filetime = 0;
    struct mp3info info;
    int bytecount;
    
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

    /* If the file time hasn't been established, this may be a fixed
       rate MP3, so just use the default formula */

    filetime = info.file_time;
    
    if(filetime == 0)
    {
        /* 
         * Now song length is 
         * ((filesize)/(bytes per frame))*(time per frame) 
         */
        filetime = entry->filesize/info.frame_size*info.frame_time;
    }

    entry->tpf = info.frame_time;
    entry->bpf = info.frame_size;
    
    entry->vbr = info.is_vbr;
    entry->has_toc = info.has_toc;
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
 * Arguments: entry - the entry to check and update with the new information
 *
 * Returns: void
 */
bool mp3info(struct mp3entry *entry, char *filename) 
{
    int fd;
    fd = open(filename, O_RDONLY);
    if(-1 == fd)
        return true;

    memset(entry, 0, sizeof(struct mp3entry));
   
    strncpy(entry->path, filename, sizeof(entry->path));
 
    entry->title = NULL;
    entry->filesize = filesize(fd);
    entry->id3v2len = getid3v2len(fd);
    entry->tracknum = 0;
    entry->genre = 0xff;

    if (entry->id3v2len)
        setid3v2title(fd, entry);
    entry->length = getsonglength(fd, entry);

    /* Subtract the meta information from the file size to get
       the true size of the MP3 stream */
    entry->filesize -= entry->first_frame_offset;
    
    /* only seek to end of file if no id3v2 tags were found */
    if (!entry->id3v2len) {
        if(!entry->title)
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
        if(mp3info(&mp3, argv[i])) {
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
