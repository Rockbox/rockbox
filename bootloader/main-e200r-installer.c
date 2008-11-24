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
#include "common.h"
#include "cpu.h"
#include "file.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "font.h"
#include "storage.h"
#include "button.h"
#include "disk.h"
#include "crc32-mi4.h"
#include <string.h>
#include "i2c.h"
#include "backlight-target.h"
#include "power.h"

/* Bootloader version */
char version[] = APPSVERSION;

#define START_SECTOR_OF_ROM 1
#define ROMSECTOR_TO_HACK 63
#define HACK_OFFSET 498
#define KNOWN_CRC32 0x5a09c266    /* E200R CRC before patching */
#define PATCHED_CRC32 0x0a162b34  /* E200R CRC after patching */

static unsigned char knownBytes[] = {0x00, 0x24, 0x07, 0xe1};
static unsigned char changedBytes[] = {0xc0, 0x46, 0xc0, 0x46 };

/* 
   CRC32s of sector 63 from E200 bootloaders - so we can tell users if they're
   trying to use e200rpatcher with a vanilla e200.

   These are all known E200 bootloaders as of 8 November 2007.

*/

static uint32_t e200_crcs[] =
{
    0xbeceba58,
    0x4e6b038f,
    0x5e4f4219,
    0xae087742,
    0x3dd94852,
    0x72fa69f3,
    0x4ce0d10b
};

#define NUM_E200_CRCS ((int)((sizeof(e200_crcs) / sizeof(uint32_t))))

static bool is_e200(uint32_t crc)
{
    int i;

    for (i = 0 ; i < NUM_E200_CRCS ; i++) 
    {
        if (crc == e200_crcs[i])
            return true;
    }

    return false;
}


void* main(void)
{
    int i;
    int btn;
    int num_partitions;
    int crc32;
    char sector[512];
    struct partinfo* pinfo;

    chksum_crc32gentab ();

    system_init();
    kernel_init();
    lcd_init();
    font_init();
    button_init();
    i2c_init();
    _backlight_on();
    
    lcd_set_foreground(LCD_WHITE);
    lcd_set_background(LCD_BLACK);
    lcd_clear_display();

    btn = button_read_device();
    verbose = true;

    lcd_setfont(FONT_SYSFIXED);

    printf("Rockbox e200R installer");
    printf("Version: %s", version);
    printf(MODEL_NAME);
    printf("");

    i=storage_init();
    disk_init(IF_MV(0));
    num_partitions = disk_mount_all();

    if (num_partitions<=0)
    {
        error(EDISK,num_partitions);
    }

    pinfo = disk_partinfo(1);

#if 0 /* not needed in release builds */
    printf("--- Partition info ---");
    printf("start: %x", pinfo->start);
    printf("size: %x", pinfo->size);
    printf("type: %x", pinfo->type);
    printf("reading: %x", (START_SECTOR_OF_ROM + ROMSECTOR_TO_HACK)*512);
#endif

    storage_read_sectors(0,
                         pinfo->start + START_SECTOR_OF_ROM + ROMSECTOR_TO_HACK,
                         1 , sector);
    crc32 = chksum_crc32 (sector, 512);

#if 0 /* not needed in release builds */
    printf("--- Hack Status ---");
    printf("Sector checksum: %x", crc32);
#endif

    if (crc32 == PATCHED_CRC32)
    {
        /* Bootloader already patched */
        printf("Already unlocked");
        printf("Proceed to Step 2");
    } else if ((crc32 == KNOWN_CRC32) && 
                !memcmp(&sector[HACK_OFFSET], knownBytes, 
                sizeof(knownBytes)/sizeof(*knownBytes)))
    {
        /* E200R bootloader detected - patch it */
        memcpy(&sector[HACK_OFFSET], changedBytes,
                sizeof(changedBytes)/sizeof(*changedBytes));
        storage_write_sectors(0,
                        pinfo->start + START_SECTOR_OF_ROM + ROMSECTOR_TO_HACK,
                        1 , sector);
        printf("Firmware unlocked");
        printf("Proceed to Step 2");
    } else if (is_e200(crc32))
    {
        printf("Vanilla E200 detected!");
        printf("Please install using");
        printf("Sansapatcher");
    }
    else
    {
        printf("Unknown bootloader");
        printf("Rockbox installer cannot");
        printf("continue");
    }

    /* Turn button lights off */
    GPIOG_OUTPUT_VAL &=~0x80;

    printf("");

    if (button_hold())
        printf("Release Hold and");

    printf("Press any key to shutdown");

    while(button_read_device() == BUTTON_NONE);

    power_off();

    return NULL;
}
