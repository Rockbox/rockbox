/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Marcin Bukat
 * Copyright (C) 2018 Roman Stolyarov
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
#include "xduoolinux_codec.h"

int button_map(int keycode)
{
    switch(keycode)
    {
        case KEY_BACK:
            return BUTTON_HOME;

        case KEY_MENU:
            return BUTTON_OPTION;

        case KEY_UP:
            return BUTTON_PREV;

        case KEY_DOWN:
            return BUTTON_NEXT;

        case KEY_ENTER:
            return BUTTON_PLAY;

        case KEY_VOLUMEUP:
            return BUTTON_VOL_UP;

        case KEY_VOLUMEDOWN:
            return BUTTON_VOL_DOWN;

        case KEY_POWER:
            return BUTTON_POWER;

        default:
            return 0;
    }
}

bool headphones_inserted(void)
{
#ifdef BOOTLOADER
    int ps = 0;
#else
    int ps = xduoo_get_outputs();
#endif

    return (ps == 2 || ps == 3);
}

bool lineout_inserted(void)
{
#ifdef BOOTLOADER
    int ps = 0;
#else
    int ps = xduoo_get_outputs();
#endif
    return (ps == 1);
}
