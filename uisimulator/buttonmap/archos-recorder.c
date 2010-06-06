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
        case SDLK_KP4:
        case SDLK_LEFT:
            new_btn = BUTTON_LEFT;
            break;
        case SDLK_KP6:
        case SDLK_RIGHT:
            new_btn = BUTTON_RIGHT;
            break;
        case SDLK_KP8:
        case SDLK_UP:
            new_btn = BUTTON_UP;
            break;
        case SDLK_KP2:
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
            new_btn = BUTTON_F1;
            break;
        case SDLK_KP_MULTIPLY:
        case SDLK_F2:
            new_btn = BUTTON_F2;
            break;
        case SDLK_KP_MINUS:
        case SDLK_F3:
            new_btn = BUTTON_F3;
            break;
        case SDLK_KP5:
        case SDLK_SPACE:
            new_btn = BUTTON_PLAY;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
#if defined (ARCHOS_RECORDER)
    { SDLK_F1,        94, 205, 22, "F1" },
    { SDLK_F2,       136, 204, 21, "F2" },
    { SDLK_F3,       174, 204, 24, "F3" },
    { SDLK_KP_PLUS,   75, 258, 19, "On" },
    { SDLK_KP_ENTER,  76, 307, 15, "Off" },
    { SDLK_KP5,      151, 290, 20, "Select" },
    { SDLK_KP8,      152, 251, 23, "Up" },
    { SDLK_KP4,      113, 288, 26, "Left" },
    { SDLK_KP6,      189, 291, 23, "Right" },
    { SDLK_KP2,      150, 327, 27, "Down" },
#elif defined (ARCHOS_FMRECORDER) || defined (ARCHOS_RECORDERV2)
    { SDLK_F1,         88, 210, 28, "F1" },
    { SDLK_F2,        144, 212, 28, "F2" },
    { SDLK_F3,        197, 212, 28, "F3" },
    { SDLK_KP5,       144, 287, 21, "Select" },
    { SDLK_KP_PLUS,    86, 320, 13, "Menu" },
    { SDLK_KP_ENTER,  114, 347, 13, "Stop" },
    { SDLK_y,         144, 288, 31, "None" },
    { SDLK_KP8,       144, 259, 25, "Up" },
    { SDLK_KP2,       144, 316, 31, "Down" },
    { SDLK_KP6,       171, 287, 32, "Right" },
#endif
    { SDLK_KP4,       117, 287, 31, "Left" },
    { 0, 0, 0, 0, "None" }
};
