/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Vitja Makarov
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
        case SDLK_UP:
            new_btn = BUTTON_RIGHT;
            break;
        case SDLK_DOWN:
            new_btn = BUTTON_LEFT;
            break;
        case SDLK_LEFT:
            new_btn = BUTTON_STOP;
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_RIGHT:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_PLUS:
            new_btn = BUTTON_VOLUP;
            break;
        case SDLK_MINUS:
            new_btn = BUTTON_VOLDOWN;
            break;
        case SDLK_SPACE:
            new_btn = BUTTON_MENU;
            break;
        case SDLK_BACKSPACE:
            new_btn = BUTTON_POWER;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { 0, 0, 0, 0, "None" }
};
