/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Maurus Cuelenaere
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
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_PLUS:
        case SDLK_PLUS:
        case SDLK_GREATER:
        case SDLK_RIGHTBRACKET:
        case SDLK_KP_MULTIPLY:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_KP_MINUS:
        case SDLK_MINUS:
        case SDLK_LESS:
        case SDLK_LEFTBRACKET:
        case SDLK_KP_DIVIDE:
            new_btn = BUTTON_VOL_DOWN;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
            new_btn = BUTTON_MENU;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_MINUS,   113, 583, 28, "Minus" },
    { SDLK_PLUS,    227, 580, 28, "Plus" },
    { SDLK_RETURN,  171, 583, 34, "Menu" },
    { 0, 0, 0, 0, "None" }
};
