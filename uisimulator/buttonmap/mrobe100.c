/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Robert Kukla
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
        case SDLK_F9:
            new_btn = BUTTON_RC_HEART;
            break;
        case SDLK_F10:
            new_btn = BUTTON_RC_MODE;
            break;
        case SDLK_F11:
            new_btn = BUTTON_RC_VOL_DOWN;
            break;
        case SDLK_F12:
            new_btn = BUTTON_RC_VOL_UP;
            break;
        case SDLK_LEFT:
            new_btn = BUTTON_RC_FF;
            break;
        case SDLK_RIGHT:
            new_btn = BUTTON_RC_REW;
            break;
        case SDLK_UP:
            new_btn = BUTTON_RC_PLAY;
            break;
        case SDLK_DOWN:
            new_btn = BUTTON_RC_DOWN;
            break;
        case SDLK_KP_1:
            new_btn = BUTTON_DISPLAY;
            break;
        case SDLK_KP_7:
            new_btn = BUTTON_MENU;
            break;
        case SDLK_KP_9:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_KP_4:
            new_btn = BUTTON_LEFT;
            break;
        case SDLK_KP_6:
            new_btn = BUTTON_RIGHT;
            break;
        case SDLK_KP_8:
            new_btn = BUTTON_UP;
            break;
        case SDLK_KP_2:
            new_btn = BUTTON_DOWN;
            break;
        case SDLK_KP_5:
        case SDLK_SPACE:
            new_btn = BUTTON_SELECT;
            break;
        case SDLK_KP_MULTIPLY:
        case SDLK_F8:
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP_7,      80, 233, 30, "Menu" },
    { SDLK_KP_8,     138, 250, 19, "Up" },
    { SDLK_KP_9,     201, 230, 27, "Play" },
    { SDLK_KP_4,      63, 305, 25, "Left" },
    { SDLK_KP_5,     125, 309, 28, "Select" },
    { SDLK_KP_6,     200, 307, 35, "Right" },
    { SDLK_KP_1,      52, 380, 32, "Display" },
    { SDLK_KP_2,     125, 363, 30, "Down" },
    { SDLK_KP_9,     168, 425, 10, "Play" },
    { SDLK_KP_4,     156, 440, 11, "Left" },
    { SDLK_KP_6,     180, 440, 13, "Right" },
    { SDLK_KP_7,     169, 452, 10, "Menu" },
    { SDLK_KP_MULTIPLY, 222, 15, 16, "Power" },
    { 0, 0, 0, 0, "None" }
};
