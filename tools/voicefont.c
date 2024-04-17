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
 * Details at http://www.rockbox.org/wiki/VoiceBuilding
 *
 ****************************************************************************/

#include "voicefont.h"

#include <stdio.h>
#include <string.h>

#define HEADER_SIZE (20)
#define MAX_NAME_LEN (80)
#define MAX_VOICE_ENTRIES (2048)
/* endian conversion macros */
#if defined(__BIG_ENDIAN__)
#define UINT_TO_BE(x) (x)
#else
#define UINT_TO_BE(x) ((((unsigned)(x)>>24) & 0x000000ff) |\
                  (((unsigned)(x)>>8)  & 0x0000ff00) |\
                  (((unsigned)(x)<<8)  & 0x00ff0000) |\
                  (((unsigned)(x)<<24) & 0xff000000))
#endif

int voicefont(FILE* voicefontids,int targetnum,char* filedir, FILE* output, unsigned int version)
{

    int i,j;

    /* two tables, one for normal strings, one for voice-only (>0x8000) */
    static char names[MAX_VOICE_ENTRIES][MAX_NAME_LEN]; /* worst-case space */
    char name[MAX_NAME_LEN]; /* one string ID */
    static int pos[MAX_VOICE_ENTRIES]; /* position of sample */
    static int size[MAX_VOICE_ENTRIES]; /* length of clip */
    int voiceonly[MAX_VOICE_ENTRIES]; /* flag if this is voice only */
    int count = 0;
    int count_voiceonly = 0;
    unsigned int value; /* value to be written to file */
    static unsigned char buffer[65535]; /* clip buffer, allow only 64K */
    int fields;
    char line[255]; /* one line from the .lang file */
    char encfilename1[1024];
    char encfilename2[1024];
    char* encfilename;
    FILE* pEncFile;

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

    if (count > MAX_VOICE_ENTRIES)
    {
        return -1;
    }

    fseek(output, HEADER_SIZE + count*8, SEEK_SET); /* space for header */

    for (i=0; i<count; i++)
    {
        pos[i] = ftell(output);
        sprintf(encfilename1, "%s%s.enc", filedir, names[i]);
        sprintf(encfilename2, "%s%s.wav.enc", filedir, names[i]);
        encfilename = encfilename1;
        pEncFile = fopen(encfilename, "rb");
        if (pEncFile == NULL)
        {   /* alternatively, try the lame default filename */
            encfilename = encfilename2;
            pEncFile = fopen(encfilename, "rb");
            if (pEncFile == NULL)
            {
                printf("enc file %s not found!\n", encfilename1);
                size[i] = 0;
                continue;
            }
        }
        printf("processing %s", encfilename);

        size[i] = fread(buffer, 1, sizeof(buffer), pEncFile);
        fclose(pEncFile);
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
                if (!(voiceonly[i] == 1))
                    continue;
            }

            value = UINT_TO_BE(pos[i]); /* position */
            fwrite(&value, sizeof(value), 1,output);
            value = UINT_TO_BE(size[i]); /* size */
            fwrite(&value, sizeof(value), 1, output);
            printf(": [%d]%s : %s {%x, %x}\n", i,
                   (voiceonly[i] ==1 ? "[V]":""), names[i],
                   UINT_TO_BE(pos[i]), UINT_TO_BE(size[i])); /* debug */
        } /* for i */
    } /* for j */

    fclose(output);

    return 0;


}
#ifndef RBUTIL
int main (int argc, char** argv)
{
    FILE *ids, *output;

    if (argc < 2)
    {
        printf("Makes a Rockbox voicefont from a collection of encoded clips.\n");
        printf("Usage: voicefont <string id list file> <target id> <enc path> <output file>\n");
        printf("\n");
        printf("Example: \n");
        printf("voicefont voicefontids.txt 2 voice/ voicefont.bin\n");
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

    if (voicefont(ids, atoi(argv[2]),argv[3],output, 400) < 0)
    {
        printf("Error too many voicefont entries!\n");
        return -3;
    }
    return 0;
}
#endif
