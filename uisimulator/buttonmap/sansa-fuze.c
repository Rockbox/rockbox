/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Thomas Martitz
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
        case SDLK_PAGEUP:
        case SDLK_KP_9:
            new_btn = BUTTON_UP;
            break;
        case SDLK_PAGEDOWN:
        case SDLK_KP_3:
            new_btn = BUTTON_DOWN;
            break;
        case SDLK_KP_MINUS:
        case SDLK_KP_1:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_h:
            new_btn = BUTTON_HOLD;
            break;
        case SDLK_KP_MULTIPLY:
            new_btn = BUTTON_HOME;
            break;
        case SDLK_KP_5:
        case SDLK_SPACE:
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
            new_btn = BUTTON_SELECT;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP_7,  70, 400, 15, "Back" },
    { SDLK_KP_9, 142, 272, 15, "Play/Pause" },
    { SDLK_KP_5, 142, 330, 26, "Select" },
    { SDLK_KP_8,  70, 261, 26, "Up" },
    { SDLK_KP_2, 219, 386, 26, "Down" },
    { SDLK_KP_6, 199, 332, 26, "Right" },
    { SDLK_KP_4,  77, 334, 26, "Left" },
    { SDLK_KP_3, 140, 392, 26, "Menu" },
    { SDLK_KP_MINUS, 265, 299, 17, "Power" },
    { SDLK_h,   270, 354, 19, "Hold" },
    { SDLK_KP_MULTIPLY, 231, 269, 16, "Home" },
    { 0, 0, 0, 0, "None" }
};
