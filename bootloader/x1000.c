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

/* Unified bootloader for all X1000 targets. This is a bit messy.
 *
 * Features:
 * - Text based user interface
 * - USB mass storage access
 * - Bootloader installation / backup / restore
 *
 * Possible future improvements:
 * - Allow booting original firmware from the UI
 */

#include "system.h"
#include "core_alloc.h"
#include "kernel/kernel-internal.h"
#include "i2c.h"
#include "power.h"
#include "lcd.h"
#include "font.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "storage.h"
#include "file_internal.h"
#include "disk.h"
#include "usb.h"
#include "rb-loader.h"
#include "loader_strerror.h"
#include "version.h"
#include "boot-x1000.h"
#include "installer-x1000.h"
#include "x1000/x1000bootloader.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* Flags to indicate if hardware was already initialized */
bool usb_inited = false;

/* Set to true if a SYS_USB_CONNECTED event is seen
 * Set to false if a SYS_USB_DISCONNECTED event is seen */
bool is_usb_connected = false;

void init_usb(void)
{
    if(usb_inited)
        return;

    usb_init();
    usb_start_monitoring();
    usb_inited = true;
}

void usb_mode(void)
{
    init_usb();

    if(!is_usb_connected)
        splash2(0, "Waiting for USB", "Press " BL_QUIT_NAME " to go back");

    while(!is_usb_connected)
        if(get_button(TIMEOUT_BLOCK) == BL_QUIT)
            return;

    splash(0, "USB mode");
    usb_acknowledge(SYS_USB_CONNECTED_ACK);

    while(is_usb_connected)
        get_button(TIMEOUT_BLOCK);

    splash(3*HZ, "USB disconnected");
}

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
