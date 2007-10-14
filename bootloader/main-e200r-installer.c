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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "ata.h"
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
#define KNOWN_CRC32 0x5a09c266
char knownBytes[] = {0x00, 0x24, 0x07, 0xe1};
char changedBytes[] = {0xc0, 0x46, 0xc0, 0x46 };
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
    __backlight_on();
    
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

    i=ata_init();
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
    ata_read_sectors(IF_MV2(0,) 
                        pinfo->start + START_SECTOR_OF_ROM + ROMSECTOR_TO_HACK,
                        1 , sector);
    crc32 = chksum_crc32 (sector, 512);
#if 0 /* not needed in release builds */
    printf("--- Hack Status ---");
    printf("Sector checksum: %x", crc32);
#endif
    if ((crc32 == KNOWN_CRC32) && 
        !memcmp(&sector[HACK_OFFSET], knownBytes, 
                sizeof(knownBytes)/sizeof(*knownBytes)))
    {
    
        memcpy(&sector[HACK_OFFSET], changedBytes,
                sizeof(changedBytes)/sizeof(*changedBytes));
        ata_write_sectors(IF_MV2(0,) 
                        pinfo->start + START_SECTOR_OF_ROM + ROMSECTOR_TO_HACK,
                        1 , sector);
        printf("Firmware Unlocked");
        printf("Proceed to Step 2");
    }
    else
    {
        printf("Unknown bootloader");
        printf("Rockbox installer cannot");
        printf("continue");
    }
    GPIOG_OUTPUT_VAL &=~0x80;
    printf("");
    if (button_hold())
        printf("Release Hold and");
    printf("Press any key to shutdown");
    while(button_read_device() == BUTTON_NONE)
        ;
    power_off();
    return NULL;
}

