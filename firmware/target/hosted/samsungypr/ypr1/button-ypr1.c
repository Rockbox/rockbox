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
/* debug screen */
#include "button.h"
#include "font.h"
#include "action.h"
#include "list.h"
#include "stdio.h"

enum {
    STATE_UNKNOWN,
    STATE_UP,
    STATE_DOWN,
};

static int last_x = 0;
static int last_y = 0;
static int last_touch_state = STATE_UNKNOWN;
static mcs5000_raw_data touchData;

int button_read_device(int *data)
{
    int key = BUTTON_NONE;
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

    read_size = mcs5000_read(&touchData);

    if (read_size == sizeof(mcs5000_raw_data)) {
        /* Generate UP and DOWN events */
        if (touchData.inputInfo & INPUT_TYPE_SINGLE) {
            last_touch_state = STATE_DOWN;
        }
        else {
            last_touch_state = STATE_UP;
        }
        /* Swap coordinates here */
#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
        last_x = (touchData.yHigh << 8) | touchData.yLow;
        last_y = (touchData.xHigh << 8) | touchData.xLow;
#else
        last_x = (touchData.xHigh << 8) | touchData.xLow;
        last_y = (touchData.yHigh << 8) | touchData.yLow;
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
    return !gpio_control(DEV_CTRL_GPIO_IS_HIGH, GPIO_HEADPHONE_SENSE, 0, 0);
}

void button_init_device(void)
{
    /* Setup GPIO pin for headphone sense, copied from OF
     * Pins for the other buttons are already set up by OF button module
     */
    gpio_control(DEV_CTRL_GPIO_SET_MUX, GPIO_HEADPHONE_SENSE, 4, 0);
    gpio_control(DEV_CTRL_GPIO_SET_INPUT, GPIO_HEADPHONE_SENSE, 4, 0);

    /* Turn on touchscreen */
    mcs5000_init();
    mcs5000_power();
    mcs5000_set_hand(RIGHT_HAND);
}

#ifdef BUTTON_DRIVER_CLOSE
/* I'm not sure it's called at shutdown...give a check! */
void button_close_device(void)
{
    gpio_control(DEV_CTRL_GPIO_UNSET_MUX, GPIO_HEADPHONE_SENSE, 0, 0);

    /* Turn off touchscreen device */
    mcs5000_shutdown();
    mcs5000_close();
}
#endif /* BUTTON_DRIVER_CLOSE */

static const char* mcs5000_debug_get_value(int selected_item, void* data,
                                    char* buffer, size_t buffer_len)
{
    (void)data;
    int ver;
    unsigned char reg_data = -1;
    switch(selected_item) {
        case 0:
            mcs5000_ioctl(DEV_CTRL_TOUCH_GET_VER, &ver);
            snprintf(buffer, buffer_len, "FW revision: %d", ver);
            break;
        case 1:
            snprintf(buffer, buffer_len, "X,Y,Z: %d,%d,%d", last_x, last_y, touchData.z);
            break;
        case 2:
            snprintf(buffer, buffer_len, "Radius: %d", touchData.width);
            break;
        case 3:
            snprintf(buffer, buffer_len, "State: %d", last_touch_state);
            break;
        case 4:
            snprintf(buffer, buffer_len, "Gesture: %d", touchData.gesture);
            break;
        case 5:
             ver = mcs5000_read_reg(MCS5000_TS_FIRMWARE_VER, &reg_data);
             snprintf(buffer, buffer_len, "REG[0x%02x]: 0x%02x - %d", MCS5000_TS_FIRMWARE_VER, reg_data, ver);
             break;
        case 6:
             ver = mcs5000_read_reg(MCS5000_TS_MODULE_REV, &reg_data);
             snprintf(buffer, buffer_len, "REG[0x%02x]: 0x%02x - %d", MCS5000_TS_MODULE_REV, reg_data, ver);
             break;
//         default:
//             mcs5000_read_reg(selected_item-5, &reg_data);
//             snprintf(buffer, buffer_len, "REG[0x%02x]: 0x%02x", selected_item-5, reg_data);
            break;
    }
    return buffer;
}

bool mcs5000_debug_screen(void)
{
    struct gui_synclist lists;
    int action;
    gui_synclist_init(&lists, mcs5000_debug_get_value, NULL, false, 1, NULL);
    gui_synclist_set_title(&lists, "MCS5000 touch controller", NOICON);
    gui_synclist_set_icon_callback(&lists, NULL);
    gui_synclist_set_color_callback(&lists, NULL);
    gui_synclist_set_nb_items(&lists, 7);

    while(1)
    {
        gui_synclist_draw(&lists);
        list_do_action(CONTEXT_STD, HZ / 15, &lists, &action, LIST_WRAP_UNLESS_HELD);
        if(action == ACTION_STD_CANCEL)
            return false;
        if(action == ACTION_STD_OK) {
            return true;
        }
        yield();
    }

    return true;
}
