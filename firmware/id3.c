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
 * by David Härdeman.
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
#define CHECKSYNC(x) (((x >> 21) & 0x07FF) == 0x7FF)
#define BYTE0(x) ((x >> 24) & 0xFF)
#define BYTE1(x) ((x >> 16) & 0xFF)
#define BYTE2(x) ((x >> 8)  & 0xFF)
#define BYTE3(x) ((x >> 0)  & 0xFF)

#define UNSYNC(b1,b2,b3,b4) (((b1 & 0x7F) << (3*7)) | \
                             ((b2 & 0x7F) << (2*7)) | \
                             ((b3 & 0x7F) << (1*7)) | \
                             ((b4 & 0x7F) << (0*7)))

#define HASID3V2(entry) entry->id3v2len > 0
#define HASID3V1(entry) entry->id3v1len > 0

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
const int freqtab[2][4] =
{
    {44100, 48000, 32000, 0},
    {22050, 24000, 16000, 0},
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
    char buffer[31];
    int offsets[3] = {-95,-65,-125};
    int i;

    for(i=0;i<3;i++) {
        if(-1 == lseek(fd, offsets[i], SEEK_END))
            return false;

        buffer[30]=0;
        read(fd, buffer, 30);
        stripspaces(buffer);
        
        if(buffer[0]) {
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
    unsigned int minframesize;
    int size;
    unsigned int readsize = 0, headerlen;
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

    /* Set minimun frame size according to ID3v2 version */
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
        if(headerlen < 1)
            continue;
        
        /* Check for certain frame headers */
        if(!strncmp(header, "TPE1", strlen("TPE1")) || 
           !strncmp(header, "TP1", strlen("TP1"))) {
            readsize++;
            headerlen--;
            if(headerlen > (size - readsize))
                headerlen = (size - readsize);
            artist = buffer + readsize;
            artistn = headerlen;
            readsize += headerlen;
        }
        else if(!strncmp(header, "TIT2", strlen("TIT2")) || 
                !strncmp(header, "TT2", strlen("TT2"))) {
            readsize++;
            headerlen--;
            if(headerlen > (size - readsize))
                headerlen = (size - readsize);
            title = buffer + readsize;
            titlen = headerlen;
            readsize += headerlen;
        }
        else if(!strncmp(header, "TALB", strlen("TALB"))) {
            readsize++;
            headerlen--;
            if(headerlen > (size - readsize))
                headerlen = (size - readsize);
            album = buffer + readsize;
            albumn = headerlen;
            readsize += headerlen;
        }
        else if(!strncmp(header, "TRCK", strlen("TRCK"))) {
            readsize++;
            headerlen--;
            if(headerlen > (size - readsize))
                headerlen = (size - readsize);
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

/*
 * Calculates the length (in milliseconds) of an MP3 file. Currently this code
 * doesn't care about VBR (Variable BitRate) files since it would have to scan
 * through the entire file but this should become a config option in the
 * future.
 *
 * Modified to only use integers.
 *
 * Arguments: file - the file to calculate the length upon
 *            entry - the entry to update with the length
 *
 * Returns: the song length in milliseconds, 
 *          -1 means that it couldn't be calculated
 */
static int getsonglength(int fd, struct mp3entry *entry)
{
    unsigned int filetime = 0;
    unsigned long header=0;
    unsigned char tmp;
    unsigned char frame[200];
    unsigned int framecount;
    
    int version;
    int layer;
    int bitindex;
    int bitrate;
    int freqindex;
    int frequency;

    long bpf;
    long tpf;

    /* Start searching after ID3v2 header */ 
    if(-1 == lseek(fd, entry->id3v2len, SEEK_SET))
        return -1;
	
    /* Fill up header with first 24 bits */
    for(version = 0; version < 3; version++) {
        header <<= 8;
        if(!read(fd, &tmp, 1))
            return -1;
        header |= tmp;
    }
	
    /* Loop trough file until we find a frame header */
 restart:
    do {
        header <<= 8;
        if(!read(fd, &tmp, 1))
            return -1;
        header |= tmp;
    } while(!CHECKSYNC(header));
	
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
    case 2:
        version = 2;
        break;
    case 3:
        version = 1;
        break;
    default:
        return -1;
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
        return -1;
    }
    
    /* Bitrate */
    bitindex = (header & 0xF000) >> 12;
    bitrate = bitrate_table[version-1][layer-1][bitindex];
    if(bitrate == 0)
        return -1;

    /* Sampling frequency */
    freqindex = (header & 0x0C00) >> 10;
    frequency = freqtab[version-1][freqindex];
    if(frequency == 0)
        return -1;

#ifdef DEBUG_VERBOSE
    fprintf(stderr,
            "Version %i, lay %i, biti %i, bitr %i, freqi %i, freq %i\n", 
            version, layer, bitindex, bitrate, freqindex, frequency);
#endif
    entry->version = version;
    entry->layer = layer;
    entry->bitrate = bitrate;
    entry->frequency = frequency;
	
    /* Calculate bytes per frame, calculation depends on layer */
    switch(layer) {
    case 1:
        bpf = bitrate_table[version - 1][layer - 1][bitindex];
        bpf *= 48000;
        bpf /= freqtab[version-1][freqindex] << (version - 1);
        break;
    case 2:
    case 3:
        bpf = bitrate_table[version - 1][layer - 1][bitindex];
        bpf *= 144000;
        bpf /= freqtab[version-1][freqindex] << (version - 1);
        break;
    default:
        bpf = 1;
    }
	
    /* Calculate time per frame */
    tpf = bs[layer] / freqtab[version-1][freqindex] << (version - 1);
        
    /* OK, we have found a frame. Let's see if it has a Xing header */
    if(read(fd, frame, 200) < 0)
        return -1;

    if(frame[32] == 'X' &&
       frame[33] == 'i' &&
       frame[34] == 'n' &&
       frame[35] == 'g')
    {
        /* Yes, it is a VBR file */
        entry->vbr = true;
        
        if(frame[39] & 0x01) /* Is the frame count there? */
        {
            framecount = (frame[40] << 24) | (frame[41] << 16) |
                (frame[42] << 8) | frame[43];

            filetime = framecount * tpf;
        }
        /* We don't care about the file size and the TOC just yet. Maybe
           another time. */
    }

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

    /* Ignore the tag if it is too big */
    if(entry->id3v2len > sizeof(entry->id3v2buf))
        entry->id3v2len = 0;
    
    if(HASID3V2(entry))
        setid3v2title(fd, entry);
    entry->length = getsonglength(fd, entry);

    entry->id3v1len = getid3v1len(fd);
    if(HASID3V1(entry) && !entry->title)
        setid3v1title(fd, entry);

    close(fd);

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
