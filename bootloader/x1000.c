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
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef FIIO_M3K
# include "installer-fiiom3k.h"
#endif

#if defined(FIIO_M3K)
# define BL_RECOVERY    BUTTON_VOL_UP
# define BL_UP          BUTTON_VOL_UP
# define BL_DOWN        BUTTON_VOL_DOWN
# define BL_SELECT      BUTTON_PLAY
# define BL_QUIT        BUTTON_POWER
# define BL_UP_NAME     "VOL+"
# define BL_DOWN_NAME   "VOL-"
# define BL_SELECT_NAME "PLAY"
# define BL_QUIT_NAME   "POWER"
#else
# error "Missing keymap!"
#endif

enum {
    MENUITEM_HEADING,
    MENUITEM_ACTION,
};

struct menuitem {
    int type;
    const char* text;
    void(*action)(void);
};

void clearscreen(void);
void putversion(void);
void putcenter_y(int y, const char* msg);
void putcenter_line(int line, const char* msg);
void splash2(long delay, const char* msg, const char* msg2);
void splash(long delay, const char* msg);

void init_lcd(void);
void init_usb(void);
int init_disk(void);

void recovery_menu(void) __attribute__((noreturn));

void boot_rockbox(void);
void usb_mode(void);
void shutdown(void);
void reboot(void);
void bootloader_install(void);
void bootloader_backup(void);
void bootloader_restore(void);

/* Defines the recovery menu contents */
const struct menuitem recovery_items[] = {
    {MENUITEM_HEADING,  "System",                   NULL},
    {MENUITEM_ACTION,       "Start Rockbox",        &boot_rockbox},
    {MENUITEM_ACTION,       "USB mode",             &usb_mode},
    {MENUITEM_ACTION,       "Shutdown",             &shutdown},
    {MENUITEM_ACTION,       "Reboot",               &reboot},
    {MENUITEM_HEADING,  "Bootloader",               NULL},
    {MENUITEM_ACTION,       "Install or update",    &bootloader_install},
    {MENUITEM_ACTION,       "Backup",               &bootloader_backup},
    {MENUITEM_ACTION,       "Restore",              &bootloader_restore},
};

/* Final load address of rockbox binary.
 * NOTE: this is really the load address of the bootloader... it relies
 * on the fact that bootloader and app are linked at the same address. */
extern unsigned char loadaddress[];

/* Temp buffer to contain the binary in memory */
extern unsigned char loadbuffer[];
extern unsigned char loadbufferend[];
#define MAX_LOAD_SIZE (loadbufferend - loadbuffer)

/* Flags to indicate if hardware was already initialized */
bool lcd_inited = false;
bool usb_inited = false;
bool disk_inited = false;

/* Jump to loaded binary */
void exec(void* dst, const void* src, size_t bytes)
    __attribute__((noinline, noreturn, section(".icode")));

void exec(void* dst, const void* src, size_t bytes)
{
    memcpy(dst, src, bytes);
    commit_discard_idcache();

    typedef void(*entry_fn)(void) __attribute__((noreturn));
    entry_fn fn = (entry_fn)dst;
    fn();
}

void clearscreen(void)
{
    init_lcd();
    lcd_clear_display();
    putversion();
}

void putversion(void)
{
    int x = (LCD_WIDTH - SYSFONT_WIDTH*strlen(rbversion)) / 2;
    int y = LCD_HEIGHT - SYSFONT_HEIGHT;
    lcd_putsxy(x, y, rbversion);
}

void putcenter_y(int y, const char* msg)
{
    int x = (LCD_WIDTH - SYSFONT_WIDTH*strlen(msg)) / 2;
    lcd_putsxy(x, y, msg);
}

void putcenter_line(int line, const char* msg)
{
    int y = LCD_HEIGHT/2 + (line - 1)*SYSFONT_HEIGHT;
    putcenter_y(y, msg);
}

void splash2(long delay, const char* msg, const char* msg2)
{
    clearscreen();
    putcenter_line(0, msg);
    if(msg2)
        putcenter_line(1, msg2);
    lcd_update();
    sleep(delay);
}

void splash(long delay, const char* msg)
{
    splash2(delay, msg, NULL);
}

void init_lcd(void)
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

/* TODO: This does not work properly after a USB boot.
 *
 * The USB core is not properly reset by just re-initializing it, and I can't
 * figure out how to make it work. Setting the DWC_DCTL.SDIS bit will force a
 * disconnect (the usb-designware driver does this already as part of its init
 * but it doesn't seem to cause a disconnect).
 *
 * But the host still doesn't detect us properly when we reconnect. Linux gives
 * messages "usb 1-3: config 1 has no interfaces?" in dmesg and no mass storage
 * interfaces show up.
 *
 * Re-plugging the cable seems to reset everything to a working state and there
 * are no issues, but that's annoying.
 */
void init_usb(void)
{
    if(usb_inited)
        return;

    usb_init();
    usb_start_monitoring();
    usb_inited = true;
}

