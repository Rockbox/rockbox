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

static const char* const fiiom3k_action_names[] = {
    "install",
    "backup",
    "restore",
};

static const char* const fiiom3k_install_action_params[] =
    {"spl", "bootloader", "backup", "without-backup", NULL};

static const char* const fiiom3k_backuprestore_action_params[] =
    {"spl", "image", NULL};

static const char* const* fiiom3k_action_params[] = {
    fiiom3k_install_action_params,
    fiiom3k_backuprestore_action_params,
    fiiom3k_backuprestore_action_params,
};

static const jz_device_action_fn fiiom3k_action_funcs[] = {
    jz_fiiom3k_install,
    jz_fiiom3k_backup,
    jz_fiiom3k_restore,
};

static const jz_device_info infotable[] = {
    {
        .name = "fiiom3k",
        .description = "FiiO M3K",
        .device_type = JZ_DEVICE_FIIOM3K,
        .cpu_type = JZ_CPU_X1000,
        .vendor_id = 0xa108,
        .product_id = 0x1000,
        .num_actions = sizeof(fiiom3k_action_names)/sizeof(void*),
        .action_names = fiiom3k_action_names,
        .action_funcs = fiiom3k_action_funcs,
        .action_params = fiiom3k_action_params,
    },
};

static const int infotable_size = sizeof(infotable)/sizeof(struct jz_device_info);

/** \brief Get the number of entries in the device info list */
int jz_get_num_device_info(void)
{
    return infotable_size;
}

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
