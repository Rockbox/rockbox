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
    int headerlen = 6;
    FILE* file;

    if (argc < 3) {
       printf("usage: %s [-fm] [-v2] <input file> <output file>\n",argv[0]);
       return -1;
    }

    if (!strcmp(argv[1], "-fm") || !strcmp(argv[1], "-v2")) {
        headerlen = 24;
        iname = argv[2];
        oname = argv[3];
    }
    
    /* open file and check size */
    file = fopen(iname,"rb");
    if (!file) {
       perror(oname);
       return -1;
    }
    fseek(file,0,SEEK_END);
    length = ftell(file) - headerlen; /* skip header */
    fseek(file,headerlen,SEEK_SET); 
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

    /* descramble */
    slen = length/4;
    for (i = 0; i < length; i++) {
      unsigned long addr = ((i % slen) << 2) + i/slen;
      unsigned char data = inbuf[i];
      data = ~((data >> 1) | ((data << 7) & 0x80)); /* poor man's ROR */
      outbuf[addr] = data;
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
