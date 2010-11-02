/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Marcin Bukat
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
            new_btn = BUTTON_REW;
            break;
        case SDLK_DOWN:
            new_btn = BUTTON_FF;
            break;
        case SDLK_SPACE:
            new_btn = BUTTON_FUNC;
            break;
        case SDLK_RETURN:
        case SDLK_p:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_LEFT:
            new_btn = BUTTON_VOL_DOWN;
            break;
        case SDLK_RIGHT:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_ESCAPE:
        case SDLK_r:
            new_btn = BUTTON_REC;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_ESCAPE,  369, 257, 20, "Rec" },
    { SDLK_RETURN,  369, 305, 20, "Play/Stop" },
    { SDLK_UP,      353, 168,  10, "Rew" },
    { SDLK_DOWN,    353, 198,  10, "FF" },
    { SDLK_SPACE,   353, 186,  10, "Func" },
    { SDLK_LEFT,     123, 67, 20, "Vol Down" },
    { SDLK_RIGHT,     206, 67, 20, "Vol Up" },
    { SDLK_h,     369, 402, 30, "Hold" },
    { 0, 0, 0, 0, "None" }
};
