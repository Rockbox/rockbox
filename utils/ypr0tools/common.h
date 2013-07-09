/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 Lorenzo Miori
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

#ifndef _COMMON_H_
#define _COMMON_H_

#include <openssl/md5.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <stdint.h>

/*
 * Firmware description
 */

#define GENERIC_HEADER_LINES        5
#define MAX_HEADER_LEN              1000
/* Empty space used by bootloader to store checksums */
#define MBOOT_CHECKSUM_OFFSET       96
/* Length of the reserved space */
#define MBOOT_CHECKSUM_LENGTH       992

/* In case we don't have RevisionInfo.txt file, mock values are fine */
#define YPR0_VERSION        "Version : V1.25\n"
#define YPR0_TARGET         "Target : KR\n"
#define YPR0_USER           "User : rockbox\n"
#define YPR0_DIR            "Dir : /.rockbox\n"
#define YPR0_TIME           "BuildTime : 11/04/20 14:17:34\n"

#define YPR0_COMPONENTS_COUNT       4
char* firmware_components[] = {"MBoot", "Linux", "RootFS", "Sysdata"};
char* firmware_filenames[]  = {"MBoot.bin", "zImage", "cramfs-fsl.rom", "SYSDATA.bin"};

struct firmware_data
{
    char* component_data[YPR0_COMPONENTS_COUNT];
    size_t component_size[YPR0_COMPONENTS_COUNT];
    char component_checksum[YPR0_COMPONENTS_COUNT][MD5_DIGEST_LENGTH*2+1];
};

enum samsung_error_t
{
    SAMSUNG_SUCCESS = 0,
    SAMSUNG_READ_ERROR = -1,
    SAMSUNG_FORMAT_ERROR = -2,
    SAMSUNG_MD5_ERROR = -3,
    SAMSUNG_WRITE_ERROR = -4,
};

uint8_t g_yp_key[128] =
{
  0xa3, 0x04, 0xb9, 0xcd, 0x34, 0x13, 0x4a, 0x19, 0x19, 0x31, 0xdf, 0xbb,
  0x8f, 0x3d, 0x7f, 0x09, 0x42, 0x3c, 0x96, 0x33, 0x41, 0xa9, 0x95, 0xf1,
  0xd0, 0xac, 0x16, 0x37, 0x57, 0x35, 0x28, 0xe7, 0x0b, 0xc2, 0x12, 0x09,
  0x39, 0x42, 0xd2, 0x96, 0xf5, 0x00, 0xd2, 0x23, 0x37, 0x24, 0xe2, 0x8e,
  0x50, 0x3c, 0x6e, 0x23, 0xeb, 0x68, 0xed, 0x31, 0xb7, 0xee, 0xc0, 0xc7,
  0x09, 0xf8, 0x39, 0x9d, 0x51, 0xed, 0x17, 0x95, 0x64, 0x09, 0xe0, 0xf9,
  0xf0, 0xef, 0x86, 0xc0, 0x04, 0x46, 0x89, 0x8a, 0x6e, 0x27, 0x69, 0xde,
  0xc7, 0x31, 0x1e, 0xee, 0x3c, 0x3f, 0x17, 0x05, 0x44, 0xbb, 0xbb, 0x1d,
  0x3d, 0x5d, 0x6e, 0xf2, 0x78, 0x15, 0xd6, 0x3c, 0xcc, 0x7d, 0x67, 0x1a,
  0xb8, 0xd2, 0x79, 0x54, 0x97, 0xa2, 0x58, 0x58, 0xf7, 0x4e, 0x5e, 0x50,
  0x42, 0x69, 0xdc, 0xe7, 0x3a, 0x87, 0x2e, 0x22
};

void cyclic_xor(void *data, int datasize, void *xor, int xorsize);
size_t get_filesize(FILE* handle);
void md5sum(char* component_checksum, char* data, unsigned long size);

#endif /* _COMMON_H_ */