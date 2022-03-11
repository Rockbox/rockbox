/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Dave Chapman
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
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ipodpatcher.h"

#include "arc4.h"

static inline int le2int(unsigned char* buf)
{
   int32_t res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

   return res;
}

static inline void int2le(unsigned int val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
}

static inline uint32_t getuint32le(unsigned char* buf)
{
  int32_t res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

  return res;
}

/* testMarker and GetSecurityBlockKey based on code from BadBlocks and
   Kingstone, posted at http://ipodlinux.org/Flash_Decryption

*/

static bool testMarker(int marker)
{
    int mask, decrypt, temp1, temp2;

    mask = (marker&0xff)|((marker&0xff)<<8)|((marker&0xff)<<16)|((marker&0xff)<<24);
    decrypt = marker ^ mask;
    temp1=(int)((unsigned int)decrypt>>24);
    temp2=decrypt<<8;

    if (temp1==0)
        return false;

    temp2=(int)((unsigned int)temp2>>24);
    decrypt=decrypt<<16;
    decrypt=(int)((unsigned int)decrypt>>24);

    if ((temp1 < temp2) && (temp2 < decrypt))
    {
        temp1 = temp1 & 0xf;
        temp2 = temp2 & 0xf;
        decrypt = decrypt & 0xf;

        if ((temp1 > temp2) && (temp2 > decrypt) && (decrypt != 0))
        {
            return true;
        }
    }
    return false;
}

static int GetSecurityBlockKey(unsigned char *data, unsigned char* this_key)
{
    int constant = 0x54c3a298;
    int key=0;
    int nkeys = 0;
    int aMarker=0;
    int pos=0;
    int c, count;
    int temp1;
    static const int offset[8]={0x5,0x25,0x6f,0x69,0x15,0x4d,0x40,0x34};

    for (c = 0; c < 8; c++)
    {
        pos = offset[c]*4;
        aMarker = getuint32le(data + pos);

        if (testMarker(aMarker))
        {
            if (c<7)
                pos =(offset[c+1]*4)+4;
            else
                pos =(offset[0]*4)+4;

            key=0;

            temp1=aMarker;

            for (count=0;count<2;count++){
                int word = getuint32le(data + pos);
                temp1 = aMarker;
                temp1 = temp1^word;
                temp1 = temp1^constant;
                key = temp1;
                pos = pos+4;
            }
            int r1=0x6f;
            int r2=0;
            int r12;
            int r14;
            unsigned int r_tmp;

            for (count=2;count<128;count=count+2){
                r2=getuint32le(data+count*4);
                r12=getuint32le(data+(count*4)+4);
                r_tmp=(unsigned int)r12>>16;
                r14=r2 | ((int)r_tmp);
                r2=r2&0xffff;
                r2=r2 | r12;
                r1=r1^r14;
                r1=r1+r2;
            }
            key=key^r1;

            // Invert key, little endian
            this_key[0] = key & 0xff;
            this_key[1] = (key >> 8) & 0xff;
            this_key[2] = (key >> 16) & 0xff;
            this_key[3] = (key >> 24) & 0xff;
            nkeys++;
        }
    }
    return nkeys;
}

static int find_key(struct ipod_t* ipod, int aupd, unsigned char* key)
{
    int n;

    /* Firstly read the security block and find the RC4 key.  This is
       in the sector preceeding the AUPD image. */

    if(ipod->sectorbuf == NULL) {
        fprintf(stderr,"[ERR]  Buffer not initialized.");
        return -1;
    }
    fprintf(stderr, "[INFO] Reading security block at offset 0x%08x\n",ipod->ipod_directory[aupd].devOffset-ipod->sector_size);
    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[aupd].devOffset-ipod->sector_size) < 0) {
        return -1;
    }

    if ((n = ipod_read(ipod, 512)) < 0) {
        return -1;
    }

    n = GetSecurityBlockKey(ipod->sectorbuf, key);

    if (n != 1)
    {
        fprintf(stderr, "[ERR]  %d keys found in security block, can not continue\n",n);
        return -1;
    }

    return 0;
}

