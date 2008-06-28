/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Telechips firmware checksum support for scramble
 *
 * Copyright (C) 2007 Dave Chapman
 *
 * Thanks to Hein-Pieter van Braam for his work in identifying the
 * CRC algorithm used.
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>

static uint32_t crctable[256];

/* Simple implementation of a function to reverse the bottom n bits in x */
static uint32_t bitreverse(uint32_t x,int n)
{
    int i;
    uint32_t mask = 1<<(n-1);
    uint32_t res = 0;

    for (i=0; i<n; i++)
    {
        if (x & 1)
            res |=  mask;

        x >>= 1;
        mask >>= 1;
    }
    return res;
}

/* Generate a lookup table for a reverse CRC32 */
static void gentable(uint32_t poly)
{
    int i;
    uint32_t r;
    uint32_t index;

    for (index = 0; index < 256; index++)
    {
        r = bitreverse(index,8) << 24;
        for (i=0; i<8; i++)
        {
            if (r & (1 << 31))
                r = (r << 1) ^ poly;
            else
                r<<=1;
        }
        crctable[index] = bitreverse(r,32);
    }
}

/* Perform a reverse CRC32 */
static uint32_t calc_crc(unsigned char *message, int size)
{
    uint32_t crc = 0;
    int i;

    for (i=0; i < size; i++){
        if ((i < 0x10) || (i >= 0x18)) {
            crc = crctable[((crc ^ (message[i])) & 0xff)] ^ (crc >> 8);
        }
    }

    return crc;
}

/* Endian-safe functions to read/write a 32-bit little-endian integer */

static uint32_t get_uint32le(unsigned char* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static void put_uint32le(unsigned char* p, uint32_t x)
{
    p[0] = x & 0xff;
    p[1] = (x >> 8) & 0xff;
    p[2] = (x >> 16) & 0xff;
    p[3] = (x >> 24) & 0xff;
}

/* A simple checksum - seems to be used by the TCC76x firmwares */
void telechips_encode_sum(unsigned char* buf, int length)
{
    uint32_t sum;
    int i;

    /* Set checksum field to 0 */
    put_uint32le(buf + 0x10, 0);

    /* Perform a simple sum, treating the file as a series of 32-bit
       little-endian integers */
    sum = 0;
    for (i=0; i < length; i+=4) {
        sum += get_uint32le(buf + i);
    }
    /* Negate the sum - this means that the sum of the whole file
       (including this value) will be equal to zero */
    sum = -sum;

    /* Set the checksum field */
    put_uint32le(buf + 0x10, sum);
}


/* Two reverse CRC32 checksums - seems to be used by the TCC77x firmwares */
void telechips_encode_crc(unsigned char* buf, int length)
{
    uint32_t crc1,crc2;

    /* Generate the CRC table */
    gentable(0x8001801BL);

    /* Clear the existing CRC values */
    put_uint32le(buf+0x10, 0);
    put_uint32le(buf+0x18, 0);

    /* Write the length */
    put_uint32le(buf+0x1c, length);

    /* Calculate the first CRC - over the entire file */
    crc1 = calc_crc(buf, length);

    /* What happens next depends on the filesize */
    if (length >= 128*1024)
    {
        put_uint32le(buf+0x18, crc1);

        crc2 = calc_crc(buf, 128*1024);
        put_uint32le(buf+0x10, crc2);
    } else {
        put_uint32le(buf+0x10, crc1);
    }
}
