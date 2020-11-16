/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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
#include "mkboot.h"

#define SECTOR_SIZE 0x200
#define RAW_IMAGE_SIZE 0x400000
#define TOTAL_IMAGE_SIZE (RAW_IMAGE_SIZE + HEADER2_SIZE)
#define ALIGNED_IMAGE_SIZE (TOTAL_IMAGE_SIZE + SECTOR_SIZE - (TOTAL_IMAGE_SIZE % SECTOR_SIZE))
#define HEADER1_SIZE SECTOR_SIZE
#define HEADER2_SIZE 0x20

#ifndef RBUTIL
static void usage(void)
{
    printf("usage: mkboot <target> <firmware file> <boot file> <output file>\n");
    printf("available targets:\n"
           "\t-h100     Iriver H1x0\n"
           "\t-h300     Iriver H3x0\n"
           "\t-iax5     iAudio X5\n"
           "\t-iam5     iAudio M5\n");

    exit(1);
}
#endif

#ifndef RBUTIL
int main(int argc, char *argv[])
{
    if(argc != 5)
    {
        usage();
        return 1;
    }

    if ( ! strcmp(argv[1], "-h100"))
        return mkboot_iriver(argv[2], argv[3], argv[4], 0x1f0000);
    
    if ( ! strcmp(argv[1], "-h300"))
        return mkboot_iriver(argv[2], argv[3], argv[4], 0x3f0000);
    
    if ( ! strcmp(argv[1], "-iax5"))
        return mkboot_iaudio(argv[2], argv[3], argv[4], 0);
        
    if ( ! strcmp(argv[1], "-iam5"))
        return mkboot_iaudio(argv[2], argv[3], argv[4], 1);

    usage();
    return 1;
}
#endif

/*
 * initial header size plus
 * the rounded up size (includes the raw data and its length header) plus
 * the checksums for the raw data and its length header
 */
static unsigned char image[ALIGNED_IMAGE_SIZE + HEADER1_SIZE + ALIGNED_IMAGE_SIZE / SECTOR_SIZE];

int mkboot_iriver(const char* infile, const char* bootfile, const char* outfile, int origin)
{
    FILE *f;
    int i;
    int len;
    int actual_length, total_length, binary_length, num_chksums;

    memset(image, 0xff, sizeof(image));

    /* First, read the iriver original firmware into the image */
    f = fopen(infile, "rb");
    if(!f) {
        perror(infile);
        return -1;
    }

    i = fread(image, 1, 16, f);
    if(i < 16) {
        perror(infile);
        fclose(f);
        return -2;
    }

    /* This is the length of the binary image without the scrambling
       overhead (but including the ESTFBINR header) */
    binary_length = image[4] + (image[5] << 8) +
        (image[6] << 16) + (image[7] << 24);

    /* Read the rest of the binary data, but not the checksum block */
    len = binary_length+0x200-16;
    i = fread(image+16, 1, len, f);
    if(i < len) {
        perror(infile);
        fclose(f);
        return -3;
    }
    
    fclose(f);

    /* Now, read the boot loader into the image */
    f = fopen(bootfile, "rb");
    if(!f) {
        perror(bootfile);
        fclose(f);
        return -4;
    }

    fseek(f, 0, SEEK_END);
    len = ftell(f);

    fseek(f, 0, SEEK_SET);

    i = fread(image+0x220 + origin, 1, len, f);
    if(i < len) {
        perror(bootfile);
        fclose(f);
        return -5;
    }

    fclose(f);

    f = fopen(outfile, "wb");
    if(!f) {
        perror(outfile);
        return -6;
    }

    /* Patch the reset vector to start the boot loader */
    image[0x220 + 4] = image[origin + 0x220 + 4];
    image[0x220 + 5] = image[origin + 0x220 + 5];
    image[0x220 + 6] = image[origin + 0x220 + 6];
    image[0x220 + 7] = image[origin + 0x220 + 7];

    /* This is the actual length of the binary, excluding all headers */
    actual_length = origin + len;

    /* Patch the ESTFBINR header */
    image[0x20c] = (actual_length >> 24) & 0xff;
    image[0x20d] = (actual_length >> 16) & 0xff;
    image[0x20e] = (actual_length >> 8) & 0xff;
    image[0x20f] = actual_length & 0xff;
    
    image[0x21c] = (actual_length >> 24) & 0xff;
    image[0x21d] = (actual_length >> 16) & 0xff;
    image[0x21e] = (actual_length >> 8) & 0xff;
    image[0x21f] = actual_length & 0xff;

    /* This is the length of the binary, including the ESTFBINR header and
       rounded up to the nearest 0x200 boundary */
    binary_length = (actual_length + 0x20 + 0x1ff) & 0xfffffe00;

    /* The number of checksums, i.e number of 0x200 byte blocks */
    num_chksums = binary_length / 0x200;

    /* The total file length, including all headers and checksums */
    total_length = binary_length + num_chksums + 0x200;

    /* Patch the scrambler header with the new length info */
    image[0] = total_length & 0xff;
    image[1] = (total_length >> 8) & 0xff;
    image[2] = (total_length >> 16) & 0xff;
    image[3] = (total_length >> 24) & 0xff;
    
    image[4] = binary_length & 0xff;
    image[5] = (binary_length >> 8) & 0xff;
    image[6] = (binary_length >> 16) & 0xff;
    image[7] = (binary_length >> 24) & 0xff;
    
    image[8] = num_chksums & 0xff;
    image[9] = (num_chksums >> 8) & 0xff;
    image[10] = (num_chksums >> 16) & 0xff;
    image[11] = (num_chksums >> 24) & 0xff;
    
    i = fwrite(image, 1, total_length, f);
    if(i < total_length) {
        perror(outfile);
        fclose(f);
        return -7;
    }

    printf("Wrote 0x%x bytes in %s\n", total_length, outfile);
    
    fclose(f);
    
    return 0;
}

