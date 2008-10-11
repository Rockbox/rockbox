/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * mkamsboot.c - a tool for merging bootloader code into an Sansa V2
 *               (AMS) firmware file
 *
 * Copyright (C) 2008 Dave Chapman
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

/*

Insert a Rockbox bootloader into an AMS original firmware file.

We replace the main firmware block (bytes 0x400..0x400+firmware_size)
as follows:


 ----------------------  0x0
|                      |
|    Dual-boot code    |
|                      |
|----------------------|
|     EMPTY SPACE      |
|----------------------|
|                      |
| compressed RB image  |
|                      |
|----------------------|
|                      |
| compressed OF image  |
|                      |
|----------------------|
| UCL unpack function  |
 ----------------------

This entire block fits into the space previously occupied by the main
firmware block - the space saved by compressing the OF image is used
to store the compressed version of the Rockbox bootloader.  The OF
image is typically about 120KB, which allows us to store a Rockbox
bootloader with an uncompressed size of about 60KB-70KB.

mkamsboot then corrects the checksums and writes a new legal firmware
file which can be installed on the device.

When the Sansa device boots, this firmware block is loaded to RAM at
address 0x0 and executed.

Firstly, the dual-boot code will copy the UCL unpack function to the
end of RAM.

Then, depending on the detection of the dual-boot keypress, either the
OF image or the Rockbox image is copied to the end of RAM (just before
the ucl unpack function) and uncompress it to the start of RAM.

Finally, the ucl unpack function branches to address 0x0, passing
execution to the uncompressed firmware.


*/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <ucl/ucl.h>

/* Headers for ARM code binaries */
#include "uclimg.h"
#include "bootimg_clip.h"
#include "bootimg_e200v2.h"

/* Win32 compatibility */
#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef VERSION
#define VERSION "0.1"
#endif

enum
{
    MODEL_UNKNOWN = -1,
    MODEL_FUZE = 0,
    MODEL_CLIP,
    MODEL_CLIPV2,
    MODEL_E200,
    MODEL_M200,
    MODEL_C200
};

static const char* model_names[] = 
{
    "Fuze",
    "Clip",
    "Clip V2",
    "E200",
    "M200",
    "C200"
};

static const unsigned char* bootloaders[] = 
{
    NULL,
    bootimg_clip,
    NULL,
    bootimg_e200v2,
    NULL,
    NULL
};

static const int bootloader_sizes[] = 
{
    0,
    sizeof(bootimg_clip),
    0,
    sizeof(bootimg_e200v2),
    0,
    0
};


/* This magic should appear at the start of any UCL file */
static const unsigned char uclmagic[] = {
    0x00, 0xe9, 0x55, 0x43, 0x4c, 0xff, 0x01, 0x1a
};


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

static int get_model(int model_id)
{
    switch(model_id)
    {
        case 0x1e:
            return MODEL_FUZE;
        case 0x22:
            return MODEL_CLIP;
        case 0x23:
            return MODEL_C200;
        case 0x24:
            return MODEL_E200;
        case 0x25:
            return MODEL_M200;
        case 0x27:
            return MODEL_CLIPV2;
    }

    return MODEL_UNKNOWN;
}


static unsigned char* uclpack(unsigned char* inbuf, int insize, int* outsize)
{
    int maxsize;
    unsigned char* outbuf;
    int r;

    /* The following formula comes from the UCL documentation */
    maxsize = insize + (insize / 8) + 256;

    /* Allocate some memory for the output buffer */
    outbuf = malloc(maxsize);

    if (outbuf == NULL) {
        return NULL;
    }
        
    r = ucl_nrv2e_99_compress(
            (const ucl_bytep) inbuf,
            (ucl_uint) insize,
            (ucl_bytep) outbuf,
            (ucl_uintp) outsize,
            0, 10, NULL, NULL);

    if (r != UCL_E_OK || *outsize > maxsize)
    {
        /* this should NEVER happen, and implies memory corruption */
        fprintf(stderr, "internal error - compression failed: %d\n", r);
        free(outbuf);
        return NULL;
    }

    return outbuf;
}

static unsigned char* load_file(char* filename, off_t* bufsize)
{
    int fd;
    unsigned char* buf;
    off_t n;

    fd = open(filename, O_RDONLY|O_BINARY);
    if (fd < 0)
    {
        fprintf(stderr,"[ERR]  Could not open %s for reading\n",filename);
        return NULL;
    }

    *bufsize = filesize(fd);

    buf = malloc(*bufsize);
    if (buf == NULL) {
        fprintf(stderr,"[ERR]  Could not allocate memory for %s\n",filename);
        return NULL;
    }

    n = read(fd, buf, *bufsize);

    if (n != *bufsize) {
        fprintf(stderr,"[ERR]  Could not read file %s\n",filename);
        return NULL;
    }

    return buf;
}


