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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <stdint.h>

#if defined(WIN32)
#  define DIR_SEPARATOR '\\'
#else
#  define DIR_SEPARATOR '/'
#endif

#define MAX_PATH                    255

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

#define MD5_DIGEST_LENGTH           16

extern char* firmware_components[];
extern char* firmware_filenames[];
extern uint8_t g_yp_key[128];

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

void cyclic_xor(void *data, int datasize, void *xor, int xorsize);
size_t get_filesize(FILE* handle);
void join_path(char* destination, char* first, char* second);
void md5sum(char* component_checksum, char* data, unsigned long size);

#endif /* _COMMON_H_ */
