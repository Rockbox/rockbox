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

#include <stdlib.h>
#include <stdio.h>
#include "inttypes.h"
#include "string.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "storage.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"
#include "rbunicode.h"
#include "usb.h"
#include "mmu-arm.h"
#include "rtc.h"

#include <stdarg.h>

char version[] = APPSVERSION;

void shutdown(void)
{
    /* We need to gracefully spin down the disk to prevent clicks. */
    if (ide_powered())
    {
        /* Make sure ATA has been initialized. */
        ata_init();
        
        /* And put the disk into sleep immediately. */
        ata_sleepnow();
    }
    
    _backlight_off();

    power_off();
}

void main(void)
{
    unsigned char* loadbuffer;
    int buffer_size;
    int rc;
    int(*kernel_entry)(void);

    system_init();
    lcd_init();
    backlight_init();
    button_init();
    font_init();
    kernel_init(); /* Need the kernel to sleep */
    adc_init();

    lcd_setfont(FONT_SYSFIXED);
    
    if(!(GPGDAT&BUTTON_POWER) && charger_inserted())
    {
        while(!(GPGDAT&BUTTON_POWER) && charger_inserted())
        {
            char msg[20];
            if(charging_state())
            {
                snprintf(msg,sizeof(msg),"Charging");
            }
            else
            {
                snprintf(msg,sizeof(msg),"Charge Complete");
            }
            reset_screen();
            lcd_putsxy( (LCD_WIDTH - (SYSFONT_WIDTH * strlen(msg))) / 2,
                    (LCD_HEIGHT - SYSFONT_HEIGHT) / 2, msg);
            lcd_update();
            
#if defined(HAVE_RTC_ALARM)            
            /* Check if the alarm went off while charging */
            if(rtc_check_alarm_flag())
            {
                GSTATUS4=1; /* Normally this is set in crt0.s */
                break;
            }
#endif
        }
        if(!(GPGDAT&BUTTON_POWER) 
#if defined(HAVE_RTC_ALARM)
            && !GSTATUS4
#endif
            )
        {
            shutdown();
        }
    }

    if(button_hold())
    {
        const char msg[] = "HOLD is enabled";
        reset_screen();
        lcd_putsxy( (LCD_WIDTH - (SYSFONT_WIDTH * strlen(msg))) / 2,
                    (LCD_HEIGHT - SYSFONT_HEIGHT) / 2, msg);
        lcd_update();
        
        sleep(2*HZ);
        
        shutdown();
    }

    power_init();
    usb_init();

    /* Enter USB mode without USB thread */
    if(usb_detect() == USB_INSERTED)
    {
        const char msg[] = "Bootloader USB mode";
        reset_screen();
        lcd_putsxy( (LCD_WIDTH - (SYSFONT_WIDTH * strlen(msg))) / 2,
                    (LCD_HEIGHT - SYSFONT_HEIGHT) / 2, msg);
        lcd_update();

        storage_enable(false);
        sleep(HZ/20);
        usb_enable(true);

        while (usb_detect() == USB_INSERTED)
            sleep(HZ);

        usb_enable(false);

        reset_screen();
        lcd_update();
    }
    
    reset_screen();

    /* Show debug messages if button is pressed */
    if(button_read_device())
        verbose = true;

    printf("Rockbox boot loader");
    printf("Version %s", version);

    sleep(50); /* ATA seems to error without this pause */

    rc = storage_init();
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

    printf("Loading firmware");

    loadbuffer = (unsigned char*) 0x31000000;
    buffer_size = (unsigned char*)0x31400000 - loadbuffer;

    rc = load_firmware(loadbuffer, BOOTFILE, buffer_size);
    if(rc < 0)
        error(EBOOTFILE, rc);

    if (rc == EOK)
    {
        kernel_entry = (void*) loadbuffer;
        rc = kernel_entry();
    }
}

