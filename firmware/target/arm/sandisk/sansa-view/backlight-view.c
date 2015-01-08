/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Robert Keevil
 * Copyright (C) 2014 by Szymon Dziok
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

bool backlight_hw_init(void)
{
    GPIO_SET_BITWISE(GPIOD_ENABLE, 0x01);
    GPIO_SET_BITWISE(GPIOD_OUTPUT_EN, 0x01);
    GPIO_SET_BITWISE(GPIOA_ENABLE, 0x01);
    GPIO_SET_BITWISE(GPIOA_OUTPUT_EN, 0x01);
    GPIO_SET_BITWISE(GPIOA_ENABLE, 0x02);
    GPIO_SET_BITWISE(GPIOA_OUTPUT_EN, 0x02);
    GPIO_SET_BITWISE(GPIOR_ENABLE, 0x10);
    GPIO_SET_BITWISE(GPIOR_ENABLE, 0x20);
    GPIO_SET_BITWISE(GPIOR_ENABLE, 0x40);
    GPIO_SET_BITWISE(GPIOR_ENABLE, 0x80);
    GPIO_SET_BITWISE(GPIOA_OUTPUT_EN, 0x10);
    GPIO_SET_BITWISE(GPIOA_OUTPUT_EN, 0x20);
    GPIO_SET_BITWISE(GPIOA_OUTPUT_EN, 0x40);
    GPIO_SET_BITWISE(GPIOA_OUTPUT_EN, 0x80);
    return true;
}

void backlight_hw_brightness(int brightness)
{
    (void)brightness;
}

void backlight_hw_off(void)
{
    GPIO_CLEAR_BITWISE(GPIOD_OUTPUT_VAL, 0x01);
}

void backlight_hw_on(void)
{
    GPIO_SET_BITWISE(GPIOD_OUTPUT_VAL, 0x01);
}

void buttonlight_hw_off(void)
{
    GPIO_CLEAR_BITWISE(GPIOA_OUTPUT_VAL, 0x02); /* vertical buttonlight */
    GPIO_CLEAR_BITWISE(GPIOA_OUTPUT_VAL, 0x01); /* horizontal buttonlight */

    GPIO_CLEAR_BITWISE(GPIOR_OUTPUT_VAL, 0x80); /* scrollwheel bottom led */
    GPIO_CLEAR_BITWISE(GPIOR_OUTPUT_VAL, 0x40); /* scrollwheel right led */
    GPIO_CLEAR_BITWISE(GPIOR_OUTPUT_VAL, 0x20); /* scrollwheel top led */
    GPIO_CLEAR_BITWISE(GPIOR_OUTPUT_VAL, 0x10); /* scrollwheel left led */
}

void buttonlight_hw_on(void)
{
    GPIO_SET_BITWISE(GPIOA_OUTPUT_VAL, 0x02); /* vertical buttonlight */
    GPIO_SET_BITWISE(GPIOA_OUTPUT_VAL, 0x01); /* horizontal buttonlight */

    GPIO_SET_BITWISE(GPIOR_OUTPUT_VAL, 0x80); /* scrollwheel bottom led */
    GPIO_SET_BITWISE(GPIOR_OUTPUT_VAL, 0x40); /* scrollwheel right led */
    GPIO_SET_BITWISE(GPIOR_OUTPUT_VAL, 0x20); /* scrollwheel top led */
    GPIO_SET_BITWISE(GPIOR_OUTPUT_VAL, 0x10); /* scrollwheel left led */
}
