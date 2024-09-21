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
            new_btn = BUTTON_SCROLL_BACK;
            break;
        case SDLK_KP_2:
        case SDLK_DOWN:
            new_btn = BUTTON_SCROLL_FWD;
            break;
        case SDLK_KP_9:
        case SDLK_PAGEUP:
            new_btn = BUTTON_UP;
            break;
        case SDLK_KP_3:
        case SDLK_PAGEDOWN:
            new_btn = BUTTON_DOWN;
            break;
        case SDLK_KP_1:
        case SDLK_HOME:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_7:
        case SDLK_END:
            new_btn = BUTTON_REC;
            break;
        case SDLK_KP_5:
        case SDLK_SPACE:
            new_btn = BUTTON_SELECT;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP_7,         5,  92, 18, "Record" },
    { SDLK_KP_9,       128, 295, 43, "Play" },
    { SDLK_KP_4,        42, 380, 33, "Left" },
    { SDLK_KP_5,       129, 378, 36, "Select" },
    { SDLK_KP_6,       218, 383, 30, "Right" },
    { SDLK_KP_3,       129, 461, 29, "Down" },
    { SDLK_KP_1,        55, 464, 20, "Menu" },
    { SDLK_KP_8,        92, 338, 17, "Scroll Back" },
    { SDLK_KP_2,       167, 342, 17, "Scroll Fwd"  },
    { 0, 0, 0, 0, "None" }
};
