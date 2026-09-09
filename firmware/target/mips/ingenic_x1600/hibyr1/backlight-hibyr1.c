/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#include "backlight.h"
#include "backlight-target.h"
#include "lcd.h"
#include "gpio-x1600.h"

/* On/off backlight; PWM brightness control is not implemented. */
#define BL_GPIO GPIO_PC(0)

bool backlight_hw_init(void)
{

    /* Take the pad away from the PWM and drive it high */
    gpio_set_function(BL_GPIO, GPIOF_OUTPUT(1));
    return true;
}

void backlight_hw_on(void)
{
    gpio_set_function(BL_GPIO, GPIOF_OUTPUT(1));
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true);
#endif
}

void backlight_hw_off(void)
{
    gpio_set_function(BL_GPIO, GPIOF_OUTPUT(0));
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false);
#endif
}
