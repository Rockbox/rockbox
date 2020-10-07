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

/* Default greylib viewport struct */
struct viewport _grey_default_vp;

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
    _grey_info.vp->drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

/* Return the current draw mode */
int  grey_get_drawmode(void)
{
    return _grey_info.vp->drawmode;
}

/* Set the foreground shade for subsequent drawing operations */
void grey_set_foreground(unsigned brightness)
{
    _grey_info.vp->fg_pattern = brightness;
}

/* Return the current foreground shade */
unsigned grey_get_foreground(void)
{
    return _grey_info.vp->fg_pattern;
}

/* Set the background shade for subsequent drawing operations */
void grey_set_background(unsigned brightness)
{
    _grey_info.vp->bg_pattern = brightness;
}

/* Return the current background shade */
unsigned grey_get_background(void)
{
    return _grey_info.vp->bg_pattern;
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
    _grey_info.vp->font = newfont;
}

/* Get width and height of a text when printed with the current font */
int grey_getstringsize(const unsigned char *str, int *w, int *h)
{
    return rb->font_getstringsize(str, w, h, _grey_info.vp->font);
}

/* Helper to establish visible area between viewport and framebuffer */
static void grey_update_clip_rect(void)
{
    if (!(_grey_info.flags & GREY_BUFFERED))
        return; /* no chunky buffer */

    struct viewport *vp = _grey_info.vp;

    if (!vp || !_grey_info.curbuffer)
        return;

    /* Get overall intersection of framebuffer and viewport in viewport
       coordinates so that later clipping of drawing is kept as simple as
       possible. If l <= r and/or b <= t after intersecting, draw routines
       will see this as an empty area. */
    _grey_info.clip_l = _grey_info.cb_x - vp->x;
    _grey_info.clip_t = _grey_info.cb_y - vp->y;
    _grey_info.clip_r = _grey_info.clip_l + _grey_info.cb_width;
    _grey_info.clip_b = _grey_info.clip_t + _grey_info.cb_height;

    if (_grey_info.clip_l < 0)
        _grey_info.clip_l = 0;

    if (_grey_info.clip_t < 0)
        _grey_info.clip_t = 0;

    if (_grey_info.clip_r > vp->width)
        _grey_info.clip_r = vp->width;

    if (_grey_info.clip_b > vp->height)
        _grey_info.clip_b = vp->height;
}

/* Set current grey viewport for draw routines */
struct viewport *grey_set_viewport(struct viewport *vp)
{
    struct viewport *last_vp = _grey_info.vp;
    if (vp == NULL)
        vp = &_grey_default_vp;

    if (_grey_info.vp != vp)
    {
        _grey_info.vp = vp;
        grey_update_clip_rect();
    }
    return last_vp; 
}

/* Set viewport to default settings */
void grey_viewport_set_fullscreen(struct viewport *vp,
                                  const enum screen_type screen)
{
    if (vp == NULL)
        vp = &_grey_default_vp;

    vp->x = 0;
    vp->y = 0;
    vp->width = _grey_info.width;
    vp->height = _grey_info.height;
    vp->fg_pattern = 0;
    vp->bg_pattern = 255;
    vp->drawmode = DRMODE_SOLID;
    vp->font = FONT_SYSFIXED;

    if (vp == _grey_info.vp)
        grey_update_clip_rect(); /* is current one in use */

    (void)screen;
}

void grey_viewport_set_pos(struct viewport *vp,
                           int x, int y, int width, int height)
{
    if (vp == NULL || vp == &_grey_default_vp)
        return; /* Cannot be moved or resized */

    if (width < 0)
        width = 0; /* 'tis okay */

    if (height < 0)
        height = 0;

    vp->x = x;
    vp->y = y;
    vp->width = width;
    vp->height = height;

    if (vp == _grey_info.vp)
        grey_update_clip_rect(); /* is current one in use */
}

/* Set current grey chunky buffer for draw routines */
void grey_set_framebuffer(unsigned char *buffer)
{
    if (!(_grey_info.flags & GREY_BUFFERED))
        return; /* no chunky buffer */

    if (buffer == NULL)
        buffer = _grey_info.buffer; /* Default */

    if (buffer != _grey_info.curbuffer)
    {
        _grey_info.curbuffer = buffer;

        if (buffer == _grey_info.buffer)
        {
            /* Setting to default fb resets dimensions */
            grey_framebuffer_set_pos(0, 0, 0, 0);
        }
    }
}

/* Specify the dimensions of the current framebuffer */
void grey_framebuffer_set_pos(int x, int y, int width, int height)
{
    if (!(_grey_info.flags & GREY_BUFFERED))
        return; /* no chunky buffer */

    if (_grey_info.curbuffer == _grey_info.buffer)
    {
        /* This cannot be moved or resized */
        x = 0;
        y = 0;
        width = _grey_info.width;
        height = _grey_info.height;
    }
    else if (width <= 0 || height <= 0)
        return;

    if (x == _grey_info.cb_x && y == _grey_info.cb_y &&
        width == _grey_info.cb_width && height == _grey_info.cb_height)
        return; /* No change */

    _grey_info.cb_x = x;
    _grey_info.cb_y = y;
    _grey_info.cb_width = width;
    _grey_info.cb_height = height;

    grey_update_clip_rect();    
}
