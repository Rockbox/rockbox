/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
        case SDLK_END:
        case SDLK_KP_3:
        case SDLK_ESCAPE:
        case SDLK_DELETE:
            new_btn = BUTTON_POWER;
            break;
#ifdef SONY_NWZE360
        case SDLK_KP_PLUS:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_KP_MINUS:
            new_btn = BUTTON_VOL_DOWN;
            break;
#endif
        case SDLK_KP_1:
        case SDLK_HOME:
        case SDLK_BACKSPACE:
            new_btn = BUTTON_BACK;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        case SDLK_SPACE:
        case SDLK_INSERT:
        case SDLK_KP_5:
            new_btn = BUTTON_PLAY;
            break;
    }
    return new_btn;
}

#if defined(SONY_NWZE360)
struct button_map bm[] = {
    { SDLK_LEFT,        100, 548, 30, "Left" },
    { SDLK_RIGHT,       240, 548, 30, "Right" },
    { SDLK_UP,          170, 478, 30, "Up" },
    { SDLK_DOWN,        170, 619, 30, "Down" },
    { SDLK_BACKSPACE,    81, 484, 35, "Back" },
    { SDLK_DELETE,      256, 484, 35, "Power" },
    { SDLK_RETURN,      170, 548, 40, "Play" },
    { SDLK_KP_MINUS,    339, 128, 30, "Volume -" },
    { SDLK_KP_PLUS,     339,  68, 30, "Volume +" },
    { 0, 0, 0, 0, "None" }
};
#elif defined(SONY_NWZE370)
struct button_map bm[] = {
    { SDLK_LEFT,         56, 299, 20, "Left" },
    { SDLK_RIGHT,       139, 299, 20, "Right" },
    { SDLK_UP,           98, 256, 20, "Up" },
    { SDLK_DOWN,         98, 340, 20, "Down" },
    { SDLK_BACKSPACE,    44, 259, 35, "Back" },
    { SDLK_DELETE,      152, 259, 35, "Power" },
    { SDLK_RETURN,       98, 299, 40, "Play" },
    { 0, 0, 0, 0, "None" }
};
#else
#error please define button map
#endif
 
