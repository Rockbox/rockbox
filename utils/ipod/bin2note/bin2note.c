/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * bin2note - a program to insert binary code in an iPod Nano 2nd
 *            Generation notes file
 *
 * Based on research by stooo, TheSeven and others.
 *
 * Copyright (C) 2009 Dave Chapman
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

static off_t filesize(int fd)
{
    struct stat buf;

    fstat(fd,&buf);
    return buf.st_size;
}

void write_utf16le(unsigned char* buf, int len, FILE* fp)
{
    int i;
    char tmp[2];

    tmp[1] = 0;

    for (i=0;i<len;i++) {
       tmp[0] = buf[i];
       fwrite(tmp, 1, sizeof(tmp), fp);
    }
}

void insert_link(unsigned char* buf, uint32_t pointer)
{
   char link[] = "<a href=\"AAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAA%xx"
                 "%xx%xx%xx\"></a>";
    char tmp[32];
    unsigned int i;

    sprintf(tmp, "%%%02x%%%02x%%%02x%%%02x",
                 pointer & 0xff,
                 (pointer >> 8) & 0xff,
                 (pointer >> 16) & 0xff,
                 (pointer >> 24) & 0xff);

    memcpy(link + 0x11d, tmp, 12);

    /* UTF-16 little-endian BOM */
    buf[0] = 0xff;
    buf[1] = 0xfe;

    /* UTF-16 little-endian URL */
    for (i=0;i<strlen(link);i++) {
       buf[i*2+2] = link[i];
       buf[i*2+3] = 0;
    }
}

#define MAX_NOTES_SIZE 4096
#define MAX_PAYLOAD_SIZE (MAX_NOTES_SIZE - 0x260 - 4)

int main (int argc, char* argv[])
{
    char* infile;
    char* htmname;
    int fdin,fdout;
    unsigned char buf[MAX_NOTES_SIZE];
    int len;
    int n;
    int i;

    if (argc != 3) {
        fprintf(stderr,"Usage: bin2note file.bin file.htm\n");
        return 1;
    }

    infile=argv[1];
    htmname=argv[2];

    fdin = open(infile,O_RDONLY|O_BINARY);
    if (fdin < 0) {
        fprintf(stderr,"Can not open %s\n",infile);
        return 1;
    }

    len = filesize(fdin);

    if (len > MAX_PAYLOAD_SIZE) {
        fprintf(stderr,"Payload too big!\n");
        close(fdin);
        return 1;
    }

    /* **** Input file is OK, now build the note **** */
    
    /* Insert URL at start of note */
    insert_link(buf, 0x08640568);

    /* Load code at offset 0x260 */
    n = read(fdin,buf + 0x260,len);
    if (n < len) {
        fprintf(stderr,"Short read, aborting\n");
        return 1;
    }
    close(fdin);

    /* Fill the remaining buffer with NOPs (mov r1,r1) - 0xe1a01001 */
    for (i=0x260 + len; i < MAX_NOTES_SIZE-4; i+=4) {
        buf[i] = 0x01;
        buf[i+1] = 0x10;
        buf[i+2] = 0xa0;
        buf[i+3] = 0xe1;
    }

    /* Finally append a branch back to our code - 0x260 in the note */
    buf[MAX_NOTES_SIZE-4] = 0x97;
    buf[MAX_NOTES_SIZE-3] = 0xfc;
    buf[MAX_NOTES_SIZE-2] = 0xff;
    buf[MAX_NOTES_SIZE-1] = 0xea;

    fdout = open(htmname, O_CREAT|O_TRUNC|O_BINARY|O_WRONLY, 0666);
    if (fdout < 0) {
        fprintf(stderr,"Could not open output file\n");
        return 1;
    }

    if (write(fdout, buf, sizeof(buf)) != sizeof(buf)) {
        fprintf(stderr,"Error writing output file\n");
        close(fdout);
        return 1;
    }

    close(fdout);
    return 0;
}
