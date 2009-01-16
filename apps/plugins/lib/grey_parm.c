/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* New greyscale framework
* Parameter handling
*
* This is a generic framework to display 129 shades of grey on low-depth
* bitmap LCDs (Archos b&w, Iriver & Ipod 4-grey) within plugins.
*
* Copyright (C) 2008 Jens Arnold
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

#include "plugin.h"
#include "grey.h"

/* Set position of the top left corner of the greyscale overlay
   Note that depending on the target LCD, either x or y gets rounded
   to the nearest multiple of 4 or 8 */
void grey_set_position(int x, int y)
{
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
    _grey_info.bx = (x + 4) >> 3;
    x = 8 * _grey_info.bx;
#else /* vertical packing or vertical interleaved */
#if (LCD_DEPTH == 1) || (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
    _grey_info.by = (y + 4) >> 3;
    y = 8 * _grey_info.by;
#elif LCD_DEPTH == 2
    _grey_info.by = (y + 2) >> 2;
    y = 4 * _grey_info.by;
#endif
#endif /* LCD_PIXELFORMAT */
    _grey_info.x = x;
    _grey_info.y = y;
    
    if (_grey_info.flags & _GREY_RUNNING)
    {
#ifdef SIMULATOR
        rb->sim_lcd_ex_update_rect(_grey_info.x, _grey_info.y,
                                              _grey_info.width,
                                              _grey_info.height);
        grey_deferred_lcd_update();
#else
        _grey_info.flags |= _GREY_DEFERRED_UPDATE;
#endif
    }
}

/* Set the draw mode for subsequent drawing operations */
void grey_set_drawmode(int mode)
{
    _grey_info.drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

/* Return the current draw mode */
int  grey_get_drawmode(void)
{
    return _grey_info.drawmode;
}

/* Set the foreground shade for subsequent drawing operations */
void grey_set_foreground(unsigned brightness)
{
    _grey_info.fg_brightness = brightness;
}

/* Return the current foreground shade */
unsigned grey_get_foreground(void)
{
    return _grey_info.fg_brightness;
}

/* Set the background shade for subsequent drawing operations */
void grey_set_background(unsigned brightness)
{
    _grey_info.bg_brightness = brightness;
}

/* Return the current background shade */
unsigned grey_get_background(void)
{
    return _grey_info.bg_brightness;
}

/* Set draw mode, foreground and background shades at once */
void grey_set_drawinfo(int mode, unsigned fg_brightness, unsigned bg_brightness)
{
    grey_set_drawmode(mode);
    grey_set_foreground(fg_brightness);
    grey_set_background(bg_brightness);
}

/* Set font for the text output routines */
void grey_setfont(int newfont)
{
    _grey_info.curfont = newfont;
}

/* Get width and height of a text when printed with the current font */
int  grey_getstringsize(const unsigned char *str, int *w, int *h)
{
    return rb->font_getstringsize(str, w, h, _grey_info.curfont);
}
