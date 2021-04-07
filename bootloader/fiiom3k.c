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

#include "system.h"
#include "kernel/kernel-internal.h"
#include "i2c.h"
#include "power.h"
#include "lcd.h"
#include "backlight.h"
#include "button.h"
#include "storage.h"
#include "file_internal.h"
#include "disk.h"
#include "rb-loader.h"
#include "loader_strerror.h"

/* Load address where the binary needs to be placed */
extern unsigned char loadaddress[];

/* Fixed buffer to contain the loaded binary in memory */
extern unsigned char loadbuffer[];
extern unsigned char loadbufferend[];
#define MAX_LOAD_SIZE (loadbufferend - loadbuffer)

void exec(void* dst, const void* src, int bytes)
    __attribute__((noreturn, section(".icode")));

void exec(void* dst, const void* src, int bytes)
{
    memcpy(dst, src, bytes);
    commit_discard_idcache();
    __asm__ __volatile__ ("jr %0\n"
                          "nop\n"
                          :: "r"(dst));
    __builtin_unreachable();
}

static void error(const char* msg)
{
    /* Initialization of the LCD/buttons only if needed */
    lcd_init();
    backlight_init();
    button_init();

    lcd_clear_display();
    lcd_puts(0, 0, msg);
    lcd_puts(0, 2, "Press POWER to power off");
    lcd_update();

    while(button_get(true) != BUTTON_POWER);
    power_off();
}

void main(void)
{
    system_init();
    kernel_init();
    i2c_init();
    power_init();
    enable_irq();

    if(storage_init() < 0)
        error("Storage initialization failed");

    filesystem_init();

    if(!storage_present(0))
        error("No SD card present");

    if(disk_mount_all() <= 0)
        error("Unable to mount filesystem");

    int loadsize = load_firmware(loadbuffer, BOOTFILE, MAX_LOAD_SIZE);
    if(loadsize <= 0)
        error(loader_strerror(loadsize));

    disable_irq();

    exec(loadaddress, loadbuffer, loadsize);
}
