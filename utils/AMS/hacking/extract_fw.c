/*

extract_fw.c - extract the main firmware image from a Sansa V2 (AMS) firmware
               file

Copyright (C) Dave Chapman 2008

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


/* Win32 compatibility */
#ifndef O_BINARY
#define O_BINARY 0
#endif


static off_t filesize(int fd) {
    struct stat buf;

    if (fstat(fd,&buf) < 0) {
        perror("[ERR]  Checking filesize of input file");
        return -1;
    } else {
        return(buf.st_size);
    }
}

static uint32_t get_uint32le(unsigned char* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

void usage(void)
{
    printf("Usage: extract_fw <firmware file> <output file>\n");

    exit(1);
}

int main(int argc, char* argv[])
{
    char *infile, *outfile;
    int fdin, fdout;
    off_t len;
    uint32_t n;
    unsigned char* buf;
    uint32_t firmware_size;

    if(argc != 3) {
        usage();
    }

    infile = argv[1];
    outfile = argv[2];

    /* Open the firmware file */
    fdin = open(infile,O_RDONLY|O_BINARY);

    if (fdin < 0) {
        fprintf(stderr,"[ERR]  Could not open %s for reading\n",infile);
        return 1;
    }

    if ((len = filesize(fdin)) < 0)
        return 1;

    /* We will need no more memory than the total size plus the bootloader size
       padded to a boundary */
    if ((buf = malloc(len)) == NULL) {
        fprintf(stderr,"[ERR]  Could not allocate buffer for input file (%d bytes)\n",(int)len);
        return 1;
    }

    n = read(fdin, buf, len);

    if (n != (uint32_t)len) {
        fprintf(stderr,"[ERR] Could not read firmware file\n");
        return 1;
    }

    close(fdin);

    /* Get the firmware size */
    firmware_size = get_uint32le(&buf[0x0c]);

    fdout = open(outfile, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,0666);

    if (fdout < 0) {
        fprintf(stderr,"[ERR]  Could not open %s for writing\n",outfile);
        return 1;
    }

    n = write(fdout, buf + 0x400, firmware_size);

    if (n != (uint32_t)firmware_size) {
        fprintf(stderr,"[ERR] Could not write firmware block\n");
        return 1;
    }

    /* Clean up */
    close(fdout);
    free(buf);

    return 0;
}
