/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Module wrapper for GPIO, using /dev/r0GPIO (r0Gpio.ko) of Samsung YP-R0
 *
 * Copyright (c) 2011 Lorenzo Miori
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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <gpio-target.h> /* includes common ioctl device definitions */
#include <sys/ioctl.h>

static int r0_gpio_dev = 0;

void gpio_init(void)
{
    r0_gpio_dev = open("/dev/r0GPIO", O_RDONLY);
    if (r0_gpio_dev < 0)
        printf("/dev/r0GPIO open error!");
}

void gpio_close(void)
{
    if (r0_gpio_dev >= 0)
        close(r0_gpio_dev);
}

int gpio_control(int request, int num, int mode, int val)
{
    struct gpio_info r = { .num = num, .mode = mode, .val = val, };
    return ioctl(r0_gpio_dev, request, &r);
}
