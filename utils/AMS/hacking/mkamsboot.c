/*

mkamsboot.c - a tool for merging bootloader code into an Sansa V2
              (AMS) firmware file

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


/*

Insert a Rockbox bootloader into an AMS original firmware file.

We replace the main firmware block (bytes 0x400..padded_firmware_size+0x400)
with the following:

Bytes 0..(firmware_size-ucl_size) - Our bootloader code
Bytes (firmware_size-ucl_size)..firmware_size - UCL compressed OF image
Bytes firmware_size..padded_firmware_size - UCL decompress function

mkamsboot writes the following values at offsets into our bootloader code:

0x20 - Entry point (plus 1 - for thumb mode) of the ucl_unpack function
0x24 - Location of the UCL compressed version of the original firmware block

mkamsboot then corrects the length (to include the UCL decompress
function) and checksum in the main firmware headers (both copies),
creating a new legal firmware file which can be installed on the
device.

Our bootloader first checks for the "dual-boot" keypress, and then either:

a) Branches to the ucl_unpack function, which will then branch to 0x0 after 
   decompressing the OF.

b) Continues running with our test code

This method uses no RAM outside the padded area of the original
firmware block - the UCL compression can happen in-place when the
compressed image is stored at the end of the destination buffer.

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


#define PAD_TO_BOUNDARY(x) (((x) + 0x1ff) & ~0x1ff)


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

static void put_uint32le(unsigned char* p, uint32_t x)
{
    p[0] = x & 0xff;
    p[1] = (x >> 8) & 0xff;
    p[2] = (x >> 16) & 0xff;
    p[3] = (x >> 24) & 0xff;
}

static int calc_checksum(unsigned char* buf, uint32_t n)
{
    uint32_t sum = 0;
    uint32_t i;

    for (i=0;i<n;i+=4)
        sum += get_uint32le(buf + i);

    return sum;
}

void usage(void)
{
    printf("Usage: mkamsboot <firmware file> <ucl image> <boot file> <ucl unpack file> <output file>\n");

    exit(1);
}

int main(int argc, char* argv[])
{
    char *infile, *uclfile, *bootfile, *uclunpackfile, *outfile;
    int fdin, fducl, fdboot, fduclunpack, fdout;
    off_t len;
    uint32_t n;
    unsigned char* buf;
    uint32_t firmware_size;
    uint32_t firmware_paddedsize;
    uint32_t bootloader_size;
    uint32_t ucl_size;
    uint32_t uclunpack_size;
    uint32_t sum,filesum;
    uint32_t i;

    if(argc != 6) {
        usage();
    }

    infile = argv[1];
    uclfile = argv[2];
    bootfile = argv[3];
    uclunpackfile = argv[4];
    outfile = argv[5];

    /* Open the bootloader file */
    fdboot = open(bootfile, O_RDONLY|O_BINARY);
    if (fdboot < 0)
    {
        fprintf(stderr,"[ERR]  Could not open %s for reading\n",bootfile);
        return 1;
    }

    bootloader_size = filesize(fdboot);


    /* Open the UCL-compressed image of the firmware block */
    fduclunpack = open(uclunpackfile, O_RDONLY|O_BINARY);
    if (fduclunpack < 0)
    {
        fprintf(stderr,"[ERR]  Could not open %s for reading\n",uclunpackfile);
        return 1;
    }

    uclunpack_size = filesize(fduclunpack);


    /* Open the UCL-compressed image of the firmware block */
    fducl = open(uclfile, O_RDONLY|O_BINARY);
    if (fducl < 0)
    {
        fprintf(stderr,"[ERR]  Could not open %s for reading\n",uclfile);
        return 1;
    }

    ucl_size = filesize(fducl);


    /* Open the firmware file */
    fdin = open(infile,O_RDONLY|O_BINARY);

    if (fdin < 0) {
        fprintf(stderr,"[ERR]  Could not open %s for reading\n",infile);
        return 1;
    }

    if ((len = filesize(fdin)) < 0)
        return 1;

    /* Allocate memory for the OF image - we don't change the size */
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

    /* Round size up to next multiple of 0x200 */

    firmware_paddedsize = PAD_TO_BOUNDARY(firmware_size);

    fprintf(stderr,"Original firmware size - %d bytes\n",firmware_size);
    fprintf(stderr,"Padded firmware size - %d bytes\n",firmware_paddedsize);
    fprintf(stderr,"Bootloader size - %d bytes\n",bootloader_size);
    fprintf(stderr,"UCL image size - %d bytes\n",ucl_size);
    fprintf(stderr,"UCL unpack function size - %d bytes\n",uclunpack_size);
    fprintf(stderr,"Original total size of firmware - %d bytes\n",(int)len);

    /* Check we have room for our bootloader - in the future, we could UCL
       pack this image as well if we need to. */
    if (bootloader_size > (firmware_size - ucl_size)) {
        fprintf(stderr,"[ERR] Bootloader too large (%d bytes, %d available)\n",
                bootloader_size, firmware_size - ucl_size);
        return 1;
    }

    /* Check we have enough room for the UCL unpack function.  This
       needs to be outside the firmware block, so if we wanted to
       support every firmware version, we could store this function in
       the main firmware block, and then copy it to an unused part of
       RAM. */
    if (uclunpack_size > (firmware_paddedsize - firmware_size)) {
        fprintf(stderr,"[ERR] UCL unpack function too large (%d bytes, %d available)\n",
                uclunpack_size, firmware_paddedsize - firmware_size);
        return 1;
    }

    /* Zero the original firmware area - not needed, but helps debugging */
    memset(buf + 0x400, 0, firmware_size);

    /* Locate our bootloader code at the start of the firmware block */
    n = read(fdboot, buf + 0x400, bootloader_size);

    if (n != bootloader_size) {
        fprintf(stderr,"[ERR] Could not load bootloader file\n");
        return 1;
    }
    close(fdboot);

    /* Locate the compressed image of the original firmware block at the end
       of the firmware block */
    n = read(fducl, buf + 0x400 + firmware_size - ucl_size, ucl_size);

    if (n != ucl_size) {
        fprintf(stderr,"[ERR] Could not load ucl file\n");
        return 1;
    }
    close(fducl);


    /* Locate our UCL unpack function in the padding after the firmware block */
    n = read(fduclunpack, buf + 0x400 + firmware_size, uclunpack_size);

    if (n != uclunpack_size) {
        fprintf(stderr,"[ERR] Could not load uclunpack file\n");
        return 1;
    }
    close(fduclunpack);

    put_uint32le(&buf[0x420], firmware_size + 1);  /* UCL unpack entry point */
    put_uint32le(&buf[0x424], firmware_size - ucl_size);  /* Location of OF */

    /* Update checksum */
    sum = calc_checksum(buf + 0x400,firmware_size + uclunpack_size);

    put_uint32le(&buf[0x04], sum);
    put_uint32le(&buf[0x204], sum);

    /* Update firmware size */
    put_uint32le(&buf[0x0c], firmware_size + uclunpack_size);
    put_uint32le(&buf[0x20c], firmware_size + uclunpack_size);

    /* Update the whole-file checksum */
    filesum = 0;
    for (i=0;i < (unsigned)len - 4; i+=4)
        filesum += get_uint32le(&buf[i]);

    put_uint32le(buf + len - 4, filesum);


    /* Write the new firmware */
    fdout = open(outfile, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,0666);

    if (fdout < 0) {
        fprintf(stderr,"[ERR]  Could not open %s for writing\n",outfile);
        return 1;
    }

    n = write(fdout, buf, len);

    if (n != (unsigned)len) {
        fprintf(stderr,"[ERR]  Could not write firmware file\n");
        return 1;
    }

    close(fdout);

    return 0;

}
