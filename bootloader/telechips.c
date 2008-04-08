/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Dave Chapman
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
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
#include <string.h>
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "button.h"
#include "adc.h"
#include "adc-target.h"
#include "backlight.h"
#include "backlight-target.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"

#if defined(COWON_D2)
#include "pcf50606.h"
#define LOAD_ADDRESS 0x20000000 /* DRAM_START */
#endif

char version[] = APPSVERSION;

extern int line;

#define MAX_LOAD_SIZE (8*1024*1024) /* Arbitrary, but plenty. */

void show_debug_screen(void)
{
    int button;
    int power_count = 0;
    int count = 0;
    bool do_power_off = false;

    while(!do_power_off) {
        line = 0;
        printf("Hello World!");

        button = button_read_device();

        /* Power-off if POWER button has been held for a long time
           This loop is currently running at about 100 iterations/second
         */
        if (button & POWEROFF_BUTTON) {
            power_count++;
            if (power_count > 200)
               do_power_off = true;
        } else {
            power_count = 0;
        }

        printf("Btn: 0x%08x",button);

#if defined(COWON_D2)
        int i;

        printf("GPIOA: 0x%08x",GPIOA);
        printf("GPIOB: 0x%08x",GPIOB);
        printf("GPIOC: 0x%08x",GPIOC);
        printf("GPIOD: 0x%08x",GPIOD);
        printf("GPIOE: 0x%08x",GPIOE);

        for (i = 0; i<4; i++)
        {
            printf("ADC%d: 0x%04x",i,adc_read(i));
        }

        /* TODO: Move this stuff out to a touchscreen driver and establish
           how such a beast is going to work. Since it needs I2C read/write,
           it can't easily go on an interrupt-based tick task. */
        {
            int x,y;
            unsigned char buf[5];
            
            pcf50606_write(PCF5060X_ADCC2, (0xE<<1) | 1); /* ADC start X+Y */
            pcf50606_read_multiple(PCF5060X_ADCC1, buf, 5);
            x = (buf[2] << 2) | (buf[3] & 3);
            y = (buf[4] << 2) | ((buf[3] & 0xC) >> 2);
            printf("X: 0x%03x Y: 0x%03x",x,y);

            x = (x*LCD_WIDTH) / 1024;
            y = (y*LCD_HEIGHT) / 1024;
            lcd_hline(x-5, x+5, y);
            lcd_vline(x, y-5, y+5);

            pcf50606_write(PCF5060X_ADCC2, (0xF<<1) | 1); /* ADC start P1+P2 */
            pcf50606_read_multiple(PCF5060X_ADCC1, buf, 5);
            x = (buf[2] << 2) | (buf[3] & 3);
            y = (buf[4] << 2) | ((buf[3] & 0xC) >> 2);
            printf("P1: 0x%03x P2: 0x%03x",x,y);
        }
#endif

        count++;
        printf("Count: %d",count);
    }

    lcd_clear_display();
    line = 0;
    printf("POWER-OFF");

    /* Power-off */
    power_off();

    printf("(NOT) POWERED OFF");
    while (true);
}


void* main(void)
{
#if defined(COWON_D2) && defined(TCCBOOT)
    int rc;
    unsigned char* loadbuffer = (unsigned char*)LOAD_ADDRESS;
#endif

    power_init();
    system_init();
    lcd_init();

    adc_init();
    button_init();
    backlight_init();

    font_init();
    lcd_setfont(FONT_SYSFIXED);

    _backlight_on();

/* Only load the firmware if TCCBOOT is defined - this ensures SDRAM_START is
   available for loading the firmware. Otherwise display the debug screen. */
#if defined(COWON_D2) && defined(TCCBOOT)
    printf("Rockbox boot loader");
    printf("Version %s", version);

    printf("ATA");
    rc = ata_init();
    if(rc)
    {
        reset_screen();
        error(EATA, rc);
    }

    printf("mount");
    rc = disk_mount_all();
    if (rc<=0)
    {
        error(EDISK,rc);
    }

    rc = load_firmware(loadbuffer, BOOTFILE, MAX_LOAD_SIZE);

    if (rc < 0)
    {
        error(EBOOTFILE,rc);
    }
    else if (rc == EOK)
    {
        int(*kernel_entry)(void);

        /* wait for hold release to allow debug statements to be read */
        while (button_hold()) {};
    
        kernel_entry = (void*) loadbuffer;
        
        /* allow entry to the debug screen if still holding power */
        if (!(button_read_device() & POWEROFF_BUTTON)) rc = kernel_entry();
    }
#endif
    show_debug_screen();

    return 0;
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */
void usb_acknowledge(void)
{
}

void usb_wait_for_disconnect(void)
{
}
