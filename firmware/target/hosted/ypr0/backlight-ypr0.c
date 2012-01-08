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

/*TODO: see if LCD sleep could be implemented in a better way -> ie using a rockbox feature */
/* Turn off LCD power supply */
static void _backlight_lcd_sleep(void)
{
    int fp = open("/sys/class/graphics/fb0/blank", O_RDWR);
    write(fp, "1", 1);
    close(fp);
}
/* Turn on LCD screen */
static void _backlight_lcd_power(void)
{
    int fp = open("/sys/class/graphics/fb0/blank", O_RDWR);
    write(fp, "0", 1);
    close(fp);
}

bool _backlight_init(void)
{
    /* We have nothing to do */
    return true;
}

void _backlight_on(void)
{
    if (!backlight_on_status)
    {
        /* Turn on lcd power before backlight */
        _backlight_lcd_power();
        /* Original app sets this to 0xb1 when backlight is on... */
        ascodec_write_pmu(AS3543_BACKLIGHT, 0x1, 0xb1);
    }

    backlight_on_status = true;

}

void _backlight_off(void)
{
    if (backlight_on_status) {
        /* Disabling the DCDC15 completely, keeps brightness register value */
        ascodec_write_pmu(AS3543_BACKLIGHT, 0x1, 0x00);
        /* Turn off lcd power then */
        _backlight_lcd_sleep();
    }

    backlight_on_status = false;
}

void _backlight_set_brightness(int brightness)
{
    /* Just another check... */
    if (brightness > MAX_BRIGHTNESS_SETTING)
        brightness = MAX_BRIGHTNESS_SETTING;
    if (brightness < MIN_BRIGHTNESS_SETTING)
        brightness = MIN_BRIGHTNESS_SETTING;
    ascodec_write_pmu(AS3543_BACKLIGHT, 0x3, brightness << 3 & 0xf8);
}
