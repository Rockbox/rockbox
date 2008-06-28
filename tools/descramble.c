/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
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

#include "iriver.h"
#include "gigabeat.h"

int iaudio_decode(char *iname, char *oname);

unsigned int le2int(unsigned char* buf)
{
   unsigned int res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

   return res;
}

void int2le(unsigned int val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
}

void usage(void)
{
    printf("usage: descramble [options] <input file> <output file>\n");
    printf("options:\n"
           "\t-fm     Archos FM recorder format\n"
           "\t-v2     Archos V2 recorder format\n"
           "\t-mm     Archos Multimedia format\n"
           "\t-iriver iRiver format\n"
           "\t-gigabeat Toshiba Gigabeat format\n"
           "\t-iaudio iAudio format\n"
          "\nNo option assumes Archos standard player/recorder format.\n");
    exit(1);
}

int main (int argc, char** argv)
{
    unsigned long length,i,slen;
    unsigned char *inbuf,*outbuf;
    char *iname = argv[1];
    char *oname = argv[2];
    unsigned char header[32];
    int headerlen = 6;
    int descramble = 1;
    FILE* file;

    if (argc < 3) {
        usage();
    }

    if (!strcmp(argv[1], "-fm") || !strcmp(argv[1], "-v2")) {
        headerlen = 24;
        iname = argv[2];
        oname = argv[3];
    }

    if (!strcmp(argv[1], "-mm")) {
        headerlen = 16;
        iname = argv[2];
        oname = argv[3];
        descramble = 0;
    }

    if(!strcmp(argv[1], "-iriver")) {
        /* iRiver code dealt with in the iriver.c code */
        iname = argv[2];
        oname = argv[3];
        return iriver_decode(iname, oname, FALSE, STRIP_NONE) ? -1 : 0;
    }
    if(!strcmp(argv[1], "-gigabeat")) {
        iname = argv[2];
        oname = argv[3];
        gigabeat_code(iname, oname);
        return 0;
    }
    
    if(!strcmp(argv[1], "-iaudio")) {
        iname = argv[2];
        oname = argv[3];
        return iaudio_decode(iname, oname);
    }
    
    /* open file and check size */
    file = fopen(iname,"rb");
    if (!file) {
       perror(iname);
       return -1;
    }
    fseek(file,0,SEEK_END);
    length = ftell(file) - headerlen; /* skip header */
    fseek(file,0,SEEK_SET);
    i = fread(header, 1, headerlen, file);
    if ( !i ) {
       perror(iname);
       return -1;
    }
    
    inbuf = malloc(length);
    outbuf = malloc(length);
    if ( !inbuf || !outbuf ) {
       printf("out of memory!\n");
       return -1;
    }

    /* read file */
    i=fread(inbuf,1,length,file);
    if ( !i ) {
       perror(iname);
       return -1;
    }
    fclose(file);

    if (descramble) {
        /* descramble */
        slen = length/4;
        for (i = 0; i < length; i++) {
            unsigned long addr = ((i % slen) << 2) + i/slen;
            unsigned char data = inbuf[i];
            data = ~((data >> 1) | ((data << 7) & 0x80)); /* poor man's ROR */
            outbuf[addr] = data;
        }
    }
    else {
        void* tmpptr;
        unsigned int j=0;
        int stringlen = 32;
        int unpackedsize;
        unsigned char xorstring[32];

        unpackedsize = header[4] | header[5] << 8;
        unpackedsize |= header[6] << 16 | header[7] << 24;

        length = header[8] | header[9] << 8;
        length |= header[10] << 16 | header[11] << 24;

        /* calculate the xor string used */
        for (i=0; i<(unsigned long)stringlen; i++) {
            int top=0, topchar=0, c;
            int bytecount[256];
            memset(bytecount, 0, sizeof(bytecount));

            /* gather byte frequency statistics */
            for (c=i; c<(int)length; c+=stringlen)
                bytecount[inbuf[c]]++;

            /* find the most frequent byte */
            for (c=0; c<256; c++) {
                if (bytecount[c] > top) {
                    top = bytecount[c];
                    topchar = c;
                }
            }
            xorstring[i] = topchar;
        }
        printf("XOR string: %.*s\n", stringlen, xorstring);
        
        /* xor the buffer */
        for (i=0; i<length; i++)
            outbuf[i] = inbuf[i] ^ xorstring[i & (stringlen-1)];

        /* unpack */
        tmpptr = realloc(inbuf, unpackedsize);
        memset(tmpptr, 0, unpackedsize);
        inbuf = outbuf;
        outbuf = tmpptr;

        for (i=0; i<length;) {
            int bit;
            int head = inbuf[i++];

            for (bit=0; bit<8 && i<length; bit++) {
                if (head & (1 << (bit))) {
                    outbuf[j++] = inbuf[i++];
                }
                else {
                    int x;
                    int byte1 = inbuf[i];
                    int byte2 = inbuf[i+1];
                    int count = (byte2 & 0x0f) + 3;
                    int src =
                        (j & 0xfffff000) + (byte1 | ((byte2 & 0xf0)<<4)) + 18;
                    if (src > (int)j)
                        src -= 0x1000;

                    for (x=0; x<count; x++)
                        outbuf[j++] = outbuf[src+x];
                    i += 2;
                }
            }
        }
        length = j;
    }

    /* write file */
    file = fopen(oname,"wb");
    if ( !file ) {
       perror(argv[2]);
       return -1;
    }
    if ( !fwrite(outbuf,length,1,file) ) {
       perror(argv[2]);
       return -1;
    }
    fclose(file);
    
    free(inbuf);
    free(outbuf);
    
    return 0;	
}

int iaudio_decode(char *iname, char *oname)
{
    size_t len;
    int length;
    FILE *file;
    char *outbuf;
    int i;
    unsigned char sum = 0;
    unsigned char filesum;
    
    file = fopen(iname, "rb");
    if (!file) {
        perror(iname);
        return -1;
    }
    fseek(file,0,SEEK_END);
    length = ftell(file);
    
    fseek(file,0,SEEK_SET); 
    outbuf = malloc(length);

    if ( !outbuf ) {
        printf("out of memory!\n");
        return -1;
    }

    len = fread(outbuf, 1, length, file);
    if(len < (size_t)length) {
        perror(iname);
        return -2;
    }

    fclose(file);
    
    for(i = 0; i < length-0x1030;i++)
        sum += outbuf[0x1030 + i];

    filesum = outbuf[0x102b];

    if(filesum != sum) {
        printf("Checksum mismatch!\n");
        return -1;
    }

    file = fopen(oname, "wb");
    if (!file) {
        perror(oname);
        return -3;
    }
    
    len = fwrite(outbuf+0x1030, 1, length-0x1030, file);
    if(len < (size_t)length-0x1030) {
        perror(oname);
        return -4;
    }

    fclose(file);
    return 0;
}
