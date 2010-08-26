/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Barry Wardell
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
        case SDLK_KP9:
        case SDLK_PAGEUP:
            new_btn = BUTTON_UP;
            break;
        case SDLK_KP3:
        case SDLK_PAGEDOWN:
            new_btn = BUTTON_DOWN;
            break;
        case SDLK_KP1:
        case SDLK_HOME:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP7:
        case SDLK_END:
            new_btn = BUTTON_REC;
            break;
        case SDLK_KP5:
        case SDLK_SPACE:
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
    { SDLK_KP7,         5,  92, 18, "Record" },
    { SDLK_KP9,       128, 295, 43, "Play" },
    { SDLK_KP4,        42, 380, 33, "Left" },
    { SDLK_KP5,       129, 378, 36, "Select" },
    { SDLK_KP6,       218, 383, 30, "Right" },
    { SDLK_KP3,       129, 461, 29, "Down" },
    { SDLK_KP1,        55, 464, 20, "Menu" },
    { SDLK_KP8,        92, 338, 17, "Scroll Back" },
    { SDLK_KP2,       167, 342, 17, "Scroll Fwd"  },
    { 0, 0, 0, 0, "None" }
};
