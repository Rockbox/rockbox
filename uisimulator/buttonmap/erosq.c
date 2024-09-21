/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2020 Solomon Peachy
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
#include "config.h"
#include "buttonmap.h"

int key_to_button(int keyboard_button)
{
    int new_btn = BUTTON_NONE;
    switch (keyboard_button)
    {
        case SDLK_KP_4:
        case SDLK_LEFT:
            new_btn = BUTTON_PREV;
            break;
        case SDLK_KP_6:
        case SDLK_RIGHT:
            new_btn = BUTTON_NEXT;
            break;
        case SDLK_KP_8:
        case SDLK_UP:
            new_btn = BUTTON_SCROLL_BACK;
            break;
        case SDLK_KP_2:
        case SDLK_DOWN:
            new_btn = BUTTON_SCROLL_FWD;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_BACKSPACE:
            new_btn = BUTTON_BACK;
            break;
        case SDLK_KP_PERIOD:
        case SDLK_INSERT:
            new_btn = BUTTON_MENU;
            break;
        case SDLK_KP_MINUS:
        case SDLK_PAGEUP:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_KP_PLUS:
        case SDLK_PAGEDOWN:
            new_btn = BUTTON_VOL_DOWN;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP_PERIOD,   60, 599, 25, "Menu" },
    { SDLK_KP_8,        213, 473, 20, "Scroll Back" },
    { SDLK_KP_2,         50, 473, 20, "Scroll Fwd" },
    { SDLK_KP_4,        323, 384, 30, "Prev" },
    { SDLK_KP_6,        323, 465, 30, "Next" },
    { SDLK_BACKSPACE,  323, 578, 25, "Back" },
    { SDLK_KP_PLUS,    397,  91, 30, "Vol Up" },
    { SDLK_KP_MINUS,   397, 185, 43, "Vol Dn" },
    { SDLK_KP_ENTER,   134, 473, 40, "Play" },
    { SDLK_ESCAPE,     338,  10, 25, "Power" },
    { 0, 0, 0 , 0, "None" }
};
