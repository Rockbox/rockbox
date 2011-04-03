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

void _backlight_set_brightness(int brightness)
{
    ascodec_write_pmu(AS3543_BACKLIGHT, 2, brightness * 10);
}

bool _backlight_init(void)
{
    ascodec_write_pmu(AS3543_BACKLIGHT, 1, 0x80);
    ascodec_write_pmu(AS3543_BACKLIGHT, 2, backlight_brightness * 10);

    /* needed for button light */
    if (amsv2_variant == 0)
    {
        GPIOB_DIR |= 1<<5;
    }
    else
    {
        ascodec_write_pmu(0x1a, 1, 0x30);   /* MUX_PWGD = PWM */
    }

    return true;
}

void _backlight_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
    ascodec_write_pmu(AS3543_BACKLIGHT, 1, 0x80);
}

void _backlight_off(void)
{
    ascodec_write_pmu(AS3543_BACKLIGHT, 1, 0x0);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off visible display */
#endif
}

void _buttonlight_on(void)
{
    if (amsv2_variant == 0)
    {
        GPIOB_PIN(5) = (1<<5);
    }
    else
    {
        ascodec_write_pmu(0x1a, 6, 0x80);   /* PWM inverted */
    }
}

void _buttonlight_off(void)
{
    if (amsv2_variant == 0)
    {
        GPIOB_PIN(5) = 0;
    }
    else
    {
        ascodec_write_pmu(0x1a, 6, 0);      /* PWM not inverted */
    }
}
