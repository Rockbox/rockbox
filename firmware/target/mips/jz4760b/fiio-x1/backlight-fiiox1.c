/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#include "gpio-jz4760b.h"

/* there are two versions of the backlight hardware, they use different pins */
static bool backlight_type;

bool backlight_hw_init(void)
{
    /* strangely the OF seems to setup the pin as output to read it... */
    jz_gpio_set_function(0, 29, PIN_GPIO_IN);
    jz_gpio_enable_pullup(0, 29, false);
    jz_gpio_set_output(0, 29, true);
    backlight_type = jz_gpio_get_input(0, 29);
    jz_gpio_set_output(0, 29, false);
    return true;
}

void backlight_hw_on(void)
{
}

void backlight_hw_off(void)
{
}

void backlight_hw_brightness(int brightness)
{
}
