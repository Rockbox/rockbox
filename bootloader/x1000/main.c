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

#include "x1000bootloader.h"
#include "system.h"
#include "core_alloc.h"
#include "kernel/kernel-internal.h"
#include "power.h"
#include "button.h"
#include "storage.h"
#include "disk.h"
#include "file_internal.h"
#include "usb.h"
#include "i2c-x1000.h"
#include "boot-x1000.h"
#include <stdbool.h>

void main(void)
{
    system_init();
    core_allocator_init();
    kernel_init();
    i2c_init();
    power_init();
    button_init();
    enable_irq();

    if(storage_init() < 0) {
        splash(5*HZ, "storage_init() failed");
        power_off();
    }

    filesystem_init();

    usb_init();
    usb_start_monitoring();

    /* It's OK if this doesn't mount anything. Any disk access should
     * be guarded by a call to check_disk() to see if the disk is really
     * present, blocking with an "insert SD card" prompt if appropriate. */
    disk_mount_all();

    /* If USB booting, the user probably needs to enter recovery mode;
     * let's not force them to hold down the recovery key. */
    bool recovery_mode = get_boot_flag(BOOT_FLAG_USB_BOOT);

#ifdef HAVE_BUTTON_DATA
    int bdata;
    if(button_read_device(&bdata) & BL_RECOVERY)
#else
    if(button_read_device() & BL_RECOVERY)
#endif
        recovery_mode = true;

    /* If boot fails, it will return and continue on below */
    if(!recovery_mode)
        boot_rockbox();

    /* This function does not return. */
    recovery_menu();
}
