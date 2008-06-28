/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Bitmap graphics on player LCD!
*
* Copyright (C) 2005 Jens Arnold
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

#ifndef __PGFX_H__
#define __PGFX_H__

#include "plugin.h"

#ifdef HAVE_LCD_CHARCELLS /* Player only :) */

bool pgfx_init(const struct plugin_api* newrb, int cwidth, int cheight);
void pgfx_release(void);
void pgfx_display(int cx, int cy);
void pgfx_display_block(int cx, int cy, int x, int y);
void pgfx_update(void);

void pgfx_set_drawmode(int mode);
int  pgfx_get_drawmode(void);

void pgfx_clear_display(void);
void pgfx_drawpixel(int x, int y);
void pgfx_drawline(int x1, int y1, int x2, int y2);
void pgfx_hline(int x1, int x2, int y);
void pgfx_vline(int x, int y1, int y2);
void pgfx_drawrect(int x, int y, int width, int height);
void pgfx_fillrect(int x, int y, int width, int height);
void pgfx_bitmap_part(const unsigned char *src, int src_x, int src_y,
                      int stride, int x, int y, int width, int height);
void pgfx_bitmap(const unsigned char *src, int x, int y, int width, int height);

#define pgfx_mono_bitmap_part pgfx_bitmap_part
#define pgfx_mono_bitmap pgfx_bitmap

#endif /* HAVE_LCD_CHARCELLS */
#endif /* __PGFX_H__ */
