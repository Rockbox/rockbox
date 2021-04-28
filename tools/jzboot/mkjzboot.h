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

#ifndef MKJZBOOT_H
#define MKJZBOOT_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* The images produced by mkjzboot are simple 'blob lists'. Each blob can be
 * individually UCL-compressed and targets can use as many blobs as they need
 * and in any order they need.
 *
 * All integer constants are serialized as little-endian. Blob sizes are the
 * size in bytes of the blob data, but padding will be added at the end so
 * that the next blob structure is always aligned to a 4-byte boundary.
 *
 * File format definition:
 *
 * struct file {
 *     char file_magic[4];
 *     char model_magic[4];
 *     char rbversion[32];
 *     uint32_t blob_count;
 *     struct blob blobs[blob_count];
 * };
 *
 * struct blob {
 *     char type[4];
 *     uint32_t flags;
 *     uint32_t size;
 *     uint32_t crc32;
 *     char data[size];
 *     char padding[0-3 bytes];
 * };
 */

/*** BEGIN CONSTANTS ***/
/* (These must be kept in sync with target code & the build system) */

/* magic strings, used to identify the file type and player model */
#define JZBOOT_MAGIC    "rbx1"
#define FIIOM3K_MAGIC   "_m3k"

/* length of the version string; the field is padded with NULs */
#define VERSTR_LENGTH   32

/* blob types, targets can define anything they like */
#define BLOBTYPE_SPL    "_spl"
#define BLOBTYPE_BOOT   "boot"

/* blob flags */
#define BLOBFLAG_NONE   0x00    /* no special handling */
#define BLOBFLAG_UCL    0x01    /* blob is UCL compressed (method NRV2e) */

/*** END CONSTANTS ***/

#define FLASHTYPE_NAND  0x00
#define FLASHTYPE_NOR   0xaa

uint8_t* loadfile(const char* filename, size_t* length);
uint8_t* uclpack(const uint8_t* buf, size_t size, size_t* outsize);
uint8_t calc_crc7(const uint8_t* buf, size_t len);
uint32_t crc_32(const void *src, uint32_t len, uint32_t crc32);
void to_le32(uint8_t* p, uint32_t x);

uint8_t* mkspl_flash(const uint8_t* spl_data, size_t spl_length,
                     int flashtype, uint8_t ppb, uint8_t bpp,
                     size_t* outsize);

int write_blob(FILE* outf, const char* type, uint32_t flags,
               const uint8_t* data, size_t length);
int write_blobfile(FILE* outf, const char* type, uint32_t flags,
                   const char* filename);
int write_blobheader(FILE* outf, const char* model,
                     const char* rbversion, int num_blobs);

int mkboot_fiiom3k(int argc, char** argv);

#endif /* MKJZBOOT_H */
