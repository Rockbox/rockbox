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

int main (int argc, char** argv)
{
    unsigned long length,i,slen;
    unsigned char *inbuf,*outbuf;
    unsigned char *iname = argv[1];
    unsigned char *oname = argv[2];
    unsigned char header[32];
    int headerlen = 6;
    int descramble = 1;
    FILE* file;

    if (argc < 3) {
       printf("usage: %s [-fm] [-v2] [-mm] <input file> <output file>\n",
              argv[0]);
       return -1;
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
    
    /* open file and check size */
    file = fopen(iname,"rb");
    if (!file) {
       perror(oname);
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

        unpackedsize = ((unsigned int*)header)[1];
        length = ((unsigned int*)header)[2];

        /* calculate the xor string used */
        for (i=0; i<stringlen; i++) {
            int top=0, topchar=0, c;
            int bytecount[256];
            memset(bytecount, 0, sizeof(bytecount));

            /* gather byte frequency statistics */
            for (c=i; c<length; c+=stringlen)
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
                    if (src > j)
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
