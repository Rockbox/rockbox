/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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
        case SDLK_KP_7:
            new_btn = BUTTON_PREV;
            break;
        case SDLK_PAGEUP:
        case SDLK_KP_9:
            new_btn = BUTTON_NEXT;
            break;
        case SDLK_KP_DIVIDE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_5:
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
        case SDLK_KP_3:
            new_btn = BUTTON_SCROLL_FWD;
            break;
        case SDLK_KP_1:
            new_btn = BUTTON_SCROLL_BACK;
            break;
        case SDLK_h:
            new_btn = BUTTON_HOLD;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_UP,          191, 505, 18, "Up" },
    { SDLK_DOWN,        191, 630, 18, "Down" },
    { SDLK_LEFT,        130, 568, 18, "Left" },
    { SDLK_RIGHT,       256, 568, 18, "Right" },
    { SDLK_KP_7,         107, 443, 20, "Prev" },
    { SDLK_KP_9,         271, 443, 20, "Next" },
    { SDLK_KP_5,         191, 568, 18, "Select" },
    { SDLK_KP_DIVIDE,   220, 43,  15, "Power" },
    { SDLK_KP_3,         231, 520, 10, "Scroll Fwd" },
    { SDLK_KP_1,         149, 520, 10, "Scroll Back" },
    { SDLK_KP_MINUS,      3, 277, 25, "Volume -" },
    { SDLK_KP_PLUS,       6, 175, 25, "Volume +" },
    { SDLK_h,           300,  42, 20, "Hold" },
    { 0, 0, 0, 0, "None" }
};
