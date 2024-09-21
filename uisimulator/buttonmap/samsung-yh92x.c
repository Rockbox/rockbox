/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Mark Arigo
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
            new_btn = BUTTON_UP;
            break;
        case SDLK_KP_2:
        case SDLK_DOWN:
            new_btn = BUTTON_DOWN;
            break;
        case SDLK_KP_5:
        case SDLK_KP_ENTER:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_KP_9:
        case SDLK_PAGEUP:
            new_btn = BUTTON_FFWD;
            break;
        case SDLK_KP_3:
        case SDLK_PAGEDOWN:
            new_btn = BUTTON_REW;
            break;
        case SDLK_KP_MINUS:
            new_btn = BUTTON_REC_SW_ON;
            break;
        case SDLK_KP_PLUS:
            new_btn = BUTTON_REC_SW_OFF;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {  
    { SDLK_KP_9,     370,  32, 15, "FF" },
    { SDLK_KP_5,     369,  84, 25, "Play" },
    { SDLK_KP_5,     367, 125, 27, "Play" },
    { SDLK_KP_3,     369, 188, 17, "Rew" },
    { SDLK_KP_MINUS,370, 315, 15, "Rec On" },
    { SDLK_KP_PLUS, 370, 347, 15, "Rec Off" },
    { SDLK_KP_4,     146, 252, 32, "Menu" },
    { SDLK_KP_8,     204, 226, 27, "Up" },
    { SDLK_KP_6,     257, 250, 34, "Select" },
    { SDLK_KP_2,     205, 294, 35, "Down" },
    { 0, 0, 0, 0, "None" }
};
