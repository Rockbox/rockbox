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
        
        case SDLK_INSERT:
        case SDLK_KP_MULTIPLY:
            new_btn = BUTTON_HOME;
            break;
        case SDLK_SPACE:
        case SDLK_KP_5:
            new_btn = BUTTON_SELECT;
            break;
        case SDLK_PAGEDOWN:
        case SDLK_KP_3:
            new_btn = BUTTON_VOL_DOWN;
            break;
        case SDLK_PAGEUP:
        case SDLK_KP_9:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_ESCAPE:
        case SDLK_KP_1:
            new_btn = BUTTON_POWER;
            break;
    }
    return new_btn;
}

#ifdef SANSA_CLIPZIP
struct button_map bm[] = {
    { SDLK_KP_MULTIPLY,  31, 171,  12, "Home" },
    { SDLK_KP_5,          81, 211,  10, "Select" },
    { SDLK_KP_8,          81, 186,  13, "Play" },
    { SDLK_KP_4,          32, 211,  14, "Left" },
    { SDLK_KP_6,         112, 211,  14, "Right" },
    { SDLK_KP_2,          81, 231,  14, "Menu" },
    { 0, 0, 0, 0, "None" }
};
#else
struct button_map bm[] = {
    { SDLK_KP_MULTIPLY, 166, 158,  12, "Home" },
    { SDLK_KP_5,         101, 233,  19, "Select" },
    /* multiple entries allow rectangular regions */
    { SDLK_KP_8,         98, 194,  19, "Play" },
    { SDLK_KP_8,         104, 194,  19, "Play" },
    { SDLK_KP_4,          62, 230,  19, "Left" },
    { SDLK_KP_4,          62, 236,  19, "Left" },
    { SDLK_KP_6,         140, 230,  19, "Right" },
    { SDLK_KP_6,         140, 236,  19, "Right" },
    { SDLK_KP_2,         98, 272,  19, "Menu" },
    { SDLK_KP_2,         104, 272,  19, "Menu" },
    { 0, 0, 0, 0, "None" }
};
#endif
