/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Martitz
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
#include "backlight-target.h"
#include "system.h"
#include "lcd.h"
#include "backlight.h"
#include "ascodec-target.h"
#include "as3514.h"

int buttonlight_is_on = 0;

static int brightness_internal = 0;

/* not functional */
void _backlight_set_brightness(int brightness)
{
    //ascodec_write(AS3514_DCDC15, brightness);
    brightness_internal = brightness << 2;
    brightness_internal += brightness + 5;
    brightness_internal <<= 25;
    brightness_internal >>= 24;
    ascodec_write(0x1c, 2); // sub register
    ascodec_write(0x1b, brightness_internal|0xff);
}

bool _backlight_init(void)
{
    GPIOB_DIR |= 1<<5; /* for buttonlight, stuff below seems to be needed
                          for buttonlight as well*/
    ascodec_write(0x1c, 1); // sub register
    ascodec_write(0x1b, ascodec_read(0x1b)|0x80);
    return true;
}

/* not functional */
void _backlight_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
#if (CONFIG_BACKLIGHT_FADING != BACKLIGHT_FADING_SW_SETTING) /* in bootloader/sim */
    /* if we set the brightness to the settings value, then fading up
     * is glitchy */
    ascodec_write(0x1c, 2); // sub register
    ascodec_write(0x1b, brightness_internal);
#endif
}

/* not functional */
void _backlight_off(void)
{
    ascodec_write(0x1c, 1); // sub register
    ascodec_write(0x1b, ascodec_read(0x1b) & ~0x80);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off visible display */
#endif
}

void _buttonlight_on(void)
{
    GPIOB_PIN(5) = (1<<5);
    buttonlight_is_on = 1;
}

void _buttonlight_off(void)
{
    GPIOB_PIN(5) = 0;
    buttonlight_is_on = 0;
}
