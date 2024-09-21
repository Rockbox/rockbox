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
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_PLUS:
        case SDLK_EQUALS:
            new_btn = BUTTON_VOLUP;
            break;
        case SDLK_KP_MINUS:
        case SDLK_MINUS:
            new_btn = BUTTON_VOLDOWN;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        case SDLK_SPACE:
        case SDLK_INSERT:
        case SDLK_KP_5:
            new_btn = BUTTON_SELECT;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_LEFT,         38, 296, 20, "Left" },
    { SDLK_RIGHT,       146, 295, 20, "Right" },
    { SDLK_UP,           93, 241, 20, "Up" },
    { SDLK_DOWN,         93, 348, 20, "Down" },
    { SDLK_ESCAPE,        2,  45, 20, "Power" },
    { SDLK_RETURN,       93, 295, 40, "Select" },
    { SDLK_KP_MINUS,    182, 100, 30, "Volume -" },
    { SDLK_KP_PLUS,     182,  45, 30, "Volume +" },
    { 0, 0, 0, 0, "None" }
};
