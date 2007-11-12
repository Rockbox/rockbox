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

char version[] = APPSVERSION;

extern int line;

void* main(void)
{
    int button;
    int power_count = 0;
    int count = 0;
    bool do_power_off = false;

    system_init();
    adc_init();
    lcd_init();
    font_init();

    _backlight_on();

    while(!do_power_off) {
        line = 0;
        printf("Hello World!");

        button = button_read_device();

        /* Power-off if POWER button has been held for a long time
           This loop is currently running at about 100 iterations/second
         */
        if (button & BUTTON_POWERPLAY) {
            power_count++;
            if (power_count > 200)
               do_power_off = true;
        } else {
            power_count = 0;
        }

        printf("Btn: 0x%08x",button);

        count++;
        printf("Count: %d",count);
    }

    lcd_clear_display();
    line = 0;
    printf("POWER-OFF");

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
