/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Roman Stolyarov
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
#ifdef HAVE_BOOTLOADER_USB_MODE
#include "usb.h"
#endif
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

#define SHOW_LOGO

extern void show_logo(void);
extern void power_off(void);

static int lcd_inited = 0;
void init_lcd(void)
{
    if(lcd_inited)
        return;

    lcd_init();
    font_init();
    lcd_setfont(FONT_SYSFIXED);

    lcd_clear_display();
    lcd_update();

    backlight_init();

    lcd_inited = true;
}

#ifdef HAVE_BOOTLOADER_USB_MODE
static void show_splash(int timeout, const char *msg)
{
    init_lcd();
    reset_screen();
    lcd_putsxy( (LCD_WIDTH - (SYSFONT_WIDTH * strlen(msg))) / 2,
                (LCD_HEIGHT - SYSFONT_HEIGHT) / 2, msg);
    lcd_update();

    sleep(timeout);
}

static int usb_inited = 0;

static void usb_mode(void)
{
    int button = 0;

    /* Init USB, but only once */
    if (!usb_inited) {
        usb_init();
        usb_start_monitoring();
        usb_inited = 1;
    }

    init_lcd();
    reset_screen();

    /* Wait for USB */
    show_splash(0, "Waiting for USB");
    while(1) {
        button = button_get_w_tmo(HZ/2);
        if (button == BUTTON_POWER)
            return;
        if (button == SYS_USB_CONNECTED)
            break; /* Hit */
    }

    /* Got the message - wait for disconnect */
    show_splash(0, "Bootloader USB mode");

    usb_acknowledge(SYS_USB_CONNECTED_ACK);

    while(1) {
        button = button_get_w_tmo(HZ/2);
        if (button == SYS_USB_DISCONNECTED)
            break;
    }

    show_splash(HZ*2, "USB disconnected");
}
#endif

/* Jump to loaded binary */
void exec(void* addr) __attribute__((noinline, noreturn, section(".icode")));

void exec(void* addr)
{
    commit_discard_idcache();
    typedef void(*entry_fn)(void) __attribute__((noreturn));
    entry_fn fn = (entry_fn)addr;
    fn();
}

static int boot_rockbox(void)
{
    int rc;

    printf("Mounting disk...");

    while((rc = disk_mount_all()) <= 0) {
        verbose = true;
#ifdef HAVE_BOOTLOADER_USB_MODE
        error(EDISK, rc, false);
        usb_mode();
#else
        error(EDISK, rc, true);
#endif
    }

    printf("Loading firmware...");
    rc = load_firmware((unsigned char *)CONFIG_SDRAM_START, BOOTFILE, 0x400000);
    if(rc <= EFILE_EMPTY) {
        printf("!! " BOOTFILE);
        return rc;
    } else {
        printf("Starting Rockbox...");
        adc_close(); /* Disable SADC, seems to fix the re-init Rockbox does */
        disable_interrupt();
        exec((void*) CONFIG_SDRAM_START);
        return 0; /* Shouldn't happen */
    }
}

#if 0
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
#endif

int main(void)
{
    int rc;

    serial_puts("\n\nSPL Stage 2\n\n");

    system_init();
    kernel_init();

    init_lcd();
#ifdef SHOW_LOGO
    show_logo();
#endif

    button_init();
    enable_irq();

    int btn = button_read_device();
    if(btn & BUTTON_POWER) {
       verbose = true;
    }

    rc = storage_init();
    if(rc) {
        verbose = true;
        error(EATA, rc, true);
    }

    filesystem_init();

#ifdef HAVE_BOOTLOADER_USB_MODE
    /* Enter USB mode if USB is plugged and POWER button is pressed */
    if (btn & BUTTON_POWER) {
        usb_mode();
        reset_screen();
    }
#endif /* HAVE_BOOTLOADER_USB_MODE */

#ifndef SHOW_LOGO
    printf(MODEL_NAME" Rockbox Bootloader");
    printf("Version %s", rbversion);
#endif

    rc = boot_rockbox();

    if(rc <= EFILE_EMPTY) {
        verbose = true;
        printf("Error: %s", loader_strerror(rc));
    }

    /* Power off */
    sleep(5*HZ);
    power_off();

    return 0;
}
