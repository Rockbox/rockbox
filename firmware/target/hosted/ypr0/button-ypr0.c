/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: button-sdl.c 30482 2011-09-08 14:53:28Z kugel $
 *
 * Copyright (C) 2011 Lorenzo Miori
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
#include <stdlib.h>         /* EXIT_SUCCESS */
#include "config.h"
#include "button.h"
#include "kernel.h"
#include "system.h"
#include "button-target.h"
#include <gpio_ypr0.h> /* For headphones sense */

/* R0 physical key codes */
enum ypr0_buttons {
    R0BTN_NONE = BUTTON_NONE,
    R0BTN_POWER = 1,
    R0BTN_UP,
    R0BTN_DOWN,
    R0BTN_RIGHT,
    R0BTN_LEFT,
    R0BTN_CENTRAL,
    R0BTN_MENU,
    R0BTN_BACK,
    R0BTN_3DOTS = 11,
};


static int r0_btn_fd = 0;

/* Samsung keypad driver doesn't allow multiple key combinations :( */
static enum ypr0_buttons r0_read_key(void)
{
    unsigned char keys;

    if (r0_btn_fd < 0)
        return 0;

    if (read(r0_btn_fd, &keys, 1))
        return keys;

    return 0;
}

/* Conversion from physical keypress code to logic key code */
static int key_to_button(enum ypr0_buttons keyboard_button)
{
    switch (keyboard_button)
    {
        default:            return BUTTON_NONE;
        case R0BTN_POWER:   return BUTTON_POWER;
        case R0BTN_UP:      return BUTTON_UP;
        case R0BTN_DOWN:    return BUTTON_DOWN;
        case R0BTN_RIGHT:   return BUTTON_RIGHT;
        case R0BTN_LEFT:    return BUTTON_LEFT;
        case R0BTN_CENTRAL: return BUTTON_SELECT;
        case R0BTN_MENU:    return BUTTON_MENU;
        case R0BTN_BACK:    return BUTTON_BACK;
        case R0BTN_3DOTS:   return BUTTON_USER;
    }
}

int button_read_device(void)
{
    return key_to_button(r0_read_key());
}

bool headphones_inserted(void)
{
    /* GPIO low - 0 - means headphones inserted */
    return !gpio_control(DEV_CTRL_GPIO_IS_HIGH, GPIO_HEADPHONE_SENSE, 0, 0);
}

/* Open the keypad device: it is offered by r0Btn.ko module */
void button_init_device(void)
{
    r0_btn_fd = open("/dev/r0Btn", O_RDONLY);
    if (r0_btn_fd < 0)
        printf("/dev/r0Btn open error!");

    /* Setup GPIO pin for headphone sense, copied from OF */
    gpio_control(DEV_CTRL_GPIO_SET_MUX, GPIO_HEADPHONE_SENSE, CONFIG_SION, PAD_CTL_47K_PU);
    gpio_control(DEV_CTRL_GPIO_SET_INPUT, GPIO_HEADPHONE_SENSE, CONFIG_SION, PAD_CTL_47K_PU);
}

#ifdef BUTTON_DRIVER_CLOSE
/* I'm not sure it's called at shutdown...give a check! */
void button_close_device(void)
{
    if (r0_btn_fd >= 0) {
        close(r0_btn_fd);
        printf("/dev/r0Btn closed!");
    }
    /* Don't know the precise meaning, but it's done as in the OF, so copied there */
    gpio_control(DEV_CTRL_GPIO_UNSET_MUX, GPIO_HEADPHONE_SENSE, CONFIG_SION, 0);
}
#endif /* BUTTON_DRIVER_CLOSE */
