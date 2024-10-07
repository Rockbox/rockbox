/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Jonathan Gordon
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


#include <stdio.h>
#include <SDL.h>
#include "button.h"
#include "buttonmap.h"
#include "touchscreen.h"

int key_to_touch(int keyboard_button, unsigned int mouse_coords)
{
    int new_btn = BUTTON_NONE;
    switch (keyboard_button)
    {
        case BUTTON_TOUCHSCREEN:
            switch (touchscreen_get_mode())
            {
                case TOUCHSCREEN_POINT:
                    new_btn = BUTTON_TOUCHSCREEN;
                    break;
                case TOUCHSCREEN_BUTTON:
                {
                    static const int touchscreen_buttons[3][3] = {
                        {BUTTON_TOPLEFT, BUTTON_TOPMIDDLE, BUTTON_TOPRIGHT},
                        {BUTTON_MIDLEFT, BUTTON_CENTER, BUTTON_MIDRIGHT},
                        {BUTTON_BOTTOMLEFT, BUTTON_BOTTOMMIDDLE, BUTTON_BOTTOMRIGHT},
                    };
                    int px_x = ((mouse_coords&0xffff0000)>>16);
                    int px_y = ((mouse_coords&0x0000ffff));
                    new_btn = touchscreen_buttons[px_y/(LCD_HEIGHT/3)][px_x/(LCD_WIDTH/3)];
                    break;
                }
            }
            break;
#ifndef APPLICATION
        case SDLK_KP_7:
        case SDLK_7:
        case SDLK_HOME:
            new_btn = BUTTON_TOPLEFT;
            break;
        case SDLK_KP_8:
        case SDLK_8:
        case SDLK_UP:
#ifdef HAVE_SCROLLWHEEL
        case SDL_BUTTON_WHEELUP:
#endif
            new_btn = BUTTON_TOPMIDDLE;
            break;
        case SDLK_KP_9:
        case SDLK_9:
        case SDLK_PAGEUP:
            new_btn = BUTTON_TOPRIGHT;
            break;
        case SDLK_KP_4:
        case SDLK_u:
        case SDLK_LEFT:
            new_btn = BUTTON_MIDLEFT;
            break;
        case SDLK_KP_5:
        case SDLK_i:
        case SDL_BUTTON_MIDDLE:
            new_btn = BUTTON_CENTER;
            break;
        case SDLK_KP_6:
        case SDLK_o:
        case SDLK_RIGHT:
            new_btn = BUTTON_MIDRIGHT;
            break;
        case SDLK_KP_1:
        case SDLK_j:
        case SDLK_END:
            new_btn = BUTTON_BOTTOMLEFT;
            break;
        case SDLK_KP_2:
        case SDLK_k:
#ifdef HAVE_SCROLLWHEEL
        case SDL_BUTTON_WHEELDOWN:
#endif
        case SDLK_DOWN:
            new_btn = BUTTON_BOTTOMMIDDLE;
            break;
        case SDLK_KP_3:
        case SDLK_l:
        case SDLK_PAGEDOWN:
            new_btn = BUTTON_BOTTOMRIGHT;
            break;
#endif
    }
    return new_btn;

}