int main(int argc, char* argv[])
{
    char *infile, *bootfile, *outfile;
    int fdout;
    off_t len;
    uint32_t n;
    unsigned char* buf;
    int firmware_size;
    off_t bootloader_size;
    uint32_t sum,filesum;
    uint8_t  model_id;
    int model;
    uint32_t i;
    unsigned char* of_packed;
    int of_packedsize;
    unsigned char* rb_unpacked;
    unsigned char* rb_packed;
    int rb_packedsize;
    int fw_version;
    int totalsize;
    unsigned char* p;

    fprintf(stderr,"mkamsboot v" VERSION " - (C) Dave Chapman 2008\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

    if(argc != 4) {
        printf("Usage: mkamsboot <firmware file> <boot file> <output file>\n\n");
        return 1;
    }

    infile = argv[1];
    bootfile = argv[2];
    outfile = argv[3];

    /* Load bootloader file */
    rb_unpacked = load_file(bootfile, &bootloader_size);
    if (rb_unpacked == NULL) {
        fprintf(stderr,"[ERR]  Could not load %s\n",bootfile);
        return 1;
    }

    /* Load original firmware file */
    buf = load_file(infile, &len);

    if (buf == NULL) {
        fprintf(stderr,"[ERR]  Could not load bootloader file\n");
        return 1;
    }

    /* TODO: Do some more sanity checks on the OF image - e.g. checksum */

    if (get_uint32le(&buf[0x204])==0x0000f000) {
        fw_version = 2;
        model_id = buf[0x219];
    } else {
        fw_version = 1;
        model_id = buf[0x215];
    }

    model = get_model(model_id);

    if (model == MODEL_UNKNOWN) {
        fprintf(stderr,"[ERR]  Unknown firmware - model id 0x%02x\n",model_id);
        return 1;
    }

    if (bootloaders[model] == NULL) {
        fprintf(stderr,"[ERR]  Unsupported model - \"%s\"\n",model_names[model]);
        free(buf);
        free(rb_unpacked);
        return 1;
    }

    printf("[INFO] Patching %s firmware\n",model_names[model]);

    /* Get the firmware size */
    firmware_size = get_uint32le(&buf[0x0c]);

    /* Compress the original firmware image */
    of_packed = uclpack(buf + 0x400, firmware_size, &of_packedsize);
    if (of_packed == NULL) {
        fprintf(stderr,"[ERR]  Could not compress original firmware\n");
        free(buf);
        free(rb_unpacked);
        return 1;
    }

    rb_packed = uclpack(rb_unpacked, bootloader_size, &rb_packedsize);
    if (rb_packed == NULL) {
        fprintf(stderr,"[ERR]  Could not compress %s\n",bootfile);
        free(buf);
        free(rb_unpacked);
        free(of_packed);
        return 1;
    }

    /* We are finished with the unpacked version of the bootloader */
    free(rb_unpacked);

    fprintf(stderr,"[INFO] Original firmware size:   %d bytes\n",firmware_size);
    fprintf(stderr,"[INFO] Packed OF size:           %d bytes\n",of_packedsize);
    fprintf(stderr,"[INFO] Bootloader size:          %d bytes\n",(int)bootloader_size);
    fprintf(stderr,"[INFO] Packed bootloader size:   %d bytes\n",rb_packedsize);
    fprintf(stderr,"[INFO] Dual-boot function size:  %d bytes\n",bootloader_sizes[model]);
    fprintf(stderr,"[INFO] UCL unpack function size: %d bytes\n",sizeof(uclimg));

    totalsize = bootloader_sizes[model] + sizeof(uclimg) + of_packedsize + 
                rb_packedsize;

    fprintf(stderr,"[INFO] Total size of new image:  %d bytes\n",totalsize);

    if (totalsize > firmware_size) {
        fprintf(stderr,"[ERR]  No room to insert bootloader, aborting\n");
        free(buf);
        free(rb_unpacked);
        free(of_packed);
        return 1;
    }

    /* Zero the original firmware area - not needed, but helps debugging */
    memset(buf + 0x400, 0, firmware_size);


    /* Insert dual-boot bootloader at offset 0 */
    memcpy(buf + 0x400, bootloaders[model], bootloader_sizes[model]);

    /* We are filling the firmware buffer backwards from the end */
    p = buf + 0x400 + firmware_size;

    /* 1 - UCL unpack function */
    p -= sizeof(uclimg);
    memcpy(p, uclimg, sizeof(uclimg));

    /* 2 - Compressed copy of original firmware */
    p -= of_packedsize;
    memcpy(p, of_packed, of_packedsize);

    /* 3 - Compressed copy of Rockbox bootloader */
    p -= rb_packedsize;
    memcpy(p, rb_packed, rb_packedsize);

    /* Write the locations of the various images to the variables at the 
       start of the dualboot image - we save the location of the last byte
       in each image, along with the size in bytes */

    /* UCL unpack function */
    put_uint32le(&buf[0x420], firmware_size - 1);
    put_uint32le(&buf[0x424], sizeof(uclimg));

    /* Compressed original firmware image */
    put_uint32le(&buf[0x428], firmware_size - sizeof(uclimg) - 1);
    put_uint32le(&buf[0x42c], of_packedsize);

    /* Compressed Rockbox image */
    put_uint32le(&buf[0x430], firmware_size - sizeof(uclimg) - of_packedsize - 1);
    put_uint32le(&buf[0x434], rb_packedsize);


    /* Update the firmware block checksum */
    sum = calc_checksum(buf + 0x400,firmware_size);

    if (fw_version == 1) {
        put_uint32le(&buf[0x04], sum);
        put_uint32le(&buf[0x204], sum);
    } else {
        /* TODO: Verify that this is correct for the v2 firmware */

        put_uint32le(&buf[0x08], sum);
        put_uint32le(&buf[0x208], sum);

        /* Update the header checksums */
        put_uint32le(&buf[0x1fc], calc_checksum(buf, 0x1fc));
        put_uint32le(&buf[0x3fc], calc_checksum(buf + 0x200, 0x1fc));
    }

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

    fprintf(stderr," *****************************************************************************\n");
    fprintf(stderr," *** THIS CODE IS UNTESTED - DO NOT USE IF YOU CAN NOT RECOVER YOUR DEVICE ***\n");
    fprintf(stderr," *****************************************************************************\n");

    return 0;

}
