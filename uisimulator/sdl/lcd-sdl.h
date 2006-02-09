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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
extern int display_zoom;

void sdl_update_rect(SDL_Surface *surface, int x_start, int y_start, int width,
    int height, int max_x, int max_y, int ui_x, int ui_y,
    Uint32 (*getpixel)(int, int));

void sdl_set_gradient(SDL_Surface *surface, SDL_Color *start, SDL_Color *end,
    int steps);

#endif // #ifndef __LCDSDL_H__

