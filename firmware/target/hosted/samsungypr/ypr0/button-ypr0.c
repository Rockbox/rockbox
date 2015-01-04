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

#include "config.h"
#include "button.h"
#include "kernel.h"
#include "system.h"
#include "button-target.h"
#include "gpio-ypr.h" /* For headphones sense and buttons */

int button_read_device(void)
{
    int key = BUTTON_NONE;

    /* Check for all the keys */
    if (!gpio_get(GPIO_USER_KEY)) {
        key |= BUTTON_USER;
    }
    if (!gpio_get(GPIO_CENTRAL_KEY)) {
        key |= BUTTON_SELECT;
    }
    if (!gpio_get(GPIO_UP_KEY)) {
        key |= BUTTON_UP;
    }
    if (!gpio_get(GPIO_DOWN_KEY)) {
        key |= BUTTON_DOWN;
    }
    if (!gpio_get(GPIO_LEFT_KEY)) {
        key |= BUTTON_LEFT;
    }
    if (!gpio_get(GPIO_RIGHT_KEY)) {
        key |= BUTTON_RIGHT;
    }
    if (!gpio_get(GPIO_MENU_KEY)) {
        key |= BUTTON_MENU;
    }
    if (!gpio_get(GPIO_BACK_KEY)) {
        key |= BUTTON_BACK;
    }
    if (gpio_get(GPIO_POWER_KEY)) {
        key |= BUTTON_POWER;
    }

    return key;
}

bool headphones_inserted(void)
{
    /* GPIO low - 0 - means headphones inserted */
    return !gpio_get(GPIO_HEADPHONE_SENSE);
}

void button_init_device(void)
{
    /* Setup GPIO pin for headphone sense, copied from OF */
    gpio_set_iomux(GPIO_HEADPHONE_SENSE, CONFIG_GPIO);
    gpio_set_pad(GPIO_HEADPHONE_SENSE, PAD_CTL_47K_PU);
    gpio_direction_input(GPIO_HEADPHONE_SENSE);
    
    /* No need to initialize any GPIO pin, since this is done loading the r0Btn module */
}

#ifdef BUTTON_DRIVER_CLOSE
/* I'm not sure it's called at shutdown...give a check! */
void button_close_device(void)
{
    /* Don't know the precise meaning, but it's done as in the OF, so copied there */
    gpio_free_iomux(GPIO_HEADPHONE_SENSE, CONFIG_GPIO);
}
#endif /* BUTTON_DRIVER_CLOSE */
