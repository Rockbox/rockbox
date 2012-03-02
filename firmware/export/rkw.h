/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2012 Marcin Bukat
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 1
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdint.h>

#define RKLD_MAGIC     0x4c44524b
#define RKW_HEADER_CRC 0x40000000
#define RKW_IMAGE_CRC  0x20000000

struct rkw_header_t {
    uint32_t magic_number;    /* Magic number. 0x4C44524B */
    uint32_t header_size;     /* Size of the header */
    uint32_t image_base;      /* Base address of the firmware image */
    uint32_t load_address;    /* Load address */
    uint32_t load_limit;      /* End of the firmware image */
    uint32_t bss_start;       /* This is the start of .bss section of the firmware I suppose */
    uint32_t reserved0;       /* reserved - I've seen only zeros in this field so far */
    uint32_t reserved1;       /* reserved - I've seen only zeros in this field so far */
    uint32_t entry_point;     /* Entry point address */
    uint32_t load_options;    /* 0x80000000 - setup flag (I don't know what it means 
                               * but is present in every RKW I saw), 
                               * 0x40000000 - check header crc, 
                               * 0x20000000 - check firmware crc
                               */
    uint32_t crc;              /* crc32 of the header (excluding crc32 field itself) */
};

const char *rkw_strerror(int8_t errno);
int load_rkw(unsigned char* buf, const char* filename, int buffer_size);
