/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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
#include <stdlib.h>

#include "config.h"
#include "common.h"
#include "cpu.h"
#include "file.h"
#include "system.h"
#include "../kernel-internal.h"
#include "lcd.h"
#include "font.h"
#include "storage.h"
#include "file_internal.h"
#include "adc.h"
#include "button.h"
#include "disk.h"
#include "crc32.h"
#include "mi4-loader.h"
#include "loader_strerror.h"
#include <string.h>
#include "power.h"
#include "version.h"
#if defined(SANSA_E200) || defined(PHILIPS_SA9200)
#include "i2c.h"
#include "backlight-target.h"
#endif
#include "usb.h"
#if defined(SANSA_E200) || defined(SANSA_C200) || defined(PHILIPS_SA9200)
#include "usb_drv.h"
#endif
#if defined(SANSA_E200) && defined(HAVE_BOOTLOADER_USB_MODE)
#include "core_alloc.h"
#endif
#if defined(SAMSUNG_YH925)
/* this function (in lcd-yh925.c) resets the screen orientation for the OF
 * for use with dualbooting */
void lcd_reset(void);
#endif

/* Show the Rockbox logo - in show_logo.c */
extern void show_logo(void);

/* Button definitions */
#if CONFIG_KEYPAD == IRIVER_H10_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_LEFT

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_LEFT

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_LEFT

#elif CONFIG_KEYPAD == MROBE100_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_POWER

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_VOL_UP

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_MENU

#elif CONFIG_KEYPAD == PHILIPS_HDD6330_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_VOL_UP

#elif (CONFIG_KEYPAD == SAMSUNG_YH820_PAD) || \
      (CONFIG_KEYPAD == SAMSUNG_YH92X_PAD)
#define BOOTLOADER_BOOT_OF      BUTTON_LEFT

#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_LEFT

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define BOOTLOADER_BOOT_OF      BUTTON_OK

#endif

/* Maximum allowed firmware image size. 10MB is more than enough */
#define MAX_LOADSIZE (10*1024*1024)

/* A buffer to load the original firmware or Rockbox into */
unsigned char *loadbuffer = (unsigned char *)DRAM_START;

/* Locations and sizes in hidden partition on Sansa */
#if (CONFIG_STORAGE & STORAGE_SD)
#define PPMI_SECTOR_OFFSET  1024
#define PPMI_SECTORS        1
#define MI4_HEADER_SECTORS  1
#define NUM_PARTITIONS      2

#else
#define NUM_PARTITIONS      1

#endif

#define MI4_HEADER_SIZE     0x200

/* PPMI header structure */
struct ppmi_header_t {
    unsigned char magic[4];
    uint32_t length;
    uint32_t pad[126];
};

#if (CONFIG_STORAGE & STORAGE_SD)
/* Load mi4 firmware from a hidden disk partition */
int load_mi4_part(unsigned char* buf, struct partinfo* pinfo,
                  unsigned int buffer_size, bool disable_rebuild)
{
    struct mi4header_t mi4header;
    struct ppmi_header_t ppmi_header;
    unsigned long sum;
    
    /* Read header to find out how long the mi4 file is. */
    storage_read_sectors(IF_MD(0,) pinfo->start + PPMI_SECTOR_OFFSET,
                         PPMI_SECTORS, &ppmi_header);
    
    /* The first four characters at 0x80000 (sector 1024) should be PPMI*/
    if( memcmp(ppmi_header.magic, "PPMI", 4) )
        return EFILE_NOT_FOUND;
    
    printf("BL mi4 size: %x", ppmi_header.length);
    
    /* Read mi4 header of the OF */
    storage_read_sectors(IF_MD(0,) pinfo->start + PPMI_SECTOR_OFFSET + PPMI_SECTORS 
                       + (ppmi_header.length/512), MI4_HEADER_SECTORS, &mi4header);
    
    /* We don't support encrypted mi4 files yet */
    if( (mi4header.plaintext) != (mi4header.mi4size-MI4_HEADER_SIZE))
        return EINVALID_FORMAT;

    /* MI4 file size */
    printf("OF mi4 size: %x", mi4header.mi4size);

    if ((mi4header.mi4size-MI4_HEADER_SIZE) > buffer_size)
        return EFILE_TOO_BIG;

    /* CRC32 */
    printf("CRC32: %x", mi4header.crc32);

    /* Rockbox model id */
    printf("Model id: %.4s", mi4header.model);

    /* Read binary type (RBOS, RBBL) */
    printf("Binary type: %.4s", mi4header.type);

    /* Load firmware */
    storage_read_sectors(IF_MD(0,) pinfo->start + PPMI_SECTOR_OFFSET + PPMI_SECTORS
                        + (ppmi_header.length/512) + MI4_HEADER_SECTORS,
                        (mi4header.mi4size-MI4_HEADER_SIZE)/512, buf);

    /* Check CRC32 to see if we have a valid file */
    sum = crc_32r (buf,mi4header.mi4size-MI4_HEADER_SIZE,0);

    printf("Calculated CRC32: %x", sum);

    if(sum != mi4header.crc32)
        return EBAD_CHKSUM;
    
#ifdef SANSA_E200    
    if (disable_rebuild)
    {
        char block[512];
        
        printf("Disabling database rebuild");
        
        storage_read_sectors(IF_MD(0,) pinfo->start + 0x3c08, 1, block);
        block[0xe1] = 0;
        storage_write_sectors(IF_MD(0,) pinfo->start + 0x3c08, 1, block);
    }
#else
    (void) disable_rebuild;
#endif

    return mi4header.mi4size-MI4_HEADER_SIZE;
}
#endif /* (CONFIG_STORAGE & STORAGE_SD) */

