/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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
        case SDLK_KP_PLUS:
        case SDLK_F8:
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        case SDLK_a:
            new_btn = BUTTON_A;
            break;
        case SDLK_KP_5:
        case SDLK_SPACE:
            new_btn = BUTTON_SELECT;
            break;
        case SDLK_KP_PERIOD:
        case SDLK_INSERT:
            new_btn = BUTTON_MENU;
            break;
        case SDLK_KP_9:
        case SDLK_PAGEUP:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_KP_3:
        case SDLK_PAGEDOWN:
            new_btn = BUTTON_VOL_DOWN;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP_PLUS,    361, 187, 22, "Power" },
    { SDLK_KP_PERIOD,  361, 270, 17, "Menu" },
    { SDLK_KP_9,        365, 345, 26, "Vol Up" },
    { SDLK_KP_3,        363, 433, 25, "Vol Down" },
    { SDLK_KP_ENTER,   365, 520, 19, "A" },
    { SDLK_KP_8,        167, 458, 35, "Up" },
    { SDLK_KP_4,         86, 537, 29, "Left" },
    { SDLK_KP_5,        166, 536, 30, "Select" },
    { SDLK_KP_6,        248, 536, 30, "Right" },
    { SDLK_KP_2,        169, 617, 28, "Down" },
    { 0, 0, 0, 0, "None" }
};
