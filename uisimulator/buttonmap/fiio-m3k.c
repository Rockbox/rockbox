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
            new_btn = BUTTON_UP;
            break;
        case SDLK_KP2:
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
        case SDL_BUTTON_WHEELUP:
            new_btn = BUTTON_SCROLL_BACK;
            break;
        case SDL_BUTTON_WHEELDOWN:
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
        case SDLK_KP5:
            new_btn = BUTTON_PLAY;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_ESCAPE,       12,  55, 15, "Power" },
    { SDLK_KP_MINUS,     12, 125, 15, "Volume -" },
    { SDLK_KP_PLUS,      12, 188, 15, "Volume +" },
    { SDLK_SPACE,        12, 255, 15, "Play" },
    { SDLK_UP,          146, 394, 20, "Up" },
    { SDLK_RETURN,      146, 438, 20, "Select" },
    { SDLK_DOWN,        146, 510, 20, "Down" },
    { SDLK_INSERT,       68, 368, 20, "Menu" },
    { SDLK_LEFT,         68, 532, 20, "Left" },
    { SDLK_RIGHT,       224, 532, 20, "Right" },
    { SDLK_BACKSPACE,   224, 368, 20, "Back" },
    { 0, 0, 0, 0, "None" }
};
