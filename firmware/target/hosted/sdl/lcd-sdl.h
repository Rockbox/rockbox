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

#ifndef __LCDSDL_H__
#define __LCDSDL_H__

#include "lcd.h"
#include "SDL.h"

/* Default display zoom level */
extern SDL_Surface *gui_surface;
extern SDL_Renderer *sdlRenderer;

void sdl_update_rect(SDL_Surface *surface, int x_start, int y_start, int width,
                     int height, int max_x, int max_y,
                     unsigned long (*getpixel)(int, int));

void sdl_gui_update(SDL_Surface *surface, int x_start, int y_start, int width,
                    int height, int max_x, int max_y, int ui_x, int ui_y);

void sdl_set_gradient(SDL_Surface *surface, SDL_Color *start, SDL_Color *end,
                      int first, int steps);

#endif /* #ifndef __LCDSDL_H__ */
