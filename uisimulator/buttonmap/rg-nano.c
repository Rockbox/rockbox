/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Szymon Dziok
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
        case SDLK_KP_5:
        case SDLK_RETURN:
        case SDLK_x:
            new_btn = BUTTON_A;
            break;
        case SDLK_ESCAPE:
        case SDLK_z:
            new_btn = BUTTON_B;
            break;
        case SDLK_s:
            new_btn = BUTTON_X;
            break;
        case SDLK_a:
            new_btn = BUTTON_Y;
            break;
        case SDLK_d:
            new_btn = BUTTON_L;
            break;
        case SDLK_c:
            new_btn = BUTTON_R;
            break;
        case SDLK_n:
            new_btn = BUTTON_FN;
            break;
        case SDLK_m:
            new_btn = BUTTON_START;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_LEFT,       52,  432, 22, "Left" },
    { SDLK_RIGHT,      137, 432, 22, "Right" },
    { SDLK_UP,         95,  390, 22, "Up" },
    { SDLK_DOWN,       95,  474, 22, "Down" },
    { SDLK_RETURN,     313, 432, 25, "A" },
    { SDLK_ESCAPE,     271, 475, 25, "B" },
    { SDLK_s,          271, 391, 25, "X" },
    { SDLK_a,          229, 432, 25, "Y" },
    { SDLK_d,          41,    8, 50, "L" },
    { SDLK_c,          329,   8, 50, "R" },
    { SDLK_n,          149, 542, 20, "SE" },
    { SDLK_m,          209, 542, 20, "ST" },
    { 0, 0, 0, 0, "None" }
};
