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

static bool lcd_inited = false;
static bool usb_inited = false;

static void init_lcd(void)
{
    if(lcd_inited)
        return;

    lcd_init();
    font_init();
    lcd_setfont(FONT_SYSFIXED);

    /* Clear screen before turning backlight on, otherwise we might
     * display random garbage on the screen */
    lcd_clear_display();
    lcd_update();

    backlight_init();

    lcd_inited = true;
}

static void do_splash2(int delay, const char* msg, const char* msg2)
{
    init_lcd();
    lcd_clear_display();
    lcd_putsxy((LCD_WIDTH - (SYSFONT_WIDTH * strlen(msg))) / 2,
               (LCD_HEIGHT - SYSFONT_HEIGHT) / 2, msg);
    if(msg2) {
        lcd_putsxy((LCD_WIDTH - (SYSFONT_WIDTH * strlen(msg2))) / 2,
                   (LCD_HEIGHT + 2*SYSFONT_HEIGHT) / 2, msg2);
    }

    lcd_putsxy((LCD_WIDTH - (SYSFONT_WIDTH * strlen(rbversion))) / 2,
               (LCD_HEIGHT - SYSFONT_HEIGHT), rbversion);
    lcd_update();
    sleep(delay);
}

static void do_splash(int delay, const char* msg)
{
    do_splash2(delay, msg, NULL);
}

static void do_usb(void)
{
    if(!usb_inited) {
        usb_init();
        usb_start_monitoring();
        usb_inited = true;
    }

    do_splash(0, "Waiting for USB");

    while(button_get(true) != SYS_USB_CONNECTED);
    do_splash(0, "USB mode");

    usb_acknowledge(SYS_USB_CONNECTED_ACK);
    while(button_get(true) != SYS_USB_DISCONNECTED);

    do_splash(3*HZ, "USB disconnected");
}

void main(void)
{
    system_init();
    kernel_init();
    i2c_init();
    power_init();
    button_init();
    enable_irq();

    if(storage_init() < 0) {
        do_splash(3*HZ, "Failed to init storage");
        power_off();
    }

    filesystem_init();

    int loadsize = 0;
    do {
        if(!storage_present(0)) {
            do_splash(HZ, "Insert SD card");
            continue;
        }

        if(disk_mount_all() <= 0) {
            do_splash(5*HZ, "Cannot mount filesystem");
            do_usb();
            continue;
        }

        loadsize = load_firmware(loadbuffer, BOOTFILE, MAX_LOAD_SIZE);
        if(loadsize <= 0) {
            do_splash2(5*HZ, "Error loading Rockbox",
                       loader_strerror(loadsize));
            do_usb();
            continue;
        }
    } while(loadsize <= 0);

    if(lcd_inited)
        backlight_hw_off();

    disable_irq();

    exec(loadaddress, loadbuffer, loadsize);
}
