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

#include "jztool.h"
#include <string.h>

static const jz_device_info infotable[] = {
    {
        .name = "fiiom3k",
        .description = "FiiO M3K",
        .device_type = JZ_DEVICE_FIIOM3K,
        .cpu_type = JZ_CPU_X1000,
        .vendor_id = 0xa108,
        .product_id = 0x1000,
    },
};

static const int infotable_size = sizeof(infotable)/sizeof(struct jz_device_info);

/** \brief Get the number of entries in the device info list */
int jz_get_num_device_info(void)
{
    return infotable_size;
}

/** \brief Lookup info for a device by type, returns NULL if not found. */
const jz_device_info* jz_get_device_info(jz_device_type type)
{
    for(int i = 0; i < infotable_size; ++i)
        if(infotable[i].device_type == type)
            return &infotable[i];

    return NULL;
}

/** \brief Lookup info for a device by name, returns NULL if not found. */
const jz_device_info* jz_get_device_info_named(const char* name)
{
    for(int i = 0; i < infotable_size; ++i)
        if(!strcmp(infotable[i].name, name))
            return &infotable[i];

    return NULL;
}

/** \brief Get a device info entry by index, returns NULL if out of range. */
const jz_device_info* jz_get_device_info_indexed(int index)
{
    if(index < infotable_size)
        return &infotable[index];
    else
        return NULL;
}
