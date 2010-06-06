/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Rafaël Carré
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
            new_btn = BUTTON_UP;
            break;
        case SDLK_KP2:
        case SDLK_DOWN:
            new_btn = BUTTON_DOWN;
            break;
        
        case SDLK_INSERT:
        case SDLK_KP_MULTIPLY:
            new_btn = BUTTON_HOME;
            break;
        case SDLK_SPACE:
        case SDLK_KP5:
            new_btn = BUTTON_SELECT;
            break;
        case SDLK_PAGEDOWN:
        case SDLK_KP3:
            new_btn = BUTTON_VOL_DOWN;
            break;
        case SDLK_PAGEUP:
        case SDLK_KP9:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_ESCAPE:
        case SDLK_KP1:
            new_btn = BUTTON_POWER;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP_MULTIPLY, 165, 158,  17, "Home" },
    { SDLK_KP5,         102, 230,  29, "Select" },
    { SDLK_KP8,         100, 179,  25, "Play" },
    { SDLK_KP4,          53, 231,  21, "Left" },
    { SDLK_KP6,         147, 232,  19, "Right" },
    { SDLK_KP2,         105, 275,  22, "Menu" },
    { 0, 0, 0, 0, "None" }
};
