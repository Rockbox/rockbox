/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2020 Solomon Peachy
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

#include "button.h"
#include "button-target.h"

#include "kernel.h"
#include "erosqlinux_codec.h"

/*
   /dev/input/event0: rotary encoder
   /dev/input/event1: all keys
*/

int button_map(int keycode)
{
    switch(keycode)
    {
        case KEY_POWER:
            return BUTTON_POWER;

        case KEY_MENU:
            return BUTTON_MENU;

        case KEY_BACK:
            return BUTTON_BACK;

        case KEY_NEXTSONG:
            return BUTTON_PREV;

        case KEY_PREVIOUSSONG:
            return BUTTON_NEXT;  // Yes, backwards!

        case KEY_PLAYPAUSE:
            return BUTTON_PLAY;

        case KEY_LEFT:
            return BUTTON_SCROLL_BACK;

        case KEY_RIGHT:
            return BUTTON_SCROLL_FWD;

        case KEY_VOLUMEUP:
            return BUTTON_VOL_UP;

        case KEY_VOLUMEDOWN:
            return BUTTON_VOL_DOWN;

        default:
            return 0;
    }
}

bool headphones_inserted(void)
{
#ifdef BOOTLOADER
    int ps = 0;
#else
    int ps = erosq_get_outputs();
#endif

    return (ps == 2);
}

bool lineout_inserted(void)
{
#ifdef BOOTLOADER
    int ps = 0;
#else
    int ps = erosq_get_outputs();
#endif

    return (ps == 1);
}
