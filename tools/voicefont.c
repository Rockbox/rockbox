/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Jörg Hohensohn
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * A tool to generate the Rockbox "voicefont", a collection of all the UI
 * strings. 
 * 
 * Details at http://www.rockbox.org/twiki/bin/view/Main/VoiceBuilding
 *
 ****************************************************************************/

#include "voicefont.h" 
 
#include <stdio.h>
#include <string.h>

#define HEADER_SIZE 20

/* endian conversion macros */
#if defined(__BIG_ENDIAN__)
#define UINT_TO_BE(x) (x)
#else
#define UINT_TO_BE(x) ((((unsigned)(x)>>24) & 0x000000ff) |\
                  (((unsigned)(x)>>8)  & 0x0000ff00) |\
                  (((unsigned)(x)<<8)  & 0x00ff0000) |\
                  (((unsigned)(x)<<24) & 0xff000000))
#endif

/* bitswap audio bytes, LSB becomes MSB and vice versa */
int BitswapAudio (unsigned char* pDest, unsigned char* pSrc, size_t len)
{
    static const unsigned char Lookup[256] = 
    {
        0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
        0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
        0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
        0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
        0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
        0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
        0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
        0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
        0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
        0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
        0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
        0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
        0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
        0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
        0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
        0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF,
    };

    while (len--)
        *pDest++ = Lookup[*pSrc++];

    return 0;
}

int voicefont(FILE* voicefontids,int targetnum,char* filedir, FILE* output, unsigned int version)
{

    int i,j;

    /* two tables, one for normal strings, one for voice-only (>0x8000) */
    static char names[1000][80]; /* worst-case space */
    char name[80]; /* one string ID */
    static int pos[1000]; /* position of sample */
    static int size[1000]; /* length of clip */
    int voiceonly[1000]; /* flag if this is voice only */
    int count = 0;
    int count_voiceonly = 0;
    unsigned int value; /* value to be written to file */
    static unsigned char buffer[65535]; /* clip buffer, allow only 64K */
    int fields;
    char line[255]; /* one line from the .lang file */
    char mp3filename1[1024];
    char mp3filename2[1024];
    char* mp3filename;
    FILE* pMp3File;
    int do_bitswap_audio = 0;


    /* We bitswap the voice file only SH based archos players, target IDs
       equal to or lower than 8. See the "Target id:" line in rockbox-info.txt
       or the target_id value in configure. */
    if (targetnum <= 8)
        do_bitswap_audio = 1;

    memset(voiceonly, 0, sizeof(voiceonly));
    while (!feof(voicefontids))
    {
        if (!fgets(line, sizeof(line), voicefontids))
            break;
        if (line[0] == '#') /* comment */
            continue;

        fields = sscanf(line, " id: %s", name);
        if (fields == 1)
        {
            strcpy(names[count], name);
            if (strncmp("VOICE_", name, 6) == 0) /* voice-only id? */
            {
                count_voiceonly++;
                voiceonly[count] = 1;
            }
            count++; /* next entry started */
            continue;
        }
    }
    fclose(voicefontids);

    fseek(output, HEADER_SIZE + count*8, SEEK_SET); /* space for header */

    for (i=0; i<count; i++)
    {
        pos[i] = ftell(output);
        sprintf(mp3filename1, "%s%s.mp3", filedir, names[i]);
        sprintf(mp3filename2, "%s%s.wav.mp3", filedir, names[i]);
        mp3filename = mp3filename1;
        pMp3File = fopen(mp3filename, "rb");
        if (pMp3File == NULL)
        {   /* alternatively, try the lame default filename */
            mp3filename = mp3filename2;
            pMp3File = fopen(mp3filename, "rb");
            if (pMp3File == NULL)
            {
                printf("mp3 file %s not found!\n", mp3filename1);
                size[i] = 0;
                continue;
            }
        }
        printf("processing %s", mp3filename);

        size[i] = fread(buffer, 1, sizeof(buffer), pMp3File);
        fclose(pMp3File);
        if (do_bitswap_audio)
            BitswapAudio(buffer, buffer, size[i]);
        fwrite(buffer, 1, size[i], output);

        printf(": %d %s %d\n", i, names[i], size[i]); /* debug */
    } /*  for i */


    fseek(output, 0, SEEK_SET);

    /* Create the file format: */

    /* 1st 32 bit value in the file is the version number    */
    value = UINT_TO_BE(version);
    fwrite(&value, sizeof(value), 1, output);

    /* 2nd 32 bit value in the file is the id number for the target
       we made the voce file for */
    value = UINT_TO_BE(targetnum);
    fwrite(&value, sizeof(value), 1, output);

    /* 3rd 32 bit value in the file is the header size (= 1st table position) */
    value = UINT_TO_BE(HEADER_SIZE); /* version, target id, header size, number1, number2 */
    fwrite(&value, sizeof(value), 1, output);

    /* 4th 32 bit value in the file is the number of clips in 1st table   */
    value = UINT_TO_BE(count-count_voiceonly);
    fwrite(&value, sizeof(value), 1, output);

    /* 5th bit value in the file is the number of clips in 2nd table */
    value = UINT_TO_BE(count_voiceonly);
    fwrite(&value, sizeof(value), 1, output);

    /* then followed by offset/size pairs for each clip */
    for (j=0; j<2; j++) /* now 2 tables */
    {
        for (i=0; i<count; i++)
        {
            if (j == 0) /* first run, skip the voice only ones */
            {
                if (voiceonly[i] == 1)
                    continue;
            }
            else /* second run, skip the non voice only ones */
            {
                if (!voiceonly[i] == 1)
                    continue;
            }

            value = UINT_TO_BE(pos[i]); /* position */
            fwrite(&value, sizeof(value), 1,output);
            value = UINT_TO_BE(size[i]); /* size */
            fwrite(&value, sizeof(value), 1, output);
        } /* for i */
    } /* for j */


    /*
     * after this the actual bitswapped mp3 data follows,
     * which we already have written, see above.
     */

    fclose(output);

    return 0;

    
}
#ifndef RBUTIL
int main (int argc, char** argv)
{
    FILE *ids, *output;

    if (argc < 2)
    {
        printf("Makes a Rockbox voicefont from a collection of mp3 clips.\n");
        printf("Usage: voicefont <string id list file> <target id> <mp3 path> <output file>\n");
        printf("\n");
        printf("Example: \n");
        printf("voicefont voicefontids.txt 2 voice\\ voicefont.bin\n");
        return -1;
    }
    
    ids = fopen(argv[1], "r");
    if (ids == NULL)
    {
        printf("Error opening language file %s\n", argv[1]);
        return -2;
    }

    output = fopen(argv[4], "wb");
    if (output == NULL)
    {
        printf("Error opening output file %s\n", argv[4]);
        return -2;
    }
    
    voicefont(ids, atoi(argv[2]),argv[3],output, 400);
    return 0;
}
#endif

