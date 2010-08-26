/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Thomas Martitz
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
        case SDLK_KP4:
        case SDLK_LEFT:
            new_btn = BUTTON_LEFT;
            break;
        case SDLK_KP6:
        case SDLK_RIGHT:
            new_btn = BUTTON_RIGHT;
            break;
        case SDLK_KP8:
        case SDLK_UP:
            new_btn = BUTTON_SCROLL_BACK;
            break;
        case SDLK_KP2:
        case SDLK_DOWN:
            new_btn = BUTTON_SCROLL_FWD;
            break;
        case SDLK_PAGEUP:
        case SDLK_KP9:
            new_btn = BUTTON_UP;
            break;
        case SDLK_PAGEDOWN:
        case SDLK_KP3:
            new_btn = BUTTON_DOWN;
            break;
        case SDLK_KP_MINUS:
        case SDLK_KP1:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_MULTIPLY:
            new_btn = BUTTON_HOME;
            break;
        case SDLK_KP5:
        case SDLK_SPACE:
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
            new_btn = BUTTON_SELECT;
            break;
        case SDL_BUTTON_WHEELUP:
            new_btn = BUTTON_SCROLL_BACK;
            break;
        case SDL_BUTTON_WHEELDOWN:
            new_btn = BUTTON_SCROLL_FWD;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP8,          70, 265, 35, "Scroll Back" },
    { SDLK_KP9,         141, 255, 31, "Play" },
    { SDLK_KP_MULTIPLY, 228, 267, 18, "Home" },
    { SDLK_LEFT,         69, 329, 31, "Left" },
    { SDLK_SPACE,       141, 330, 20, "Select" },
    { SDLK_RIGHT,       214, 331, 23, "Right" },
    { SDLK_KP3,         142, 406, 30, "Menu" },
    { SDLK_DOWN,        221, 384, 24, "Scroll Fwd" },
    { SDLK_KP_MINUS,    270, 299, 25, "Power" },
    { SDLK_h,           269, 358, 26, "Hold" },
    { 0, 0, 0, 0, "None" }
};
