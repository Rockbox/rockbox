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

#define OF_FIRMWARE_LEN 0x100000 /* size of the OF file */
#define MPIO_STRING_OFFSET 0xfffe0 /* offset of the version string in OF */
#define BOOTLOADER_MAX_SIZE 0x1f800 /* free space size */

struct mpio_model {
    /* Descriptive name of this model */
    const char* model_name;
    /* Model name used in the Rockbox header in ".mpio" files - these match the
       -add parameter to the "scramble" tool */
    const char* rb_model_name;
    /* Model number used to initialise the checksum in the Rockbox header in
       ".mpio" files - these are the same as MODEL_NUMBER in config-target.h */
    const int rb_model_num;
    /* Strings which indentifies OF version */
    const char* of_model_string;
};

static const struct mpio_model mpio_models[] = {
    [MODEL_HD200] =
        { "MPIO HD200", "hd20", 69, "HD200  HDD Audio Ver113005" },
    [MODEL_HD300] =
        { "MPIO HD300", "hd30", 70, "HD300  HDD Audio Ver113006" },
};


/* MPIO HD200 and HD300 firmware is plain binary image
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

static long checksum(unsigned char* buf, int model, unsigned long length)
{
    unsigned long chksum = model;
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
    int model_index;
    unsigned long file_checksum;
    unsigned char header[8];

    memset(image, 0xff, sizeof(image));

    /* First, read the mpio original firmware into the image */
    f = fopen(infile, "rb");
    if(!f)
    {
        fprintf(stderr, "[ERR]  Can not open %s file for reading\n", infile);
        return -1;
    }

    i = fread(image, 1, OF_FIRMWARE_LEN, f);
    if(i < OF_FIRMWARE_LEN)
    {
        fprintf(stderr, "[ERR]  %s file read error\n", infile);        
        fclose(f);
        return -2;
    }

    fclose(f);

    /* Now check if we have OF file loaded based on presence
     * of the version string in firmware 
     */

    for(model_index = 0; model_index < NUM_MODELS; model_index++)
        if (strcmp(mpio_models[model_index].of_model_string,
            (char*)(image + MPIO_STRING_OFFSET)) == 0)
            break;

    if(model_index == NUM_MODELS)
    {
        fprintf(stderr, "[ERR]  Unknown MPIO original firmware version\n");
        return -3;
    }

    fprintf(stderr, "[INFO] Loading original firmware file for %s\n",
            mpio_models[model_index].model_name);

    /* Now, read the boot loader into the image */
    f = fopen(bootfile, "rb");
    if(!f)
    {
        fprintf(stderr, "[ERR]  Can not open %s file for reading\n", bootfile);
        return -4;
    }

    fprintf(stderr, "[INFO] Loading Rockbox bootloader file\n");

    /* get bootloader size
     * excluding header
     */
    fseek(f, 0, SEEK_END);
    len = ftell(f) - 8;

    if (len > BOOTLOADER_MAX_SIZE)
    {
        fprintf(stderr, "[ERR]  Bootloader doesn't fit in firmware file.\n");
        fprintf(stderr, "[ERR]  This bootloader is %d bytes long\n", len);
        fprintf(stderr, "[ERR]  and maximum allowed size is %d bytes\n",
                BOOTLOADER_MAX_SIZE);
        return -5;
    }

    /* Now check if the place we want to put
     * our bootloader is free
     */
    for(i=0;i<len;i++)
    {
        if (image[origin+i] != 0)
        {
            fprintf(stderr, "[ERR]  Place for bootloader in OF file not empty\n");
            return -6;
        }
    }

    fseek(f, 0, SEEK_SET);

    /* get bootloader header*/
    fread(header,1,8,f);

    if ( memcmp(header + 4, mpio_models[model_index].rb_model_name, 4) != 0 )
    {
        fprintf(stderr, "[ERR]  Original firmware and rockbox bootloader mismatch!\n");
        fprintf(stderr, "[ERR]  Double check that you have bootloader for %s\n",
                mpio_models[model_index].model_name);
        return -7;
    }

    /* omit header */
    fseek(f, 8, SEEK_SET);

    i = fread(image + origin, 1, len, f);
    if(i < len)
    {
        fprintf(stderr, "[ERR]  %s file read error\n", bootfile);
        fclose(f);
        return -8;
    }

    fclose(f);

    /* calculate checksum and compare with data
     * from header
     */
    file_checksum = checksum(image + origin, mpio_models[model_index].rb_model_num, len);

    if ( file_checksum != get_uint32be(header) )
    {
        fprintf(stderr,"[ERR]  Bootloader checksum error\n");
        return -9;
    }

    f = fopen(outfile, "wb");
    if(!f)
    {
        fprintf(stderr, "[ERR]  Can not open %s file for writing\n" ,outfile);
        return -10;
    }

    fprintf(stderr, "[INFO] Patching reset vector\n");

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
    if(i < OF_FIRMWARE_LEN)
    {
        fprintf(stderr,"[ERR]  %s file write error\n", outfile);
        fclose(f);
        return -11;
    }

    fprintf(stderr,"[INFO] Wrote 0x%x bytes in %s\n", OF_FIRMWARE_LEN, outfile);
    fprintf(stderr,"[INFO] Patching succeeded!\n");
   
    fclose(f);
    
    return 0;
}
