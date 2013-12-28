/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Module wrapper for GPIO, using kernel module of Samsung YP-R0/YP-R1
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
#include <gpio-ypr.h> /* includes common ioctl device definitions */
#include <sys/ioctl.h>

static int gpio_dev = 0;

void gpio_init(void)
{
    gpio_dev = open(GPIO_DEVICE, O_RDONLY);
    if (gpio_dev < 0)
        printf("GPIO device open error!");
}

void gpio_close(void)
{
    if (gpio_dev >= 0)
        close(gpio_dev);
}

int gpio_control(int request, int num, int mode, int val)
{
    struct gpio_info r = { .num = num, .mode = mode, .val = val, };
    return ioctl(gpio_dev, request, &r);
}
