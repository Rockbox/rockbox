/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Greg White
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
#include "system.h"
#include <stdio.h>
#include "kernel.h"
#include "gcc_extensions.h"
#include "string.h"
#include "adc.h"
#include "powermgmt.h"
#include "storage.h"
#include "dir.h"
#include "disk.h"
#include "common.h"
#include "power.h"
#include "backlight.h"
#include "usb.h"
#include "button.h"
#include "font.h"
#include "lcd.h"
#include "usb-target.h"
#include "version.h"

/* Show the Rockbox logo - in show_logo.c */
extern int show_logo(void);

#define TAR_CHUNK 512
#define TAR_HEADER_SIZE 157

/* Where files sent via MTP are stored */
static const char basedir[] = "/Content/0b00/00/";
/* Can use memory after vector table up to 0x01f00000 */
static char * const tarbuf = (char *)0x00000040;
static const size_t tarbuf_size = 0x01f00000 - 0x00000040;
/* Firmware data */
static void * const load_buf = 0x00000000;
static const size_t load_buf_size = 0x20000000 - 0x100000;
static const void * const start_addr = 0x00000000;

/* Show a message + "Shutting down...", then power off the device */
static void display_message_and_power_off(int timeout, const char *msg)
{
    verbose = true;
    printf(msg);
    printf("Shutting down...");
    sleep(timeout);
    power_off();
}

static void check_battery_safe(void)
{
    if (battery_level_safe())
        return;

    display_message_and_power_off(HZ, "Battery low");
}

/* TODO: Handle charging while connected */
static void handle_usb(int connect_timeout)
{
    long end_tick = 0;
    int button;

    /* We need full button and backlight handling now */
    backlight_init();
    button_init();
    backlight_on();

    /* Start the USB driver */
    usb_init();
    usb_start_monitoring();

    /* Wait for threads to connect or cable is pulled */
    printf("USB: Connecting");

    if (connect_timeout != TIMEOUT_BLOCK)
        end_tick = current_tick + connect_timeout;

    while (1)
    {
        button = button_get_w_tmo(HZ/2);

        if (button == SYS_USB_CONNECTED)
            break; /* Hit */

        if (connect_timeout != TIMEOUT_BLOCK &&
            TIME_AFTER(current_tick, end_tick))
        {
            /* Timed out waiting for the connect - will happen when connected
             * to a charger through the USB port */
            printf("USB: Timed out");
            break;
        }

        if (!usb_plugged())
            break; /* Cable pulled */
    }

    if (button == SYS_USB_CONNECTED)
    {
        /* Switch to verbose mode if not in it so that the status updates
         * are shown */
        verbose = true;
        /* Got the message - wait for disconnect */
        printf("Bootloader USB mode");

        usb_acknowledge(SYS_USB_CONNECTED_ACK);

        while (1)
        {
            button = button_get_w_tmo(HZ/2);
            if (button == SYS_USB_DISCONNECTED)
                break;

            check_battery_safe();
        }

        backlight_on();
        /* Sleep a little to let the backlight ramp up */
        sleep(HZ*5/4);
    }

    /* Put drivers initialized for USB connection into a known state */
    usb_close();
    button_close();
    backlight_close();
}

static void untar(int tar_fd)
{
    char header[TAR_HEADER_SIZE];
    char *ptr;
    char path[102];
    int fd, i;
    int ret;
    size_t size = filesize(tar_fd);

    if (size > tarbuf_size)
    {
        printf("tar file too large"); /* Paranoid but proper */
        return;
    }

    ret = read(tar_fd, tarbuf, filesize(tar_fd));
    if (ret < 0)
    {
        printf("couldn't read tar file (%d)", ret);
        return;
    }
    ptr = tarbuf;

    while (1)
    {
        memcpy(header, ptr, TAR_HEADER_SIZE);

        if (*header == '\0')  /* Check for EOF */
            break;

        /* Parse the size field */
        size = 0;
        for (i = 124 ; i < 124 + 11 ; i++) {
            size = (8 * size) + header[i] - '0';
        }

        /* Skip rest of header */
        ptr += TAR_CHUNK;

        /* Make the path absolute */
        strcpy(path, "/");
        strcat(path, header);

        if (header[156] == '0')  /* file */
        {
            int wc;

            fd = creat(path, 0666);
            if (fd < 0)
            {
                printf("failed to create file (%d)", fd);
            }
            else
            {
                wc = write(fd, ptr, size);
                if (wc < 0)
                {
                    printf("write failed (%d)", wc);
                    break;
                }
                close(fd);
            }
            ptr += (size + TAR_CHUNK-1) & (~(TAR_CHUNK-1));
        }
        else if (header[156] == '5')  /* directory */
        {
            int ret;

            /* Remove the trailing slash */
            if (path[strlen(path) - 1] == '/')
                path[strlen(path) - 1] = '\0';

            /* Create the dir */
            ret = mkdir(path);
            if (ret < 0 && ret != -4)
            {
                printf("failed to create dir (%d)", ret);
            }
        }
    }
}

