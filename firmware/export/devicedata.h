/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 by Amaury Pouly
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
#ifndef __RB_DEVICEDATA__
#define __RB_DEVICEDATA__

#ifndef __ASSEMBLER__
#include <stdint.h>
#include "system.h"
#endif

/* /!\ This file can be included in assembly files /!\ */

/** The device data will be filled by the bootloader with information that might
 * be relevant for Rockbox. The bootloader will search for the structure using
 * the magic header within the first DEVICE_DATA_SEARCH_SIZE bytes of the binary.
 * Typically, this structure should be as close as possible to the entry point */

/* Search size for the data structure after entry point */
#define DEVICE_DATA_SEARCH_SIZE   1024

#define DEVICE_DATA_MAGIC0    ('r' | 'b' << 8 | 'd' << 16 | 'e' << 24)
#define DEVICE_DATA_MAGIC1    ('v' | 'i' << 8 | 'c' << 16 | 'e' << 24)
#define DEIVCE_DATA_VERSION 0

/* maximum size of payload */
#define DEVICE_DATA_PAYLOAD_SIZE  4

#ifndef __ASSEMBLER__
/* This is the C structure */
struct device_data_t
{
    union
    {
        uint32_t crc; /* crc of payload data (CRC32 with 0xffffffff for initial value) */
        uint32_t magic[2]; /* DEVICE_DATA_MAGIC0/1 */
    };

    uint32_t length; /* length of the payload */

    /* add fields here */
    union
    {
        struct
        {
#if defined(EROS_QN)
            uint8_t hw_rev;
#endif
            uint8_t version;
        };
        uint8_t payload[DEVICE_DATA_PAYLOAD_SIZE];
    };
} __attribute__((packed));


void fill_devicedata(struct device_data_t *data);
bool write_devicedata(unsigned char* buf, int len);
#ifndef BOOTLOADER
extern struct device_data_t device_data;

void verify_device_data(void) INIT_ATTR;

#endif

#else /* __ASSEMBLER__ */

/* This assembler macro implements an empty device data structure with just the magic
 * string and payload size */
.macro put_device_data_here
.global device_data
device_data:
    .word   DEVICE_DATA_MAGIC0
    .word   DEVICE_DATA_MAGIC1
    .word   DEVICE_DATA_PAYLOAD_SIZE
    .space  BOOT_DATA_PAYLOAD_SIZE, 0xff /* payload, initialised with value 0xff */
.endm

#endif

#endif /* __RB_DEVICEDATA__ */
