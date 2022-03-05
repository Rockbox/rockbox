/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021-2022 Aidan MacDonald
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

#include "x1000bootloader.h"
#include "core_alloc.h"
#include "rb-loader.h"
#include "loader_strerror.h"
#include "system.h"
#include "kernel.h"
#include "power.h"
#include "boot-x1000.h"

extern int init_disk(void);

void boot_rockbox(void)
{
    if(init_disk() != 0)
        return;

    size_t max_size = 0;
    int handle = core_alloc_maximum("rockbox", &max_size, &buflib_ops_locked);
    if(handle < 0) {
        splash(5*HZ, "Out of memory");
        return;
    }

    unsigned char* loadbuffer = core_get_data(handle);
    int rc = load_firmware(loadbuffer, BOOTFILE, max_size);
    if(rc <= 0) {
        core_free(handle);
        splash2(5*HZ, "Error loading Rockbox", loader_strerror(rc));
        return;
    }

    gui_shutdown();

    x1000_boot_rockbox(loadbuffer, rc);
}

void shutdown(void)
{
    splash(HZ, "Shutting down");
    power_off();
    while(1);
}

void reboot(void)
{
    splash(HZ, "Rebooting");
    system_reboot();
    while(1);
}
