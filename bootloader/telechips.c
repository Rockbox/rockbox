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
#include "backlight-target.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"

#if defined(COWON_D2)
#include "i2c.h"
#endif

char version[] = APPSVERSION;

extern int line;

void* main(void)
{
    int button;
    int power_count = 0;
    int count = 0;
    int i;
    bool do_power_off = false;

    system_init();
    adc_init();
    lcd_init();
    font_init();

#if defined(COWON_D2)
    lcd_enable(true);
#endif

    _backlight_on();

#if defined(COWON_D2)
    ata_init();
#endif

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
        printf("GPIOA: 0x%08x",GPIOA);
        printf("GPIOB: 0x%08x",GPIOB);
        printf("GPIOC: 0x%08x",GPIOC);
        printf("GPIOD: 0x%08x",GPIOD);
        printf("GPIOE: 0x%08x",GPIOE);
        
        for (i = 0; i<4; i++)
        {
            printf("ADC%d: 0x%04x",i,adc_read(i));
        }

        /* TODO: Establish how the touchscreen driver is going to work. 
           Since it needs I2C read/write, it can't easily go on a tick task */
        {
            unsigned char buf[] = { 0x2f, (0xE<<1) | 1, /* ADC start for X+Y */
                                    0, 0, 0 };
            int x,y;
            i2c_write(0x10, buf, 2);
            i2c_readmem(0x10, 0x2e, buf, 5);
            x = (buf[2] << 2) | (buf[3] & 3);
            y = (buf[4] << 2) | ((buf[3] & 0xC) >> 2);
            printf("X: 0x%03x Y: 0x%03x",x,y);

            buf[0] = 0x2f;
            buf[1] = (0xF<<1) | 1;   /* ADC start for P1+P2 */
            i2c_write(0x10, buf, 2);
            i2c_readmem(0x10, 0x2e, buf, 5);
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

#if defined(COWON_D2)
    lcd_enable(false);
#endif

    /* TODO: Power-off */
    while(1);

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
