/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 by Open Neo
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

#define MY_FIRMWARE_TYPE "Rockbox"
#define MY_HEADER_VERSION 1

int main (int argc, char** argv)
{
    unsigned long length,i,slen;
    unsigned char *inbuf;
    unsigned short checksum=0;
    unsigned char *iname = argv[1];
    unsigned char *oname = argv[2];
    unsigned char header[17];
    FILE* file;

    if (argc < 3) {
       printf("usage: %s <input file> <output file>\n",argv[0]);
       return -1;
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
    
    if (length >= 0x32000) {
        printf("error: max firmware size is 200KB!\n");
        fclose(file);
        return -1;
    }
    
    fseek(file,0,SEEK_SET); 
    inbuf = malloc(length);
    if ( !inbuf ) {
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

    /* calculate checksum */
    for (i=0;i<length;i++)
       checksum+=inbuf[i];

    /* make header */
    memset(header, 0, sizeof(header));
    strncpy(header,MY_FIRMWARE_TYPE,9);
    header[9]='\0'; /*shouldn't have to, but to be SURE */
    header[10]=MY_HEADER_VERSION&0xFF;
    header[11]=(checksum>>8)&0xFF;
    header[12]=checksum&0xFF;
    header[13]=(sizeof(header)>>24)&0xFF;
    header[14]=(sizeof(header)>>16)&0xFF;
    header[15]=(sizeof(header)>>8)&0xFF;
    header[16]=sizeof(header)&0xFF;

    /* write file */
    file = fopen(oname,"wb");
    if ( !file ) {
       perror(oname);
       return -1;
    }
    if ( !fwrite(header,sizeof(header),1,file) ) {
       perror(oname);
       return -1;
    }
    if ( !fwrite(inbuf,length,1,file) ) {
       perror(oname);
       return -1;
    }
    fclose(file);
    
    free(inbuf);

    printf("\r\nHeader Info:\r\n\t"
           "Header Type:\t\t%s\r\n\t"
           "Header Version:\t\t%d\r\n\t"
           "Header Checksum:\t0x%x\r\n\t"
           "Data Start:\t\t0x%x\r\n\r\n",
           header,header[10],checksum,sizeof(header));
    return 0;
}
