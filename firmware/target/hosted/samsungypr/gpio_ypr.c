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
/* debug screen */
#include "button.h"
#include "font.h"
#include "action.h"
#include "list.h"

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

static int gpiolist_get_color(int selected_item, void * data) {
    (void)data;
    if (selected_item <= GPIO1_31) {
        return LCD_RGBPACK(0xff, 0, 0);
    }
    else {
        if (selected_item <= GPIO2_31) {
            return LCD_RGBPACK(0, 0xff, 0);
        }
        else {
            if (selected_item <= GPIO3_31) {
                return LCD_RGBPACK(0, 0, 0xff);
            }
            else {
                return -1;
            }
        }
    }
}

static const char* gpiolist_get_value(int selected_item, void* data,
                                    char* buffer, size_t buffer_len)
{
    (void)data;
    snprintf(buffer, buffer_len, "    Bank %d Pin %d: %d",
             selected_item / (GPIO1_31+1) + 1, selected_item % (GPIO1_31+1),
             gpio_control(DEV_CTRL_GPIO_GET_VAL, selected_item, 0, 0));
    return buffer;
}

bool gpio_debug_screen(void)
{
    struct gui_synclist lists;
    int action;
    gui_synclist_init(&lists, gpiolist_get_value, NULL, false, 1, NULL);
    gui_synclist_set_title(&lists, "GPIO ports", NOICON);
    gui_synclist_set_icon_callback(&lists, NULL);
    gui_synclist_set_color_callback(&lists, gpiolist_get_color);
    gui_synclist_set_nb_items(&lists, GPIO3_31+1);

    while(1)
    {
        global_status.runtime += ((current_tick - lasttime) / HZ);

        lasttime = current_tick;
        gui_synclist_draw(&lists);
        list_do_action(CONTEXT_STD, HZ / 10, &lists, &action, LIST_WRAP_UNLESS_HELD);
        if(action == ACTION_STD_CANCEL)
            return false;
        if(action == ACTION_STD_OK) {
            return true;
        }
        yield();
    }

    return true;
}