/* iAudio firmware update file header size */
#define HEADER_SIZE 0x1030
/* Address of flash contents that get overwritten by a firmware update.
 * Contents before this address contain the preloader and are not affected
 * by a firmware update.
 * -> Firmware update file contents starting at offset HEADER_SIZE end up
 * in flash at address FLASH_START
 */
#define FLASH_START        0x00010000
/* Start of unused space in original firmware (flash address, not file
 * offset!) where we patch in the Rockbox loader */
#define ROCKBOX_BOOTLOADER 0x00150000
/* End of unused space in original firmware */
#define BOOTLOADER_LIMIT   0x00170000 

/* Patch the Rockbox bootloader into free space in the original firmware
 * (starting at 0x150000). The preloader starts execution of the OF at
 * 0x10000 which normally contains a jsr 0x10010. We also patch this to
 * do a jsr 0x150000 to the Rockbox dual boot loader instead. If it then
 * decides to start the OF instead of Rockbox, it simply does a jmp
 * 0x10010 instead of loading Rockbox from disk.
 */
int mkboot_iaudio(const char* infile, const char* bootfile, const char* outfile, int model_nr)
{
    size_t flength, blength;
    unsigned char *bbuf, *fbuf, *p;
    const unsigned char fsig[] = {
        0x4e, 0xb9, 0x00, 0x01, 0x00, 0x10 };               /* jsr 0x10010 */
    unsigned char bsig[2][8] = {
        /* dualboot signatures */
        { 0x60, 0x06, 0x44, 0x42, 0x69, 0x61, 0x78, 0x35 }, /* X5 */
        { 0x60, 0x06, 0x44, 0x42, 0x69, 0x61, 0x6d, 0x35 }, /* M5 */ };
    FILE *ffile, *bfile, *ofile;
    unsigned char sum = 0;
    int i;

    /* read input files */
    if ((bfile = fopen(bootfile, "rb")) == NULL) {
        perror("Cannot open Rockbox bootloader file.\n");
        return 1;
    }

    fseek(bfile, 0, SEEK_END);
    blength = ftell(bfile);
    fseek(bfile, 0, SEEK_SET);

    if (blength + ROCKBOX_BOOTLOADER >= BOOTLOADER_LIMIT) {
        fprintf(stderr, "Rockbox bootloader is too big.\n");
        return 1;
    }
 
    if ((ffile = fopen(infile, "rb")) == NULL) {
        perror("Cannot open original firmware file.");
        return 1;
    }
  
    fseek(ffile, 0, SEEK_END);
    flength = ftell(ffile);
    fseek(ffile, 0, SEEK_SET);

    bbuf = malloc(blength);
    fbuf = malloc(flength);

    if (!bbuf || !fbuf) {
        fprintf(stderr, "Out of memory.\n");
        return 1;
    }

    if (   fread(bbuf, 1, blength, bfile) < blength
        || fread(fbuf, 1, flength, ffile) < flength) {
        fprintf(stderr, "Read error.\n");
        return 1;
    }
    fclose(bfile);
    fclose(ffile);

    /* verify format of input files */
    if (blength < 0x10 || memcmp(bbuf, bsig[model_nr], sizeof(bsig[0]))) {
        fprintf(stderr, "Rockbox bootloader format error (is it bootloader.bin?).\n");
        return 1;
    }
    if (flength < HEADER_SIZE-FLASH_START+BOOTLOADER_LIMIT
        || memcmp(fbuf+HEADER_SIZE, fsig, sizeof(fsig))) {
        fprintf(stderr, "Original firmware format error.\n");
        return 1;
    }

    /* verify firmware is not overrun */
    for (i = ROCKBOX_BOOTLOADER; i < BOOTLOADER_LIMIT; i++) {
        if (fbuf[HEADER_SIZE-FLASH_START+i] != 0xff) {
            fprintf(stderr, "Original firmware has grown too much.\n");
            return 1;
        }
    }

    /* change jsr 0x10010 to jsr DUAL_BOOTLOADER */
    p = fbuf + HEADER_SIZE + 2;
    *p++ = (ROCKBOX_BOOTLOADER >> 24) & 0xff;
    *p++ = (ROCKBOX_BOOTLOADER >> 16) & 0xff;
    *p++ = (ROCKBOX_BOOTLOADER >>  8) & 0xff;
    *p++ = (ROCKBOX_BOOTLOADER      ) & 0xff;

    p = fbuf + HEADER_SIZE + ROCKBOX_BOOTLOADER - FLASH_START;
    memcpy(p, bbuf, blength);

    /* recalc checksum */
    for (i = HEADER_SIZE; (size_t)i < flength; i++)
        sum += fbuf[i];
    fbuf[0x102b] = sum;

    /* write output */
    if ((ofile = fopen(outfile, "wb")) == NULL) {
        perror("Cannot open output file");
        return 1;
    }
    if (fwrite(fbuf, 1, flength, ofile) < flength) {
        fprintf(stderr, "Write error.\n");
        return 1;
    }
    fclose(ofile);
    free(bbuf);
    free(fbuf);

    return 0;
}