int read_aupd(struct ipod_t* ipod, char* filename)
{
    int length;
    int i;
    int outfile;
    int n;
    int aupd;
    struct rc4_key_t rc4;
    unsigned char key[4];
    unsigned long chksum=0;

    if(ipod->sectorbuf == NULL) {
        fprintf(stderr,"[ERR]  Buffer not initialized.");
        return -1;
    }
    aupd = 0;
    while ((aupd < ipod->nimages) && (ipod->ipod_directory[aupd].ftype != FTYPE_AUPD))
    {
        aupd++;
    }

    if (aupd == ipod->nimages)
    {
        fprintf(stderr,"[ERR]  No AUPD image in firmware partition.\n");
        return -1;
    }

    length = ipod->ipod_directory[aupd].len;

    fprintf(stderr,"[INFO] Reading firmware (%d bytes)\n",length);

    if (find_key(ipod, aupd, key) < 0)
    {
       return -1;
    }

    fprintf(stderr, "[INFO] Decrypting AUPD image with key %02x%02x%02x%02x\n",key[0],key[1],key[2],key[3]);

    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[aupd].devOffset) < 0) {
        return -1;
    }

    i = (length+ipod->sector_size-1) & ~(ipod->sector_size-1);

    if ((n = ipod_read(ipod,i)) < 0) {
        return -1;
    }

    if (n < i) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n",
                i,n);
        return -1;
    }

    /* Perform the decryption - this is standard (A)RC4 */
    matrixArc4Init(&rc4, key, 4);
    matrixArc4(&rc4, ipod->sectorbuf, ipod->sectorbuf, length);

    chksum = 0;
    for (i = 0; i < (int)length; i++) {
         /* add 8 unsigned bits but keep a 32 bit sum */
         chksum += ipod->sectorbuf[i];
    }

    if (chksum != ipod->ipod_directory[aupd].chksum)
    {
        fprintf(stderr,"[ERR]  Decryption failed - checksum error\n");
        return -1;
    }
    fprintf(stderr,"[INFO] Decrypted OK (checksum matches header)\n");

    outfile = open(filename,O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,0666);
    if (outfile < 0) {
        fprintf(stderr,"[ERR]  Couldn't open file %s\n",filename);
        return -1;
    }

    n = write(outfile,ipod->sectorbuf,length);
    if (n != length) {
        fprintf(stderr,"[ERR]  Write error - %d\n",n);
    }
    close(outfile);

    return 0;
}

int write_aupd(struct ipod_t* ipod, char* filename)
{
    unsigned int length;
    int i;
    int x;
    int n;
    int infile;
    int newsize;
    int aupd;
    unsigned long chksum=0;
    struct rc4_key_t rc4;
    unsigned char key[4];

    if(ipod->sectorbuf == NULL) {
        fprintf(stderr,"[ERR]  Buffer not initialized.");
        return -1;
    }
    /* First check that the input file is the correct type for this ipod. */
    infile=open(filename,O_RDONLY);
    if (infile < 0) {
        fprintf(stderr,"[ERR]  Couldn't open input file %s\n",filename);
        return -1;
    }
    
    length = filesize(infile);
    newsize=(length+ipod->sector_size-1)&~(ipod->sector_size-1);

    fprintf(stderr,"[INFO] Padding input file from 0x%08x to 0x%08x bytes\n",
            length,newsize);

    if (newsize > BUFFER_SIZE) {
        fprintf(stderr,"[ERR]  Input file too big for buffer\n");
        if (infile >= 0) close(infile);
        return -1;
    }

    /* Find aupd image number */
    aupd = 0;
    while ((aupd < ipod->nimages) && (ipod->ipod_directory[aupd].ftype != FTYPE_AUPD))
    {
        aupd++;
    }

    if (aupd == ipod->nimages)
    {
        fprintf(stderr,"[ERR]  No AUPD image in firmware partition.\n");
        return -1;
    }

    if (length != ipod->ipod_directory[aupd].len)
    {
        fprintf(stderr,"[ERR]  AUPD image (%d bytes) differs in size to %s (%d bytes).\n",
                       ipod->ipod_directory[aupd].len, filename, length);
        return -1;
    }

    if (find_key(ipod, aupd, key) < 0)
    {
       return -1;
    }

    fprintf(stderr, "[INFO] Encrypting AUPD image with key %02x%02x%02x%02x\n",key[0],key[1],key[2],key[3]);

    /* We now know we have enough space, so write it. */

    fprintf(stderr,"[INFO] Reading input file...\n");
    n = read(infile,ipod->sectorbuf,length);
    if (n < 0) {
        fprintf(stderr,"[ERR]  Couldn't read input file\n");
        close(infile);
        return -1;
    }
    close(infile);

    /* Pad the data with zeros */
    memset(ipod->sectorbuf+length,0,newsize-length);

    /* Calculate the new checksum (before we encrypt) */
    chksum = 0;
    for (i = 0; i < (int)length; i++) {
         /* add 8 unsigned bits but keep a 32 bit sum */
         chksum += ipod->sectorbuf[i];
    }

    /* Perform the encryption - this is standard (A)RC4 */
    matrixArc4Init(&rc4, key, 4);
    matrixArc4(&rc4, ipod->sectorbuf, ipod->sectorbuf, length);

    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[aupd].devOffset) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    if ((n = ipod_write(ipod,newsize)) < 0) {
        perror("[ERR]  Write failed\n");
        return -1;
    }

    if (n < newsize) {
        fprintf(stderr,"[ERR]  Short write - requested %d bytes, received %d\n"
                      ,newsize,n);
        return -1;
    }
    fprintf(stderr,"[INFO] Wrote %d bytes to firmware partition\n",n);

    x = ipod->diroffset % ipod->sector_size;

    /* Read directory */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }

    n=ipod_read(ipod, ipod->sector_size);
    if (n < 0) { return -1; }

    /* Update checksum */
    fprintf(stderr,"[INFO] Updating checksum to 0x%08x (was 0x%08x)\n",(unsigned int)chksum,le2int(ipod->sectorbuf + x + aupd*40 + 28));
    int2le(chksum,ipod->sectorbuf+x+aupd*40+28);

    /* Write directory */    
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }
    n=ipod_write(ipod, ipod->sector_size);
    if (n < 0) { return -1; }

    return 0;
}

