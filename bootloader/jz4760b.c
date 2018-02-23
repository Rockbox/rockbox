/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#include "system.h"
#include "backlight.h"
#include "lcd.h"
#include "gpio-jz4760b.h"
#include "button.h"
#include "system-fiiox1.h"

void main(void) NORETURN_ATTR;
void main(void)
{
    system_init();
    backlight_init();
    button_init_device();
#if 1
    fiiox1_enable_blue_led(true);
    lcd_init();
    while(1)
    {
        lcd_clear_display();
        int line = 0;
        lcd_puts(0, line++, "Hello world");
        lcd_putsf(0, line++, "Hardware V%d", fiiox1_get_hw_version());
        lcd_putsf(0, line++, "Backlight type: %d", fiiox1_get_backlight_type());
        for(int i = 0; i < 6; i++)
            lcd_putsf(0, line++, "P%c=%08x", 'A' + i, jz_gpio_get_input_mask(i, 0xffffffff));
        int btn = button_read_device();
        char buf[64];
        buf[0] = 0;
        if(btn & BUTTON_POWER) strcat(buf, " power");
        if(btn & BUTTON_LEFT) strcat(buf, " left");
        if(btn & BUTTON_RIGHT) strcat(buf, " right");
        if(btn & BUTTON_MENU) strcat(buf, " menu");
        if(btn & BUTTON_BACK) strcat(buf, " back");
        if(btn & BUTTON_VOL_DOWN) strcat(buf, " vol-");
        if(btn & BUTTON_VOL_UP) strcat(buf, " vol+");
        if(btn & BUTTON_SELECT) strcat(buf, " select");
        lcd_putsf(0, line++, "Buttons:%s", buf);
        //lcd_putsf(0, line++, "Charging: %d", !jz_gpio_get_input(1, 13));
        lcd_putsf(0, line++, "USB: %d", jz_gpio_get_input(5, 10));
        lcd_update();
    }
#endif
#if 0
    for(int i = 0; i <= 16; i++)
    {
        backlight_hw_brightness(i);
        mdelay(500);
    }
#endif
    /* never returns */
    while(1) {}
}
