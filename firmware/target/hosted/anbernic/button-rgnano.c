/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Hairo R. Carela
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************9*************************/


#include <stdio.h>
#include <SDL.h>
#include "button.h"
#include "button-target.h"

int key_to_button(int keyboard_key)
{
    int new_btn = BUTTON_NONE;
    switch (keyboard_key)
    {
        case SDLK_s:
            new_btn = BUTTON_START;
            break;
        case SDLK_k:
            new_btn = BUTTON_FN;
            break;
        case SDLK_a:
            new_btn = BUTTON_A;
            break;
        case SDLK_b:
            new_btn = BUTTON_B;
            break;
        case SDLK_x:
            new_btn = BUTTON_X;
            break;
        case SDLK_y:
            new_btn = BUTTON_Y;
            break;
        case SDLK_m:
            new_btn = BUTTON_L;
            break;
        case SDLK_n:
            new_btn = BUTTON_R;
            break;
        case SDLK_u:
            new_btn = BUTTON_UP;
            break;
        case SDLK_d:
            new_btn = BUTTON_DOWN;
            break;
        case SDLK_l:
            new_btn = BUTTON_LEFT;
            break;
        case SDLK_r:
            new_btn = BUTTON_RIGHT;
            break;
        default:
            break;
    }
    return new_btn;
}
