/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#include "jz4740.h"
#include "backlight.h"
#include "font.h"
#include "lcd.h"
#include "usb.h"
#include "system.h"
#include "button.h"
#include "common.h"
#include "storage.h"
#include "disk.h"
#include "string.h"

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
    
    /* Init backlight */
    backlight_init();
    
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
            {
                usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
                break;
            }
        }
    }

    reset_screen();
}

static void boot_of(void)
{
    /* Init backlight */
    backlight_init();
}

int main(void)
{
    int rc, dummy;
    void (*kernel_entry)(void);
    
    kernel_init();
    lcd_init();
    font_init();
    lcd_setfont(FONT_SYSFIXED);
    button_init();
    storage_init();

    reset_screen();
    
#ifdef HAVE_TOUCHSCREEN
    rc = button_read_device(&dummy);
#else
    rc = button_read_device();
#endif
    
    if(rc & BUTTON_VOL_UP)
        usb_mode();
    else if(button_hold())
        boot_of();
    else if(rc)
        verbose = true;
    
    /* Only enable backlight when button is pressed */
    if(verbose)
        backlight_init();
    
    rc = storage_init();
    if(rc)
        error(EATA, rc);

    rc = disk_mount_all();
    if (rc <= 0)
        error(EDISK,rc);
    
    printf("Loading firmware");
    rc = load_firmware((unsigned char *)CONFIG_SDRAM_START, BOOTFILE, 0x400000);
    if(rc < 0)
        printf("Error: %s", strerror(rc));

    if (rc == EOK)
    {
        printf("Starting Rockbox...");
        disable_interrupt();
        kernel_entry = (void*) CONFIG_SDRAM_START;
        kernel_entry();
    }
    
    /* Halt */
    while (1)
        core_idle();
    
    return 0;
}
