/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Marcoen Hirschberg
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
        case SDLK_KP_1:
            new_btn = BUTTON_PREV;
            break;
        case SDLK_KP_3:
            new_btn = BUTTON_NEXT;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        case SDLK_a:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_KP_PERIOD:
        case SDLK_INSERT:
            new_btn = BUTTON_MENU;
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
        case SDLK_SPACE:
            new_btn = BUTTON_SELECT;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { 0, 0, 0, 0, "None" }
};
