/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Maurus Cuelenaere
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
        case SDLK_KP1:
            new_btn = BUTTON_PREV;
            break;
        case SDLK_KP3:
            new_btn = BUTTON_NEXT;
            break;
        case SDLK_KP7:
            new_btn = BUTTON_BACK;
            break;
        case SDLK_p:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_KP9:
            new_btn = BUTTON_MENU;
            break;
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
        case SDLK_KP5:
        case SDLK_SPACE:
            new_btn = BUTTON_SELECT;
            break;
        case SDLK_KP_MULTIPLY:
        case SDLK_F8:
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_z:
            new_btn = BUTTON_VOL_DOWN;
            break;
        case SDLK_s:
            new_btn = BUTTON_VOL_UP;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP7,      52, 414, 35, "Custom" },
    { SDLK_KP8,     185, 406, 55, "Up" },
    { SDLK_KP9,     315, 421, 46, "Play" },
    { SDLK_KP4,     122, 500, 41, "Left" },
    { SDLK_KP6,     247, 493, 49, "Right" },
    { SDLK_KP1,      58, 577, 49, "Back" },
    { SDLK_KP2,     186, 585, 46, "Down" },
    { SDLK_KP3,     311, 569, 47, "Menu" },
    { 0, 0, 0, 0, "None" }
};
