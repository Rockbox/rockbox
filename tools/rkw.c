/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2012 Marcin Bukat
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
#include <stdint.h>

#define RKLD_MAGIC 0x4c44524b
#define RKW_HEADER_SIZE 0x2c

/* slightly modified version from crc32.c in rockbox */
static uint32_t rkw_crc32(const void *src, uint32_t len)
{
    const unsigned char *buf = (const unsigned char *)src;

    /* polynomial 0x04c10db7 */
    static const uint32_t crc32_lookup[16] =
    {   /* lookup table for 4 bits at a time is affordable */
        0x00000000, 0x04C10DB7, 0x09821B6E, 0x0D4316D9,
        0x130436DC, 0x17C53B6B, 0x1A862DB2, 0x1E472005,
        0x26086DB8, 0x22C9600F, 0x2F8A76D6, 0x2B4B7B61,
        0x350C5B64, 0x31CD56D3, 0x3C8E400A, 0x384F4DBD
    };

    uint32_t crc32 = 0;
    unsigned char byte;
    uint32_t t;

    while (len--)
    {
        byte = *buf++; /* get one byte of data */

        /* upper nibble of our data */
        t = crc32 >> 28; /* extract the 4 most significant bits */
        t ^= byte >> 4; /* XOR in 4 bits of data into the extracted bits */
        crc32 <<= 4; /* shift the CRC register left 4 bits */
        crc32 ^= crc32_lookup[t]; /* do the table lookup and XOR the result */

        /* lower nibble of our data */
        t = crc32 >> 28; /* extract the 4 most significant bits */
        t ^= byte & 0x0F; /* XOR in 4 bits of data into the extracted bits */
        crc32 <<= 4; /* shift the CRC register left 4 bits */
        crc32 ^= crc32_lookup[t]; /* do the table lookup and XOR the result */
    }

    return crc32;
}

static void int2le(unsigned int val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
}

int rkw_encode(char *iname, char *oname, unsigned long modelnum)
{
    size_t len;
    int length;
    int rkwlength;
    FILE *file;
    uint32_t binary_crc, header_crc;
    unsigned char *outbuf;

    file = fopen(iname, "rb");
    if (!file)
    {
       perror(iname);
       return -1;
    }

    fseek(file,0,SEEK_END);
    length = ftell(file);
    
    fseek(file,0,SEEK_SET);

    /* length of the RKW header + binary length + 4 bytes of CRC */
    rkwlength = (length + RKW_HEADER_SIZE + 4);

    outbuf = malloc(rkwlength);

    if (!outbuf)
    {
       printf("out of memory!\n");
       fclose(file);
       return -1;
    }

    /* Clear the buffer to zero */
    memset(outbuf, 0, rkwlength);

    /* Build the RKW header */
    int2le(RKLD_MAGIC, outbuf);                /* magic */
    int2le(RKW_HEADER_SIZE, outbuf+0x04);      /* header size */
    int2le(0x60000000, outbuf+0x08);           /* base address */
    int2le(0x60000000, outbuf+0x0c);           /* load address */
    int2le(0x60000000+length, outbuf+0x10);    /* end address */
    int2le(0x6035a5e4, outbuf+0x14);           /* points to some unknown struct */
    int2le(modelnum, outbuf+0x18);             /* reserved (we abuse the format
                                                * to store modelnum here
                                                */
    int2le(0, outbuf+0x1c);                    /* reserved */
    int2le(0x60000000, outbuf+0x20);           /* entry point */
    int2le(0xe0000000, outbuf+0x24);           /* flags */

    header_crc = rkw_crc32(outbuf, RKW_HEADER_SIZE - 4);

    int2le(header_crc, outbuf+0x28);           /* header CRC */

    /* Copy the binary */
    len = fread(outbuf + RKW_HEADER_SIZE, 1, length, file);
    if(len < (size_t)length)
    {
        perror(iname);
        free(outbuf);
        fclose(file);
        return -2;
    }
    fclose(file);

    /* calc binary CRC and put at the end */
    binary_crc = rkw_crc32 (outbuf + RKW_HEADER_SIZE, length);
    int2le(binary_crc, outbuf + rkwlength - 4);

    file = fopen(oname, "wb");
    if (!file)
    {
        perror(oname);
        free(outbuf);
        return -3;
    }
    
    len = fwrite(outbuf, 1, rkwlength, file);
    if(len < (size_t)length)
    {
        perror(oname);
        fclose(file);
        free(outbuf);
        return -4;
    }

    fclose(file);
    free(outbuf);
    fprintf(stderr, "File encoded successfully\n" );

    return 0;
}
