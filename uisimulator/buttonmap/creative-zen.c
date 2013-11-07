/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
        case SDLK_KP9:
        case SDLK_PAGEUP:
            new_btn = BUTTON_MENU;
            break;
        case SDLK_KP1:
        case SDLK_END:
            new_btn = BUTTON_SHORTCUT;
            break;
        case SDLK_KP3:
        case SDLK_PAGEDOWN:
            new_btn = BUTTON_PLAYPAUSE;
            break;
        case SDLK_ESCAPE:
        case SDLK_DELETE:
            new_btn = BUTTON_POWER;
            break;
#ifdef CREATIVE_ZENMOZAIC
        case SDLK_KP_PLUS:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_KP_MINUS:
            new_btn = BUTTON_VOL_DOWN;
            break;
#endif
        case SDLK_KP7:
        case SDLK_HOME:
        case SDLK_BACKSPACE:
            new_btn = BUTTON_BACK;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        case SDLK_KP5:
            new_btn = BUTTON_SELECT;
            break;
    }
    return new_btn;
}

#if defined(CREATIVE_ZEN)
struct button_map bm[] = {
    { SDLK_LEFT,        388, 170, 14, "Left" },
    { SDLK_RIGHT,       481, 170, 14, "Right" },
    { SDLK_UP,          435, 123, 14, "Up" },
    { SDLK_DOWN,        435, 216, 14, "Down" },
    { SDLK_RETURN,      435, 170, 20, "Select" },
    { SDLK_HOME,        406,  61, 20, "Back" },
    { SDLK_PAGEUP,      462,  61, 20, "Menu" },
    { SDLK_DELETE,      519, 170, 20, "Power" },
    { SDLK_END,         406, 275, 20, "Shortcut" },
    { SDLK_PAGEDOWN,    462, 275, 20, "Play" },
    { 0, 0, 0, 0, "None" }
};
#else
#error please define button map
#endif
 
 
