/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Jörg Hohensohn [IDC]Dragon
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

/* Code copied from firmware_flash plugin. */

#include "crc32.h"

/* Tool function to calculate a CRC32 across some buffer */
/* third argument is either 0xFFFFFFFF to start or value from last piece */
uint32_t crc_32(const void *src, uint32_t len, uint32_t crc32)
{
    const unsigned char *buf = (const unsigned char *)src;
    
    /* CCITT standard polynomial 0x04C11DB7 */
    static const unsigned crc32_lookup[16] =
    {   /* lookup table for 4 bits at a time is affordable */
        0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9,
        0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
        0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61,
        0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD
    };

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


/* crc_32r (derived from tinf crc32 which was taken from zlib)
 * CRC32 algorithm taken from the zlib source, which is
 * Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler
 */

/* Tool function to calculate a CRC32 (reversed polynomial) across some buffer */
/* third argument is either the starting value or value from last piece */
uint32_t crc_32r(const void *src, uint32_t len, uint32_t crc32)
{
    const unsigned char* buf = src;

    /* reversed polynomial from other crc32 function -- 0xEDB88320 */
    static const unsigned crc32_lookup[16] =
    {
        0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
        0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
        0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
        0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
    };

    for(uint32_t i = 0; i < len; i++)
    {
        crc32 ^= buf[i];
        crc32 = crc32_lookup[crc32 & 0x0F] ^ (crc32 >> 4);
        crc32 = crc32_lookup[crc32 & 0x0F] ^ (crc32 >> 4);
    }

    return crc32;
}
