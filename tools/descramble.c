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
    FILE* file;

    if (argc < 3) {
       printf("usage: %s <input file> <output file>\n",argv[0]);
       return -1;
    }
    
    /* open file and check size */
    file = fopen(argv[1],"rb");
    if (!file) {
       perror(argv[1]);
       return -1;
    }
    fseek(file,0,SEEK_END);
    length = ftell(file) - 6; /* skip 6-byte header */
    fseek(file,6,SEEK_SET); 
    inbuf = malloc(length);
    outbuf = malloc(length);
    if ( !inbuf || !outbuf ) {
       printf("out of memory!\n");
       return -1;
    }

    /* read file */
    i=fread(inbuf,1,length,file);
    if ( !i ) {
       perror(argv[1]);
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
    file = fopen(argv[2],"wb");
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
