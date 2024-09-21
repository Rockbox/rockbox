/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Thomas Martitz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************9*************************/


#include <stdio.h>
#include <SDL.h>
#include "button.h"
#include "buttonmap.h"

#ifndef HAS_BUTTON_HOLD
void touchscreen_enable_device(bool en)
{
    (void)en;
}
#endif

int key_to_button(int keyboard_key)
{
    int new_btn = BUTTON_NONE;
    switch (keyboard_key)
    {
#if (CONFIG_PLATFORM & PLATFORM_MAEMO4)
        case SDLK_ESCAPE:
#endif
        case SDLK_KP_7:
            new_btn = BUTTON_TOPLEFT;
            break;
        case SDLK_KP_8:
        case SDLK_UP:
            new_btn = BUTTON_TOPMIDDLE;
            break;
#if (CONFIG_PLATFORM & PLATFORM_MAEMO4)
        case SDLK_F7:
#endif
        case SDLK_KP_9:
            new_btn = BUTTON_TOPRIGHT;
            break;
#if (CONFIG_PLATFORM & PLATFORM_PANDORA)
        case SDLK_RSHIFT:
#endif
        case SDLK_KP_4:
        case SDLK_LEFT:
            new_btn = BUTTON_MIDLEFT;
            break;
#if (CONFIG_PLATFORM & PLATFORM_MAEMO|PLATFORM_PANDORA)
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
#endif
        case SDLK_KP_5:
            new_btn = BUTTON_CENTER;
            break;
#if (CONFIG_PLATFORM & PLATFORM_PANDORA)
        case SDLK_RCTRL:
#endif
        case SDLK_KP_6:
        case SDLK_RIGHT:
            new_btn = BUTTON_MIDRIGHT;
            break;
#if (CONFIG_PLATFORM & PLATFORM_MAEMO4)
        case SDLK_F6:
#endif
        case SDLK_KP_1:
            new_btn = BUTTON_BOTTOMLEFT;
            break;
        case SDLK_KP_2:
        case SDLK_DOWN:
            new_btn = BUTTON_BOTTOMMIDDLE;
            break;
#if (CONFIG_PLATFORM & PLATFORM_MAEMO4)
        case SDLK_F8:
#endif
        case SDLK_KP_3:
            new_btn = BUTTON_BOTTOMRIGHT;
            break;
        case SDL_BUTTON_RIGHT:
            new_btn = BUTTON_MIDLEFT;
            break;
        case SDL_BUTTON_MIDDLE:
            new_btn = BUTTON_MIDRIGHT;
            break;
        default:
            break;
    }
    return new_btn;
}
