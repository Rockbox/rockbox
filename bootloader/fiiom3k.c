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
#include "installer-fiiom3k.h"
#include "spl-x1000.h"
#include "x1000/cpm.h"

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
static bool disk_inited = false;

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

static void put_version(void)
{
    lcd_putsxy((LCD_WIDTH - (SYSFONT_WIDTH * strlen(rbversion))) / 2,
               (LCD_HEIGHT - SYSFONT_HEIGHT), rbversion);
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

    put_version();
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

    do_splash2(0, "Waiting for USB", "Press POWER to go back");

    int btn;
    while(1) {
        btn = button_get(true);
        if(btn == SYS_USB_CONNECTED)
           break;
        else if(btn == BUTTON_POWER)
            return;
    }

    do_splash(0, "USB mode");
    usb_acknowledge(SYS_USB_CONNECTED_ACK);
    while(button_get(true) != SYS_USB_DISCONNECTED);

    do_splash(3*HZ, "USB disconnected");
}

static int init_disk(void)
{
    if(disk_inited)
        return 0;

    while(!storage_present(0)) {
        do_splash2(0, "Insert SD card", "Press POWER for recovery");
        int btn = button_get_w_tmo(HZ);
        if(btn == BUTTON_POWER)
            return 1;
    }

    if(disk_mount_all() <= 0) {
        do_splash(5*HZ, "Cannot mount filesystem");
        return 1;
    }

    disk_inited = true;
    return 0;
}

static void do_boot(void)
{
    if(init_disk() != 0)
        return;

    int loadsize = load_firmware(loadbuffer, BOOTFILE, MAX_LOAD_SIZE);
    if(loadsize <= 0) {
        do_splash2(5*HZ, "Error loading Rockbox",
                   loader_strerror(loadsize));
        do_usb();
        return;
    }

    if(lcd_inited)
        backlight_hw_off();

    disable_irq();
    exec(loadaddress, loadbuffer, loadsize);
}

#define INSTALL 0
#define BACKUP  1
#define RESTORE 2

static void do_install(int which)
{
    int rc = init_disk();
    if(rc != 0) {
        do_splash2(5*HZ, "Install aborted", "No SD card present");
        return;
    }

    const char* msg;
    if(rc == INSTALL)
        msg = "Installing";
    else if(rc == BACKUP)
        msg = "Backing up";
    else
        msg = "Restoring backup";

    do_splash(0, msg);

    if(which == INSTALL)
        rc = install_boot("/bootloader.m3k");
    else if(which == BACKUP)
        rc = backup_boot("/fiiom3k-boot.bin");
    else
        rc = restore_boot("/fiiom3k-boot.bin");

    char buf[32];
    snprintf(buf, sizeof(buf), "Failed! Error: %d", rc);
    const char* msg1 = rc == 0 ? "Success" : buf;
    const char* msg2 = "Press POWER to continue";
    do_splash2(0, msg1, msg2);

    button_clear_queue();
    while(button_get(true) != BUTTON_POWER);
}

static void recovery_menu(void)
{
    static const char* items[] = {
        "--- Rockbox recovery menu ---",
        "[System]",
        "   Start Rockbox",
        "   USB mode",
        "   Shutdown",
        "   Reboot",
        "[Bootloader]",
        "   Install or update",
        "   Backup",
        "   Restore",
        "",
        "",
        "",
        "",
        "",
        "VOL+/VOL-         move cursor",
        "PLAY              select item",
        "POWER               power off",
    };

    static const int nitems = sizeof(items) / sizeof(char*);

    init_lcd();

    int selection = 2;
    do {
        /* Draw menu */
        lcd_clear_display();

        for(int i = 0; i < nitems; ++i)
            lcd_puts(0, i, items[i]);

        if(items[selection][0] == ' ')
            lcd_puts(0, selection, "=>");

        put_version();
        lcd_update();

        /* Clear queue to avoid accidental input */
        button_clear_queue();

        /* Get the button */
        int btn = button_get(true);

        /* Process user input */
        if(btn == BUTTON_VOL_UP) {
            for(int i = selection-1; i >= 0; --i) {
                if(items[i][0] == ' ') {
                    selection = i;
                    break;
                }
            }

            continue;
        } else if(btn == BUTTON_VOL_DOWN) {
            for(int i = selection+1; i < nitems; ++i) {
                if(items[i][0] == ' ') {
                    selection = i;
                    break;
                }
            }

            continue;
        } else if(btn == BUTTON_POWER) {
            selection = 4; /* Shutdown */
        } else if(btn != BUTTON_PLAY) {
            continue;
        }

        /* User pressed PLAY so decide what action to take */
        switch(selection) {
        case 2: /* Start rockbox */
            do_boot();
            break;

        case 3: /* USB mode */
            do_usb();
            break;

        case 4: /* Shutdown */
            do_splash(HZ, "Shutting down");
            power_off();
            break;

        case 5: /* Reboot */
            do_splash(HZ, "Rebooting");
            system_reboot();
            break;

        case 7: /* Install bootloader */
            do_install(INSTALL);
            break;

        case 8: /* Backup bootloader */
            do_install(BACKUP);
            break;

        case 9: /* Restore bootloader */
            do_install(RESTORE);
            break;

        default:
            break;
        }
    } while(1);
}

void main(void)
{
    bool recovery_mode = false;

    /* This hack is needed because when USB booting, we cannot initialize
     * clocks in the SPL -- it may break the mask ROM's USB code. So if the
     * SPL has not already initialized the clocks, we need to do that now.
     *
     * Also use this as a sign that we should enter the recovery menu since
     * this is probably the expected result if the user is USB booting...
     */
    if(jz_readf(CPM_MPCR, ENABLE)) {
        spl_handle_pre_boot(0);
        recovery_mode = true;
    }

    system_init();
    core_allocator_init();
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

    if(button_read_device() & BUTTON_VOL_UP)
        recovery_mode = true;

    if(!recovery_mode)
        do_boot();

    /* If boot fails or user holds Vol+, go to recovery menu */
    recovery_menu();
}
