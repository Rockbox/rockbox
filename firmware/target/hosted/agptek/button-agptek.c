/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Marcin Bukat
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
#include <linux/input.h>

#include "sysfs.h"
#include "button.h"
#include "button-target.h"

int button_map(int keycode)
{
    switch(keycode)
    {
        case KEY_LEFT:
            return BUTTON_LEFT;

        case KEY_RIGHT:
            return BUTTON_RIGHT;

        case KEY_UP:
            return BUTTON_UP;

        case KEY_DOWN:
            return BUTTON_DOWN;

        case KEY_PLAYPAUSE:
            return BUTTON_SELECT;

        case KEY_VOLUMEUP:
            return BUTTON_VOLUP;

        case KEY_VOLUMEDOWN:
            return BUTTON_VOLDOWN;

        case KEY_POWER:
            return BUTTON_POWER;

        default:
            return 0;
    }
}

bool headphones_inserted(void)
{
    int status = 0;
    const char * const sysfs_hp_switch = "/sys/class/switch/headset/state";
    sysfs_get_int(sysfs_hp_switch, &status);

    return status ? true : false;
}
