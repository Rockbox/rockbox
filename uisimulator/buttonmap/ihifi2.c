/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Roman Stolyarov
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


#include <SDL.h>
#include "button.h"
#include "buttonmap.h"

int key_to_button(int keyboard_button)
{
    int new_btn = BUTTON_NONE;
    switch (keyboard_button)
    {
        case SDLK_KP_8:
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_4:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
        case SDLK_RETURN:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_KP_5:
        case SDLK_UP:
            new_btn = BUTTON_PREV;
            break;
        case SDLK_KP_6:
        case SDLK_PAGEUP:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_KP_1:
        case SDLK_BACKSPACE:
            new_btn = BUTTON_HOME;
            break;
        case SDLK_KP_2:
        case SDLK_DOWN:
            new_btn = BUTTON_NEXT;
            break;
        case SDLK_KP_3:
        case SDLK_PAGEDOWN:
            new_btn = BUTTON_VOL_DOWN;
            break;
    }
    return new_btn;
}

#if (CONFIG_KEYPAD == IHIFI_770_PAD)
struct button_map bm[] = {
    { SDLK_KP_8,     210,   0, 20, "Power" },
    { SDLK_KP_4,      94, 430, 40, "Play" },
    { SDLK_KP_5,     190, 430, 40, "Prev" },
    { SDLK_KP_6,     285, 430, 40, "Vol Up" },
    { SDLK_KP_1,      94, 508, 40, "Home" },
    { SDLK_KP_2,     190, 508, 40, "Next" },
    { SDLK_KP_3,     285, 508, 40, "Vol Down" },
    { 0, 0, 0, 0, "None" }
};
#elif (CONFIG_KEYPAD == IHIFI_800_PAD)
struct button_map bm[] = {
    { SDLK_KP_8,     214, 468, 25, "Power" },
    { SDLK_KP_4,     168, 580, 25, "Play" },
    { SDLK_KP_5,      60, 524, 25, "Prev" },
    { SDLK_KP_6,     102, 455, 25, "Vol Up" },
    { SDLK_KP_1,     246, 580, 25, "Home" },
    { SDLK_KP_2,      60, 580, 25, "Next" },
    { SDLK_KP_3,      50, 455, 25, "Vol Down" },
    { 0, 0, 0, 0, "None" }
};
#else
#error please define button map
#endif
