/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Karl Kurbjun
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
        case SDLK_F9:
            new_btn = BUTTON_RC_HEART;
            break;
        case SDLK_F10:
            new_btn = BUTTON_RC_MODE;
            break;
        case SDLK_F11:
            new_btn = BUTTON_RC_VOL_DOWN;
            break;
        case SDLK_F12:
            new_btn = BUTTON_RC_VOL_UP;
            break;
        case SDLK_MINUS:
        case SDLK_LESS:
        case SDLK_LEFTBRACKET:
        case SDLK_KP_DIVIDE:
            new_btn = BUTTON_LEFT;
            break;
        case SDLK_PLUS:
        case SDLK_GREATER:
        case SDLK_RIGHTBRACKET:
        case SDLK_KP_MULTIPLY:
            new_btn = BUTTON_RIGHT;
            break;
        case SDLK_PAGEUP:
            new_btn = BUTTON_RC_PLAY;
            break;
        case SDLK_PAGEDOWN:
            new_btn = BUTTON_RC_DOWN;
            break;
        case SDLK_F8:
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP_9,     171, 609, 9, "Play" },
    { SDLK_KP_4,     158, 623, 9, "Left" },
    { SDLK_KP_6,     184, 622, 9, "Right" },
    { SDLK_KP_7,     171, 638, 11, "Menu" },
    { 0, 0, 0, 0, "None" }
};
