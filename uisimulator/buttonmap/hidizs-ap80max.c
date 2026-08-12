/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2021 by Solomon Peachy
 * Copyright (C) 2026 by Jerry Shen
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
        case SDLK_UP:
            return BUTTON_SCROLL_BACK;
        case SDLK_DOWN:
            return BUTTON_SCROLL_FWD;
        case SDLK_LEFT:
            return BUTTON_PREV;
        case SDLK_RIGHT:
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
    { SDLK_ESCAPE, 525, 270, 35, "Power" },
    { SDLK_LEFT,   526, 432, 25, "Previous" },
    { SDLK_SPACE,  526, 515, 25, "Play" },
    { SDLK_RIGHT,  526, 598, 25, "Next" },
    { SDLK_UP,  525, 230, 35, "Scroll Fwd" },
    { SDLK_DOWN,  525, 310, 35, "Scroll Back" },
    { 0, 0, 0, 0, "None" }
};
