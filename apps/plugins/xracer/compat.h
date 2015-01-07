/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#if (LCD_DEPTH < 4)
#include "lib/grey.h"
#else
#include "lib/xlcd.h"
#endif

/* this file contains some #defines to make this as platform-neutral as possible */
#if (LCD_DEPTH < 4)
/* greylib API */

#define FILL_TRI(x1, y1, x2, y2, x3, y3) grey_filltriangle(x1, y1, x2, y2, x3, y3)
#define FILL_RECT(x, y, w, h) grey_fillrect(x, y, w, h)
#define DRAW_HLINE(x1, x2, y) grey_hline(x1, x2, y)
#define LCD_RGBPACK(r, g, b) GREY_BRIGHTNESS(((r+g+b)/3))
#define CLEAR_DISPLAY() grey_clear_display()
#define SET_FOREGROUND(x) grey_set_foreground(x)
#define SET_BACKGROUND(x) grey_set_background(x)
#undef LCD_BLACK
#undef LCD_WHITE
#define LCD_BLACK GREY_BLACK
#define LCD_WHITE GREY_WHITE
#define LCD_UPDATE() grey_update()
#define PUTSXY(x, y, s) grey_putsxy(x, y, s)
#define RGB_UNPACK_RED(x) (x)
#define RGB_UNPACK_GREEN(x) (x)
#define RGB_UNPACK_BLUE(x) (x)

#else
/* rockbox API */

#define FILL_TRI(x1, y1, x2, y2, x3, y3) xlcd_filltriangle(x1, y1, x2, y2, x3, y3)
#define FILL_RECT(x, y, w, h) rb->lcd_fillrect(x, y, w, h)
#define DRAW_HLINE(x1, x2, y) rb->lcd_hline(x1, x2, y)
#define CLEAR_DISPLAY() rb->lcd_clear_display()
#define SET_FOREGROUND(x) rb->lcd_set_foreground(x)
#define SET_BACKGROUND(x) rb->lcd_set_background(x)
#define LCD_UPDATE() rb->lcd_update()
#define PUTSXY(x, y, s) rb->lcd_putsxy(x, y, s)

#endif
