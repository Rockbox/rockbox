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
 * A nice reference for MPEG header info:
 * http://rockbox.haxx.se/docs/mpeghdr.html
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "file.h"
#include "debug.h"
#include "atoi.h"

#include "id3.h"

/* Some utility macros used in getsonglength() */
#define BYTE0(x) ((x >> 24) & 0xFF)
#define BYTE1(x) ((x >> 16) & 0xFF)
#define BYTE2(x) ((x >> 8)  & 0xFF)
#define BYTE3(x) ((x >> 0)  & 0xFF)

#define UNSYNC(b1,b2,b3,b4) (((b1 & 0x7F) << (3*7)) | \
                             ((b2 & 0x7F) << (2*7)) | \
                             ((b3 & 0x7F) << (1*7)) | \
                             ((b4 & 0x7F) << (0*7)))

/* Table of bitrates for MP3 files, all values in kilo.
 * Indexed by version, layer and value of bit 15-12 in header.
 */
const int bitrate_table[2][3][16] =
{
    {
        {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0},
        {0,32,48,56, 64,80, 96, 112,128,160,192,224,256,320,384,0},
        {0,32,40,48, 56,64, 80, 96, 112,128,160,192,224,256,320,0}
    },
    {
        {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,0},
        {0, 8,16,24,32,40,48, 56, 64, 80, 96,112,128,144,160,0},
        {0, 8,16,24,32,40,48, 56, 64, 80, 96,112,128,144,160,0}
    }
};

/* Table of samples per frame for MP3 files.
 * Indexed by layer. Multiplied with 1000.
 */
const int bs[4] = {0, 384000, 1152000, 1152000};

/* Table of sample frequency for MP3 files.
 * Indexed by version and layer.
 */
const int freqtab[][4] =
{
    {11025, 12000, 8000, 0},  /* MPEG version 2.5 */
    {44100, 48000, 32000, 0}, /* MPEG Version 1 */
    {22050, 24000, 16000, 0}, /* MPEG version 2 */
};

/*
 * Removes trailing spaces from a string.
 *
 * Arguments: buffer - the string to process
 *
 * Returns: void
 */
static void 
stripspaces(char *buffer) 
{
    int i = 0;
    while(*(buffer + i) != '\0')
        i++;
    
    for(;i >= 0; i--) {
        if(*(buffer + i) == ' ')
            *(buffer + i) = '\0';
        else if(*(buffer + i) == '\0')
            continue;
        else
            break;
    }
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
    unsigned char buffer[31];
    int offsets[4] = {-95,-65,-125,-31};
    int i;

    for (i=0;i<4;i++) {
        if (-1 == lseek(fd, offsets[i], SEEK_END))
            return false;

        buffer[30]=0;
        read(fd, buffer, 30);
        stripspaces(buffer);
        
        if (buffer[0] || i == 3) {
            switch(i) {
                case 0:
                    strcpy(entry->id3v1buf[0], buffer);
                    entry->artist = entry->id3v1buf[0];
                    break;
                case 1:
                    strcpy(entry->id3v1buf[1], buffer);
                    entry->album = entry->id3v1buf[1];
                    break;
                case 2:
                    strcpy(entry->id3v1buf[2], buffer);
                    entry->title = entry->id3v1buf[2];
                    break;
                case 3:
                    /* id3v1.1 uses last two bytes of comment field for track
                       number: first must be 0 and second is track num */
                    if (buffer[28] == 0)
                        entry->tracknum = buffer[29];
                    break;
            }
        }
    }

    return true;
}


/*
 * Sets the title of an MP3 entry based on its ID3v2 tag.
 *
 * Arguments: file - the MP3 file to scen for a ID3v2 tag
 *            entry - the entry to set the title in
 *
 * Returns: true if a title was found and created, else false
 */
