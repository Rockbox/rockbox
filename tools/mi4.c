/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
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

#include <stdio.h>
#include <stdlib.h>

/* crc_tab[] -- this crcTable is being build by chksum_crc32GenTab().
 *		so make sure, you call it before using the other
 *		functions!
 */
static unsigned int crc_tab[256];

/* chksum_crc() -- to a given block, this one calculates the
 *				crc32-checksum until the length is
 *				reached. the crc32-checksum will be
 *				the result.
 */
static unsigned int chksum_crc32 (unsigned char *block, unsigned int length)
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
 *				calculate the crcTable for crc32-checksums.
 *				it is generated to the polynom [..]
 */

static void chksum_crc32gentab (void)
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

static void int2le(unsigned int val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
}

int mi4_encode(char *iname, char *oname, int version, int magic,
                char *model, char *type)
{
    size_t len;
    int length;
    int mi4length;
    FILE *file;
    unsigned int crc = 0;
    unsigned char *outbuf;

    file = fopen(iname, "rb");
    if (!file) {
       perror(iname);
       return -1;
    }
    fseek(file,0,SEEK_END);
    length = ftell(file);
    
    fseek(file,0,SEEK_SET);

    /* Add 4 bytes to length (for magic), the 0x200 byte header and
       then round to an even 0x400 bytes
     */
    mi4length = (length+4+0x200+0x3ff)&~0x3ff;

    outbuf = malloc(mi4length);

    if ( !outbuf ) {
       printf("out of memory!\n");
       return -1;
    }

    /* Clear the buffer to zero */
    memset(outbuf, 0, mi4length);

    len = fread(outbuf+0x200, 1, length, file);
    if(len < (size_t)length) {
        perror(iname);
        return -2;
    }
    fclose(file);

    /* We need to write some data into the actual image - before calculating
       the CRC. */
    int2le(0x00000100,   &outbuf[0x2e0]);   /* magic */
    int2le(magic,        &outbuf[0x2e4]);   /* magic */
    int2le(length+4,     &outbuf[0x2e8]);   /* length plus 0xaa55aa55 */

    int2le(0xaa55aa55,   &outbuf[0x200+length]);  /* More Magic */
    
    strncpy((char *)outbuf+0x1f8, type, 4);  /* type of binary (RBBL, RBOS) */
    strncpy((char *)outbuf+0x1fc, model, 4); /* 4 character model id */
    
    /* Calculate CRC32 checksum */
    chksum_crc32gentab ();
    crc = chksum_crc32 (outbuf+0x200,mi4length-0x200);

    strncpy((char *)outbuf, "PPOS", 4);     /* Magic */
    int2le(version,      &outbuf[0x04]);    /* .mi4 version */
    int2le(length+4,     &outbuf[0x08]);    /* Length of firmware plus magic */
    int2le(crc,          &outbuf[0x0c]);    /* CRC32 of mi4 file */
    int2le(0x00000002,   &outbuf[0x10]);    /* Encryption type: 2 = TEA */
    int2le(mi4length,    &outbuf[0x14]);    /* Total .mi4 length */
    int2le(mi4length-0x200, &outbuf[0x18]); /* Length of plaintext part */

    /* v3 files require a dummy DSA signature */
    if (version == 0x00010301) {
        outbuf[0x2f]=0x01;                
    }

    file = fopen(oname, "wb");
    if (!file) {
        perror(oname);
        return -3;
    }
    
    len = fwrite(outbuf, 1, mi4length, file);
    if(len < (size_t)length) {
        perror(oname);
        return -4;
    }

    fclose(file);

    fprintf(stderr, "File encoded successfully\n" );

    return 0;
}