#ifdef HAVE_BOOTLOADER_USB_MODE
/* Return USB_HANDLED if session took place else return USB_EXTRACTED */
static int handle_usb(int connect_timeout)
{
    static struct event_queue q SHAREDBSS_ATTR;
    struct queue_event ev;
    int usb = USB_EXTRACTED;
    long end_tick = 0;
    
    if (!usb_plugged())
        return USB_EXTRACTED;

    queue_init(&q, true);
    usb_init();
    usb_start_monitoring();

    printf("USB: Connecting");

    if (connect_timeout != TIMEOUT_BLOCK)
        end_tick = current_tick + connect_timeout;

    while (1)
    {
        /* Sleep no longer than 1/2s */
        queue_wait_w_tmo(&q, &ev, HZ/2);

        if (ev.id == SYS_USB_CONNECTED)
        {
            /* Switch to verbose mode if not in it so that the status updates
             * are shown */
            verbose = true;
            /* Got the message - wait for disconnect */
            printf("Bootloader USB mode");

            usb = USB_HANDLED;
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
#if defined(SANSA_E200) && defined(HAVE_BOOTLOADER_USB_MODE)
            /* E200 misses unplug randomly
               probably fine for other targets too but needs tested */
            while (usb_wait_for_disconnect_w_tmo(&q, HZ * 5) > 0)
            {
                /* timeout */
                if (!usb_plugged())
                    break;
            }
#else
            usb_wait_for_disconnect(&q);
#endif
            break;
        }

        if (connect_timeout != TIMEOUT_BLOCK &&
            TIME_AFTER(current_tick, end_tick))
        {
            /* Timed out waiting for the connect - will happen when connected
             * to a charger instead of a host port and the charging pin is
             * the same as the USB pin */
            printf("USB: Timed out");
            break;
        }

        if (!usb_plugged())
            break; /* Cable pulled */
    }

    usb_close();
    queue_delete(&q);

    return usb;
}
#elif (defined(SANSA_E200) || defined(SANSA_C200) || defined(PHILIPS_SA9200) \
    || defined (SANSA_VIEW)) && !defined(USE_ROCKBOX_USB)
/* Return USB_INSERTED if cable present */
static int handle_usb(int connect_timeout)
{
    int usb_retry = 0;
    int usb = USB_EXTRACTED;

    usb_init();
    while (usb_drv_powered() && usb_retry < 5 && usb != USB_INSERTED)
    {
        usb_retry++;
        sleep(HZ/4);
        usb = usb_detect();
    }

    if (usb != USB_INSERTED)
        usb = USB_EXTRACTED;

    return usb;
    (void)connect_timeout;
}
#else
/* Ignore cable state */
static int handle_usb(int connect_timeout)
{
    return USB_EXTRACTED;
    (void)connect_timeout;
}
#endif /* HAVE_BOOTLOADER_USB_MODE */

