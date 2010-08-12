/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rafaël Carré
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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

#include <stdio.h>
#include <system.h>
#include <inttypes.h>
#include "config.h"
#include "gcc_extensions.h"
#include "lcd.h"
#ifdef USE_ROCKBOX_USB
#include "usb.h"
#include "sysfont.h"
#endif /* USE_ROCKBOX_USB */
#include "backlight.h"
#include "button-target.h"
#include "common.h"
#include "storage.h"
#include "disk.h"
#include "panic.h"
#include "power.h"

int show_logo(void);

#ifdef USE_ROCKBOX_USB
static void usb_mode(void)
{
    if(usb_detect() != USB_INSERTED)
    {
        const char msg[] = "Plug USB cable";
        reset_screen();
        lcd_putsxy( (LCD_WIDTH - (SYSFONT_WIDTH * sizeof(msg))) / 2,
                    (LCD_HEIGHT - SYSFONT_HEIGHT) / 2, msg);
        lcd_update();

        /* wait until USB is plugged */
        while(usb_detect() != USB_INSERTED) ;
    }

    const char msg[] = "Bootloader USB mode";
    reset_screen();
    lcd_putsxy( (LCD_WIDTH - (SYSFONT_WIDTH * sizeof(msg))) / 2,
                (LCD_HEIGHT - SYSFONT_HEIGHT) / 2, msg);
    lcd_update();

    while(usb_detect() == USB_INSERTED)
        sleep(HZ);

    reset_screen();
    lcd_update();
}
#endif /* USE_ROCKBOX_USB */

void main(void) NORETURN_ATTR;
void main(void)
{
    unsigned char* loadbuffer;
    int buffer_size;
    void(*kernel_entry)(void);
    int ret;

    system_init();
    kernel_init();

    enable_irq();

    lcd_init();
    show_logo();

    backlight_init();

    button_init_device();
    int btn = button_read_device();

#if !defined(SANSA_FUZE) && !defined(SANSA_CLIP) && !defined(SANSA_CLIPV2) \
    && !defined(SANSA_CLIPPLUS)
    if (button_hold())
    {
        verbose = true;
        lcd_clear_display();
        printf("Hold switch on");
        printf("Shutting down...");
        sleep(HZ);
        power_off();
    }
#endif

    /* Enable bootloader messages if any button is pressed */
    if (btn)
    {
        lcd_clear_display();
        verbose = true;
    }

    ret = storage_init();
    if(ret < 0)
        error(EATA, ret, true);

#ifdef USE_ROCKBOX_USB
    usb_init();
    usb_start_monitoring();

    /* Enter USB mode if USB is plugged and SELECT button is pressed */
    if(btn & BUTTON_SELECT && usb_detect() == USB_INSERTED)
        usb_mode();
#endif /* USE_ROCKBOX_USB */

    while(!disk_init(IF_MV(0)))
#ifdef USE_ROCKBOX_USB
        usb_mode();
#else
        panicf("disk_init failed!");
#endif

    while((ret = disk_mount_all()) <= 0)
    {
#ifdef USE_ROCKBOX_USB
        error(EDISK, ret, false);
        usb_mode();
#else
        error(EDISK, ret, true);
#endif
    }

    printf("Loading firmware");

    loadbuffer = (unsigned char*)DRAM_ORIG; /* DRAM */
    buffer_size = (int)(loadbuffer + (DRAM_SIZE) - TTB_SIZE);

    while((ret = load_firmware(loadbuffer, BOOTFILE, buffer_size)) < 0)
    {
#ifdef USE_ROCKBOX_USB
        error(EBOOTFILE, ret, false);
        usb_mode();
#else
        error(EBOOTFILE, ret, true);
#endif
    }

    kernel_entry = (void*) loadbuffer;
    cpucache_invalidate();
    printf("Executing");
    kernel_entry();
    printf("ERR: Failed to boot");

    /* never returns */
    while(1) ;
}
