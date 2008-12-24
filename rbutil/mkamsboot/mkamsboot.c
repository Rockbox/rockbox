/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "nrv2e_d8.h"
#include "md5.h"

#include "dualboot_clip.h"
#include "dualboot_e200v2.h"
#include "dualboot_fuze.h"
#include "dualboot_m200v4.h"
#include "dualboot_c200v2.h"

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
    MODEL_E200V2,
    MODEL_M200V4,
    MODEL_C200V2,
};

static const char* model_names[] = 
{
    "Fuze",
    "Clip",
    "Clip V2",
    "e200 v2",
    "m200 v4",
    "c200 v2"
};

static const unsigned char* bootloaders[] = 
{
    dualboot_fuze,
    dualboot_clip,
    NULL,
    dualboot_e200v2,
    dualboot_m200v4,
    dualboot_c200v2,
};

static const int bootloader_sizes[] = 
{
    sizeof(dualboot_fuze),
    sizeof(dualboot_clip),
    0,
    sizeof(dualboot_e200v2),
    sizeof(dualboot_m200v4),
    sizeof(dualboot_c200v2),
};

/* Model names used in the Rockbox header in ".sansa" files - these match the
   -add parameter to the "scramble" tool */
static const char* rb_model_names[] =
{
    "fuze",
    "clip",
    NULL,
    "e2v2",
    "m2v4",
    "c2v2",
};

/* Model numbers used to initialise the checksum in the Rockbox header in
   ".sansa" files - these are the same as MODEL_NUMBER in config-target.h */
static const int rb_model_num[] =
{
    43,
    40,
    0,
    41,
    42,
    44
};

struct md5sums {
    int model;
    char *version;
    int fw_version;
    char *md5;
};

/* Checksums of unmodified original firmwares - for safety, and device
   detection */
static struct md5sums sansasums[] = {
    /* NOTE: Different regional versions of the firmware normally only
             differ in the filename - the md5sums are identical */
    { MODEL_E200V2, "3.01.11",   1, "e622ca8cb6df423f54b8b39628a1f0a3" },
    { MODEL_E200V2, "3.01.14",   1, "2c1d0383fc3584b2cc83ba8cc2243af6" },
    { MODEL_E200V2, "3.01.16",   1, "12563ad71b25a1034cf2092d1e0218c4" },

    { MODEL_FUZE, "1.01.11",   1, "cac8ffa03c599330ac02c4d41de66166" },
    { MODEL_FUZE, "1.01.15",   1, "df0e2c1612727f722c19a3c764cff7f2" },
    { MODEL_FUZE, "1.01.22",   1, "5aff5486fe8dd64239cc71eac470af98" },

    { MODEL_C200V2, "3.02.05",   1, "b6378ebd720b0ade3fad4dc7ab61c1a5" },

    { MODEL_M200V4, "4.00.45",   1, "82e3194310d1514e3bbcd06e84c4add3" },
    { MODEL_M200V4, "4.01.08-A", 1, "fc9dd6116001b3e6a150b898f1b091f0" },
    { MODEL_M200V4, "4.01.08-E", 1, "d3fb7d8ec8624ee65bc99f8dab0e2369" },

    { MODEL_CLIP, "1.01.17",   1, "12caad785d506219d73f538772afd99e" },
    { MODEL_CLIP, "1.01.18",   1, "d720b266bd5afa38a198986ef0508a45" },
    { MODEL_CLIP, "1.01.20",   1, "236d8f75189f468462c03f6d292cf2ac" },
    { MODEL_CLIP, "1.01.29",   1, "c12711342169c66e209540cd1f27cd26" },
    { MODEL_CLIP, "1.01.30",   1, "f2974d47c536549c9d8259170f1dbe4d" },
};

#define NUM_MD5S (sizeof(sansasums)/sizeof(sansasums[0]))

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

