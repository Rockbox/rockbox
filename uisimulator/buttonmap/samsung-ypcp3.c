/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
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

/* The YP-CP3 has the YP-R0's keys, and the same keyboard layout here */
int key_to_button(int keyboard_button)
{
    int new_btn = BUTTON_NONE;
    switch (keyboard_button)
    {
        case SDLK_KP_4:
        case SDLK_LEFT:
            new_btn = BUTTON_LEFT;
            break;
        case SDLK_KP_6:
        case SDLK_RIGHT:
            new_btn = BUTTON_RIGHT;
            break;
        case SDLK_KP_8:
        case SDLK_UP:
            new_btn = BUTTON_UP;
            break;
        case SDLK_KP_2:
        case SDLK_DOWN:
            new_btn = BUTTON_DOWN;
            break;
        case SDLK_PAGEUP:
        case SDLK_KP_9:
            new_btn = BUTTON_MENU;
            break;
        case SDLK_KP_7:
            new_btn = BUTTON_BACK;
            break;
        case SDLK_KP_5:
        case SDLK_SPACE:
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
            new_btn = BUTTON_SELECT;
            break;
        case SDLK_KP_3:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_1:
            new_btn = BUTTON_USER;
            break;
    }
    return new_btn;
}

/* The face carries the joystick and, below it, Back and Menu; User (Rec)
 * and Power sit on the top edge, above them. Select comes first: the first
 * area a click falls in wins. */
struct button_map bm[] = {
    { SDLK_KP_5,       517,  90, 11, "Select" },
    { SDLK_KP_8,       517,  66, 13, "Up" },
    { SDLK_KP_2,       517, 114, 13, "Down" },
    { SDLK_KP_4,       493,  90, 13, "Left" },
    { SDLK_KP_6,       541,  90, 13, "Right" },
    { SDLK_KP_7,       519, 159, 11, "Back" },
    { SDLK_KP_9,       519, 193, 11, "Menu" },
    { SDLK_KP_1,       480,   8, 12, "User" },
    { SDLK_KP_3,       531,   8, 12, "Power" },
    { 0, 0, 0, 0, "None" }
};