static void setid3v2title(int fd, struct mp3entry *entry) 
{
    int minframesize;
    int size;
    int readsize = 0, headerlen;
    char *title = NULL;
    char *artist = NULL;
    char *album = NULL;
    char *tracknum = NULL;
    char header[10];
    unsigned short int version;
    int titlen=0, artistn=0, albumn=0, tracknumn=0;   
    char *buffer = entry->id3v2buf;
	
    /* 10 = headerlength */
    if(entry->id3v2len < 10)
        return;

    /* Check version */
    lseek(fd, 0, SEEK_SET);
    if(10 != read(fd, header, 10))
        return;

    version = (unsigned short int)header[3];
    
    /* Read all frames in the tag */
    size = entry->id3v2len - 10;

    if(size >= (int)sizeof(entry->id3v2buf))
        size = sizeof(entry->id3v2buf)-1;

    if(size != read(fd, buffer, size))
        return;

    *(buffer + size) = '\0';

    /* Set minimum frame size according to ID3v2 version */
    if(version > 2)
        minframesize = 12;
    else
        minframesize = 8;

    /* 
     * We must have at least minframesize bytes left for the 
     * remaining frames to be interesting 
     */
    while(size - readsize > minframesize) {
        
        /* Read frame header and check length */
        if(version > 2) {
            memcpy(header, (buffer + readsize), 10);
            readsize += 10;
            headerlen = UNSYNC(header[4], header[5], 
                               header[6], header[7]);
        } else {
            memcpy(header, (buffer + readsize), 6);			
            readsize += 6;
            headerlen = (header[3] << 16) + 
                (header[4] << 8) + 
                (header[5]);
        }

        /* Get only the part of the header that is within our buffer */
        if(headerlen > (size-readsize))
            headerlen = (size - readsize);
        
        /* Check for certain frame headers */
        if(!strncmp(header, "TPE1", strlen("TPE1")) || 
           !strncmp(header, "TP1", strlen("TP1"))) {
            readsize++;
            headerlen--;
            artist = buffer + readsize;
            artistn = headerlen;
            readsize += headerlen;
        }
        else if(!strncmp(header, "TIT2", strlen("TIT2")) || 
                !strncmp(header, "TT2", strlen("TT2"))) {
            readsize++;
            headerlen--;
            title = buffer + readsize;
            titlen = headerlen;
            readsize += headerlen;
        }
        else if(!strncmp(header, "TALB", strlen("TALB"))) {
            readsize++;
            headerlen--;
            album = buffer + readsize;
            albumn = headerlen;
            readsize += headerlen;
        }
        else if(!strncmp(header, "TRCK", strlen("TRCK"))) {
            readsize++;
            headerlen--;
            tracknum = buffer + readsize;
            tracknumn = headerlen;
            readsize += headerlen;
        } else {
            readsize += headerlen;
        }
    }
    
    if(artist) {
        entry->artist = artist;
        artist[artistn]=0;
    }
  
    if(title) {
        entry->title = title;
        title[titlen]=0;
    }

    if(album) {
        entry->album = album;
        album[albumn]=0;
    }

    if(tracknum) {
        tracknum[tracknumn] = 0;
        entry->tracknum = atoi(tracknum);
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
    else if(read(fd, buf, 4) != 4)
        offset = 0;
    else
        offset = UNSYNC(buf[0], buf[1], buf[2], buf[3]) + 10;

    return offset;
}

static int getfilesize(int fd)
{
    int size;
    
    /* seek to the end of it */
    size = lseek(fd, 0, SEEK_END);
    if(-1 == size)
        return 0; /* unknown */

    return size;
}

/*
 * Calculates the size of the ID3v1 tag.
 *
 * Arguments: file - the file to search for a tag.
 *
 * Returns: the size of the tag or 0 if none was found
 */
static int getid3v1len(int fd) 
{
    char buf[3];
    int offset;

    /* Check if we find "TAG" 128 bytes from EOF */
    if((lseek(fd, -128, SEEK_END) == -1) ||
       (read(fd, buf, 3) != 3) ||
       (strncmp(buf, "TAG", 3) != 0))
        offset = 0;
    else
        offset = 128;
	
    return offset;
}

/* check if 'head' is a valid mp3 frame header */
static bool mp3frameheader(unsigned long head)
{
    if ((head & 0xffe00000) != 0xffe00000) /* bad sync? */
        return false;
    if (!((head >> 17) & 3)) /* no layer? */
        return false;
    if (((head >> 12) & 0xf) == 0xf) /* bad bitrate? */
        return false;
    if (!((head >> 12) & 0xf)) /* no bitrate? */
        return false;
    if (((head >> 10) & 0x3) == 0x3) /* bad sample rate? */
        return false;
    if (((head >> 19) & 1) == 1 &&
        ((head >> 17) & 3) == 3 &&
        ((head >> 16) & 1) == 1)
        return false;
    if ((head & 0xffff0000) == 0xfffe0000)
        return false;
	
    return true;
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
    unsigned long header=0;
    unsigned char tmp;
    unsigned char frame[156];
    unsigned char* xing;
    
    enum {
        MPEG_VERSION2_5,
        MPEG_VERSION1,
        MPEG_VERSION2
    } version;
    int layer;
    int bitindex;
    int bitrate;
    int freqindex;
    int frequency;
    int chmode;
    int bytecount;
    int bittable; /* which bitrate table to use */

    long bpf;
    long tpf;

    /* Start searching after ID3v2 header */ 
    if(-1 == lseek(fd, entry->id3v2len, SEEK_SET))
        return 0;
	
    /* Fill up header with first 24 bits */
    for(version = 0; version < 3; version++) {
        header <<= 8;
        if(!read(fd, &tmp, 1))
            return 0;
        header |= tmp;
    }
	
    /* Loop trough file until we find a frame header */
    bytecount = entry->id3v2len - 1;
  restart:
    do {
        header <<= 8;
        if(!read(fd, &tmp, 1))
            return 0;
        header |= tmp;

        /* Quit if we haven't found a valid header within 128K */
        bytecount++;
        if(bytecount > 0x20000)
            return 0;
    } while(!mp3frameheader(header));
	
    /* 
     * Some files are filled with garbage in the beginning, 
     * if the bitrate index of the header is binary 1111 
     * that is a good indicator
     */
    if((header & 0xF000) == 0xF000)
        goto restart;

#ifdef DEBUG_VERBOSE
    fprintf(stderr,
            "We found %x-%x-%x-%x and checksync %i and test %x\n", 
            BYTE0(header), BYTE1(header), BYTE2(header), BYTE3(header), 
            CHECKSYNC(header), (header & 0xF000) == 0xF000);
#endif
    /* MPEG Audio Version */
    switch((header & 0x180000) >> 19) {
        case 0:
            /* MPEG version 2.5 is not an official standard */
            version = MPEG_VERSION2_5;
            bittable = MPEG_VERSION2; /* use the V2 bit rate table */
            break;

        case 2:
            /* MPEG version 2 (ISO/IEC 13818-3) */
            version = MPEG_VERSION2;
            bittable = MPEG_VERSION2;
            break;

        case 3:
            /* MPEG version 1 (ISO/IEC 11172-3) */
            version = MPEG_VERSION1;
            bittable = MPEG_VERSION1;
            break;
        default:
            goto restart;
    }

    /* Layer */
    switch((header & 0x060000) >> 17) {
        case 1:
            layer = 3;
            break;
        case 2:
            layer = 2;
            break;
        case 3:
            layer = 1;
            break;
        default:
            goto restart;
    }

    /* Bitrate */
    bitindex = (header & 0xF000) >> 12;
    bitrate = bitrate_table[bittable-1][layer-1][bitindex];
    if(bitrate == 0)
        goto restart;

    /* Sampling frequency */
    freqindex = (header & 0x0C00) >> 10;
    frequency = freqtab[version][freqindex];
    if(frequency == 0)
        goto restart;

#ifdef DEBUG_VERBOSE
    DEBUGF( "Version %i, lay %i, biti %i, bitr %i, freqi %i, freq %i, chmode %d\n", 
            version, layer, bitindex, bitrate, freqindex, frequency, chmode);
#endif
    entry->version = version;
    entry->layer = layer;
    entry->frequency = frequency;
	
    /* Calculate bytes per frame, calculation depends on layer */
    switch(layer) {
        case 1:
            bpf = bitrate_table[bittable - 1][layer - 1][bitindex];
            bpf *= 48000;
            bpf /= freqtab[version][freqindex] << (bittable - 1);
            break;
        case 2:
        case 3:
            bpf = bitrate_table[bittable - 1][layer - 1][bitindex];
            bpf *= 144000;
            bpf /= freqtab[version][freqindex] << (bittable - 1);
            break;
        default:
            bpf = 1;
    }
	
    /* Calculate time per frame */
    tpf = bs[layer] / (freqtab[version][freqindex] << (bittable - 1));
        
    entry->bpf = bpf;
    entry->tpf = tpf;
 
    /* OK, we have found a frame. Let's see if it has a Xing header */
    if(read(fd, frame, sizeof frame) < 0)
        return -1;

    /* Channel mode (stereo/mono) */
    chmode = (header & 0xc0) >> 6;

    /* calculate position of Xing VBR header */
    if ( version == 1 ) {
        if ( chmode == 3 ) /* mono */
            xing = frame + 17;
        else
            xing = frame + 32;
    }
    else {
        if ( chmode == 3 ) /* mono */
            xing = frame + 9;
        else
            xing = frame + 17;
    }

    if (xing[0] == 'X' &&
        xing[1] == 'i' &&
        xing[2] == 'n' &&
        xing[3] == 'g')
    {
        /* Yes, it is a VBR file */
        entry->vbr = true;
        entry->vbrflags = xing[7];
        int i = 8; /* Where to start parsing info */
        
        if (entry->vbrflags & VBR_FRAMES_FLAG) /* Is the frame count there? */
        {
            int framecount = (xing[i] << 24) | (xing[i+1] << 16) |
                (xing[i+2] << 8) | xing[i+3];
 
            filetime = framecount * tpf;
            i += 4;
        }

        if (entry->vbrflags & VBR_BYTES_FLAG) /* is byte count there? */
        {
            int bytecount = (xing[i] << 24) | (xing[i+1] << 16) |
                (xing[i+2] << 8) | xing[i+3];
 
            bitrate = bytecount * 8 / filetime;
            i += 4;
        }

        if (entry->vbrflags & VBR_TOC_FLAG) /* is table-of-contents there? */
        {
            memcpy( entry->toc, xing+i, 100 );
        }

        /* Make sure we skip this frame in playback */
        bytecount += bpf;
    }

    /* Is it a LAME Info frame? */
    if (xing[0] == 'I' &&
        xing[1] == 'n' &&
        xing[2] == 'f' &&
        xing[3] == 'o')
    {
        /* Make sure we skip this frame in playback */
        bytecount += bpf;
    }


    entry->bitrate = bitrate;

    /* If the file time hasn't been established, this may be a fixed
       rate MP3, so just use the default formula */
    if(filetime == 0)
    {
        /* 
         * Now song length is 
         * ((filesize)/(bytes per frame))*(time per frame) 
         */
        filetime = entry->filesize/bpf*tpf;
    }

    DEBUGF("Old ID3V2 length: %x\n", entry->id3v2len);
    entry->id3v2len = bytecount;
    DEBUGF("New ID3V2 length: %x\n", bytecount);
    
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
    entry->filesize = getfilesize(fd);
    entry->id3v2len = getid3v2len(fd);
    entry->tracknum = 0;

    if (entry->id3v2len)
        setid3v2title(fd, entry);
    entry->length = getsonglength(fd, entry);

    entry->id3v1len = getid3v1len(fd);
    if(entry->id3v1len && !entry->title)
        setid3v1title(fd, entry);

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
        if(mp3info(&mp3, argv[i])) {
            printf("Failed to get %s\n", argv[i]);
            return 0;
        }

        printf("****** File: %s\n"
               "      Title: %s\n"
               "     Artist: %s\n"
               "      Album: %s\n"
               "     Length: %s / %d s\n"
               "    Bitrate: %d\n"
               "  Frequency: %d\n",
               argv[i],
               mp3.title?mp3.title:"<blank>",
               mp3.artist?mp3.artist:"<blank>",
               mp3.album?mp3.album:"<blank>",
               secs2str(mp3.length),
               mp3.length/1000,
               mp3.bitrate,
               mp3.frequency);
    }
    
    return 0;
}

#endif

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "rockbox-mode.el")
 * end:
 */
