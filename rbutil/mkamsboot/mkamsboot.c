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

Insert a Rockbox bootloader into a Sansa AMS original firmware file.

Layout of a Sansa AMS original firmware file:

 ----------------------  0x0
|        HEADER        |
|----------------------| 0x400
|    FIRMWARE BLOCK    |
|----------------------| 0x400 + firmware block size
|    LIBRARIES/DATA    |
 ----------------------  END

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
to store the compressed version of the Rockbox bootloader.

On version 1 firmwares, the OF image is typically about 120KB, which allows
us to store a Rockbox bootloader with an uncompressed size of about 60KB-70KB.
Version 2 firmwares are bigger and are stored in SDRAM (instead of IRAM).
In both cases, the RAM we are using is mapped at offset 0x0.

mkamsboot then corrects the checksums and writes a new legal firmware
file which can be installed on the device.

When the Sansa device boots, this firmware block is loaded to RAM at
address 0x0 and executed.

Firstly, the dual-boot code will copy the UCL unpack function to the
end of RAM.

Then, depending on the detection of the dual-boot keypress, either the
OF image or the Rockbox image is copied to the end of RAM (just before
the ucl unpack function) and uncompressed to the start of RAM.

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

#include "mkamsboot.h"

#include "md5.h"

/* Header for ARM code binaries */
#include "dualboot.h"

/* Win32 compatibility */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/* 4 for m200, 2 for e200/c200, 1 or 2 for fuze/clip, 1 for clip+ */
const unsigned short hw_revisions[] = {
    [MODEL_FUZE]    = 1,
    [MODEL_CLIP]    = 1,
    [MODEL_CLIPV2]  = 2,
    [MODEL_E200V2]  = 2,
    [MODEL_M200V4]  = 4,
    [MODEL_C200V2]  = 2,
    [MODEL_CLIPPLUS]= 1,
    [MODEL_FUZEV2]  = 2,
};

/* version 2 is used in Clipv2, Clip+ and Fuzev2 firmwares */
const unsigned short fw_revisions[] = {
    [MODEL_FUZE]    = 1,
    [MODEL_CLIP]    = 1,
    [MODEL_CLIPV2]  = 2,
    [MODEL_E200V2]  = 1,
    [MODEL_M200V4]  = 1,
    [MODEL_C200V2]  = 1,
    [MODEL_CLIPPLUS]= 2,
    [MODEL_FUZEV2]  = 2,
};

/* Descriptive name of these models */
const char* model_names[] = {
    [MODEL_FUZE]    = "Fuze",
    [MODEL_CLIP]    = "Clip",
    [MODEL_CLIPV2]  = "Clip",
    [MODEL_CLIPPLUS]= "Clip+",
    [MODEL_E200V2]  = "e200",
    [MODEL_M200V4]  = "m200",
    [MODEL_C200V2]  = "c200",
    [MODEL_FUZEV2]  = "Fuze",
};

/* Dualboot functions for these models */
static const unsigned char* bootloaders[] = {
    [MODEL_FUZE]    = dualboot_fuze,
    [MODEL_CLIP]    = dualboot_clip,
    [MODEL_CLIPV2]  = dualboot_clipv2,
    [MODEL_E200V2]  = dualboot_e200v2,
    [MODEL_M200V4]  = dualboot_m200v4,
    [MODEL_C200V2]  = dualboot_c200v2,
    [MODEL_CLIPPLUS]= dualboot_clipplus,
    [MODEL_FUZEV2]  = dualboot_fuzev2,
};

/* Size of dualboot functions for these models */
const int bootloader_sizes[] = {
    [MODEL_FUZE]    = sizeof(dualboot_fuze),
    [MODEL_CLIP]    = sizeof(dualboot_clip),
    [MODEL_CLIPV2]  = sizeof(dualboot_clipv2),
    [MODEL_E200V2]  = sizeof(dualboot_e200v2),
    [MODEL_M200V4]  = sizeof(dualboot_m200v4),
    [MODEL_C200V2]  = sizeof(dualboot_c200v2),
    [MODEL_CLIPPLUS]= sizeof(dualboot_clipplus),
    [MODEL_FUZEV2]  = sizeof(dualboot_fuzev2),
};

/* Model names used in the Rockbox header in ".sansa" files - these match the
   -add parameter to the "scramble" tool */
static const char* rb_model_names[] = {
    [MODEL_FUZE]    = "fuze",
    [MODEL_CLIP]    = "clip",
    [MODEL_CLIPV2]  = "clv2",
    [MODEL_E200V2]  = "e2v2",
    [MODEL_M200V4]  = "m2v4",
    [MODEL_C200V2]  = "c2v2",
    [MODEL_CLIPPLUS]= "cli+",
    [MODEL_FUZEV2]  = "fuz2",
};

