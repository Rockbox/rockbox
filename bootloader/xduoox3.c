/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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

#include "config.h"
#include "jz4760b.h"
#include "../kernel-internal.h"
#include "backlight.h"
#include "font.h"
#include "lcd.h"
#include "file.h"
#include "usb.h"
#include "system.h"
#include "button.h"
#include "common.h"
#include "rb-loader.h"
#include "loader_strerror.h"
#include "storage.h"
#include "file_internal.h"
#include "disk.h"
#include "string.h"
#include "adc.h"
#include "version.h"

#include "xdebug.h"

extern void show_logo(void);
extern void power_off(void);

static void show_splash(int timeout, const char *msg)
{
    reset_screen();
    lcd_putsxy( (LCD_WIDTH - (SYSFONT_WIDTH * strlen(msg))) / 2,
                (LCD_HEIGHT - SYSFONT_HEIGHT) / 2, msg);
    lcd_update();

    sleep(timeout);
}

static void usb_mode(void)
{
    int button;

    /* Init USB */
    usb_init();
    usb_start_monitoring();

    /* Wait for threads to connect */
    show_splash(HZ/2, "Waiting for USB");

    while (1)
    {
        button = button_get_w_tmo(HZ/2);

        if (button == SYS_USB_CONNECTED)
            break; /* Hit */
    }

    if (button == SYS_USB_CONNECTED)
    {
        /* Got the message - wait for disconnect */
        show_splash(0, "Bootloader USB mode");

        usb_acknowledge(SYS_USB_CONNECTED_ACK);

        while (1)
        {
            button = button_get(true);
            if (button == SYS_USB_DISCONNECTED)
                break;
        }
    }
}

static int boot_rockbox(void)
{
    int rc;
    void (*kernel_entry)(void);

    printf("Mounting disk...\n");
    rc = disk_mount_all();
    if (rc <= 0)
    {
        verbose = true;
        error(EDISK,rc, true);
    }

    printf("Loading firmware...\n");
    rc = load_firmware((unsigned char *)CONFIG_SDRAM_START, BOOTFILE, 0x400000);
    if(rc <= EFILE_EMPTY)
        return rc;
    else
    {
        printf("Starting Rockbox...\n");
        adc_close(); /* Disable SADC, seems to fix the re-init Rockbox does */

        disable_interrupt();
        kernel_entry = (void*) CONFIG_SDRAM_START;
        kernel_entry();

        return 0; /* Shouldn't happen */
    }
}

static void reset_configuration(void)
{
    int rc;

    rc = disk_mount_all();
    if (rc <= 0)
    {
        verbose = true;
        error(EDISK,rc, true);
    }

    if(rename(ROCKBOX_DIR "/config.cfg", ROCKBOX_DIR "/config.old") == 0)
        show_splash(HZ/2, "Configuration reset successfully!");
    else
        show_splash(HZ/2, "Couldn't reset configuration!");
}

int main(void)
{
    int rc;

    serial_puts("\n\nSPL Stage 2\n\n");

    kernel_init();
    lcd_init();
    font_init();
    lcd_setfont(FONT_SYSFIXED);
    button_init();
    backlight_init();

    show_logo();

    filesystem_init();

    rc = storage_init();
    if(rc)
    {
        verbose = true;
        error(EATA, rc, true);
    }

    /* Don't mount the disks yet, there could be file system/partition errors
       which are fixable in USB mode */

    reset_screen();

    printf(MODEL_NAME" Rockbox Bootloader\n");
    printf("Version %s\n", rbversion);

    rc = boot_rockbox();

    if(rc <= EFILE_EMPTY)
    {
        verbose = true;
        printf("Error: %s", loader_strerror(rc));
    }

    /* Halt */
    while (1)
        core_idle();

    return 0;
}
