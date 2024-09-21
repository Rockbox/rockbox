/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jens Arnold
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
            new_btn = BUTTON_LEFT;
            break;
        case SDLK_LEFT:
            new_btn = BUTTON_RC_REW;
            break;
        case SDLK_KP_6:
            new_btn = BUTTON_RIGHT;
            break;
        case SDLK_RIGHT:
            new_btn = BUTTON_RC_FF;
            break;
        case SDLK_KP_8:
            new_btn = BUTTON_VOL_UP;
            break;
        case SDLK_UP:
            new_btn = BUTTON_RC_VOL_UP;
            break;
        case SDLK_KP_2:
            new_btn = BUTTON_VOL_DOWN;
            break;
        case SDLK_DOWN:
            new_btn = BUTTON_RC_VOL_DOWN;
            break;
        case SDLK_KP_PERIOD:
            new_btn = BUTTON_MODE;
            break;
        case SDLK_INSERT:
            new_btn = BUTTON_RC_MODE;
            break;
        case SDLK_KP_DIVIDE:
            new_btn = BUTTON_REC;
            break;
        case SDLK_F1:
            new_btn = BUTTON_RC_REC;
            break;
        case SDLK_KP_5:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_SPACE:
            new_btn = BUTTON_RC_PLAY;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
            new_btn = BUTTON_RC_MENU;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
    { SDLK_KP_5,         256,  72, 29, "Play" },
    { SDLK_KP_6,         255, 137, 28, "Right" },
    { SDLK_KP_4,         257, 201, 26, "Left" },
    { SDLK_KP_8,         338,  31, 27, "Up" },
    { SDLK_KP_2,         339,  92, 23, "Down" },
    { SDLK_KP_PERIOD,   336,  50, 23, "Mode" },
    { SDLK_KP_DIVIDE,   336, 147, 23, "Rec" },
    { SDLK_h,           336, 212, 30, "Hold" },
    /* remote */
    { SDLK_SPACE,       115, 308, 20, "RC Play" },
    { SDLK_RIGHT,        85, 308, 20, "RC Rew" },
    { SDLK_LEFT,        143, 308, 20, "RC FF" },
    { SDLK_UP,          143, 498, 20, "RC Up" },
    { SDLK_DOWN,         85, 498, 20, "RC Down" },
    { SDLK_INSERT,      212, 308, 30, "RC Mode" },
    { SDLK_F1,          275, 308, 25, "RC Rec" },
    { SDLK_KP_ENTER,    115, 498, 20, "RC Menu" },
    { 0, 0, 0, 0, "None" }
};
