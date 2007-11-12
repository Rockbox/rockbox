/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include "inttypes.h"
#include "string.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "lcd-remote.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "usb.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "powermgmt.h"
#include "file.h"

#include "pcf50606.h"
#include "common.h"

#include <stdarg.h>

/* Maximum allowed firmware image size. 10MB is more than enough */
#define MAX_LOADSIZE (10*1024*1024)

#define DRAM_START 0x31000000

int usb_screen(void)
{
   return 0;
}

char version[] = APPSVERSION;

/* Reset the cookie for the crt0 crash check */
inline void __reset_cookie(void)
{
    asm(" move.l #0,%d0");
    asm(" move.l %d0,0x10017ffc");
}

void start_firmware(void)
{
    asm(" move.w #0x2700,%sr");
    __reset_cookie();
    asm(" move.l %0,%%d0" :: "i"(DRAM_START));
    asm(" movec.l %d0,%vbr");
    asm(" move.l %0,%%sp" :: "m"(*(int *)DRAM_START));
    asm(" move.l %0,%%a0" :: "m"(*(int *)(DRAM_START+4)));
    asm(" jmp (%a0)");
}

void shutdown(void)
{
    printf("Shutting down...");
    
    /* We need to gracefully spin down the disk to prevent clicks. */
    if (ide_powered())
    {
        /* Make sure ATA has been initialized. */
        ata_init();
        
        /* And put the disk into sleep immediately. */
        ata_sleepnow();
    }

    sleep(HZ*2);
    
    /* Backlight OFF */
    _backlight_off();
#ifdef HAVE_REMOTE_LCD
    _remote_backlight_off();
#endif
    
    __reset_cookie();
    power_off();
}

/* Print the battery voltage (and a warning message). */
void check_battery(void)
{
    int battery_voltage, batt_int, batt_frac;
    
    battery_voltage = battery_adc_voltage();
    batt_int = battery_voltage / 1000;
    batt_frac = (battery_voltage % 1000) / 10;

    printf("Batt: %d.%02dV", batt_int, batt_frac);

    if (battery_voltage <= 350) 
    {
        printf("WARNING! BATTERY LOW!!");
        sleep(HZ*2);
    }
}

void main(void)
{
    int i;
    int rc;
    bool rc_on_button = false;
    bool on_button = false;
    bool rec_button = false;
    bool hold_status = false;
    int data;

    (void)rc_on_button;
    (void)on_button;
    (void)rec_button;
    (void)hold_status;
    (void)data;
    power_init();

    system_init();
    kernel_init();

    set_cpu_frequency(CPUFREQ_NORMAL);
    coldfire_set_pllcr_audio_bits(DEFAULT_PLLCR_AUDIO_BITS);

    set_irq_level(0);
    lcd_init();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_init();
#endif
    backlight_init();
    font_init();
    adc_init();
    button_init();

    printf("Rockbox boot loader");
    printf("Version %s", version);
    
    check_battery();

    rc = ata_init();
    if(rc)
    {
        printf("ATA error: %d", rc);
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

    printf("Loading firmware");
    i = load_firmware((unsigned char *)DRAM_START, BOOTFILE, MAX_LOADSIZE);
    printf("Result: %s", strerror(i));

    if (i < EOK) {
        printf("Error!");
        printf("Can't load rockbox.iaudio:");
        printf(strerror(rc));
        sleep(HZ*3);
        power_off();
    } else {
        start_firmware();
    }
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */
void screen_dump(void)
{
}
