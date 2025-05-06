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
#include "surfanslinux_codec.h"

/*
   /dev/input/event0: rotary encoder (left/right)
   /dev/input/event1: adc (prev/play)
   /dev/input/event2: touchscreen
   /dev/input/event3: gpios (power/next)
*/

int button_map(int keycode)
{
    switch(keycode)
    {
        case KEY_LEFT:
            return BUTTON_SCROLL_BACK;
        case KEY_RIGHT:
            return BUTTON_SCROLL_FWD;
        case KEY_PLAYPAUSE:
            return BUTTON_PLAY;
        case KEY_NEXTSONG:
            return BUTTON_NEXT;
        case KEY_PREVIOUSSONG:
            return BUTTON_PREV;
        case KEY_POWER:
            return BUTTON_POWER;
        case KEY_KPMINUS:
            return BUTTON_TOUCH;
        default:
            return 0;
    }
}

bool headphones_inserted(void)
{
#ifdef BOOTLOADER
    int ps = 0;
#else
    int ps = surfans_get_outputs();
#endif

    return (ps == 2 || ps == 3);
}
