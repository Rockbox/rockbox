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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "iriver.h"

void int2le(unsigned int val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
}

void int2be(unsigned int val, unsigned char* addr)
{
    addr[0] = (val >> 24) & 0xff;
    addr[1] = (val >> 16) & 0xff;
    addr[2] = (val >> 8) & 0xff;
    addr[3] = val & 0xFF;
}

void usage(void)
{
    printf("usage: scramble [options] <input file> <output file> [xor string]\n");
    printf("options:\n"
           "\t-fm     Archos FM recorder format\n"
           "\t-v2     Archos V2 recorder format\n"
           "\t-ofm    Archos Ondio FM recorder format\n"
           "\t-osp    Archos Ondio SP format\n"
           "\t-neo    SSI Neo format\n"
           "\t-mm=X   Archos Multimedia format (X values: A=JBMM, B=AV1xx, C=AV3xx)\n"
           "\t-iriver iRiver format\n"
           "\t-add=X  Rockbox iRiver \"add-up\" checksum format\n"
           "\t        (X values: h100, h120, h140, h300)\n"
           "\nNo option results in Archos standard player/recorder format.\n");

    exit(1);
}

int main (int argc, char** argv)
{
    unsigned long length,i,slen;
    unsigned char *inbuf,*outbuf;
    unsigned short crc=0;
    unsigned long crc32=0; /* 32 bit checksum */
    unsigned char header[24];
    unsigned char *iname = argv[1];
    unsigned char *oname = argv[2];
    unsigned char *xorstring;
    int headerlen = 6;
    FILE* file;
    int version;
    unsigned long irivernum;
    char irivermodel[5];
    enum { none, scramble, xor, add } method = scramble;

    if (argc < 3) {
        usage();
    }

    if(!strcmp(argv[1], "-fm")) {
        headerlen = 24;
        iname = argv[2];
        oname = argv[3];
        version = 4;
    }
    
    else if(!strcmp(argv[1], "-v2")) {
        headerlen = 24;
        iname = argv[2];
        oname = argv[3];
        version = 2;
    }

    else if(!strcmp(argv[1], "-ofm")) {
        headerlen = 24;
        iname = argv[2];
        oname = argv[3];
        version = 8;
    }

    else if(!strcmp(argv[1], "-osp")) {
        headerlen = 24;
        iname = argv[2];
        oname = argv[3];
        version = 16;
    }

    else if(!strcmp(argv[1], "-neo")) {
        headerlen = 17;
        iname = argv[2];
        oname = argv[3];
        method = none;
    }
    else if(!strncmp(argv[1], "-mm=", 4)) {
        headerlen = 16;
        iname = argv[2];
        oname = argv[3];
        method = xor;
        version = argv[1][4];
        if (argc > 4)
            xorstring = argv[4];
        else {
            printf("Multimedia needs an xor string\n");
            return -1;
        }
    }
    else if(!strncmp(argv[1], "-add=", 5)) {
        iname = argv[2];
        oname = argv[3];
        method = add;

        if(!strcmp(&argv[1][5], "h120"))
            irivernum = 0;
        else if(!strcmp(&argv[1][5], "h140"))
            irivernum = 0; /* the same as the h120 */
        else if(!strcmp(&argv[1][5], "h100"))
            irivernum = 1;
        else if(!strcmp(&argv[1][5], "h300"))
            irivernum = 2;
        else {
            fprintf(stderr, "unsupported model: %s\n", &argv[1][5]);
            return 2;
        }
        /* we store a 4-letter model name too, for humans */
        strcpy(irivermodel, &argv[1][5]);
        crc32 = irivernum; /* start checksum calcs with this */
    }

    else if(!strcmp(argv[1], "-iriver")) {
        /* iRiver code dealt with in the iriver.c code */
        iname = argv[2];
        oname = argv[3];
        iriver_encode(iname, oname, FALSE);
        return 0;
    }
    
    /* open file */
    file = fopen(iname,"rb");
    if (!file) {
       perror(iname);
       return -1;
    }
    fseek(file,0,SEEK_END);
    length = ftell(file);
    length = (length + 3) & ~3; /* Round up to nearest 4 byte boundary */
    
    if ((method == scramble) && ((length + headerlen) >= 0x32000)) {
        printf("error: max firmware size is 200KB!\n");
        fclose(file);
        return -1;
    }
    
    fseek(file,0,SEEK_SET); 
    inbuf = malloc(length);
    if (method == xor)
        outbuf = malloc(length*2);
    else if(method == add)
        outbuf = malloc(length + 8);
    else
        outbuf = malloc(length);
    if ( !inbuf || !outbuf ) {
       printf("out of memory!\n");
       return -1;
    }
    if(length> 4) {
        /* zero-fill the last 4 bytes to make sure there's no rubbish there
           when we write the size-aligned file later */
        memset(outbuf+length-4, 0, 4);
    }

    /* read file */
    i=fread(inbuf,1,length,file);
    if ( !i ) {
       perror(iname);
       return -1;
    }
    fclose(file);

    switch (method)
    {
        case add:
            for (i = 0; i < length/2; i++) {
                unsigned short *inbuf16 = (unsigned short *)inbuf;
                /* add 16 unsigned bits but keep a 32 bit sum */
                crc32 += inbuf16[i];
            }
            break;
        case scramble:
            slen = length/4;
            for (i = 0; i < length; i++) {
                unsigned long addr = (i >> 2) + ((i % 4) * slen);
                unsigned char data = inbuf[i];
                data = ~((data << 1) | ((data >> 7) & 1)); /* poor man's ROL */
                outbuf[addr] = data;
            }
            break;

        case xor:
            /* "compress" */
            slen = 0;
            for (i=0; i<length; i++) {
                if (!(i&7))
                    outbuf[slen++] = 0xff; /* all data is uncompressed */
                outbuf[slen++] = inbuf[i];
            }
            break;
    }

    if(method != add) {
        /* calculate checksum */
        for (i=0;i<length;i++)
            crc += inbuf[i];
    }

    memset(header, 0, sizeof header);
    switch (method)
    {
        case add:
        {
            unsigned long *headp = (unsigned long *)header;
            headp[0] = crc32; /* checksum */
            memcpy(&header[4], irivermodel, 4); /* 4 bytes model name */
            memcpy(outbuf, inbuf, length); /* the input buffer to output*/
            headerlen = 8;
        }
        break;
        case scramble:
            if (headerlen == 6) {
                int2be(length, header);
                header[4] = (crc >> 8) & 0xff;
                header[5] = crc & 0xff;
            }
            else {
                header[0] =
                    header[1] =
                    header[2] =
                    header[3] = 0xff; /* ??? */
                
                header[6] = (crc >> 8) & 0xff;
                header[7] = crc & 0xff;
                
                header[11] = version;
                
                header[15] = headerlen; /* really? */
                
                int2be(length, &header[20]);
            }
            break;

        case xor:
        {
            int xorlen = strlen(xorstring);

            /* xor data */
            for (i=0; i<slen; i++)
                outbuf[i] ^= xorstring[i & (xorlen-1)];
            
            /* calculate checksum */
            for (i=0; i<slen; i++)
                crc += outbuf[i];

            header[0] = header[2] = 'Z';
            header[1] = header[3] = version;
            int2le(length, &header[4]);
            int2le(slen, &header[8]);
            int2le(crc, &header[12]);
            length = slen;
            break;
        }

#define MY_FIRMWARE_TYPE "Rockbox"
#define MY_HEADER_VERSION 1
        default:
            strncpy(header,MY_FIRMWARE_TYPE,9);
            header[9]='\0'; /*shouldn't have to, but to be SURE */
            header[10]=MY_HEADER_VERSION&0xFF;
            header[11]=(crc>>8)&0xFF;
            header[12]=crc&0xFF;
            int2be(sizeof(header), &header[12]);
            break;
    }

    /* write file */
    file = fopen(oname,"wb");
    if ( !file ) {
       perror(oname);
       return -1;
    }
    if ( !fwrite(header,headerlen,1,file) ) {
       perror(oname);
       return -1;
    }
    if ( !fwrite(outbuf,length,1,file) ) {
       perror(oname);
       return -1;
    }
    fclose(file);
    
    free(inbuf);
    free(outbuf);
    
    return 0;
}
