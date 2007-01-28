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
#include "power.h"

/* Maximum allowed firmware image size. 10MB is more than enough */
#define MAX_LOADSIZE (10*1024*1024)

/* A buffer to load the original firmware or Rockbox into */
unsigned char *loadbuffer = (unsigned char *)DRAM_START;

/* Bootloader version */
char version[] = APPSVERSION;

void* main(void)
{
    char buf[256];
    int i;
    int rc;
    unsigned short* identify_info;
    struct partinfo* pinfo;

    system_init();
    kernel_init();
    lcd_init();
    font_init();
    button_init();

    lcd_setfont(FONT_SYSFIXED);

    printf("Rockbox boot loader");
    printf("Version: 20%s", version);
    printf(MODEL_NAME);

    i=ata_init();
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
        printf("ATA error: %d", i);
        sleep(HZ*5);
        power_off();
    }

    disk_init();
    rc = disk_mount_all();
    if (rc<=0)
    {
        printf("No partition found");
        sleep(HZ*5);
        power_off();
    }

    pinfo = disk_partinfo(0);
    printf("Partition 0: 0x%02x %ld MB", pinfo->type, pinfo->size / 2048);

    i=button_read_device();
    if(i==BUTTON_LEFT)
    {
        /* Load original mi4 firmware. This expects a file called 
           "/System/OF.bin" on the player. It should be a mi4 firmware decrypted 
           and header stripped using mi4code. It reads the file in to a memory
           buffer called loadbuffer. The rest of the loading is done in crt0.S
        */
        printf("Loading original firmware...");
        rc=load_raw_firmware(loadbuffer, "/System/OF.bin", MAX_LOADSIZE);
        if (rc < EOK) {
            printf("Error!");
            printf("Can't load /System/OF.bin:");
            printf(strerror(rc));
            sleep(HZ*5);
            power_off();
        }
    } else {
        printf("Loading Rockbox...");
        rc=load_firmware(loadbuffer, BOOTFILE, MAX_LOADSIZE);
        if (rc < EOK) {
            printf("Error!");
            printf("Can't load %s:", BOOTFILE);
            printf(strerror(rc));
            sleep(HZ*5);
            power_off();
        }
    }
    
    return (void*)loadbuffer;
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */
void usb_acknowledge(void)
{
}

void usb_wait_for_disconnect(void)
{
}
