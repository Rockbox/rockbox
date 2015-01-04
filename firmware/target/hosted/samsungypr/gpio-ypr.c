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
 * Copyright (c) 2011-2015 Lorenzo Miori
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
#include <sys/ioctl.h>

#include "panic.h"
#include "gpio-ypr.h" /* includes common ioctl device definitions */

static int gpio_dev = 0;

#ifdef GPIO_DEBUG
// 3 banks of 32 pins
static const char *pin_use[3][32];

void gpio_acquire(unsigned bank, unsigned pin, const char *name)
{
    if(pin_use[bank][pin] != NULL && pin_use[bank][pin] != name)
        panicf("acquire B%dP%02d for %s, was %s!", bank, pin, name, pin_use[bank][pin]);
    pin_use[bank][pin] = name;
}

void gpio_release(unsigned bank, unsigned pin, const char *name)
{
    if(pin_use[bank][pin] != NULL && pin_use[bank][pin] != name)
        panicf("release B%dP%02d for %s: was %s!", bank, pin, name, pin_use[bank][pin]);
    pin_use[bank][pin] = NULL;
}

const char *gpio_blame(unsigned bank, unsigned pin)
{
    return pin_use[bank][pin];
}
#endif

void gpio_init(void)
{
    gpio_dev = open(GPIO_DEVICE, O_RDONLY);
    if (gpio_dev < 0)
        panicf("GPIO device open error!");
}

void gpio_close(void)
{
    if (gpio_dev >= 0)
        close(gpio_dev);
}

static int gpio_control(int request, int num, int mode, int val)
{
    struct gpio_info r = { .num = num, .mode = mode, .val = val, };
    return ioctl(gpio_dev, request, &r);
}

void gpio_set_iomux(int pin, int iomux)
{
    if (gpio_control(DEV_CTRL_GPIO_SET_MUX, pin, iomux, 0) != 0)
    {
        panicf("Unable to set iomux to pin %d", pin);
    }
}

void gpio_free_iomux(int pin, int iomux)
{
    if (gpio_control(DEV_CTRL_GPIO_UNSET_MUX, pin, iomux, 0) != 0)
    {
        panicf("Unable to free iomux to pin %d", pin);
    }
}

void gpio_set_pad(int pin, int type)
{
    if (gpio_control(DEV_CTRL_GPIO_SET_TYPE, pin, type, 0) != 0)
    {
        panicf("Unable to set pad to pin %d", pin);
    }
}

void gpio_direction_output(int pin)
{
    if (gpio_control(DEV_CTRL_GPIO_SET_OUTPUT, pin, 0, 0) != 0)
    {
        panicf("Unable to set output direction to pin %d", pin);
    }
}

void gpio_direction_input(int pin)
{
    if (gpio_control(DEV_CTRL_GPIO_SET_INPUT, pin, 0, 0) != 0)
    {
        panicf("Unable to set input direction to pin %d", pin);
    }
}

bool gpio_get(int pin)
{
    return gpio_control(DEV_CTRL_GPIO_GET_VAL, pin, 0, 0);
}

void gpio_set(int pin, bool value)
{
    int ret = -1;

    if (value)
        ret = gpio_control(DEV_CTRL_GPIO_SET_HIGH, pin, 0, 0);
    else
        ret = gpio_control(DEV_CTRL_GPIO_SET_LOW, pin, 0, 0);

    if (ret != 0)
    {
        panicf("Unable to set input direction to pin %d", pin);
    }
}
