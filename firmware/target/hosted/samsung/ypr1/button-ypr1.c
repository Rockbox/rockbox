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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "sys/ioctl.h"

#include <gpio-target.h> /* For headphones sense and buttons */
#include "r1TouchIoctl.h"
#include "mcs5000.h"

static enum {
    STATE_UNKNOWN,
    STATE_UP,
    STATE_DOWN,
} last_touch_state = STATE_UNKNOWN;

static int last_x = 0;
static int last_y = 0;
static int max_x = -1;
static int max_y = -1;

int mcs5000_dev = -1;

int button_read_device(int *data)
{
    int key = BUTTON_NONE;
    static TsRawData someData;
    int read_size;

    /* Check for all the keys */
    if (!gpio_control(DEV_CTRL_GPIO_IS_HIGH, GPIO_VOL_UP_KEY, 0, 0)) {
        key |= BUTTON_VOL_UP;
    }
    if (!gpio_control(DEV_CTRL_GPIO_IS_HIGH, GPIO_VOL_DOWN_KEY, 0, 0)) {
        key |= BUTTON_VOL_DOWN;
    }
    if (gpio_control(DEV_CTRL_GPIO_IS_HIGH, GPIO_POWER_KEY, 0, 0)) {
        key |= BUTTON_POWER;
    }
    /** Events to discard (what it *seems*):
     *    - x and y are swapped for portrait
     *    - x (really y) is okay
     *    - y (really x) needs to be - XXX
     * 
     * To recap, it's time to shutdown...
     * we have the basic support done BUT a funamental thing is still missing:
     * what are the absolute values for this touch??
     * 
     */
    

    read_size = read(mcs5000_dev, &someData, sizeof(someData));

    if (read_size == sizeof(TsRawData)) {
        //printf("X: %i,%i - Y: %i,%i - Z: %i\n", someData.xHigh, someData.xLow, someData.yHigh, someData.yLow, someData.z);
        //printf("T: %i - G: %i - W: %i\n", someData.inputInfo, someData.gesture, someData.width);
        /* Generate UP and DOWN events */
        if (someData.inputInfo & INPUT_TYPE_SINGLE) {
            last_touch_state = STATE_DOWN;
        }
        else {
            last_touch_state = STATE_UP;
        }
        /* Swap coordinates here */
#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
        last_y = (someData.xHigh << 8) | someData.xLow;
        last_x = (someData.yHigh << 8) | someData.yLow;
#else
        last_x = (someData.xHigh << 8) | someData.xLow;
        last_y = (someData.yHigh << 8) | someData.yLow;
#endif
        if (last_y > max_y) {
            max_y = last_y;
            //printf("Max y @ %i", max_y);
        }

        if (last_x > max_x) {
            max_x = last_x;
            //printf("Max x @ %i", max_x);
        }
        //printf("-> (%i, %i)\n", last_x, last_y);
    }

    
    
    int tkey = touchscreen_to_pixels(last_x, last_y, data);

    if (last_touch_state == STATE_DOWN) {
            key |= tkey;
    }
    
    return key;
}

bool headphones_inserted(void)
{
    /* GPIO low - 0 - means headphones inserted */
    return !gpio_control(DEV_CTRL_GPIO_IS_HIGH, GPIO_HEADPHONE_SENSE, 0, 0);
}

void button_init_device(void)
{
    /* Setup GPIO pin for headphone sense, copied from OF */
    gpio_control(DEV_CTRL_GPIO_SET_MUX, GPIO_HEADPHONE_SENSE, 4, 0);
    gpio_control(DEV_CTRL_GPIO_SET_INPUT, GPIO_HEADPHONE_SENSE, 4, 0);
    
    mcs5000_dev = open("/dev/r1Touch", O_RDONLY);
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_ON);
//    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_RESET);
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_IDLE);
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_RIGHTHAND);
    
    /* No need to initialize any GPIO pin, since this is done loading the r0Btn module */
}

#ifdef BUTTON_DRIVER_CLOSE
/* I'm not sure it's called at shutdown...give a check! */
void button_close_device(void)
{
    /* Don't know the precise meaning, but it's done as in the OF, so copied there */
    gpio_control(DEV_CTRL_GPIO_UNSET_MUX, GPIO_HEADPHONE_SENSE, 0, 0);
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_RESET);
    ioctl(mcs5000_dev, DEV_CTRL_TOUCH_OFF);
    close(mcs5000_dev);
}
#endif /* BUTTON_DRIVER_CLOSE */
