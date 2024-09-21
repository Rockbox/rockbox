/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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
        case SDLK_KP_PLUS:
        case SDLK_F8:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_ESCAPE:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        case SDLK_a:
            new_btn = BUTTON_POWER;
            break;
        case SDLK_KP_DIVIDE:
        case SDLK_F1:
            new_btn = BUTTON_REC;
            break;
        case SDLK_KP_5:
        case SDLK_SPACE:
            new_btn = BUTTON_SELECT;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
#if defined (IAUDIO_M5)
    { SDLK_KP_ENTER,  333,  41, 17, "Enter" },
    { SDLK_h,         334,  74, 21, "Hold" },
    { SDLK_KP_DIVIDE, 333, 142, 24, "Record" },
    { SDLK_KP_PLUS,   332, 213, 20, "Play" },
    { SDLK_KP_5,       250, 291, 19, "Select" },
    { SDLK_KP_8,       249, 236, 32, "Up" },
    { SDLK_KP_4,       194, 292, 29, "Left" },
    { SDLK_KP_6,       297, 290, 27, "Right" },
    { SDLK_KP_2,       252, 335, 28, "Down" },
#elif defined (IAUDIO_X5)
    { SDLK_KP_ENTER,  275,  38, 17, "Power" },
    { SDLK_h,         274,  83, 16, "Hold" },
    { SDLK_KP_DIVIDE, 276, 128, 22, "Record" },
    { SDLK_KP_PLUS,   274, 186, 22, "Play" },
    { SDLK_KP_5,       200, 247, 16, "Select" },
    { SDLK_KP_8,       200, 206, 16, "Up" },
    { SDLK_KP_4,       163, 248, 19, "Left" },
    { SDLK_KP_6,       225, 247, 24, "Right" },
    { SDLK_KP_2,       199, 279, 20, "Down" },
#endif
    { 0, 0, 0, 0, "None" }
};