/* Model numbers used to initialise the checksum in the Rockbox header in
   ".sansa" files - these are the same as MODEL_NUMBER in config-target.h */
static const int rb_model_num[] = {
    [MODEL_FUZE]    = 43,
    [MODEL_CLIP]    = 40,
    [MODEL_CLIPV2]  = 60,
    [MODEL_E200V2]  = 41,
    [MODEL_M200V4]  = 42,
    [MODEL_C200V2]  = 44,
    [MODEL_CLIPPLUS]= 66,
    [MODEL_FUZEV2]  = 68,
};

/* Checksums of unmodified original firmwares - for safety, and device
   detection */
static struct md5sums sansasums[] = {
    /* NOTE: Different regional versions of the firmware normally only
             differ in the filename - the md5sums are identical */

    /*   model       version    md5                        */
    { MODEL_E200V2, "3.01.11", "e622ca8cb6df423f54b8b39628a1f0a3" },
    { MODEL_E200V2, "3.01.14", "2c1d0383fc3584b2cc83ba8cc2243af6" },
    { MODEL_E200V2, "3.01.16", "12563ad71b25a1034cf2092d1e0218c4" },

    { MODEL_FUZE,   "1.01.11", "cac8ffa03c599330ac02c4d41de66166" },
    { MODEL_FUZE,   "1.01.15", "df0e2c1612727f722c19a3c764cff7f2" },
    { MODEL_FUZE,   "1.01.22", "5aff5486fe8dd64239cc71eac470af98" },
    { MODEL_FUZE,   "1.02.26", "7c632c479461c48c8833baed74eb5e4f" },
    { MODEL_FUZE,   "1.02.28", "5b34260f6470e75f702a9c6825471752" },
    { MODEL_FUZE,   "1.02.31", "66d01b37462a5ef7ccc6ad37188b4235" },

    { MODEL_C200V2, "3.02.05", "b6378ebd720b0ade3fad4dc7ab61c1a5" },

    { MODEL_M200V4, "4.00.45", "82e3194310d1514e3bbcd06e84c4add3" },
    { MODEL_M200V4, "4.01.08-A", "fc9dd6116001b3e6a150b898f1b091f0" },
    { MODEL_M200V4, "4.01.08-E", "d3fb7d8ec8624ee65bc99f8dab0e2369" },

    { MODEL_CLIP,   "1.01.17", "12caad785d506219d73f538772afd99e" },
    { MODEL_CLIP,   "1.01.18", "d720b266bd5afa38a198986ef0508a45" },
    { MODEL_CLIP,   "1.01.20", "236d8f75189f468462c03f6d292cf2ac" },
    { MODEL_CLIP,   "1.01.29", "c12711342169c66e209540cd1f27cd26" },
    { MODEL_CLIP,   "1.01.30", "f2974d47c536549c9d8259170f1dbe4d" },
    { MODEL_CLIP,   "1.01.32", "d835d12342500732ffb9c4ee54abec15" },

    { MODEL_CLIPV2, "2.01.16", "c57fb3fcbe07c2c9b360f060938f80cb" },
    { MODEL_CLIPV2, "2.01.32", "0ad3723e52022509089d938d0fbbf8c5" },

    { MODEL_CLIPPLUS, "01.02.09", "656d38114774c2001dc18e6726df3c5d" },

    { MODEL_FUZEV2, "2.01.17", "8b85fb05bf645d08a4c8c3e344ec9ebe" },
    { MODEL_FUZEV2, "2.02.26", "d4f6f85c3e4a8ea8f2e5acc421641801" },
};

#define NUM_MD5S (sizeof(sansasums)/sizeof(sansasums[0]))

static unsigned int model_memory_size(int model)
{
    if(model == MODEL_CLIPV2)
    {
        /* The decompressed Clipv2 OF is around 380kB.
         * Since it doesn't fit in the 0x50000 bytes IRAM, the OF starts
         * with DRAM mapped at 0x0
         *
         * We could use all the available memory (supposedly 8MB)
         * but 1MB ought to be enough for our use
         */
        return 1 << 20;
    }
    else
    {   /* The OF boots with IRAM (320kB) mapped at 0x0 */
        return 320 << 10;
    }
}

int firmware_revision(int model)
{
    return fw_revisions[model];
}

