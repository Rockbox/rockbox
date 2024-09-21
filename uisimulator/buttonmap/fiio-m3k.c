/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 by Solomon Peachy
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
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_PLUS:
        case SDLK_EQUALS:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_KP_MINUS:
        case SDLK_MINUS:
            new_btn = BUTTON_VOL_DOWN;
            break;
        case SDLK_KP_PERIOD:
        case SDLK_INSERT:
            new_btn = BUTTON_MENU;
            break;
        case SDLK_PAGEUP:
            new_btn = BUTTON_SCROLL_BACK;
            break;
        case SDLK_PAGEDOWN:
            new_btn = BUTTON_SCROLL_FWD;
            break;
        case SDLK_BACKSPACE:
            new_btn = BUTTON_BACK;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
            new_btn = BUTTON_SELECT;
            break;
        case SDLK_SPACE:
        case SDLK_KP_5:
            new_btn = BUTTON_PLAY;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_ESCAPE,       14,  63, 15, "Power" },
    { SDLK_KP_MINUS,     14, 219, 15, "Volume -" },
    { SDLK_KP_PLUS,      14, 139, 15, "Volume +" },
    { SDLK_SPACE,        14, 299, 15, "Play" },
    { SDLK_UP,          170, 444, 25, "Up" },
    { SDLK_RETURN,      170, 519, 25, "Select" },
    { SDLK_DOWN,        170, 599, 25, "Down" },
    { SDLK_INSERT,       79, 427, 25, "Menu" },
    { SDLK_LEFT,         79, 620, 25, "Left" },
    { SDLK_RIGHT,       260, 620, 25, "Right" },
    { SDLK_BACKSPACE,   260, 427, 25, "Back" },
    { 0, 0, 0, 0, "None" }
};
