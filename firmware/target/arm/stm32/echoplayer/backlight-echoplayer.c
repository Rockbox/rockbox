/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#include "gpio-stm32h7.h"

bool backlight_hw_init(void)
{
    /* FIXME: override PWM config for now, rev1 has broken backlight */
    gpio_configure_single(GPIO_BACKLIGHT,
                          GPIOF_OUTPUT(1, GPIO_TYPE_PUSH_PULL,
                                       GPIO_SPEED_LOW, GPIO_PULL_DISABLED));
    return true;
}

void backlight_hw_on(void)
{
    gpio_set_level(GPIO_BACKLIGHT, 1);
}

void backlight_hw_off(void)
{
    gpio_set_level(GPIO_BACKLIGHT, 0);
}

void backlight_hw_brightness(int brightness)
{
    (void)brightness;
}
