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
#include <unistd.h>
#include <string.h>
#include <errno.h>

struct mp3entry {
    char *path;
    char *title;
    char *artist;
    char *album;
    int bitrate;
    int frequency;
    int id3v2len;
    int id3v1len;
    int filesize; /* in bytes */
    int length;   /* song length */
};

typedef struct mp3entry mp3entry;

typedef unsigned char bool;
#define TRUE 1
#define FALSE 0

/* Some utility macros used in getsonglength() */
#define CHECKSYNC(x) (((x >> 21) & 0x07FF) == 0x7FF)
#define BYTE0(x) ((x >> 24) & 0xFF)
#define BYTE1(x) ((x >> 16) & 0xFF)
#define BYTE2(x) ((x >> 8)  & 0xFF)
#define BYTE3(x) ((x >> 0)  & 0xFF)

#define UNSYNC(b1,b2,b3,b4) (((b1 & 0x7F) << (3*7)) + \
                             ((b2 & 0x7F) << (2*7)) + \
                             ((b3 & 0x7F) << (1*7)) + \
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
 * Returns: TRUE if a title was found and created, else FALSE
 */
static bool 
setid3v1title(FILE *file, mp3entry *entry) 
{
    char buffer[31];
    int offsets[3] = {-95,-65,-125};
    int i;

    static char keepit[3][32];

    for(i=0;i<3;i++) {
        if(fseek(file, offsets[i], SEEK_END) != 0)
            return FALSE;

        buffer[0]=0;
        fgets(buffer, 31, file);
        stripspaces(buffer);
        
        if(buffer[0]) {
            switch(i) {
            case 0:
                strcpy(keepit[0], buffer);
                entry->artist = keepit[0];
                break;
            case 1:
                strcpy(keepit[1], buffer);
                entry->album = keepit[1];
                break;
            case 2:
                strcpy(keepit[2], buffer);
                entry->title = keepit[2];
                break;
            }
        }
    }

    return TRUE;
}


/*
 * Sets the title of an MP3 entry based on its ID3v2 tag.
 *
 * Arguments: file - the MP3 file to scen for a ID3v2 tag
 *            entry - the entry to set the title in
 *
 * Returns: TRUE if a title was found and created, else FALSE
 */
static void
setid3v2title(FILE *file, mp3entry *entry) 
{
    unsigned int minframesize;
    unsigned int size;
    unsigned int readsize = 0, headerlen;
    char *title = NULL;
    char *artist = NULL;
    char *album = NULL;
    char header[10];
    unsigned short int version;
    static char buffer[512];
    int titlen, artistn, albumn;   
	
    /* 10 = headerlength */
    if(entry->id3v2len < 10)
        return;

    /* Check version */
    fseek(file, 0, SEEK_SET);
    fread(header, sizeof(char), 10, file);
    version = (unsigned short int)header[3];
	
    /* Read all frames in the tag */
    size = entry->id3v2len - 10;

    if(size >= sizeof(buffer))
        size = sizeof(buffer)-1;

    if(size != fread(buffer, sizeof(char), size, file)) {
        free(buffer);
        return;
    }
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
}

/*
 * Calculates the size of the ID3v2 tag.
 *
 * Arguments: file - the file to search for a tag.
 *
 * Returns: the size of the tag or 0 if none was found
 */
static int 
getid3v2len(FILE *file) 
{
    char buf[6];
    int offset;
	
    /* Make sure file has a ID3 tag */
    if((fseek(file, 0, SEEK_SET) != 0) ||
       (fread(buf, sizeof(char), 6, file) != 6) ||
       (strncmp(buf, "ID3", strlen("ID3")) != 0))
        offset = 0;
    /* Now check what the ID3v2 size field says */
    else if(fread(buf, sizeof(char), 4, file) != 4)
        offset = 0;
    else
        offset = UNSYNC(buf[0], buf[1], buf[2], buf[3]) + 10;
	
    return offset;
}

static int
getfilesize(FILE *file)
{
    /* seek to the end of it */
    if(fseek(file, 0, SEEK_END)) 
        return 0; /* unknown */

    return ftell(file);
}

/*
 * Calculates the size of the ID3v1 tag.
 *
 * Arguments: file - the file to search for a tag.
 *
 * Returns: the size of the tag or 0 if none was found
 */
static int 
getid3v1len(FILE *file) 
{
    char buf[3];
    int offset;

    /* Check if we find "TAG" 128 bytes from EOF */
    if((fseek(file, -128, SEEK_END) != 0) ||
       (fread(buf, sizeof(char), 3, file) != 3) ||
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
static int 
getsonglength(FILE *file, mp3entry *entry)
{
    long header;
    int version;
    int layer;
    int bitindex;
    int bitrate;
    int freqindex;
    int frequency;

    long bpf;
    long tpf;

    /* Start searching after ID3v2 header */ 
    if(fseek(file, entry->id3v2len, SEEK_SET))
        return -1;
	
    /* Fill up header with first 24 bits */
    for(version = 0; version < 3; version++) {
        header <<= 8;
        if(!fread(&header, 1, 1, file))
            return -1;
    }
	
    /* Loop trough file until we find a frame header */
 restart:
    do {
        header <<= 8;
        if(!fread(&header, 1, 1, file))
            return -1;
    } while(!CHECKSYNC(header));
	
    /* 
     * Some files are filled with garbage in the beginning, 
     * if the bitrate index of the header is binary 1111 
     * that is a good is a good indicator
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
    entry->bitrate = bitrate;
    entry->frequency = frequency;
	
    /* Calculate bytes per frame, calculation depends on layer */
    switch(layer) {
    case 1:
        bpf = bitrate_table[version - 1][layer - 1][bitindex];
        bpf *= 12000.0 * 4.0;
        bpf /= freqtab[version-1][freqindex] << (version - 1);
        break;
    case 2:
    case 3:
        bpf = bitrate_table[version - 1][layer - 1][bitindex];
        bpf *= 144000;
        bpf /= freqtab[version-1][freqindex] << (version - 1);
        break;
    default:
        bpf = 1.0;
    }
	
    /* Calculate time per frame */
    tpf = bs[layer] / freqtab[version-1][freqindex] << (version - 1);

    /* 
     * Now song length is 
     * ((filesize)/(bytes per frame))*(time per frame) 
     */
    return entry->filesize*tpf/bpf;
}


/*
 * Checks all relevant information (such as ID3v1 tag, ID3v2 tag, length etc)
 * about an MP3 file and updates it's entry accordingly.
 *
 * Arguments: entry - the entry to check and update with the new information
 *
 * Returns: void
 */
bool
mp3info(mp3entry *entry, char *filename) 
{
    FILE *file;
    if((file = fopen(filename, "r")) == NULL)
        return TRUE;

    memset(entry, 0, sizeof(mp3entry));

    entry->path = filename;

    entry->filesize = getfilesize(file);
    entry->id3v2len = getid3v2len(file);
    entry->id3v1len = getid3v1len(file);
    entry->length = getsonglength(file, entry);
    entry->title = NULL;

    if(HASID3V2(entry))
        setid3v2title(file, entry);

    if(HASID3V1(entry) && !entry->title)
        setid3v1title(file, entry);
	
    fclose(file);

    return FALSE;
}

#ifdef DEBUG_STANDALONE

char *secs2str(int ms)
{
    static char buffer[32];
    int secs = ms/1000;
    ms %= 1000;
    sprintf(buffer, "%d:%02d.%d", secs/60, secs%60, ms/100);
    return buffer;
}

int main(int argc, char **argv)
{
    int i;
    for(i=1; i<argc; i++) {
        mp3entry mp3;
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
