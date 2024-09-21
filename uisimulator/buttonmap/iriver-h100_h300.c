/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
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
#include "config.h"
#include "button.h"
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
            new_btn = BUTTON_ON;
            break;
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        case SDLK_a:
            new_btn = BUTTON_OFF;
            break;
        case SDLK_KP_DIVIDE:
        case SDLK_F1:
            new_btn = BUTTON_REC;
            break;
        case SDLK_KP_5:
        case SDLK_SPACE:
            new_btn = BUTTON_SELECT;
            break;
        case SDLK_KP_PERIOD:
        case SDLK_INSERT:
            new_btn = BUTTON_MODE;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {                         
#if defined (IRIVER_H120) || defined (IRIVER_H100)
    { SDLK_KP_DIVIDE,   46, 162, 13, "Record" },
    { SDLK_KP_PLUS,    327,  36, 16, "Play" },
    { SDLK_KP_ENTER,   330,  99, 18, "Stop" },
    { SDLK_KP_PERIOD,  330, 163, 18, "AB" },
    { SDLK_KP_5,        186, 227, 27, "5" },
    { SDLK_KP_8,        187, 185, 19, "8" },
    { SDLK_KP_4,        142, 229, 23, "4" },
    { SDLK_KP_6,        231, 229, 22, "6" },
    { SDLK_KP_2,        189, 272, 28, "2" },
/* Remote Buttons */
    { SDLK_KP_ENTER,   250, 404, 20, "Stop" },
    { SDLK_SPACE,      285, 439, 29, "Space" },
    { SDLK_h,          336, 291, 24, "Hold" },
#elif defined (IRIVER_H300)
    { SDLK_KP_PLUS,     56, 335, 20, "Play" },
    { SDLK_KP_8,        140, 304, 29, "Up" },
    { SDLK_KP_DIVIDE,  233, 331, 23, "Record" },
    { SDLK_KP_ENTER,    54, 381, 24, "Stop" },
    { SDLK_KP_4,        100, 353, 17, "Left" },
    { SDLK_KP_5,        140, 351, 19, "Navi" },
    { SDLK_KP_6,        185, 356, 19, "Right" },
    { SDLK_KP_PERIOD,  230, 380, 20, "AB" },
    { SDLK_KP_2,        142, 402, 24, "Down" },
    { SDLK_KP_ENTER,   211, 479, 21, "Stop" },
    { SDLK_KP_PLUS,    248, 513, 29, "Play" },
#endif
    { 0, 0, 0, 0, "None" }
};
