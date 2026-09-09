/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Michael McAllister
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
    switch (keyboard_button)
    {
        case SDLK_KP_PLUS:
        case SDLK_EQUALS:
            return BUTTON_VOL_UP;
        case SDLK_KP_MINUS:
        case SDLK_MINUS:
            return BUTTON_VOL_DOWN;
        case SDLK_n:
            return BUTTON_NEXT;
        case SDLK_RETURN:
        case SDLK_SPACE:
            return BUTTON_PLAY;
        case SDLK_ESCAPE:
            return BUTTON_POWER;
        default:
            return BUTTON_NONE;
    }
}

struct button_map bm[] = {
    { SDLK_ESCAPE, 628, 183, 35, "Power" },
    { SDLK_EQUALS, 628, 331, 35, "Volume +" },
    { SDLK_MINUS,  628, 425, 35, "Volume -" },
    { SDLK_SPACE,  628, 574, 35, "Play" },
    { SDLK_n,      628, 666, 35, "Next" },
    { 0, 0, 0, 0, "None" }
};
