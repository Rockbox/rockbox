/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 by Marcin Bukat
 *
 * code taken mostly from mkboot.c 
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mkmpioboot.h"

#define OF_FIRMWARE_LEN 0x100000 /* size of HD200_UPG.SYS file */
#define MPIO_STRING_OFFSET 0xfffe0

/* We support only 1.30.05 version of OF for now */
static char *mpio_string = "HD200  HDD Audio Ver113005";

/* MPIO HD200 firmware is plain binary image
 * 4 bytes of initial SP (loaded on reset)
 * 4 bytes of initial PC (loaded on reset)
 * binary image with entry point 0x00000008
 *
 * We put our bootloader code at 0x000e0000
 * and patch reset vector to jump directly
 * into our code on reset
 */

static unsigned char image[OF_FIRMWARE_LEN];

static unsigned int get_uint32be(unsigned char* p)
{
    return ((p[0] << 24) | (p[1] << 16) | (p[2]<<8) | p[3]);
}

static long checksum(unsigned char* buf, unsigned long length)
{
    unsigned long chksum = 69; /* MPIO HD200 model number */
    unsigned long i;
    
    if(buf == NULL)
        return -1;

    for (i = 0; i < length; i++)
    {
        chksum += *buf++;
    }

return chksum;
}

int mkmpioboot(const char* infile, const char* bootfile, const char* outfile, int origin)
{
    FILE *f;
    int i;
    int len;
    unsigned long file_checksum;
    unsigned char header_checksum[4];

    memset(image, 0xff, sizeof(image));

    /* First, read the mpio original firmware into the image */
    f = fopen(infile, "rb");
    if(!f) {
        perror(infile);
        return -1;
    }

    i = fread(image, 1, OF_FIRMWARE_LEN, f);
    if(i < OF_FIRMWARE_LEN) {
        perror(infile);
        fclose(f);
        return -2;
    }

    fclose(f);

    /* Now check if we have OF file loaded based on presence
     * of the version string in firmware 
     */

    if (strcmp((char*)(image + MPIO_STRING_OFFSET),mpio_string) != 0)
    {
        perror("Loaded firmware file does not look like MPIO OF file!");
        return -3;
    }

    /* Now, read the boot loader into the image */
    f = fopen(bootfile, "rb");
    if(!f) {
        perror(bootfile);
        fclose(f);
        return -4;
    }

    /* get bootloader size
     * excluding header
     */
    fseek(f, 0, SEEK_END);
    len = ftell(f) - 8;

    /* Now check if the place we want to put
     * our bootloader is free
     */
    for(i=0;i<len;i++)
    {
        if (image[origin+i] != 0)
        {
            perror("Place for bootloader in OF file not empty");
            return -5;
        }
    }

    fseek(f, 0, SEEK_SET);

    /* get bootloader checksum from the header*/
    fread(header_checksum,1,4,f);

    /* omit header */
    fseek(f, 8, SEEK_SET);

    i = fread(image + origin, 1, len, f);
    if(i < len) {
        perror(bootfile);
        fclose(f);
        return -6;
    }

    fclose(f);

    /* calculate checksum and compare with data
     * from header
     */
    file_checksum = checksum(image + origin, len);

    if ( file_checksum != get_uint32be(header_checksum) )
    {
        printf("Bootloader checksum error\n");
        return -7;
    }

    f = fopen(outfile, "wb");
    if(!f) {
        perror(outfile);
        return -8;
    }

    /* Patch the stack pointer address */    
    image[0] = image[origin + 0];
    image[1] = image[origin + 1];
    image[2] = image[origin + 2];
    image[3] = image[origin + 3];

    /* Patch the reset vector to start the boot loader */
    image[4] = image[origin + 4];
    image[5] = image[origin + 5];
    image[6] = image[origin + 6];
    image[7] = image[origin + 7];

    i = fwrite(image, 1, OF_FIRMWARE_LEN, f);
    if(i < OF_FIRMWARE_LEN) {
        perror(outfile);
        fclose(f);
        return -9;
    }

    printf("Wrote 0x%x bytes in %s\n", OF_FIRMWARE_LEN, outfile);
    
    fclose(f);
    
    return 0;
}
