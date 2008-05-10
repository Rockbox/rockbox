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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "system.h"
#include <sprintf.h>
#include "kernel.h"
#include "string.h"
#include "adc.h"
#include "powermgmt.h"
#include "ata.h"
#include "dir.h"
#include "disk.h"
#include "common.h"
#include "backlight.h"
#include "usb.h"
#include "button.h"
#include "font.h"
#include "lcd.h"
#include "usb-target.h"

#define TAR_CHUNK 512
#define TAR_HEADER_SIZE 157

const char version[] = APPSVERSION;
/* Where files sent via MTP are stored */
static const char basedir[] = "/Content/0b00/00/";
/* Can use memory after vector table up to 0x01f00000 */
static char * const tarbuf = (char *)0x00000040;
static const size_t tarbuf_size = 0x01f00000 - 0x00000040;
/* Firmware data */
static void * const load_buf = 0x00000000;
static const size_t load_buf_size = 0x20000000 - 0x100000;
static const void * const start_addr = 0x00000000;

static void show_splash(int timeout, const char *msg)
{
    backlight_on();
    reset_screen();
    lcd_putsxy( (LCD_WIDTH - (SYSFONT_WIDTH * strlen(msg))) / 2,
                (LCD_HEIGHT - SYSFONT_HEIGHT) / 2, msg);
    lcd_update();

    sleep(timeout);
}

static bool pause_if_button_pressed(bool pre_usb)
{
    while (1)
    {
        int button = button_read_device();

        if (pre_usb && !usb_plugged())
            return false;

        /* Exit if no button or only the menu (settings reset) button */
        switch (button)
        {
        case BUTTON_MENU:
        case BUTTON_NONE:
            return true;
        }

        sleep(HZ/5);

        /* If the disk powers off, the firmware will lock at startup */
        ata_spin();
    }
}

/* TODO: Handle charging while connected */
static void handle_usb(void)
{
    int button;

    /* Check if plugged and pause to look at messages. If the cable was pulled
     * while waiting, proceed as if it never was plugged. */
    if (!usb_plugged() || !pause_if_button_pressed(true))
    {
        /* Bang on the controller */
        usb_init_device();
        return;
    }

    /** Enter USB mode **/

    /* We need full button and backlight handling now */
    backlight_init();
    button_init();

    /* Start the USB driver */
    usb_init();
    usb_start_monitoring();

    /* Wait for threads to connect or cable is pulled */
    show_splash(HZ/2, "Waiting for USB");

    while (1)
    {
        button = button_get_w_tmo(HZ/2);

        if (button == SYS_USB_CONNECTED)
            break; /* Hit */

        if (!usb_plugged())
            break; /* Cable pulled */
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

    /* Put drivers initialized for USB connection into a known state */
    backlight_on();
    usb_close();
    button_close();
    backlight_close();

    reset_screen();
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

            fd = creat(path);
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
                printf("Found rockbox binary. Moving...");
                close(fd);
                remove("/.rockbox/rockbox.gigabeat");
                int ret = rename(buf, "/.rockbox/rockbox.gigabeat");
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
static void __attribute__((noreturn)) handle_firmware_load(void)
{
    int rc = load_firmware(load_buf, "/.rockbox/rockbox.gigabeat",
                           load_buf_size);

    if(rc < 0)
        error(EBOOTFILE, rc);

    /* Pause to look at messages */
    pause_if_button_pressed(false);

    /* Put drivers into a known state */
    button_close_device();
    ata_close();
    system_prepare_fw_start();

    if (rc == EOK)
    {
        invalidate_icache();
        asm volatile ("bx %0": : "r"(start_addr));
    }

    /* Halt */
    while (1)
        core_idle();
}

static void check_battery(void)
{
    int batt = battery_adc_voltage();
    printf("Battery: %d.%03d V", batt / 1000, batt % 1000);
    /* TODO: warn on low battery or shut down */
}

void main(void)
{
    int rc;

    /* Flush and invalidate all caches (because vectors were written) */
    invalidate_icache();

    lcd_clear_display();
    printf("Gigabeat S Rockbox Bootloader");
    printf("Version %s", version);
    system_init();
    kernel_init();

    enable_interrupt(IRQ_FIQ_STATUS);

    /* Initialize KPP so we can poll the button states */
    button_init_device();

    adc_init();

    check_battery();

    rc = ata_init();
    if(rc)
    {
        reset_screen();
        error(EATA, rc);
    }

    disk_init();

    rc = disk_mount_all();
    if (rc<=0)
    {
        error(EDISK,rc);
    }

    printf("Init complete");

    /* Do USB first since a tar or binary could be added to the MTP directory
     * at the time and we can untar or move after unplugging. */
    handle_usb();
    handle_untar();
    handle_firmware_load(); /* No return */
}
