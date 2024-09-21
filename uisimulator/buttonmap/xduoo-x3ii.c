/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Roman Stolyarov
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
        case SDLK_UP:
            new_btn = BUTTON_PREV;
            break;
        case SDLK_KP_1:
        case SDLK_DOWN:
            new_btn = BUTTON_NEXT;
            break;
        case SDLK_KP_3:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
        case SDLK_RETURN:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_KP_5:
        case SDLK_END:
        case SDLK_BACKSPACE:
            new_btn = BUTTON_OPTION;
            break;
        case SDLK_KP_7:
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_9:
        case SDLK_HOME:
            new_btn = BUTTON_HOME;
            break;
        case SDLK_KP_MINUS:
        case SDLK_PAGEUP:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_KP_PLUS:
        case SDLK_PAGEDOWN:
            new_btn = BUTTON_VOL_DOWN;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP_4,    214, 537, 20, "Prev" },
    { SDLK_KP_1,    241, 488, 20, "Next" },
    { SDLK_KP_3,    150, 488, 30, "Play" },
    { SDLK_KP_5,     60, 488, 20, "Back" },
    { SDLK_KP_7,      0,  60, 25, "Power" },
    { SDLK_KP_9,     86, 537, 20, "Home" },
    { SDLK_KP_MINUS, 0, 120, 25, "Vol Up" },
    { SDLK_KP_PLUS,  0, 200, 25, "Vol Dn" },
    { 0, 0, 0, 0, "None" }
};
