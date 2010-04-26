/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 Marcin Bukat
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

#include "common.h"

#include <stdarg.h>

/* Maximum allowed firmware image size. 10MB is more than enough */
#define MAX_LOADSIZE (10*1024*1024)

#define DRAM_START 0x31000000

#define BOOTMENU_TIMEOUT (10*HZ)
#define BOOTMENU_OPTIONS 3

/* From common.c */
extern int line;
static const char *bootmenu_options[] = {
    "Boot rockbox",
    "Boot MPIO firmware",
    "Shutdown"
};

enum option_t {
    rockbox,
    mpio_firmware,
    shutdown
};

int usb_screen(void)
{
   return 0;
}

char version[] = APPSVERSION;

bool _charger_inserted(void)
{
    return (GPIO1_READ & (1<<14)) ? false : true;
}

bool _battery_full(void)
{
    return (GPIO_READ & (1<<30)) ? true : false;
}

/* Reset the cookie for the crt0 crash check */
inline void __reset_cookie(void)
{
    asm(" move.l #0,%d0");
    asm(" move.l %d0,0x10017ffc");
}

void start_rockbox(void)
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

void start_mpio_firmware(void)
{
    asm(" move.w #0x2700,%sr");
    __reset_cookie();
    asm(" movec.l %d0,%vbr");
    asm(" move.l 0,%sp");
    asm(" jmp 8");
}

void __reset(void)
{
    asm(" move.w #0x2700,%sr");
    __reset_cookie();
    asm(" movec.l %d0,%vbr");
    asm(" move.l (0), %sp");
    asm(" movea.l (4),%a0");
    asm(" jmp (%a0)");
}

void __shutdown(void)
{
    /* We need to gracefully spin down the disk to prevent clicks. */
    if (ide_powered())
    {
        /* Make sure ATA has been initialized. */
        storage_init();
        
        /* And put the disk into sleep immediately. */
        storage_sleepnow();
    }

    /* Backlight OFF */
    _backlight_off();
    __reset_cookie();

    if (_charger_inserted())
    {
        /* reset instead of power_off() */
        __reset();
    }
    else
    {
        power_off();
    }
}

/* Print the battery voltage (and a warning message). */
void check_battery(void)
{

    int battery_voltage, batt_int, batt_frac;
    
    battery_voltage = battery_adc_voltage();
    batt_int = battery_voltage / 1000;
    batt_frac = (battery_voltage % 1000) / 10;

    printf("Battery: %d.%02dV", batt_int, batt_frac);

    if (battery_voltage <= 3500) 
    {
        printf("WARNING! BATTERY LOW!!");
        sleep(HZ*2);
    }

}


void lcd_putstring_centered(const char *string)
{
    int w,h;
    font_getstringsize(string, &w, &h, FONT_SYSFIXED);
    lcd_putsxy((LCD_WIDTH-w)/2, (LCD_HEIGHT-h)/2, string);
}

void bootmenu(void)
{
    int rc;
    enum option_t i;
    enum option_t option = rockbox;
    int button;
    const char select[] = "->";
    long start_tick = current_tick;

    /* backbone of menu */
    /* run the loader */
    printf("Rockbox boot loader");
    printf("Ver: %s", version);

    check_battery();

    printf("");
    printf("=========================");

    line += BOOTMENU_OPTIONS+2; /* skip lines */

    printf("=========================");
    printf("");
    printf(" [FF] [PREV] to move ");
    printf(" [PLAY] to confirm ");

    /* content of menu and keys handling */
    while (TIME_BEFORE(current_tick,start_tick + BOOTMENU_TIMEOUT))
    {
        /* Draw the menu. */
        line = 6; /* move below header */

        for (i=0;i<BOOTMENU_OPTIONS;i++)
        {
            if (i != option)
                printf("   %s",bootmenu_options[i]);
            else
                printf("%s %s",select,bootmenu_options[i]);
        }

        line = 15;

        printf("Time left: %ds",(BOOTMENU_TIMEOUT - 
                                 (current_tick - start_tick))/HZ);

        lcd_update();

        button = button_get_w_tmo(HZ);

        switch (button)
        {
            case BUTTON_PREV:
                if (option > rockbox)
                    option--;
                else
                    option = shutdown;
                break;

            case BUTTON_NEXT:
                if (option < shutdown)
                    option++;
                else
                    option = rockbox;
                break;

            case BUTTON_PLAY:
            case (BUTTON_PLAY|BUTTON_REC):
                reset_screen();

                switch (option)
                {
                    case rockbox:
                        rc = storage_init();
                        if(rc)
                        {
                            printf("ATA error: %d", rc);
                            sleep(HZ*5);
                            __shutdown();
                        }

                        disk_init();

                        rc = disk_mount_all();
                        if (rc<=0)
                        {
                            printf("No partition found");
                            sleep(HZ*5);
                            __shutdown();
                        }

                        printf("Loading firmware");
                        rc = load_firmware((unsigned char *)DRAM_START, 
                                           BOOTFILE, MAX_LOADSIZE);
                        printf("Result: %s", strerror(rc));

                        if (rc < EOK)
                        {
                            printf("Error!");
                            printf("Can't load " BOOTFILE ": ");
                            printf(strerror(rc));
                            sleep(HZ*5);
                            __shutdown();
                        }
                        else
                        {
                            start_rockbox();
                        }

                        break;

                    case mpio_firmware:
                        start_mpio_firmware();
                        break;

                    default:
                        __shutdown();
                        break;
                }
        }
}
/* timeout */
__shutdown();
}

