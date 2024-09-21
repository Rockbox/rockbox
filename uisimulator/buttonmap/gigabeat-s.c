/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Will Robertson
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
        case SDLK_F8:
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_PLUS:
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_KP_7:
            new_btn = BUTTON_BACK;
            break;
        case SDLK_KP_5:
        case SDLK_SPACE:
            new_btn = BUTTON_SELECT;
            break;
        case SDLK_KP_9:
        case SDLK_KP_PERIOD:
        case SDLK_INSERT:
            new_btn = BUTTON_MENU;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP_PLUS, 416, 383, 23, "Play" },
    { SDLK_KP_7,     135, 442, 46, "Back" },
    { SDLK_KP_9,     288, 447, 35, "Menu" },
    { SDLK_KP_8,     214, 480, 32, "Up" },
    { SDLK_KP_4,     128, 558, 33, "Left" },
    { SDLK_KP_5,     214, 556, 34, "Select" },
    { SDLK_KP_6,     293, 558, 35, "Right" },
    { SDLK_KP_2,     214, 637, 38, "Down" },
    { 0, 0, 0, 0, "None" }
};
