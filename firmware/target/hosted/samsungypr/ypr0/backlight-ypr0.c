/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2011 by Lorenzo Miori
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
#include "backlight.h"
#include "backlight-target.h"
#include "lcd.h"
#include "as3514.h"
#include "ascodec.h"
#include <fcntl.h>
#include "unistd.h"

static bool backlight_on_status = true; /* Is on or off? */

bool backlight_hw_init(void)
{
    /* We have nothing to do */
    return true;
}

void backlight_hw_on(void)
{
    if (!backlight_on_status)
    {
        /* Turn on lcd power before backlight */
#ifdef HAVE_LCD_ENABLE
        lcd_enable(true);
#endif
        /* Original app sets this to 0xb1 when backlight is on... */
        ascodec_write_pmu(AS3543_BACKLIGHT, 0x1, 0xb1);
    }

    backlight_on_status = true;

}

void backlight_hw_off(void)
{
    if (backlight_on_status)
    {
        /* Disabling the DCDC15 completely, keeps brightness register value */
        ascodec_write_pmu(AS3543_BACKLIGHT, 0x1, 0x00);
        /* Turn off lcd power then */
#ifdef HAVE_LCD_ENABLE
        lcd_enable(false);
#endif
    }

    backlight_on_status = false;
}

void backlight_hw_brightness(int brightness)
{
    /* Just another check... */
    if (brightness > MAX_BRIGHTNESS_SETTING)
        brightness = MAX_BRIGHTNESS_SETTING;
    if (brightness < MIN_BRIGHTNESS_SETTING)
        brightness = MIN_BRIGHTNESS_SETTING;
    ascodec_write_pmu(AS3543_BACKLIGHT, 0x3, brightness << 3 & 0xf8);
}
