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
    adc_close();
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

    if (battery_voltage <= 3500) 
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

    /* We want to read the buttons as early as possible, before the user
       releases the ON button */

#ifdef IAUDIO_M3
    or_l(0x80000000, &GPIO_FUNCTION);  /* remote Play button */
    and_l(~0x80000000, &GPIO_ENABLE);
    or_l(0x00000202, &GPIO1_FUNCTION); /* main Hold & Play */
    
    if ((GPIO1_READ & 0x000000002) == 0)
        on_button = true;
        
    if ((GPIO_READ & 0x80000000) == 0)
        rc_on_button = true;
#elif defined(IAUDIO_M5) || defined(IAUDIO_X5)
    int data;

    or_l(0x0e000000, &GPIO_FUNCTION); /* main Hold & Power, remote Play */
    and_l(~0x0e000000, &GPIO_ENABLE);
    
    data = GPIO_READ;
    
    if ((data & 0x04000000) == 0)
        on_button = true;
        
    if ((data & 0x02000000) == 0)
        rc_on_button = true;
#endif

    power_init();

    system_init();
    kernel_init();

    set_cpu_frequency(CPUFREQ_NORMAL);
    coldfire_set_pllcr_audio_bits(DEFAULT_PLLCR_AUDIO_BITS);

    enable_irq();
    lcd_init();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_init();
#endif
    backlight_init();
    font_init();
    adc_init();
    button_init();
    
    if ((!on_button || button_hold())
        && (!rc_on_button || remote_button_hold())
        && !charger_inserted())
    {
        /* No need to check for USB connection here, as USB is handled
         * in the cowon loader. */
        if (on_button || rc_on_button)
            printf("Hold switch on");
        shutdown();
    }

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
