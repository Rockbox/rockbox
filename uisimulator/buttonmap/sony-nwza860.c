/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 by Amaury Pouly
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
        case SDLK_F1:
            new_btn = BUTTON_REW;
            break;
        case SDLK_F2:
            new_btn = BUTTON_FF;
            break;
        case SDLK_KP_PLUS:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_KP_MINUS:
            new_btn = BUTTON_VOL_DOWN;
            break;
        case SDLK_KP_1:
        case SDLK_HOME:
        case SDLK_BACKSPACE:
            new_btn = BUTTON_BACK;
            break;
        case SDLK_F3:
            new_btn = BUTTON_PLAY;
            break;
    }
    printf("btn: %d -> %x\n", keyboard_button, new_btn);
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_F1,          368, 490, 30, "Rewind" },
    { SDLK_F2,          368, 384, 30, "Fast Forward" },
    { SDLK_BACKSPACE,   197, 651, 50, "Home" },
    { SDLK_F3,          368, 435, 30, "Play" },
    { SDLK_KP_MINUS,    368, 166, 30, "Volume -" },
    { SDLK_KP_PLUS,     368, 226, 30, "Volume +" },
    { 0, 0, 0, 0, "None" }
};
