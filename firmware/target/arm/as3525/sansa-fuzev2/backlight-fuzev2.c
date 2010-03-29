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

void _backlight_set_brightness(int brightness)
{
    ascodec_write(AS3543_PMU_ENABLE, 8|2); // sub register
    ascodec_write(AS3543_BACKLIGHT, brightness * 10);
}

bool _backlight_init(void)
{
    GPIOB_DIR |= 1<<5; /* for buttonlight, stuff below seems to be needed
                          for buttonlight as well*/

    ascodec_write(AS3543_PMU_ENABLE, 8|1); // sub register
    ascodec_write(AS3543_BACKLIGHT, 0x80);

    ascodec_write(AS3543_PMU_ENABLE, 8|2); // sub register
    ascodec_write(AS3543_BACKLIGHT, backlight_brightness * 10);
    return true;
}

void _backlight_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
    ascodec_write(AS3543_PMU_ENABLE, 8|1); // sub register
    ascodec_write(AS3543_BACKLIGHT, 0x80);
}

void _backlight_off(void)
{
    ascodec_write(AS3543_PMU_ENABLE, 8|1); // sub register
    ascodec_write(AS3543_BACKLIGHT, 0);
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
