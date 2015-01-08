/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2011 Bertrik Sikken
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
#include "backlight-target.h"
#include "lcd.h"
#include "as3525v2.h"
#include "ascodec.h"
#include "lcd-target.h"

bool backlight_hw_init()
{
    return true;
}

void backlight_hw_on(void)
{
    /* GPIO B1 controls backlight */
    GPIOB_DIR |= (1 << 1);
    ascodec_write_pmu(AS3543_BACKLIGHT, 1, 0x90);
    GPIOB_PIN(1) = (1 << 1);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true);
#endif
}

void backlight_hw_off(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false);
#endif
    ascodec_write_pmu(AS3543_BACKLIGHT, 1, 0);
    GPIOB_PIN(1) = 0;
}

void backlight_hw_brightness(int brightness)
{
    oled_brightness(brightness);
}

