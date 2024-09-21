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
        case SDLK_PAGEUP:
            new_btn = BUTTON_MENU;
            break;
        case SDLK_END:
            new_btn = BUTTON_SHORTCUT;
            break;
        case SDLK_PAGEDOWN:
            new_btn = BUTTON_PLAYPAUSE;
            break;
        case SDLK_ESCAPE:
        case SDLK_DELETE:
            new_btn = BUTTON_POWER;
            break;
#ifdef CREATIVE_ZENXFI
        case SDLK_KP_1:
            new_btn = BUTTON_BOTTOMLEFT;
            break;
        case SDLK_KP_3:
            new_btn = BUTTON_BOTTOMRIGHT;
            break;
        case SDLK_KP_7:
            new_btn = BUTTON_TOPLEFT;
            break;
        case SDLK_KP_9:
            new_btn = BUTTON_TOPRIGHT;
            break;
#endif
        case SDLK_HOME:
        case SDLK_BACKSPACE:
            new_btn = BUTTON_BACK;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        case SDLK_KP_5:
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
#elif defined(CREATIVE_ZENXFI)
struct button_map bm[] = {
    { SDLK_LEFT,        386, 166, 13, "Left" },
    { SDLK_RIGHT,       466, 166, 13, "Right" },
    { SDLK_UP,          426, 126, 13, "Up" },
    { SDLK_DOWN,        426, 206, 13, "Down" },
    { SDLK_RETURN,      426, 166, 13, "Select" },
    { SDLK_KP_7,         386, 126, 13, "Top Left" },
    { SDLK_KP_9,         466, 126, 13, "Top Right" },
    { SDLK_KP_3,         466, 166, 13, "Bottom Right" },
    { SDLK_KP_1,         386, 166, 13, "Bottom Left" },
    { SDLK_HOME,        390,  63, 16, "Back" },
    { SDLK_PAGEUP,      463,  63, 16, "Menu" },
    { SDLK_END,         390, 267, 16, "Shortcut" },
    { SDLK_PAGEDOWN,    463, 267, 16, "Play" },
    { 0, 0, 0, 0, "None" }
};
#elif defined(CREATIVE_ZENMOZAIC)
struct button_map bm[] = {
    { SDLK_LEFT,         37, 281, 15, "Left" },
    { SDLK_RIGHT,       101, 281, 15, "Right" },
    { SDLK_UP,           69, 249, 15, "Up" },
    { SDLK_DOWN,         69, 313, 15, "Down" },
    { SDLK_RETURN,       69, 281, 15, "Select" },
    { SDLK_HOME,         37, 249, 15, "Back" },
    { SDLK_PAGEUP,      101, 249, 15, "Menu" },
    { SDLK_END,          37, 313, 15, "Shortcut" },
    { SDLK_PAGEDOWN,    101, 313, 15, "Play" },
    { 0, 0, 0, 0, "None" }
};
#elif defined(CREATIVE_ZENXFISTYLE)
struct button_map bm[] = {
    { SDLK_LEFT,        437, 157, 13, "Left" },
    { SDLK_RIGHT,       504, 157, 13, "Right" },
    { SDLK_UP,          471, 125, 13, "Up" },
    { SDLK_DOWN,        471, 192, 13, "Down" },
    { SDLK_RETURN,      471, 157, 25, "Select" },
    { SDLK_HOME,        447,  57, 15, "Back" },
    { SDLK_PAGEUP,      495,  57, 15, "Menu" },
    { SDLK_END,         447, 259, 15, "Shortcut" },
    { SDLK_PAGEDOWN,    495, 259, 15, "Play" },
    { 0, 0, 0, 0, "None" }
};
#else
#error please define button map
#endif
 
 
