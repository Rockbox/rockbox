/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 * Copyright (C) 2021-2022 Aidan MacDonald
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 ****************************************************************************/

/* Loading Rockbox and handing control to it.  Mirrors
 * bootloader/x1000/boot.c. */

#include "x1600bootloader.h"
#include "ingenic/gui-ingenic.h"
#include "core_alloc.h"
#include "kernel.h"
#include "boot-x1600.h"

int bl_preloaded_size(void)
{
    const volatile uint32_t* h = (const volatile uint32_t*)BL_PRELOAD_HDR;
    if(h[0] != BL_PRELOAD_MAGIC)
        return 0;
    uint32_t len = h[1];
    /* Reject images outside the supported size range. */
    if(len < 0x1000 || len > (16u << 20))
        return 0;
    return (int)len;
}

void boot_rockbox(void)
{
    while(get_recovery_button(TIMEOUT_NOBLOCK) != BUTTON_NONE);

    /* Prefer an image preloaded by the host over one on the card. */
    int pre = bl_preloaded_size();
    if(pre > 0) {
        gui_shutdown();
        x1600_boot_rockbox((const void*)BL_PRELOAD_IMAGE, (size_t)pre);
        /* not reached */
    }

    size_t length;
    int handle = load_rockbox(BOOTFILE, &length);
    if(handle < 0)
        return;

    /* Does not return: x1600_boot_rockbox() copies the image over the DRAM we
     * are running from and jumps to it. */
    gui_shutdown();
    x1600_boot_rockbox(core_get_data(handle), length);
}
