/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> >    <
 *   Player     |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 *
 * Based on firmware/target/mips/ingenic_x1000/boot-x1000.c
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

#include "boot-x1600.h"
#include "x1600.h"
#include "system.h"
#include <string.h>

#define HDR_BEGIN   128     /* header must begin within this many bytes */
#define HDR_LEN     256     /* header length cannot exceed this */

/* search for header value, label must be a 4-character string.
 * Returns the found value or 0 if the label wasn't found. */
static uint32_t search_header(const unsigned char* source, size_t length,
                              const char* label)
{
    size_t search_len = MIN(length, (size_t)HDR_BEGIN);
    if(search_len < 8)
        return 0;
    search_len -= 7;

    /* find the beginning marker */
    size_t i;
    for(i = 8; i < search_len; i += 4)
        if(!memcmp(&source[i], "BEGINHDR", 8))
            break;
    if(i >= search_len)
        return 0;
    i += 8;

    /* search within the header */
    search_len = MIN(length, i + HDR_LEN) - 7;
    for(; i < search_len; i += 8) {
        if(!memcmp(&source[i], "ENDH", 4)) {
            break;
        } else if(!memcmp(&source[i], label, 4)) {
            i += 4;
            /* read little-endian value */
            return (uint32_t)source[i]
                 | ((uint32_t)source[i+1] << 8)
                 | ((uint32_t)source[i+2] << 16)
                 | ((uint32_t)source[i+3] << 24);
        }
    }

    return 0;
}

static void iram_memmove(void* dest, const void* source, size_t length)
    __attribute__((section(".icode")));

static void iram_memmove(void* dest, const void* source, size_t length)
{
    unsigned char* d = dest;
    const unsigned char* s = source;

    if(s < d && d < s + length) {
        d += length;
        s += length;
        while(length--)
            *--d = *--s;
    } else {
        while(length--)
            *d++ = *s++;
    }
}

/* The copy overwrites bootloader text. This function and its stack must
 * remain below X1600_DRAM_BASE in IRAM. */
void x1600_boot_rockbox(const void* source, size_t length)
{
    uint32_t load_addr = search_header(source, length, "LOAD");
    if(!load_addr)
        load_addr = X1600_DRAM_BASE;

    disable_irq();

    /* --- Beyond this point, do not call into DRAM --- */

    /* Keep dirty destination lines coherent until the copy is written back. */
    iram_memmove((void*)load_addr, source, length);
    commit_discard_idcache();

    typedef void(*entry_fn)(void);
    entry_fn fn = (entry_fn)load_addr;
    fn();
    while(1);
}

/* RoLo. The generic MIPS fallback in firmware/rolo.c ignores the LOAD header
 * and the IRAM-resident copy, so delegate as the X1000 does. */
void rolo_restart(const unsigned char* source, unsigned char* dest, int length)
{
    (void)dest;     /* x1600_boot_rockbox() takes the address from the header */

    /* The handoff overwrites this function and must not return.
     * The RoLo path has not been tested on hardware. */
    disable_irq();

    x1600_boot_rockbox(source, length);
}
