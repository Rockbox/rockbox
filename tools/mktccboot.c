/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Dave Chapman
 *
 * Based on mkboot, Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include "telechips.h"

/*

Append a Rockbox bootloader to a Telechips original firmware file.

The first instruction in a TCC firmware file is always of the form:

   ldr     pc, [pc, #xxx]

where [pc, #xxx] is the entry point of the firmware - e.g. 0x20000020

mktccboot appends the Rockbox bootloader to the end of the original
firmware image and replaces the contents of [pc, #xxx] with the entry
point of our bootloader - i.e. the length of the original firmware plus
0x20000000.

It then stores the original entry point from [pc, #xxx] in a fixed
offset in the Rockbox boootloader, which is used by the bootloader to
dual-boot.

Finally, mktccboot corrects the length and CRCs in the main firmware
header, creating a new legal firmware file which can be installed on
the device.

*/

/* win32 compatibility */

#ifndef O_BINARY
#define O_BINARY 0
#endif

static void put_uint32le(uint32_t x, unsigned char* p)
{
    p[0] = x & 0xff;
    p[1] = (x >> 8) & 0xff;
    p[2] = (x >> 16) & 0xff;
    p[3] = (x >> 24) & 0xff;
}

static uint32_t get_uint32le(unsigned char* p)
{
    return (p[3] << 24) | (p[2] << 16) | (p[1]<<8) | p[0];
}

void usage(void)
{
    printf("Usage: mktccboot <firmware file> <boot file> <output file>\n");

    exit(1);
}

off_t filesize(int fd) {
    struct stat buf;

    if (fstat(fd,&buf) < 0) {
        perror("[ERR]  Checking filesize of input file");
        return -1;
    } else {
        return(buf.st_size);
    }
}


int main(int argc, char *argv[])
{
    char *infile, *bootfile, *outfile;
    int fdin, fdboot,fdout;
    int n;
    int inlength,bootlength;
    uint32_t ldr;
    unsigned char* image;
    int origoffset;
    
    if(argc < 3) {
        usage();
    }

    infile = argv[1];
    bootfile = argv[2];
    outfile = argv[3];

    fdin = open(infile, O_RDONLY|O_BINARY);
    if (fdin < 0)
    {
        perror(infile);
    }

    fdboot = open(bootfile, O_RDONLY|O_BINARY);
    if (fdboot < 0)
    {
        perror(bootfile);
    }

    inlength = filesize(fdin);
    bootlength = filesize(fdboot);

    image = malloc(inlength + bootlength);

    if (image==NULL)
    {
        printf("[ERR]  Could not allocate memory, aborting\n");
        return 1;
    }

    n = read(fdin, image, inlength);
    if (n != inlength)
    {
        printf("[ERR] Could not read from %s\n",infile);
        return 2;
    }

    n = read(fdboot, image + inlength, bootlength);
    if (n != bootlength)
    {
        printf("[ERR] Could not read from %s\n",bootfile);
        return 3;
    }

    ldr = get_uint32le(image);

    /* TODO: Verify it's a LDR instruction */
    origoffset = (ldr&0xfff) + 8;

    printf("original firmware entry point: 0x%08x\n",get_uint32le(image + origoffset));
    printf("New entry point: 0x%08x\n",0x20000000 + inlength + 8);

    /* Save the original firmware entry point at the start of the bootloader image */
    put_uint32le(get_uint32le(image + origoffset),image+inlength);
    put_uint32le(0x20000000 + inlength,image + inlength + 4);

    /* Change the original firmware entry point to the third word in our bootloader */
    put_uint32le(0x20000000 + inlength + 8,image+origoffset);


    telechips_encode_crc(image, inlength + bootlength);

    fdout = open(outfile, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0644);
    if (fdout < 0)
    {
        perror(bootfile);
    }

    n = write(fdout, image, inlength + bootlength);
    if (n != inlength + bootlength)
    {
        printf("[ERR] Could not write output file %s\n",outfile);
        return 3;
    }

    close(fdin);
    close(fdboot);
    close(fdout);
    
    return 0;
}
