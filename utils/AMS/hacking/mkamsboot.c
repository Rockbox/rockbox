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

The first instruction in an AMS firmware file is always of the form:

   ldr     pc, [pc, #xxx]

where [pc, #xxx] contains the entry point of the firmware - e.g. 0x00000138

mkamsboot appends the Rockbox bootloader to the end of the original
firmware block in the firmware file and shifts the remaining contents of the firmware file to make space for it.

It also replaces the contents of [pc, #xxx] with the entry point of
our bootloader - i.e. the length of the original firmware block plus 4
bytes.

It then stores the original entry point from [pc, #xxx] in the first
four bytes of the Rockbox bootloader image, which is used by the
bootloader to dual-boot.

Finally, mkamsboot corrects the length and checksum in the main
firmware headers (both copies), creating a new legal firmware file
which can be installed on the device.

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
    printf("Usage: mkamsboot <firmware file> <boot file> <output file>\n");

    exit(1);
}

int main(int argc, char* argv[])
{
    char *infile, *bootfile, *outfile;
    int fdin, fdboot,fdout;
    off_t len;
    uint32_t n;
    unsigned char* buf;
    uint32_t ldr;
    uint32_t origoffset;
    uint32_t firmware_size;
    uint32_t firmware_paddedsize;
    uint32_t bootloader_size;
    uint32_t new_paddedsize;
    uint32_t sum,filesum;
    uint32_t new_length;
    uint32_t i;

    if(argc != 4) {
        usage();
    }

    infile = argv[1];
    bootfile = argv[2];
    outfile = argv[3];

    /* Open the bootloader file */
    fdboot = open(bootfile, O_RDONLY|O_BINARY);
    if (fdboot < 0)
    {
        fprintf(stderr,"[ERR]  Could not open %s for reading\n",bootfile);
        return 1;
    }

    bootloader_size = filesize(fdboot);


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
    if ((buf = malloc(len + PAD_TO_BOUNDARY(bootloader_size))) == NULL) {
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

    /* Total new size */
    new_paddedsize = PAD_TO_BOUNDARY(firmware_size + bootloader_size);

    /* Total new size of firmware file */
    new_length = len + (new_paddedsize - firmware_paddedsize);

    fprintf(stderr,"Original firmware size - 0x%08x\n",firmware_size);
    fprintf(stderr,"Padded firmware size - 0x%08x\n",firmware_paddedsize);
    fprintf(stderr,"Bootloader size - 0x%08x\n",bootloader_size);
    fprintf(stderr,"New padded size - 0x%08x\n",new_paddedsize);
    fprintf(stderr,"Original total size of firmware - 0x%08x\n",(int)len);
    fprintf(stderr,"New total size of firmware - 0x%08x\n",new_length);

    if (firmware_paddedsize != new_paddedsize) {
        /* We don't know how to safely increase the firmware size, so abort */

        fprintf(stderr,
                "[ERR]  Bootloader too large (%d bytes - %d bytes available), aborting.\n",
                bootloader_size, firmware_paddedsize - firmware_size);

        return 1;
    }

    ldr = get_uint32le(&buf[0x400]);

    if ((ldr & 0xfffff000) != 0xe59ff000) {
        fprintf(stderr,"[ERR]  Firmware file doesn't start with an \"ldr pc, [pc, #xx]\" instruction.\n");
        return 1;
    }
    origoffset = (ldr&0xfff) + 8;

    printf("original firmware entry point: 0x%08x\n",get_uint32le(buf + 0x400 + origoffset));
    printf("New entry point: 0x%08x\n", firmware_size + 4);

#if 0
    /* Replace the "Product: Express" string with "Rockbox" */
    i = 0x400 + firmware_size - 7;
    while ((i > 0x400) && (memcmp(&buf[i],"Express",7)!=0))
        i--;

    i = (i + 3) & ~0x3;

    if (i >= 0x400) {
        printf("Replacing \"Express\" string at offset 0x%08x\n",i);
        memcpy(&buf[i],"Rockbox",7);
    } else {
        printf("Could not find \"Express\" string to replace\n");
    }
#endif

    n = read(fdboot, buf + 0x400 + firmware_size, bootloader_size);

    if (n != bootloader_size) {
        fprintf(stderr,"[ERR] Could not bootloader file\n");
        return 1;
    }
    close(fdboot);

    /* Replace first word of the bootloader with the original entry point */
    put_uint32le(buf + 0x400 + firmware_size, get_uint32le(buf + 0x400 + origoffset));

#if 1
    put_uint32le(buf + 0x400 + origoffset, firmware_size + 4);
#endif

    /* Update checksum */
    sum = calc_checksum(buf + 0x400,firmware_size + bootloader_size);

    put_uint32le(&buf[0x04], sum);
    put_uint32le(&buf[0x204], sum);

    /* Update firmware block count */
    put_uint32le(&buf[0x08], new_paddedsize / 0x200);
    put_uint32le(&buf[0x208], new_paddedsize / 0x200);

    /* Update firmware size */
    put_uint32le(&buf[0x0c], firmware_size + bootloader_size);
    put_uint32le(&buf[0x20c], firmware_size + bootloader_size);

    /* Update the whole-file checksum */
    filesum = 0;
    for (i=0;i < new_length - 4; i+=4)
        filesum += get_uint32le(&buf[i]);

    put_uint32le(buf + new_length - 4, filesum);


    /* Write the new firmware */
    fdout = open(outfile, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,0666);

    if (fdout < 0) {
        fprintf(stderr,"[ERR]  Could not open %s for writing\n",outfile);
        return 1;
    }

    write(fdout, buf, new_length);

    close(fdout);

    return 0;

}
