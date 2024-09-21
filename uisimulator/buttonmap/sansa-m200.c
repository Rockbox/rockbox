/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dominik Wenger
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
        case SDLK_PLUS:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_5:
            new_btn = BUTTON_SELECT;
            break;
        case SDLK_KP_7:
            new_btn = BUTTON_VOL_DOWN;
            break;
        case SDLK_KP_9:
            new_btn = BUTTON_VOL_UP;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP_PLUS, 54,  14, 16, "Power" },
    { SDLK_KP_7,     96,  13, 12, "Vol Down" },
    { SDLK_KP_9,    139,  14, 14, "Vol Up" },
    { SDLK_KP_5,    260,  82, 20, "Select" },
    { SDLK_KP_8,    258,  35, 30, "Play" },
    { SDLK_KP_4,    214,  84, 25, "Left" },
    { SDLK_KP_6,    300,  83, 24, "Right" },
    { SDLK_KP_2,    262, 125, 28, "Repeat" },
    { SDLK_h,      113, 151, 21, "Hold" },
    { 0, 0, 0, 0, "None" }
};
