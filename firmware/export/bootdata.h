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
#ifndef __RB_BOOTDATA__
#define __RB_BOOTDATA__

#ifndef __ASSEMBLER__
#include <stdint.h>
#endif

/* /!\ This file can be included in assembly files /!\ */

/** The boot data will be filled by the bootloader with information that might
 * be relevant for Rockbox. The bootloader will search for the structure using
 * the magic header within the first BOOT_DATA_SEARCH_SIZE bytes of the binary.
 * Typically, this structure should be as close as possible to the entry point */

/* Search size for the data structure after entry point */
#define BOOT_DATA_SEARCH_SIZE   1024

#define BOOT_DATA_MAGIC0    ('r' | 'b' << 8 | 'm' << 16 | 'a' << 24)
#define BOOT_DATA_MAGIC1    ('g' | 'i' << 8 | 'c' << 16 | '!' << 24)

/* maximum size of payload */
#define BOOT_DATA_PAYLOAD_SIZE  4

#ifndef __ASSEMBLER__
/* This is the C structure */
struct boot_data_t
{
    union
    {
        uint32_t crc; /* crc of payload data (CRC32 with 0xffffffff for initial value) */
        uint32_t magic[2]; /* BOOT_DATA_MAGIC0/1 */
    };

    uint32_t length; /* length of the payload */

    /* add fields here */
    union
    {
        struct
        {
            uint8_t boot_volume;
        };
        uint8_t payload[BOOT_DATA_PAYLOAD_SIZE];
    };
} __attribute__((packed));

#if !defined(BOOTLOADER)
extern struct boot_data_t boot_data;
#endif
#else /* __ASSEMBLER__ */

/* This assembler macro implements an empty boot structure with just the magic
 * string */
.macro put_boot_data_here
.global boot_data
boot_data:
    .word   BOOT_DATA_MAGIC0
    .word   BOOT_DATA_MAGIC1
    .word   BOOT_DATA_PAYLOAD_SIZE
    .space  BOOT_DATA_PAYLOAD_SIZE, 0xff /* payload, initialised with value 0xff */
.endm

#endif

#endif /* __RB_BOOTDATA__ */
