/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <endian.h> /* TODO: find portable alternative */

#define SPL_HEADER_SIZE 512
#define SPL_KEY_SIZE    1536
#define SPL_CODE_OFFSET (SPL_HEADER_SIZE + SPL_KEY_SIZE)

#define SPL_MAX_SIZE    (12 * 1024)
#define SPL_CODE_SIZE   (SPL_MAX_SIZE - SPL_CODE_OFFSET)

#define FLASH_TYPE_NAND 0x00
#define FLASH_TYPE_NOR  0xaa

struct flash_sig {
    uint8_t magic[8];   /* fixed magic bytes */
    uint8_t type;       /* flash type, either NAND or NOR */
    uint8_t crc7;       /* CRC7 of SPL code */
    uint8_t ppb;        /* pages per block for NAND */
    uint8_t bpp;        /* bytes per page for NAND */
    uint32_t length;    /* total size of SPL, including header/key */
};

static const uint8_t flash_sig_magic[8] =
    {0x06, 0x05, 0x04, 0x03, 0x02, 0x55, 0xaa, 0x55};

/* calculate CRC7 used in flash header */
uint8_t crc7(const uint8_t* buf, size_t len)
{
    /* table-based computation of CRC7 */
    static const uint8_t t[256] = {
        0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
        0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
        0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
        0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
        0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
        0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
        0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
        0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
        0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
        0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
        0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
        0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
        0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
        0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
        0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
        0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
        0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
        0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
        0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
        0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
        0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
        0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
        0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
        0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
        0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
        0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
        0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
        0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
        0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
        0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
        0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
        0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
    };

    uint8_t crc = 0;
    while(len--)
        crc = t[(crc << 1) ^ *buf++];
    return crc;
}

void die(const char* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

void do_flash(const uint8_t* spl_code,
              uint32_t spl_length,
              int flash_type,
              int pages_per_block,
              int bytes_per_block,
              FILE* outfile)
{
    /* verify parameters */
    if(flash_type != FLASH_TYPE_NAND
    && flash_type != FLASH_TYPE_NOR) {
        die("invalid flash type");
    }

    if(pages_per_block < 0 || pages_per_block > 0xff)
        die("invalid pages_per_block");

    if(bytes_per_block < 0 || bytes_per_block > 0xff)
        die("invalid bytes_per_block");

    /* fill flash signature */
    struct flash_sig sig;
    memcpy(sig.magic, flash_sig_magic, 8);
    sig.type = flash_type & 0xff;
    sig.crc7 = crc7(spl_code, spl_length);
    sig.ppb = pages_per_block & 0xff;
    sig.bpp = bytes_per_block & 0xff;
    sig.length = htole32(spl_length + SPL_HEADER_SIZE + SPL_KEY_SIZE);

    /* construct header */
    uint8_t hdr_buf[SPL_HEADER_SIZE];
    memset(hdr_buf, 0, SPL_HEADER_SIZE);
    memcpy(hdr_buf, &sig, sizeof(sig));

    /* create null key */
    uint8_t null_key[SPL_KEY_SIZE];
    memset(null_key, 0, SPL_KEY_SIZE);

    /* write everything out */
    if(fwrite(hdr_buf, SPL_HEADER_SIZE, 1, outfile) != 1)
        die("error writing output");
    if(fwrite(null_key, SPL_KEY_SIZE, 1, outfile) != 1)
        die("error writing output");
    if(fwrite(spl_code, spl_length, 1, outfile) != 1)
        die("error writing output");
}

uint8_t* read_input(const char* filename, uint32_t* rlength, int maxlen)
{
    /* open file */
    FILE* f = fopen(filename, "rb");
    if(!f)
        die("cannot open input file: %s", filename);

    /* get size */
    fseek(f, 0, SEEK_END);
    int sz = ftell(f);

    /* check size */
    if(sz < 0)
        die("error seeking in input file");
    if(sz > maxlen)
        die("input is too big (size: %d, max: %d)", sz, maxlen);

    /* allocate buffer */
    uint8_t* buf = malloc(sz);
    if(!buf)
        die("out of memory");

    /* read file data */
    fseek(f, 0, SEEK_SET);
    if(fread(buf, sz, 1, f) != 1)
        die("error reading input file");

    /* close file */
    fclose(f);

    /* return buffer and size */
    *rlength = sz;
    return buf;
}

int main(int argc, const char* argv[])
{
    /* variables */
    int flash_type = -1;
    int pages_per_block = -1;
    int bytes_per_page = -1;
    uint8_t* spl_code = NULL;
    uint32_t spl_length = 0;
    const char* outfile_name = NULL;

    for(int i = 1; i < argc; ++i) {
        if(strncmp(argv[i], "-type=", 6) == 0) {
            if(strcmp(argv[i]+6, "nand") == 0)
                flash_type = FLASH_TYPE_NAND;
            else if(strcmp(argv[i]+6, "nor") == 0)
                flash_type = FLASH_TYPE_NOR;
            else
                die("invalid type: %s", argv[i]+6);
        } else if(strncmp(argv[i], "-ppb=", 5) == 0) {
            pages_per_block = atoi(argv[i]+5);
        } else if(strncmp(argv[i], "-bpp=", 5) == 0) {
            bytes_per_page = atoi(argv[i]+5);
        } else if(spl_code == NULL) {
            spl_code = read_input(argv[i], &spl_length, SPL_CODE_SIZE);
        } else if(!outfile_name) {
            outfile_name = argv[i];
        } else {
            die("too many arguments: %s", argv[i]);
        }
    }

    if(flash_type < 0)
        die("must specify -type");

    if(flash_type == FLASH_TYPE_NAND) {
        if(pages_per_block < 0)
            die("must specify -ppb with -type nand");
        if(bytes_per_page < 0)
            die("must specify -bpp with -type nand");
    } else {
        pages_per_block = 0;
        bytes_per_page = 0;
    }

    if(spl_code == NULL)
        die("no input file given");

    if(!outfile_name)
        die("no output file given");

    FILE* outfile = fopen(outfile_name, "wb");
    if(!outfile)
        die("cannot open output file: %s", outfile_name);

    do_flash(spl_code, spl_length, flash_type,
             pages_per_block, bytes_per_page, outfile);

    fclose(outfile);
    return 0;
}
