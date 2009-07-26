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
#include "adc.h"

extern int show_logo(void);

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
            {
                usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
                break;
            }
        }
    }

    reset_screen();
}

static int boot_of(void)
{
    int fd, rc, len, i, checksum = 0;
    void (*kernel_entry)(int, void*, void*);

    /* TODO: get this from the NAND flash instead of SD */
    fd = open("/ccpmp.bin", O_RDONLY);
    if(fd < 0)
        return EFILE_NOT_FOUND;

    lseek(fd, 4, SEEK_SET);
    rc = read(fd, (char*)&len, 4); /* CPU is LE */
    if(rc < 4)
        return EREAD_IMAGE_FAILED;

    len += 8;
    printf("Reading %d bytes...", len);

    lseek(fd, 0, SEEK_SET);
    rc = read(fd, (void*)0x80004000, len);
    if(rc < len)
        return EREAD_IMAGE_FAILED;

    close(fd);

    for(i=0; i<len; i++)
        checksum += ((unsigned char*)0x80004000)[i];

    *((unsigned int*)0x80004000) = checksum;

    printf("Starting the OF...");

    /* OF requires all clocks on */
    __cpm_start_all();

    disable_interrupt();
    __dcache_writeback_all();
    __icache_invalidate_all();

    for(i=8000; i>0; i--)
        asm volatile("nop\n");

    kernel_entry = (void*) 0x80004008;
    kernel_entry(0, "Jan 10 2008", "15:34:42"); /* Reversed from the SPL */

    return 0;
}

int main(void)
{
    int rc;
#ifdef HAVE_TOUCHSCREEN
    int dummy;
#endif
    void (*kernel_entry)(void);

    kernel_init();
    lcd_init();
    font_init();
    lcd_setfont(FONT_SYSFIXED);
    button_init();
    backlight_init();

    show_logo();

    rc = storage_init();
    if(rc)
        error(EATA, rc);

#ifdef HAVE_TOUCHSCREEN
    rc = button_read_device(&dummy);
#else
    rc = button_read_device();
#endif

    if(rc)
        verbose = true;

    if(rc & BUTTON_VOL_UP)
        usb_mode();

    if(verbose)
        reset_screen();
    printf(MODEL_NAME" Rockbox Bootloader");
    printf("Version "APPSVERSION);

    rc = disk_mount_all();
    if (rc <= 0)
        error(EDISK,rc);

    if(button_hold())
    {
        rc = boot_of();
        if(rc < 0)
            printf("Error: %s", strerror(rc));
    }

    printf("Loading firmware");
    rc = load_firmware((unsigned char *)CONFIG_SDRAM_START, BOOTFILE, 0x400000);
    if(rc < 0)
        printf("Error: %s", strerror(rc));

    if (rc == EOK)
    {
        printf("Starting Rockbox...");
        adc_close();      /* Disable SADC */

        disable_interrupt();
        kernel_entry = (void*) CONFIG_SDRAM_START;
        kernel_entry();
    }

    /* Halt */
    while (1)
        core_idle();

    return 0;
}
