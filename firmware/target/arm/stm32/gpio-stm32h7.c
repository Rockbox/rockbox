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
#include "gpio-stm32h7.h"

void gpio_configure_single(int gpio, int confbits)
{
    int mode = GPIO_F_GETMODE(confbits);

    if (mode == GPIO_MODE_OUTPUT)
        gpio_set_level(gpio, GPIO_F_GETINITLEVEL(confbits));
    else if (mode == GPIO_MODE_FUNCTION)
        gpio_set_function(gpio, GPIO_F_GETAFNUM(confbits));

    gpio_set_pull(gpio, GPIO_F_GETPULL(confbits));
    gpio_set_type(gpio, GPIO_F_GETTYPE(confbits));
    gpio_set_speed(gpio, GPIO_F_GETSPEED(confbits));
    gpio_set_mode(gpio, mode);
}

void gpio_configure_port(int port, uint16_t mask, int confbits)
{
    for (int pin = 0; pin < 16; ++pin)
    {
        if (mask & BIT_N(pin))
        {
            int gpio = GPION_CREATE(port, pin);

            gpio_configure_single(gpio, confbits);
        }
    }
}

void gpio_configure_all(
    const struct gpio_setting *gpios, size_t ngpios,
    const struct pingroup_setting *pingroups, size_t ngroups)
{
    for (size_t i = 0; i < ngpios; ++i)
    {
        gpio_configure_single(gpios[i].gpio, gpios[i].func);
    }

    for (size_t i = 0; i < ngroups; ++i)
    {
        gpio_configure_port(pingroups[i].port,
                            pingroups[i].pins,
                            pingroups[i].func);
    }
}