static uint32_t get_uint32be(unsigned char* p)
{
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static void put_uint32le(unsigned char* p, uint32_t x)
{
    p[0] = x & 0xff;
    p[1] = (x >> 8) & 0xff;
    p[2] = (x >> 16) & 0xff;
    p[3] = (x >> 24) & 0xff;
}

void calc_MD5(unsigned char* buf, int len, char *md5str)
{
    int i;
    md5_context ctx;
    unsigned char md5sum[16];
    
    md5_starts(&ctx);
    md5_update(&ctx, buf, len);
    md5_finish(&ctx, md5sum);

    for (i = 0; i < 16; ++i)
        sprintf(md5str + 2*i, "%02x", md5sum[i]);
}


static uint32_t calc_checksum(unsigned char* buf, uint32_t n)
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
            return MODEL_C200V2;
        case 0x24:
            return MODEL_E200V2;
        case 0x25:
            return MODEL_M200V4;
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


static unsigned char* load_rockbox_file(char* filename, int model, off_t* bufsize)
{
    int fd;
    unsigned char* buf;
    unsigned char header[8];
    uint32_t sum;
    off_t n;
    int i;

    fd = open(filename, O_RDONLY|O_BINARY);
    if (fd < 0)
    {
        fprintf(stderr,"[ERR]  Could not open %s for reading\n",filename);
        return NULL;
    }

    /* Read Rockbox header */
    n = read(fd, header, sizeof(header));
    if (n != sizeof(header)) {
        fprintf(stderr,"[ERR]  Could not read file %s\n",filename);
        return NULL;
    }

    /* Check for correct model string */
    if (memcmp(rb_model_names[model],header + 4,4)!=0) {
        fprintf(stderr,"[ERR]  Model name \"%s\" not found in %s\n",
                       rb_model_names[model],filename);
    }

    *bufsize = filesize(fd) - sizeof(header);

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

    /* Check checksum */
    sum = rb_model_num[model];
    for (i = 0; i < *bufsize; i++) {
         /* add 8 unsigned bits but keep a 32 bit sum */
         sum += buf[i];
    }

    if (sum != get_uint32be(header)) {
        fprintf(stderr,"[ERR]  Checksum mismatch in %s\n",filename);
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
    uint32_t checksum;
    char md5sum[33]; /* 32 hex digits, plus terminating zero */

    fprintf(stderr,"mkamsboot v" VERSION " - (C) Dave Chapman and Rafaël Carré 2008\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

    if(argc != 4) {
        printf("Usage: mkamsboot <firmware file> <boot file> <output file>\n\n");
        return 1;
    }

    infile = argv[1];
    bootfile = argv[2];
    outfile = argv[3];

    /* Load original firmware file */
    buf = load_file(infile, &len);

    if (buf == NULL) {
        fprintf(stderr,"[ERR]  Could not load %s\n",infile);
        return 1;
    }

    /* Calculate MD5 checksum of OF */
    calc_MD5(buf, len, md5sum);

    fprintf(stderr,"[INFO] MD5 sum - %s\n",md5sum);

    i = 0;
    while ((i < NUM_MD5S) && (strcmp(sansasums[i].md5, md5sum) != 0))
        i++;

    if (i < NUM_MD5S) {
        model = sansasums[i].model;
        fw_version = sansasums[i].fw_version;
        fprintf(stderr,"[INFO] Original firmware MD5 checksum match - %s %s\n",
                       model_names[model], sansasums[i].version);
    } else {
        fprintf(stderr,"[WARN] ****** Original firmware unknown ******\n");
        
        if (get_uint32le(&buf[0x204])==0x0000f000) {
            fw_version = 2;
            model_id = buf[0x219];
        } else {
            fw_version = 1;
            model_id = buf[0x215];
        }

        model = get_model(model_id);

        if (model == MODEL_UNKNOWN) {
            fprintf(stderr,"[ERR]  Unknown firmware - model id 0x%02x\n",
                           model_id);
            free(buf);
            return 1;
        }
    }
    

    /* TODO: Do some more sanity checks on the OF image. Some images (like
       m200v4) dont have a checksum at the end, only padding (0xdeadbeef). */
    checksum = get_uint32le(buf + len - 4); 
    if (checksum != 0xefbeadde && checksum != calc_checksum(buf, len - 4)) {

        fprintf(stderr,"[ERR]  Whole file checksum failed - %s\n",infile);
        free(buf);
        return 1;
    }

    if (bootloaders[model] == NULL) {
        fprintf(stderr,"[ERR]  Unsupported model - \"%s\"\n",model_names[model]);
        free(buf);
        return 1;
    }

    /* Load bootloader file */
    rb_unpacked = load_rockbox_file(bootfile, model, &bootloader_size);
    if (rb_unpacked == NULL) {
        fprintf(stderr,"[ERR]  Could not load %s\n",bootfile);
        free(buf);
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
    fprintf(stderr,"[INFO] UCL unpack function size: %d bytes\n",sizeof(nrv2e_d8));

    totalsize = bootloader_sizes[model] + sizeof(nrv2e_d8) + of_packedsize + 
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
    p -= sizeof(nrv2e_d8);
    memcpy(p, nrv2e_d8, sizeof(nrv2e_d8));

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
    put_uint32le(&buf[0x424], sizeof(nrv2e_d8));

    /* Compressed original firmware image */
    put_uint32le(&buf[0x428], firmware_size - sizeof(nrv2e_d8) - 1);
    put_uint32le(&buf[0x42c], of_packedsize);

    /* Compressed Rockbox image */
    put_uint32le(&buf[0x430], firmware_size - sizeof(nrv2e_d8) - of_packedsize - 1);
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
