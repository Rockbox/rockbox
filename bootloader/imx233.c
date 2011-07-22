/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by amaury Pouly
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
#include "backlight.h"
#include "button-target.h"
#include "common.h"
#include "storage.h"
#include "disk.h"
#include "panic.h"
#include "power.h"
#include "system-target.h"
#include "fmradio_i2c.h"

#include "usb.h"

void main(uint32_t arg) NORETURN_ATTR;
void main(uint32_t arg)
{
    unsigned char* loadbuffer;
    int buffer_size;
    void(*kernel_entry)(void);
    int ret;

    system_init();
    kernel_init();

    enable_irq();

    lcd_init();
    lcd_clear_display();
    lcd_update();

    backlight_init();

    button_init_device();

    //button_debug_screen();
    printf("arg=%c%c%c%c", arg >> 24,
        (arg >> 16) & 0xff, (arg >> 8) & 0xff, (arg & 0xff));

    ret = storage_init();
    if(ret < 0)
        error(EATA, ret, true);

    #ifdef HAVE_BOOTLOADER_USB_MODE
    usb_init();
    usb_core_enable_driver(USB_DRIVER_SERIAL, true);
    usb_attach();
    while(!(button_read_device() & BUTTON_POWER))
        yield();
    power_off();
    #endif /* HAVE_BOOTLOADER_USB_MODE */

    while(!disk_init(IF_MV(0)))
        panicf("disk_init failed!");

    while((ret = disk_mount_all()) <= 0)
    {
        error(EDISK, ret, true);
    }

    if(button_read_device() & BUTTON_VOL_UP)
        printf("Booting from SD card required.");

    printf("Loading firmware");

    loadbuffer = (unsigned char*)DRAM_ORIG; /* DRAM */
    buffer_size = (int)(loadbuffer + DRAM_SIZE - TTB_SIZE);

    while((ret = load_firmware(loadbuffer, BOOTFILE, buffer_size)) < 0)
    {
        error(EBOOTFILE, ret, true);
    }

    kernel_entry = (void*) loadbuffer;
    //cpucache_invalidate();
    printf("Executing");
    kernel_entry();
    printf("ERR: Failed to boot");

    /* never returns */
    while(1) ;
}