static off_t filesize(int fd)
{
    struct stat buf;

    if (fstat(fd, &buf) < 0) {
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

/* Calculate a simple checksum used in Sansa Original Firmwares */
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
    switch(model_id) {
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
        case 0x28:
            return MODEL_CLIPPLUS;
        case 0x70:
            return MODEL_FUZEV2;
    }

    return MODEL_UNKNOWN;
}

/* Compress using nrv2e algorithm : Thumb decompressor fits in 168 bytes ! */
static unsigned char* uclpack(unsigned char* inbuf, int insize, int* outsize)
{
    int maxsize;
    unsigned char* outbuf;
    int r;

    /* The following formula comes from the UCL documentation */
    maxsize = insize + (insize / 8) + 256;

    /* Allocate some memory for the output buffer */
    outbuf = malloc(maxsize);

    if (outbuf == NULL)
        return NULL;

    r = ucl_nrv2e_99_compress(
            (const ucl_bytep) inbuf,
            (ucl_uint) insize,
            (ucl_bytep) outbuf,
            (ucl_uintp) outsize,
            0, 10, NULL, NULL);

    if (r != UCL_E_OK || *outsize > maxsize) {
        /* this should NEVER happen, and implies memory corruption */
        fprintf(stderr, "internal error - compression failed: %d\n", r);
        free(outbuf);
        return NULL;
    }

    return outbuf;
}

#define ERROR(format, ...) \
    do { \
        snprintf(errstr, errstrsize, format, __VA_ARGS__); \
        goto error; \
    } while(0)

/* Loads a Sansa AMS Original Firmware file into memory */
unsigned char* load_of_file(
        char* filename, off_t* bufsize, struct md5sums *sum,
        int* firmware_size, unsigned char** of_packed,
        int* of_packedsize, char* errstr, int errstrsize)
{
    int fd;
    unsigned char* buf =NULL;
    off_t n;
    unsigned int i=0;
    uint32_t checksum;
    int model_id;
    unsigned int last_word;

    fd = open(filename, O_RDONLY|O_BINARY);
    if (fd < 0)
        ERROR("[ERR]  Could not open %s for reading\n", filename);

    *bufsize = filesize(fd);

    buf = malloc(*bufsize);
    if (buf == NULL)
        ERROR("[ERR]  Could not allocate memory for %s\n", filename);

    n = read(fd, buf, *bufsize);

    if (n != *bufsize)
        ERROR("[ERR]  Could not read file %s\n", filename);

    /* check the file */

    /* Calculate MD5 checksum of OF */
    calc_MD5(buf, *bufsize, sum->md5);

    while ((i < NUM_MD5S) && (strcmp(sansasums[i].md5, sum->md5) != 0))
        i++;

    if (i < NUM_MD5S) {
        *sum = sansasums[i];
    } else {
        int fw_version = (get_uint32le(&buf[0x204]) == 0x0000f000) ? 2 : 1;
        model_id = buf[(fw_version == 2) ? 0x219 : 0x215];
        sum->model = get_model(model_id);

        if (sum->model == MODEL_UNKNOWN)
            ERROR("[ERR]  Unknown firmware model (v%d) - model id 0x%02x\n",
                  fw_version, model_id);

#if 1   /* comment to test new OFs */
        char tested_versions[100];
        tested_versions[0] = '\0';

        for (i = 0; i < NUM_MD5S ; i++)
            if (sansasums[i].model == sum->model) {
                if (tested_versions[0] != '\0') {
                    strncat(tested_versions, ", ",
                        sizeof(tested_versions) - strlen(tested_versions) - 1);
                }
                strncat(tested_versions, sansasums[i].version,
                    sizeof(tested_versions) - strlen(tested_versions) - 1);
            }

        ERROR("[ERR]  Original firmware unknown, please try an other version." \
              " Tested %s versions are : %s\n",
              model_names[sum->model], tested_versions);
#endif
    }

    /* TODO: Do some more sanity checks on the OF image. Some images (like
       m200v4) dont have a checksum at the end, only padding (0xdeadbeef). */
    last_word = *bufsize - 4;
    checksum = get_uint32le(buf + last_word);
    if (checksum != 0xefbeadde && checksum != calc_checksum(buf, last_word))
        ERROR("%s", "[ERR]  Whole file checksum failed\n");

    if (bootloaders[sum->model] == NULL)
        ERROR("[ERR]  Unsupported model - \"%s\"\n", model_names[sum->model]);

    /* Get the firmware size */
    if (fw_revisions[sum->model] == 1)
        *firmware_size = get_uint32le(&buf[0x0c]);
    else if (fw_revisions[sum->model] == 2)
        *firmware_size = get_uint32le(&buf[0x10]);

    /* Compress the original firmware image */
    *of_packed = uclpack(buf + 0x400, *firmware_size, of_packedsize);
    if (*of_packed == NULL)
        ERROR("[ERR]  Could not compress %s\n", filename);

    return buf;

error:
    free(buf);
    return NULL;
}

/* Loads a rockbox bootloader file into memory */
unsigned char* load_rockbox_file(
            char* filename, int model, int* bufsize, int* rb_packedsize,
            char* errstr, int errstrsize)
{
    int fd;
    unsigned char* buf = NULL;
    unsigned char* packed = NULL;
    unsigned char header[8];
    uint32_t sum;
    off_t n;
    int i;

    fd = open(filename, O_RDONLY|O_BINARY);
    if (fd < 0)
        ERROR("[ERR]  Could not open %s for reading\n", filename);

    /* Read Rockbox header */
    n = read(fd, header, sizeof(header));
    if (n != sizeof(header))
        ERROR("[ERR]  Could not read file %s\n", filename);

    /* Check for correct model string */
    if (memcmp(rb_model_names[model], header + 4, 4)!=0)
        ERROR("[ERR]  Expected model name \"%s\" in %s, not \"%4.4s\"\n",
                       rb_model_names[model], filename, (char*)header+4);

    *bufsize = filesize(fd) - sizeof(header);

    buf = malloc(*bufsize);
    if (buf == NULL)
        ERROR("[ERR]  Could not allocate memory for %s\n", filename);

    n = read(fd, buf, *bufsize);

    if (n != *bufsize)
        ERROR("[ERR]  Could not read file %s\n", filename);

    /* Check checksum */
    sum = rb_model_num[model];
    for (i = 0; i < *bufsize; i++) {
         /* add 8 unsigned bits but keep a 32 bit sum */
         sum += buf[i];
    }

    if (sum != get_uint32be(header))
        ERROR("[ERR]  Checksum mismatch in %s\n", filename);

    packed = uclpack(buf, *bufsize, rb_packedsize);
    if(packed == NULL)
        ERROR("[ERR]  Could not compress %s\n", filename);

    free(buf);
    return packed;

error:
    free(buf);
    return NULL;
}

#undef ERROR

/* Patches a Sansa AMS Original Firmware file */
void patch_firmware(
        int model, int fw_revision, int firmware_size, unsigned char* buf,
        int len, unsigned char* of_packed, int of_packedsize,
        unsigned char* rb_packed, int rb_packedsize)
{
    unsigned char *p;
    uint32_t sum, filesum;
    uint32_t ucl_dest;
    unsigned int i;

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
    put_uint32le(&buf[0x430], firmware_size - sizeof(nrv2e_d8) - of_packedsize
            - 1);
    put_uint32le(&buf[0x434], rb_packedsize);

    ucl_dest = model_memory_size(model) - 1; /* last byte of memory */
    put_uint32le(&buf[0x438], ucl_dest);

    /* Update the firmware block checksum */
    sum = calc_checksum(buf + 0x400, firmware_size);

    if (fw_revision == 1) {
        put_uint32le(&buf[0x04], sum);
        put_uint32le(&buf[0x204], sum);
    } else if (fw_revision == 2) {
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
}

/* returns != 0 if the firmware can be safely patched */
int check_sizes(int model, int rb_packed_size, int rb_unpacked_size,
        int of_packed_size, int of_unpacked_size, int *total_size,
        char *errstr, int errstrsize)
{
    unsigned int packed_size = bootloader_sizes[model] + sizeof(nrv2e_d8) +
            of_packed_size + rb_packed_size;

    /* how much memory is available */
    unsigned int memory_size = model_memory_size(model);

    /* the memory used when unpacking the OF */
    unsigned int ram_of = sizeof(nrv2e_d8) + of_packed_size + of_unpacked_size;

    /* the memory used when unpacking the bootloader */
    unsigned int ram_rb = sizeof(nrv2e_d8) + rb_packed_size + rb_unpacked_size;

    *total_size = packed_size;

#define ERROR(format, ...) \
    do { \
        snprintf(errstr, errstrsize, format, __VA_ARGS__); \
        return 0; \
    } while(0)

    /* will packed data fit in the OF file ? */
    if(packed_size > of_unpacked_size)
        ERROR(
            "[ERR]  Packed data (%d bytes) doesn't fit in the firmware "
            "(%d bytes)\n", packed_size, of_unpacked_size
            );

    else if(ram_rb > memory_size)
        ERROR("[ERR]  Rockbox can't be unpacked at runtime, needs %d bytes "
            "of memory and only %d available\n", ram_rb, memory_size
            );

    else if(ram_of > memory_size)
        ERROR("[ERR]  OF can't be unpacked at runtime, needs %d bytes "
            "of memory and only %d available\n", ram_of, memory_size
            );

    return 1;

#undef ERROR
}
