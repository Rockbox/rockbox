/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: button-sdl.c 30482 2011-09-08 14:53:28Z kugel $
 *
 * Copyright (C) 2013 Lorenzo Miori
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
#include "mcs5000.h" /* Touchscreen controller */

enum {
    STATE_UNKNOWN,
    STATE_UP,
    STATE_DOWN,
};

static int last_x = 0;
static int last_y = 0;
static int last_touch_state = STATE_UNKNOWN;

int button_read_device(int *data)
{
    int key = BUTTON_NONE;
    int read_size;
    struct mcs5000_raw_data touchpad_data;

    /* Check for all the keys */
    if (!gpio_get(GPIO_VOL_UP_KEY)) {
        key |= BUTTON_VOL_UP;
    }
    if (!gpio_get(GPIO_VOL_DOWN_KEY)) {
        key |= BUTTON_VOL_DOWN;
    }
    if (gpio_get(GPIO_POWER_KEY)) {
        key |= BUTTON_POWER;
    }

    read_size = mcs5000_read(&touchpad_data);

    if (read_size == sizeof(struct mcs5000_raw_data)) {
        /* Generate UP and DOWN events */
        if (touchpad_data.inputInfo & INPUT_TYPE_SINGLE) {
            last_touch_state = STATE_DOWN;
        }
        else {
            last_touch_state = STATE_UP;
        }
        /* Swap coordinates here */
#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
        last_x = (touchpad_data.yHigh << 8) | touchpad_data.yLow;
        last_y = (touchpad_data.xHigh << 8) | touchpad_data.xLow;
#else
        last_x = (touchpad_data.xHigh << 8) | touchpad_data.xLow;
        last_y = (touchpad_data.yHigh << 8) | touchpad_data.yLow;
#endif
    }

    int tkey = touchscreen_to_pixels(last_x, last_y, data);

    if (last_touch_state == STATE_DOWN) {
            key |= tkey;
    }

    return key;
}

#ifndef HAS_BUTTON_HOLD
void touchscreen_enable_device(bool en)
{
    if (en) {
        mcs5000_power();
    }
    else {
        mcs5000_shutdown();
    }
}
#endif

bool headphones_inserted(void)
{
    /* GPIO low - 0 - means headphones inserted */
    return !gpio_get(GPIO_HEADPHONE_SENSE);
}

void button_init_device(void)
{
    /* Setup GPIO pin for headphone sense, copied from OF
     * Pins for the other buttons are already set up by OF button module
     */
    gpio_set_iomux(GPIO_HEADPHONE_SENSE, CONFIG_ALT4);
    gpio_set_pad(GPIO_HEADPHONE_SENSE, PAD_CTL_SRE_SLOW);
    gpio_direction_input(GPIO_HEADPHONE_SENSE);

    /* Turn on touchscreen */
    mcs5000_init();
    mcs5000_power();
    mcs5000_set_hand(RIGHT_HAND);
}

#ifdef BUTTON_DRIVER_CLOSE
/* I'm not sure it's called at shutdown...give a check! */
void button_close_device(void)
{
    gpio_free_iomux(GPIO_HEADPHONE_SENSE, CONFIG_ALT4);

    /* Turn off touchscreen device */
    mcs5000_shutdown();
    mcs5000_close();
}
#endif /* BUTTON_DRIVER_CLOSE */
