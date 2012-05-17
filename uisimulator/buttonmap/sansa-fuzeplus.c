/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
        case SDLK_PAGEUP:
        case SDLK_KP9:
            new_btn = BUTTON_PLAYPAUSE;
            break;
        case SDLK_KP7:
            new_btn = BUTTON_BACK;
            break;
        case SDLK_KP5:
        case SDLK_SPACE:
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
            new_btn = BUTTON_SELECT;
            break;
        case SDLK_KP_PLUS:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_KP_MINUS:
            new_btn = BUTTON_VOL_DOWN;
            break;
        case SDLK_HOME:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP1:
            new_btn = BUTTON_BOTTOMLEFT;
            break;
        case SDLK_KP3:
            new_btn = BUTTON_BOTTOMRIGHT;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP7,        69, 401, 39, "Back" },
    { SDLK_KP8,       161, 404, 34, "Up" },
    { SDLK_KP9,       258, 400, 43, "Play/Pause" },
    { SDLK_KP4,        69, 477, 36, "Left" },
    { SDLK_KP5,       161, 476, 31, "Select" },
    { SDLK_KP6,       222, 474, 41, "Right" },
    { SDLK_KP1,        82, 535, 34, "Bottom-Left" },
    { SDLK_KP2,       162, 532, 33, "Down" },
    { SDLK_KP3,       234, 535, 42, "Bottom-Right" },
    { SDLK_KP_PLUS,     1, 128, 29, "Vol+" },
    { SDLK_KP_MINUS,    5, 187, 30, "Vol-" },
    { SDLK_HOME,      170,   6, 50, "Power" },
    { 0, 0, 0, 0, "None" }
};
