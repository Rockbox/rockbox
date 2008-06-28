/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: crc32.c 10464 2006-08-05 20:19:10Z miipekk $
 *
 * Copyright (C) 2007 Barry Wardell
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

/*
 * We can't use the CRC32 implementation in the firmware library as it uses a
 * different polynomial. The polynomial needed is 0xEDB88320L
 *
 * CRC32 implementation taken from:
 *
 * efone - Distributed internet phone system.
 *
 * (c) 1999,2000 Krzysztof Dabrowski
 * (c) 1999,2000 ElysiuM deeZine
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

/* based on implementation by Finn Yannick Jacobs */

#include "crc32-mi4.h"

/* crc_tab[] -- this crcTable is being build by chksum_crc32GenTab().
 *      so make sure, you call it before using the other
 *      functions!
 */
static unsigned int crc_tab[256];

/* chksum_crc() -- to a given block, this one calculates the
 *              crc32-checksum until the length is
 *              reached. the crc32-checksum will be
 *              the result.
 */
unsigned int chksum_crc32 (unsigned char *block, unsigned int length)
{
   register unsigned long crc;
   unsigned long i;

   crc = 0;
   for (i = 0; i < length; i++)
   {
      crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_tab[(crc ^ *block++) & 0xFF];
   }
   return (crc);
}

/* chksum_crc32gentab() --      to a global crc_tab[256], this one will
 *              calculate the crcTable for crc32-checksums.
 *              it is generated to the polynom [..]
 */

void chksum_crc32gentab (void)
{
   unsigned long crc, poly;
   int i, j;

   poly = 0xEDB88320L;
   for (i = 0; i < 256; i++)
   {
      crc = i;
      for (j = 8; j > 0; j--)
      {
     if (crc & 1)
     {
        crc = (crc >> 1) ^ poly;
     }
     else
     {
        crc >>= 1;
     }
      }
      crc_tab[i] = crc;
   }
}
