/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Thomas Martitz
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


#ifndef _BUTTON_SDL_H_
#define _BUTTON_SDL_H_

#include <stdbool.h>
#include "config.h"
#include "button-target.h"

#undef HAVE_LCD_FLIP

#undef button_init_device
#define button_init_device()

struct button_map {
    int button, x, y, radius;
    char *description;
};

int  xy2button( int x, int y);
bool button_hold(void);
void button_init_sdl(void);
#undef button_init_device
#define button_init_device() button_init_sdl()

#endif
