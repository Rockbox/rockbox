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
#include "config.h"
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
            new_btn = BUTTON_SCROLL_UP;
            break;
        case SDLK_KP_2:
        case SDLK_DOWN:
            new_btn = BUTTON_SCROLL_DOWN;
            break;
        case SDLK_KP_PLUS:
        case SDLK_F8:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_DIVIDE:
        case SDLK_F1:
            new_btn = BUTTON_REW;
            break;
        case SDLK_KP_MULTIPLY:
        case SDLK_F2:
            new_btn = BUTTON_FF;
            break;
        case SDLK_KP_5:
        case SDLK_SPACE:
            new_btn = BUTTON_PLAY;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {  
#if defined (IRIVER_H10)
    { SDLK_KP_PLUS,       38,  70, 37, "Power" },
    { SDLK_KP_4,          123, 194, 26, "Cancel" },
    { SDLK_KP_6,          257, 195, 34, "Select" },
    { SDLK_KP_8,          190, 221, 28, "Up" },
    { SDLK_KP_2,          192, 320, 27, "Down" },
    { SDLK_KP_DIVIDE,    349,  49, 20, "Rew" },
    { SDLK_KP_5,          349,  96, 20, "Play" },
    { SDLK_KP_MULTIPLY,  350, 141, 23, "FF" },
#elif defined (IRIVER_H10_5GB)
    { SDLK_KP_PLUS,      34,  76, 23, "Power" },
    { SDLK_KP_4,         106, 222, 28, "Cancel" },
    { SDLK_KP_6,         243, 220, 31, "Select" },
    { SDLK_KP_8,         176, 254, 34, "Up" },
    { SDLK_KP_2,         175, 371, 35, "Down" },
    { SDLK_KP_DIVIDE,   319,  63, 26, "Rew" },
    { SDLK_KP_5,         320, 124, 26, "Play" },
    { SDLK_KP_MULTIPLY, 320, 181, 32, "FF" },
#endif
    { 0, 0, 0, 0, "None" }
};
