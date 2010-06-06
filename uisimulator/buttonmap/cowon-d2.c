/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Rob Purchase
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
        case SDLK_KP_MULTIPLY:
        case SDLK_F8:
        case SDLK_ESCAPE:
        case SDLK_BACKSPACE:
        case SDLK_DELETE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_PLUS:
        case SDLK_EQUALS:
            new_btn = BUTTON_PLUS;
            break;
        case SDLK_KP_MINUS:
        case SDLK_MINUS:
            new_btn = BUTTON_MINUS;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        case SDLK_SPACE:
        case SDLK_INSERT:
            new_btn = BUTTON_MENU;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_DELETE,     51, 14, 17, "Power" },
    { SDLK_h,         138, 14, 16, "Hold" },
    { SDLK_MINUS,     320, 14, 10, "Minus" },
    { SDLK_INSERT,    347, 13, 13, "Menu" },
    { SDLK_KP_PLUS,   374, 14, 12, "Plus" },
    { 0, 0, 0, 0, "None" }
};