int init_disk(void)
{
    if(disk_inited)
        return 0;

    button_clear_queue();
    while(!storage_present(IF_MD(0))) {
        splash2(0, "Insert SD card", "Press " BL_QUIT_NAME " for recovery");
        if(button_get_w_tmo(HZ/4) == BL_QUIT)
            return -1;
    }

    if(disk_mount_all() <= 0) {
        splash(5*HZ, "Cannot mount disk");
        return -1;
    }

    disk_inited = true;
    return 0;
}

void put_help_line(int line, const char* str1, const char* str2)
{
    int width = LCD_WIDTH / SYSFONT_WIDTH;
    lcd_puts(0, line, str1);
    lcd_puts(width - strlen(str2), line, str2);
}

void recovery_menu(void)
{
    const int n_items = sizeof(recovery_items)/sizeof(struct menuitem);

    int selection = 0;
    while(recovery_items[selection].type != MENUITEM_ACTION)
        ++selection;

    while(1) {
        clearscreen();
        putcenter_y(0, "Rockbox recovery menu");

        int top_line = 2;

        /* draw the menu */
        for(int i = 0; i < n_items; ++i) {
            switch(recovery_items[i].type) {
            case MENUITEM_HEADING:
                lcd_putsf(0, top_line+i, "[%s]", recovery_items[i].text);
                break;

            case MENUITEM_ACTION:
                lcd_puts(3, top_line+i, recovery_items[i].text);
                break;

            default:
                break;
            }
        }

        /* draw the selection marker */
        lcd_puts(0, top_line+selection, "=>");

        /* draw the help text */
        int line = (LCD_HEIGHT - SYSFONT_HEIGHT)/SYSFONT_HEIGHT - 3;
        put_help_line(line++, BL_DOWN_NAME "/" BL_UP_NAME, "move cursor");
        put_help_line(line++, BL_SELECT_NAME, "select item");
        put_help_line(line++, BL_QUIT_NAME, "power off");

        lcd_update();

        /* handle input */
        button_clear_queue();
        switch(button_get(true)) {
        case BL_SELECT: {
            if(recovery_items[selection].action)
                recovery_items[selection].action();
        } break;

        case BL_UP:
            for(int i = selection-1; i >= 0; --i) {
                if(recovery_items[i].action) {
                    selection = i;
                    break;
                }
            }
            break;

        case BL_DOWN:
            for(int i = selection+1; i < n_items; ++i) {
                if(recovery_items[i].action) {
                    selection = i;
                    break;
                }
            }
            break;

        case BL_QUIT:
            shutdown();
            break;

        default:
            break;
        }
    }
}

void boot_rockbox(void)
{
    if(init_disk() != 0)
        return;

    int rc = load_firmware(loadbuffer, BOOTFILE, MAX_LOAD_SIZE);
    if(rc <= 0) {
        splash2(5*HZ, "Error loading Rockbox", loader_strerror(rc));
        return;
    }

    if(lcd_inited)
        backlight_hw_off();

    disable_irq();
    exec(loadaddress, loadbuffer, rc);
}

void usb_mode(void)
{
    init_usb();
    splash2(0, "Waiting for USB", "Press " BL_QUIT_NAME " to go back");

    while(1) {
        int btn = button_get(true);
        if(btn == SYS_USB_CONNECTED)
            break;
        else if(btn == BL_QUIT)
            return;
    }

    splash(0, "USB mode");
    usb_acknowledge(SYS_USB_CONNECTED_ACK);
    while(button_get(true) != SYS_USB_DISCONNECTED);

    splash(3*HZ, "USB disconnected");
}

void shutdown(void)
{
    splash(HZ, "Shutting down");
    power_off();
    while(1);
}

void reboot(void)
{
    splash(HZ, "Rebooting");
    system_reboot();
    while(1);
}

/* TODO: clean this up, make the installer generic as well */
enum {
    INSTALL,
    BACKUP,
    RESTORE,
};

#ifdef FIIO_M3K
void bootloader_action(int which)
{
    if(init_disk() != 0) {
        splash2(5*HZ, "Install aborted", "Cannot access SD card");
        return;
    }

    const char* msg;
    switch(which) {
    case INSTALL: msg = "Installing"; break;
    case BACKUP:  msg = "Backing up"; break;
    case RESTORE: msg = "Restoring";  break;
    default: return; /* can't happen */
    }

    splash(0, msg);

    int rc;
    switch(which) {
    case INSTALL: rc = install_boot("/bootloader.m3k");   break;
    case BACKUP:  rc =  backup_boot("/fiiom3k-boot.bin"); break;
    case RESTORE: rc = restore_boot("/fiiom3k-boot.bin"); break;
    default: return;
    }

    static char buf[64];
    snprintf(buf, sizeof(buf), "Failed! Error: %d", rc);
    const char* msg1 = rc == 0 ? "Success" : buf;
    const char* msg2 = "Press " BL_QUIT_NAME " to continue";
    splash2(0, msg1, msg2);

    button_clear_queue();
    while(button_get(true) != BL_QUIT);
}
#else
void bootloader_action(int which)
{
    (void)which;
    splash(5*HZ, "Not implemented!");
}
#endif

void bootloader_install(void)
{
    bootloader_action(INSTALL);
}

void bootloader_backup(void)
{
    bootloader_action(BACKUP);
}

void bootloader_restore(void)
{
    bootloader_action(RESTORE);
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
