/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: backlight-nano2g.c 28601 2010-11-14 20:39:18Z theseven $
 *
 * Copyright (C) 2009 by Dave Chapman
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
#include <stdbool.h>

#include "config.h"
#include "kernel.h"
#include "backlight.h"
#include "backlight-target.h"
#include "pmu-target.h"

#ifdef HAVE_LCD_SLEEP
bool lcd_active(void);
void lcd_awake(void);
void lcd_update(void);
#endif

void backlight_hw_brightness(int brightness)
{
    pmu_write(0x28, brightness);
}

void backlight_hw_on(void)
{
#ifdef HAVE_LCD_SLEEP
    if (!lcd_active())
    {
        lcd_awake();
        lcd_update();
        sleep(HZ/20);
    }
#endif
    pmu_write(0x29, 1);
}

void backlight_hw_off(void)
{
    pmu_write(0x29, 0);
}

bool backlight_hw_init(void)
{
    /* LEDCTL: overvoltage protection enabled, OCP limit is 500mA */
    pmu_write(0x2a, 0x05);

    pmu_write(0x2b, 0x14);  /* T_dimstep = 16 * value / 32768 */
    backlight_hw_brightness(DEFAULT_BRIGHTNESS_SETTING);

    backlight_hw_on();

    return true;
}
