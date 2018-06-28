/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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
        case SDLK_UP:
            new_btn = BUTTON_PREV;
            break;
        case SDLK_KP1:
        case SDLK_DOWN:
            new_btn = BUTTON_NEXT;
            break;
        case SDLK_KP3:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
        case SDLK_RETURN:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_KP5:
        case SDLK_END:
        case SDLK_BACKSPACE:
            new_btn = BUTTON_OPTION;
            break;
        case SDLK_KP7:
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP9:
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
    { SDLK_KP4,     68, 342, 17, "Prev" },
    { SDLK_KP1,     72, 404, 17, "Next" },
    { SDLK_KP3,    142, 374, 28, "Play" },
    { SDLK_KP5,    106, 288, 17, "Option" },
    { SDLK_KP7,     38, 244, 17, "Power" },
    { SDLK_KP9,    155, 244, 17, "Home" },
    { SDLK_KP_MINUS, 192,  74, 17, "Vol Up" },
    { SDLK_KP_PLUS,  192, 132, 17, "Vol Dn" },
    { SDLK_h,        0,  66, 17, "Hold" },
    { 0, 0, 0, 0, "None" }
};
