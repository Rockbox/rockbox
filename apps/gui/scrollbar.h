/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Kevin Ferrare
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

#ifndef _GUI_SCROLLBAR_H_
#define _GUI_SCROLLBAR_H_
#include "screen_access.h"

#ifdef HAVE_LCD_BITMAP

enum orientation {
    VERTICAL          = 0x0000,   /* Vertical orientation     */
    HORIZONTAL        = 0x0001,   /* Horizontal orientation   */
#ifdef HAVE_LCD_COLOR
    FOREGROUND        = 0x0002,   /* Do not clear background pixels */
    INNER_FILL        = 0x0004,   /* Fill inner part even if FOREGROUND */
    INNER_BGFILL      = 0x0008,   /* Fill inner part with background
                                     color even if FOREGROUND */
    INNER_FILL_MASK   = 0x000c,
#endif
};

/*
 * Draws a scrollbar on the given screen
 *  - screen : the screen to put the scrollbar on
 *  - x : x start position of the scrollbar
 *  - y : y start position of the scrollbar
 *  - width : you won't guess =(^o^)=
 *  - height : I won't tell you either !
 *  - items : total number of items on the screen
 *  - min_shown : index of the starting item on the screen
 *  - max_shown : index of the last item on the screen
 *  - orientation : either VERTICAL or HORIZONTAL
 */
extern void gui_scrollbar_draw(struct screen * screen, int x, int y,
                               int width, int height, int items,
                               int min_shown, int max_shown,
                               unsigned flags);
extern void gui_bitmap_scrollbar_draw(struct screen * screen, struct bitmap bm, int x, int y,
                            int width, int height, int items,
                            int min_shown, int max_shown,
                            unsigned flags);
extern void show_busy_slider(struct screen *s, int x, int y, int width, int height);
#endif /* HAVE_LCD_BITMAP */
#endif /* _GUI_SCROLLBAR_H_ */