void* main(void)
{
    int i;
    int btn;
    int rc;
    int num_partitions;
    struct partinfo pinfo;
#if !(CONFIG_STORAGE & STORAGE_SD)
    char buf[256];
    unsigned short* identify_info;
#endif
    int usb = USB_EXTRACTED;

    system_init();
#if defined(SANSA_E200) && defined(HAVE_BOOTLOADER_USB_MODE)
    core_allocator_init();
#endif
    kernel_init();

#ifdef HAVE_BOOTLOADER_USB_MODE
    /* loader must service interrupts */
    enable_interrupt(IRQ_FIQ_STATUS);
#endif

    lcd_init();

    font_init();
    show_logo();

    adc_init();
#ifdef HAVE_BOOTLOADER_USB_MODE
    button_init_device();
#else
    button_init();
#endif
#if defined(SANSA_E200) || defined(PHILIPS_SA9200)
    i2c_init();
    backlight_hw_on();
#endif

    if (button_hold())
    {
        verbose = true;
        lcd_clear_display();
        printf("Hold switch on");
        printf("Shutting down...");
        sleep(HZ);
        power_off();
    }

    btn = button_read_device();

    /* Enable bootloader messages if any button is pressed */
#ifdef HAVE_BOOTLOADER_USB_MODE
    lcd_clear_display();
    if (btn)
        verbose = true;
#else
    if (btn) {
        lcd_clear_display();
        verbose = true;
    }
#endif

    lcd_setfont(FONT_SYSFIXED);

    printf("Rockbox boot loader");
    printf("Version: %s", rbversion);
    printf(MODEL_NAME);

    i=storage_init();
#if !(CONFIG_STORAGE & STORAGE_SD)
    if (i==0) {
        identify_info=ata_get_identify();
        /* Show model */
        for (i=0; i < 20; i++) {
            ((unsigned short*)buf)[i]=htobe16(identify_info[i+27]);
        }
        buf[40]=0;
        for (i=39; i && buf[i]==' '; i--) {
            buf[i]=0;
        }
        printf(buf);
    } else {
        error(EATA, i, true);
    }
#endif

    filesystem_init();
    num_partitions = disk_mount_all();
    if (num_partitions<=0)
    {
        error(EDISK,num_partitions, true);
    }

    /* Just list the first 2 partitions since we don't have any devices yet 
       that have more than that */
    for(i=0; i<NUM_PARTITIONS; i++)
    {
        disk_partinfo(i, &pinfo);
        printf("Partition %d: 0x%02x %ld MB",
                i, pinfo.type, pinfo.size / 2048);
    }

    /* Now that storage is initialized, check for USB connection */
    if ((btn & BOOTLOADER_BOOT_OF) == 0)
    {
        usb_pin_init();
        usb = handle_usb(HZ*2);
        if (usb == USB_INSERTED)
            btn |= BOOTLOADER_BOOT_OF;
    }

    /* Try loading Rockbox, if that fails, fall back to the OF */
    if((btn & BOOTLOADER_BOOT_OF) == 0)
    {
        printf("Loading Rockbox...");

        rc = load_mi4(loadbuffer, BOOTFILE, MAX_LOADSIZE);
        if (rc <= EFILE_EMPTY)
        {
            bool old_verbose = verbose;
            verbose = true;
            printf("Can't load " BOOTFILE ": ");
            printf(loader_strerror(rc));
            verbose = old_verbose;
            btn |= BOOTLOADER_BOOT_OF;
            sleep(5*HZ);
        }
        else
            goto main_exit;
    }

    if(btn & BOOTLOADER_BOOT_OF)
    {
        /* Load original mi4 firmware in to a memory buffer called loadbuffer.
           The rest of the loading is done in crt0.S.
           1) First try reading from the hidden partition (on Sansa only).
           2) Next try a decrypted mi4 file in /System/OF.mi4
           3) Finally, try a raw firmware binary in /System/OF.bin. It should be
              a mi4 firmware decrypted and header stripped using mi4code.
        */
        printf("Loading original firmware...");

#if (CONFIG_STORAGE & STORAGE_SD)
        /* First try a (hidden) firmware partition */
        printf("Trying firmware partition");
        disk_partinfo(1, &pinfo);
        if(pinfo.type == PARTITION_TYPE_OS2_HIDDEN_C_DRIVE)
        {
            rc = load_mi4_part(loadbuffer, &pinfo, MAX_LOADSIZE,
                               usb == USB_INSERTED);
            if (rc <= EFILE_EMPTY) {
                printf("Can't load from partition");
                printf(loader_strerror(rc));
            } else {
                goto main_exit;
            }
        } else {
            printf("No hidden partition found.");
        }
#endif

#if defined(PHILIPS_HDD1630) || defined(PHILIPS_HDD6330) || defined(PHILIPS_SA9200)
        printf("Trying /System/OF.ebn");
        rc=load_mi4(loadbuffer, "/System/OF.ebn", MAX_LOADSIZE);
        if (rc <= EFILE_EMPTY) {
            printf("Can't load /System/OF.ebn");
            printf(loader_strerror(rc));
        } else {
            goto main_exit;
        }
#endif

        printf("Trying /System/OF.mi4");
        rc=load_mi4(loadbuffer, "/System/OF.mi4", MAX_LOADSIZE);
        if (rc <= EFILE_EMPTY) {
            printf("Can't load /System/OF.mi4");
            printf(loader_strerror(rc));
        } else {
#if defined(SAMSUNG_YH925)
            lcd_reset();
#endif
            goto main_exit;
        }

        printf("Trying /System/OF.bin");
        rc=load_raw_firmware(loadbuffer, "/System/OF.bin", MAX_LOADSIZE);
        if (rc <= EFILE_EMPTY) {
            printf("Can't load /System/OF.bin");
            printf(loader_strerror(rc));
        } else {
#if defined(SAMSUNG_YH925)
            lcd_reset();
#endif
            goto main_exit;
        }
        
        error(0, 0, true);
    }

main_exit:
#ifdef HAVE_BOOTLOADER_USB_MODE
    storage_close();
    system_prepare_fw_start();
#endif

    return (void*)loadbuffer;
}
