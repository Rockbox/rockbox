/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 by Will Robertson
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
#include "system.h"
#include "lcd.h"
#include "lcd-target.h"
#include "gpio-jz4760b.h"

/* the OF bootloader initializes the LCD for us so we do not generalise run a full setup sequence
 * but we keep the code there for reference or in case it is needed */
#define FULL_INIT

bool lcd_on;

/* LCD init */
void INIT_ATTR lcd_init_device(void)
{
#ifdef FULL_INIT
    /* PC[27:22,19:12,9:2] => lcd_{b2-b7, pclk, de, g2-g7, vsync, hsync, r2-r7}, all use function 0
     * note that in SLCD mode, we have the following mapping:
     * - slcd_dat0-17 => lcd_{b2-b7,g2-g7,r2-r7}
     * - slcd_clk => lcd_pclk
     * - slcd_cs => lcd_vsync
     * - slcd_rs => lcd_hsync
     * not sure why lcd_de is setup... */
    jz_gpio_set_function_mask(2, 0xfcff3fc, PIN_FUN(0));
    jz_gpio_enable_pullup_mask(2, 0xfcff3fc, false);

    /* PE4 is reset */
    jz_gpio_setup_std_out(4, 4, true);
    mdelay(50);
    jz_gpio_set_output(4, 4, false);
    mdelay(50);
    jz_gpio_set_output(4, 4, true);
    mdelay(150);

    /* PC9 and PC2, unure what they do */
    jz_gpio_setup_std_out(3, 9, false);
    jz_gpio_setup_std_out(3, 2, true);
#endif
}

bool lcd_active(void)
{
    return lcd_on;
}

void lcd_enable(bool state)
{
    if(state == lcd_on)
        return;

    if(state)
    {
        lcd_on = true;
        lcd_update();
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
    else
    {
        lcd_on = false;
    }
}
