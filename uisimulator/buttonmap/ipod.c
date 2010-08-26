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
            new_btn = BUTTON_SCROLL_BACK;
            break;
        case SDLK_KP2:
        case SDLK_DOWN:
            new_btn = BUTTON_SCROLL_FWD;
            break;
        case SDLK_KP_PLUS:
        case SDLK_F8:
            new_btn = BUTTON_PLAY;
            break;
        case SDLK_KP5:
        case SDLK_SPACE:
            new_btn = BUTTON_SELECT;
            break;
        case SDLK_KP_PERIOD:
        case SDLK_INSERT:
            new_btn = BUTTON_MENU;
            break;
        case SDL_BUTTON_WHEELUP:
            new_btn = BUTTON_SCROLL_BACK;
            break;
        case SDL_BUTTON_WHEELDOWN:
            new_btn = BUTTON_SCROLL_FWD;
            break;
    }
    return new_btn;
}

struct button_map bm[] = {
#if defined (IPOD_VIDEO)
    { SDLK_KP_PERIOD,  174, 350, 35, "Menu" },
    { SDLK_KP8,        110, 380, 33, "Scroll Back" },
    { SDLK_KP2,        234, 377, 34, "Scroll Fwd" },
    { SDLK_KP4,         78, 438, 47, "Left" },
    { SDLK_KP5,        172, 435, 43, "Select" },
    { SDLK_KP6,        262, 438, 52, "Right" },
    { SDLK_KP_PLUS,    172, 519, 43, "Play" },
#elif defined (IPOD_MINI) || defined(IPOD_MINI2G)
    { SDLK_KP5,       92, 267, 29, "Select" },
    { SDLK_KP4,       31, 263, 37, "Left" },
    { SDLK_KP6,      150, 268, 33, "Right" },
    { SDLK_KP_PERIOD, 93, 209, 30, "Menu" },
    { SDLK_KP_PLUS,   93, 324, 25, "Play" },
    { SDLK_KP8,       53, 220, 29, "Scroll Back" },
    { SDLK_KP2,      134, 219, 31, "Scroll Fwd" },
#elif defined (IPOD_3G)
    { SDLK_KP5,       108, 296, 26, "Select" },
    { SDLK_KP8,        70, 255, 26, "Scroll Back" },
    { SDLK_KP2,       149, 256, 28, "Scroll Fwd" },
    { SDLK_KP4,        27, 186, 22, "Left" },
    { SDLK_KP_PERIOD,  82, 185, 22, "Menu" },
    { SDLK_KP_PERIOD, 133, 185, 21, "Play" },
    { SDLK_KP6,       189, 188, 21, "Right" },
#elif defined (IPOD_4G)
    { SDLK_KP5,        96, 269, 27, "Select" },
    { SDLK_KP4,        39, 267, 30, "Left" },
    { SDLK_KP6,       153, 270, 27, "Right" },
    { SDLK_KP_PERIOD,  96, 219, 30, "Menu" },
    { SDLK_KP_PLUS,    95, 326, 27, "Play" },
    { SDLK_KP8,        57, 233, 29, "Scroll Back" },
    { SDLK_KP2,       132, 226, 29, "Scroll Fwd" },
#elif defined (IPOD_COLOR) 
    { SDLK_KP5,        128, 362, 35, "Select" },
    { SDLK_KP4,         55, 358, 38, "Left" },
    { SDLK_KP6,        203, 359, 39, "Right" },
    { SDLK_KP_PERIOD,  128, 282, 34, "Menu" },
    { SDLK_KP_PLUS,    129, 439, 41, "Play" },
    { SDLK_KP8,         76, 309, 34, "Scroll Back" },
    { SDLK_KP2,        182, 311, 45, "Scroll Fwd" },
#elif defined (IPOD_1G2G) 
    { SDLK_KP5,       112, 265, 31, "Select" },
    { SDLK_KP8,        74, 224, 28, "Scroll Back" },
    { SDLK_KP2,       146, 228, 28, "Scroll Fwd" },
    /* Dummy button to make crescent shape */
    { SDLK_y,         112, 265, 76, "None" },
    { SDLK_KP8,        74, 224, 28, "Scroll Back" },
    { SDLK_KP2,       146, 228, 28, "Scroll Fwd" },
    { SDLK_KP6,       159, 268, 64, "Right" },
    { SDLK_KP4,        62, 266, 62, "Left" },
    { SDLK_KP_PERIOD, 111, 216, 64, "Menu" },
    { SDLK_KP_PLUS,   111, 326, 55, "Down" },
#elif defined (IPOD_NANO) 
    { SDLK_KP5,       98, 316, 37, "Select" },
    { SDLK_KP4,       37, 312, 28, "Left" },
    { SDLK_KP6,      160, 313, 25, "Right" },
    { SDLK_KP_PERIOD,102, 256, 23, "Menu" },
    { SDLK_KP_PLUS,   99, 378, 28, "Play" },
    { SDLK_KP8,       58, 272, 24, "Scroll Back" },
    { SDLK_KP2,      141, 274, 22, "Scroll Fwd" },
#elif defined (IPOD_NANO2G) 
    { SDLK_KP5,       118, 346, 37, "Select" },
    { SDLK_KP4,        51, 345, 28, "Left" },
    { SDLK_KP6,       180, 346, 26, "Right" },
    { SDLK_KP_PERIOD, 114, 286, 23, "Menu" },
    { SDLK_KP_PLUS,   115, 412, 27, "Down" },
    { SDLK_KP8,        67, 303, 28, "Scroll Back" },
    { SDLK_KP2,       163, 303, 27, "Scroll Fwd" },
#endif
    { 0, 0, 0 , 0, "None" }
};