void main(void)
{
    /* messages */
    const char usb_connect_msg[] = "Bootloader USB mode";
    const char charging_msg[] = "Charging...";
    const char complete_msg[] = "Charging complete";
    const char hold_msg[] = "Hold switch on";
    const char shutdown_msg[] = "Shutting down...";

    /* helper variables for messages */
    bool blink_toggle = false;
    const char *msg;

    bool on_button = false;
    int button;

    /* We want to read the buttons as early as possible, before the user
       releases the ON button */

    or_l( ((1<<24)|(1<<4)), &GPIO1_FUNCTION); /* main Hold & Play */
    and_l( ~((1<<24)|(1<<4)), &GPIO1_ENABLE); /* HiZ */
    
    if (GPIO1_READ & (1<<24))
        on_button = true;

    power_init();

    system_init();
    kernel_init();

    set_cpu_frequency(CPUFREQ_NORMAL);
    coldfire_set_pllcr_audio_bits(DEFAULT_PLLCR_AUDIO_BITS);

    enable_irq();
    lcd_init();

    backlight_init();
    font_init();
    lcd_setfont(FONT_SYSFIXED);

    adc_init();
    button_init();
    usb_init();

    /* handle charging */
    if( _charger_inserted())
    {
        or_l((1<<15),&GPIO_OUT);

        cpu_idle_mode(true);

        while( _charger_inserted() && 
               usb_detect() != USB_INSERTED &&
               !on_button)
        {
            button = button_get_w_tmo(HZ);

            switch(button)
            {
            case BUTTON_ON:
                on_button = true;
                reset_screen();
                break;

            case BUTTON_NONE: /* Timeout */

                if(!_battery_full())
                {
                    /* To be replaced with a nice animation */
                    blink_toggle = !blink_toggle;
                    msg = charging_msg;
                }
                else
                {
                    blink_toggle = true;
                    msg = complete_msg;
                }

                reset_screen();
                if(blink_toggle)
                    lcd_putstring_centered(msg);

                check_battery();
                break;
            }

        }
        cpu_idle_mode(false);
    }

    /* handle USB in bootloader */
    if (usb_detect() == USB_INSERTED)
    {
        ide_power_enable(true);
        sleep(HZ/20);
        usb_enable(true);
        cpu_idle_mode(true);

        while (usb_detect() == USB_INSERTED)
        {
            line = 0;

            reset_screen();

            if(blink_toggle)
            {
                 lcd_putstring_centered(usb_connect_msg);
            }

            check_battery();
            blink_toggle = !blink_toggle;

            storage_spin(); /* Prevent the drive from spinning down */
            sleep(HZ);
        }

        cpu_idle_mode(false);
        usb_enable(false);

        sleep(HZ);
        reset_screen();
        lcd_update();
    }

    /* handle ON button press */
    if (on_button)
    {
        if (button_hold() &&
            !_charger_inserted() &&
            usb_detect() != USB_INSERTED)
        {
            lcd_putstring_centered(hold_msg);
            lcd_update();
            sleep(HZ*3);
            __shutdown();
        }
        
    }
    else
    {
        lcd_putstring_centered(shutdown_msg);
        lcd_update();
        sleep(HZ*3);
        __shutdown();
    }

    
    bootmenu();
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */
void screen_dump(void)
{
}
