/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2022 by Aidan MacDonald
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

#include "bootdata.h"
#include "crc32.h"
#include <stddef.h>

#ifdef BOOTLOADER
# error "not to be included in bootloader builds"
#endif

bool boot_data_valid;

static bool verify_boot_data_v0(void) INIT_ATTR;
static bool verify_boot_data_v0(void)
{
    /* validate protocol version */
    if (boot_data.version != 0)
        return false;

    /* validate length */
    if (boot_data.length != 4)
        return false;

    return true;
}

static bool verify_boot_data_v1(void) INIT_ATTR;
static bool verify_boot_data_v1(void)
{
    /* validate protocol version */
    if (boot_data.version != 1)
        return false;

    /* validate length */
    if (boot_data.length != 4)
        return false;

    return true;
}

struct verify_bd_entry
{
    int version;
    bool (*verify) (void);
};

static const struct verify_bd_entry verify_bd[] INITDATA_ATTR = {
    { 0, verify_boot_data_v0 },
    { 1, verify_boot_data_v1 },
};

void verify_boot_data(void)
{
    /* verify payload with checksum - all protocol versions */
    uint32_t crc = crc_32(boot_data.payload, boot_data.length, 0xffffffff);
    if (crc != boot_data.crc)
        return;

    /* apply verification specific to the protocol version */
    for (size_t i = 0; i < ARRAYLEN(verify_bd); ++i)
    {
        const struct verify_bd_entry *e = &verify_bd[i];
        if (e->version == boot_data.version)
        {
            if (e->verify())
                boot_data_valid = true;

            return;
        }
    }
}