/* Look for a tar file or rockbox binary in the MTP directory */
static void handle_untar(void)
{
    char buf[MAX_PATH];
    char tarstring[6];
    char model[5];
    struct dirent_uncached* entry;
    DIR_UNCACHED* dir;
    int fd;
    int rc;

    dir = opendir_uncached(basedir);

    while ((entry = readdir_uncached(dir)))
    {
        if (*entry->d_name == '.')
            continue;

        snprintf(buf, sizeof(buf), "%s%s", basedir, entry->d_name);
        fd = open(buf, O_RDONLY);

        if (fd < 0)
            continue;

        /* Check whether the file is a rockbox binary. */
        lseek(fd, 4, SEEK_SET);
        rc = read(fd, model, 4);
        if (rc == 4)
        {
            model[4] = 0;
            if (strcmp(model, "gigs") == 0)
            {
                verbose = true;
                printf("Found rockbox binary. Moving...");
                close(fd);
                remove( BOOTDIR "/" BOOTFILE);
                int ret = rename(buf, BOOTDIR "/" BOOTFILE);
                printf("returned %d", ret);
                sleep(HZ);
                break;
            }
        }

        /* Check whether the file is a tar file. */
        lseek(fd, 257, SEEK_SET);
        rc = read(fd, tarstring, 5);
        if (rc == 5)
        {
            tarstring[5] = 0;
            if (strcmp(tarstring, "ustar") == 0)
            {
                verbose = true;
                printf("Found tar file. Unarchiving...");
                lseek(fd, 0, SEEK_SET);
                untar(fd);
                close(fd);
                printf("Removing tar file");
                remove(buf);
                break;
            }
        }

        close(fd);
    }
}

/* Try to load the firmware and run it */
static void NORETURN_ATTR handle_firmware_load(void)
{
    int rc = load_firmware(load_buf, BOOTFILE, load_buf_size);

    if(rc < 0)
        error(EBOOTFILE, rc, true);

    /* Pause to look at messages */
    while (1)
    {
        int button = button_read_device();

        /* Ignore settings reset */
        if (button == BUTTON_NONE || button == BUTTON_MENU)
            break;

        sleep(HZ/5);

        check_battery_safe();

        /* If the disk powers off, the firmware will lock at startup */
        storage_spin();
    }

    /* Put drivers into a known state */
    button_close_device();
    storage_close();
    system_prepare_fw_start();

    if (rc == EOK)
    {
        cpucache_commit_discard();
        asm volatile ("bx %0": : "r"(start_addr));
    }

    /* Halt */
    while (1)
        core_idle();
}

void main(void)
{
    int rc;
    int batt;

    system_init();
    kernel_init();

    /* Keep button_device_init early to delay calls to button_read_device */
    button_init_device();

    lcd_init();
    font_init();
    show_logo();
    lcd_clear_display();

    if (button_hold())
        display_message_and_power_off(HZ, "Hold switch on");

    if (button_read_device() != BUTTON_NONE)
        verbose = true;

    printf("Gigabeat S Rockbox Bootloader");
    printf("Version " RBVERSION);

    adc_init();
    batt = battery_adc_voltage();
    printf("Battery: %d.%03d V", batt / 1000, batt % 1000);
    check_battery_safe();

    rc = storage_init();
    if(rc)
        error(EATA, rc, true);

    disk_init();

    rc = disk_mount_all();
    if (rc <= 0)
        error(EDISK, rc, true);

    printf("Init complete");

    /* Do USB first since a tar or binary could be added to the MTP directory
     * at the time and we can untar or move after unplugging. */
    if (usb_plugged())
        handle_usb(HZ*2);

    handle_untar();
    handle_firmware_load(); /* No return */
}
