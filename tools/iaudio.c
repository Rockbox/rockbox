/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Christian Gmeiner
 *
 * This particular source code file is licensed under the X11 license. See the
 * bottom of the COPYING file for details on this license.
 *
 ****************************************************************************/
 
/* This little application updates the checksum of a modifized iAudio x5
   firmware bin.
   And this is how it works:

   The byte at offset 0x102b contains the 8-bit sum of all the bytes starting with the one at 0x1030.    
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECKSUM_BIT    0x102b
#define CHECKSUM_START	0x1030

void usage(void) {

    printf("usage: iaudio <input file> <output file>\n");
    printf("\n\nThis tool updates the checksum of an iaudio fw bin\n");
    exit(1);
}

int main (int argc, char* argv[]) {

    char byte = '\0';
    char checksum = '\0';
    unsigned long length, i;
    unsigned char* inbuf;
    char* iname = argv[1];
    char* oname = argv[2];
    FILE* pFile;

    if (argc < 2) {
        usage();
    }

    /* open file */
    pFile = fopen(iname, "rb");
    if (!pFile) {
       perror(oname);
       return -1;
    }

    /* print old checksum */
    fseek (pFile, CHECKSUM_BIT, SEEK_SET);
    byte = fgetc(pFile);
    printf("Old checksum: 0x%02x\n", byte & 0xff);

    /* get file size*/
    fseek(pFile,0,SEEK_END);
    length = ftell(pFile);
    fseek(pFile,0,SEEK_SET);

    /* try to allocate memory */
    inbuf = malloc(length);
    if (!inbuf) {
       printf("out of memory!\n");
       return -1;
    }

    /* read file */
    i = fread(inbuf, 1, length, pFile);
    if (!i) {
       perror(iname);
       return -1;
    }
    fclose(pFile);

    /* calculate new checksum */
    for (i = CHECKSUM_START; i < length; i++) {
        checksum += inbuf[i];
    }
    printf("New checksum: 0x%02x\n", checksum & 0xff);

    /* save new checksum */
    inbuf[CHECKSUM_BIT] = (unsigned char) checksum;

    /* save inbuf */
    pFile = fopen(oname,"wb");
    if (!pFile) {
       perror(oname);
       return -1;
    }

    i = fwrite(inbuf, 1, length, pFile);
    if (!i) {
        perror(oname);
        return -1;
    }
    fclose(pFile);

    return 0;
}
