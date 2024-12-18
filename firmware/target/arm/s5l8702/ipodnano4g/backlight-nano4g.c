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
#include "lcd.h"
#include "lcd-s5l8702.h"
#endif


void backlight_hw_brightness(int brightness)
{
    pmu_write(D1759_REG_LEDOUT, brightness << 2);
}

void backlight_hw_on(void)
{
#ifdef HAVE_LCD_SLEEP
    if (!lcd_active())
        lcd_awake();
#endif
    pmu_write(D1759_REG_LEDCTL,
            (pmu_read(D1759_REG_LEDCTL) | D1759_LEDCTL_ENABLE));
}

void backlight_hw_off(void)
{
    pmu_write(D1759_REG_LEDCTL,
            (pmu_read(D1759_REG_LEDCTL) & ~D1759_LEDCTL_ENABLE));
}

bool backlight_hw_init(void)
{
    backlight_hw_brightness(DEFAULT_BRIGHTNESS_SETTING);
    backlight_hw_on();
    return true;
}

/* Kill the backlight, instantly. */
void backlight_hw_kill(void)
{
    backlight_hw_off();
}
