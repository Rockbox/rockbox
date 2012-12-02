/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Floating on-screen display
*
* Copyright (C) 2012 Michael Sevakis
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
#include "osd.h"

#if 1
#undef DEBUGF
#define DEBUGF(...)
#endif

/* At this time: assumes use of the default viewport for normal drawing */

/* If multiple OSD's are wanted, could convert to caller-allocated */
static struct osd
{
    enum osd_status
    {
        OSD_DISABLED = 0,       /* Disabled entirely */
        OSD_HIDDEN,             /* Hidden from view */
        OSD_VISIBLE,            /* Visible on screen */
        OSD_ERASED,             /* Erased in preparation for regular drawing */
    } status;                   /* View status */
    struct viewport vp;         /* Clipping viewport */
    struct bitmap lcd_bitmap;   /* The main LCD fb bitmap */
    struct bitmap back_bitmap;  /* The OSD backbuffer fb bitmap */
    int maxwidth;               /* How wide may it be at most? */
    int maxheight;              /* How high may it be at most? */
    long timeout;               /* Current popup stay duration */
    long hide_tick;             /* Tick when it should be hidden */
    osd_draw_cb_fn_t draw_cb;   /* Draw update callback */
} osd;

/* Framebuffer allocation macros */
#if LCD_DEPTH == 1
#  if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#    define LCD_WIDTH2BYTES(w)  (((w)+7)/8)
#    define LCD_BYTES2WIDTH(b)  ((b)*8)
#  elif LCD_PIXELFORMAT == VERTICAL_PACKING
#    define LCD_HEIGHT2BYTES(h) (((h)+7)/8)
#    define LCD_BYTES2HEIGHT(b) ((b)*8)
#  else
#    error Unknown 1-bit format; please define macros
#  endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 2
#  if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#    define LCD_WIDTH2BYTES(w)  (((w)+3)/4)
#    define LCD_BYTES2WIDTH(b)  ((b)*4)
#  elif LCD_PIXELFORMAT == VERTICAL_PACKING
#    define LCD_HEIGHT2BYTES(h) (((h)+3)/4)
#    define LCD_BYTES2HEIGHT(b) ((b)*4)
#  elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
#    define LCD_WIDTH2BYTES(w)  ((w)*2)
#    define LCD_BYTES2WIDTH(b)  ((b)/2)
#    define LCD_HEIGHT2BYTES(h) (((h)+7)/8*2)
#    define LCD_BYTES2HEIGHT(b) ((b)/2*8)
#  else
#    error Unknown 2-bit format; please define macros
#  endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 16
#  define LCD_WIDTH2BYTES(w)    ((w)*2)
#  define LCD_BYTES2WIDTH(b)    ((b)/2)
#else
#  error Unknown LCD depth; please define macros
#endif /* LCD_DEPTH */
/* Set defaults if not defined different yet. */
#ifndef LCD_WIDTH2BYTES
#  define LCD_WIDTH2BYTES(w)    (w)
#endif
#ifndef LCD_BYTES2WIDTH
#  define LCD_BYTES2WIDTH(b)    (b)
#endif
#ifndef LCD_HEIGHT2BYTES
#  define LCD_HEIGHT2BYTES(h)   (h)
#endif
#ifndef LCD_BYTES2HEIGHT
#  define LCD_BYTES2HEIGHT(b)   (b)
#endif

/* Create a bitmap framebuffer from a buffer */
static fb_data * buf_to_fb_bitmap(void *buf, size_t bufsize,
                                  int *width, int *height)
{
    /* Used as dest, the LCD functions cannot deal with alternate
       strides as of now - the stride guides the calulations. If
       that is no longer the case, then width or height can be
       used instead (and less memory needed for a small surface!).
     */
    DEBUGF("buf: %p bufsize: %lu\n", buf, (unsigned long)bufsize);

#if defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
    int h = LCD_BYTES2HEIGHT(LCD_HEIGHT2BYTES(LCD_HEIGHT));
    int w = bufsize / LCD_HEIGHT2BYTES(h);

    if (w == 0)
    {
        DEBUGF("OSD: not enough buffer\n");
        return NULL; /* not enough buffer */
    }
#else
    int w = LCD_BYTES2WIDTH(LCD_WIDTH2BYTES(LCD_WIDTH));
    int h = bufsize / LCD_WIDTH2BYTES(w);

    if (h == 0)
    {
        DEBUGF("OSD: not enough buffer\n");
        return NULL; /* not enough buffer */
    }
#endif

    DEBUGF("fbw:%d fbh:%d\n", w, h);

    *width = w;
    *height = h;
   
    return (fb_data *)buf;
}

static inline void osd_set_vp_pos(int x, int y, int width, int height)
{
    osd.vp.x = x;
    osd.vp.y = y;
    osd.vp.width = width;
    osd.vp.height = height;
}

/* Sync the backbuffer to the on-screen image */
static void osd_lcd_update_back_buffer(void)
{
    rb->lcd_set_framebuffer((fb_data *)osd.back_bitmap.data);
    rb->lcd_bmp_part(&osd.lcd_bitmap, osd.vp.x, osd.vp.y,
                     0, 0, osd.vp.width, osd.vp.height);
    /* Assume it was on default framebuffer for now */
    rb->lcd_set_framebuffer(NULL);
}

/* Erase the OSD to restore the framebuffer */
static void osd_lcd_erase(void)
{
    rb->lcd_bmp_part(&osd.back_bitmap, 0, 0, osd.vp.x, osd.vp.y,
                     osd.vp.width, osd.vp.height);
}

/* Draw the OSD image portion using the callback */
static void osd_lcd_draw_rect(int x, int y, int width, int height)
{
    rb->lcd_set_viewport(&osd.vp);
    osd.draw_cb(x, y, width, height);
    rb->lcd_set_viewport(NULL);
}

/* Draw the OSD image using the callback */
static void osd_lcd_draw(void)
{
    osd_lcd_draw_rect(0, 0, osd.vp.width, osd.vp.height);
}


/** Public APIs **/

/* Initialized the OSD and set its backbuffer */
bool osd_init(void *backbuf, size_t backbuf_size,
              osd_draw_cb_fn_t draw_cb)
{
    osd_show(OSD_HIDE);

    osd.status = OSD_DISABLED; /* Disabled unless all is okay */
    osd_set_vp_pos(0, 0, 0, 0);
    osd.maxwidth = osd.maxheight = 0;
    osd.timeout = 0;

    if (!draw_cb)
        return false;

    if (!backbuf)
        return false;

    ALIGN_BUFFER(backbuf, backbuf_size, FB_DATA_SZ);

    if (!backbuf_size)
        return false;

    rb->viewport_set_fullscreen(&osd.vp, SCREEN_MAIN);

    fb_data *backfb = buf_to_fb_bitmap(backbuf, backbuf_size,
                                       &osd.maxwidth, &osd.maxheight);

    if (!backfb)
        return false;

    osd.draw_cb = draw_cb;

    /* LCD framebuffer bitmap */
    osd.lcd_bitmap.width = LCD_BYTES2WIDTH(LCD_WIDTH2BYTES(LCD_WIDTH));
    osd.lcd_bitmap.height = LCD_BYTES2HEIGHT(LCD_HEIGHT2BYTES(LCD_HEIGHT));
#if LCD_DEPTH > 1
    osd.lcd_bitmap.format = FORMAT_NATIVE;
    osd.lcd_bitmap.maskdata = NULL;
#endif
#ifdef HAVE_LCD_COLOR
    osd.lcd_bitmap.alpha_offset = 0;
#endif
    osd.lcd_bitmap.data = (void *)rb->lcd_framebuffer;

    /* Backbuffer bitmap */
    osd.back_bitmap.width = osd.maxwidth;
    osd.back_bitmap.height = osd.maxheight;
#if LCD_DEPTH > 1
    osd.back_bitmap.format = FORMAT_NATIVE;
    osd.back_bitmap.maskdata = NULL;
#endif
#ifdef HAVE_LCD_COLOR
    osd.back_bitmap.alpha_offset = 0;
#endif
    osd.back_bitmap.data = (void *)backfb;

    DEBUGF("FB:%p BB:%p\n", osd.lcd_bitmap.data, osd.back_bitmap.data);

    /* Set the default position to the whole thing */
    osd_set_vp_pos(0, 0, osd.maxwidth, osd.maxheight);

    osd.status = OSD_HIDDEN; /* Ready when you are */
    return true;
}

/* Show/Hide the OSD on screen */
bool osd_show(unsigned flags)
{
    if (flags & OSD_SHOW)
    {
        switch (osd.status)
        {
        case OSD_DISABLED:
            break; /* No change */

        case OSD_HIDDEN:
            osd_lcd_update_back_buffer();
            osd.status = OSD_VISIBLE;
            osd_update();
            osd.hide_tick = *rb->current_tick + osd.timeout;
            break;

        case OSD_VISIBLE:
            if (flags & OSD_UPDATENOW)
                osd_update();
            /* Fall-through */
        case OSD_ERASED:
            osd.hide_tick = *rb->current_tick + osd.timeout;
            return true;
        }
    }
    else
    {
        switch (osd.status)
        {
        case OSD_DISABLED:
        case OSD_HIDDEN:
            break;

        case OSD_VISIBLE:
            osd_lcd_erase();
            rb->lcd_update_rect(osd.vp.x, osd.vp.y, osd.vp.width,
                                osd.vp.height);
        /* Fall-through */
        case OSD_ERASED:
            osd.status = OSD_HIDDEN;
            return true;
        }
    }

    return false;
}

/* Redraw the entire OSD */
bool osd_update(void)
{
    if (osd.status != OSD_VISIBLE)
        return false;

    osd_lcd_draw();

    rb->lcd_update_rect(osd.vp.x, osd.vp.y, osd.vp.width,
                        osd.vp.height);

    return true;
}

/* Redraw part of the OSD (viewport-relative coordinates) */
bool osd_update_rect(int x, int y, int width, int height)
{
    if (osd.status != OSD_VISIBLE)
        return false;

    osd_lcd_draw_rect(x, y, width, height);

    if (x + width > osd.vp.width)
        width = osd.vp.width - x;

    if (x < 0)
    {
        width += x;
        x = 0;
    }

    if (width <= 0)
        return false;

    if (y + height > osd.vp.height)
        height = osd.vp.height - y;

    if (y < 0)
    {
        height += y;
        y = 0;
    }

    if (height <= 0)
        return false;

    rb->lcd_update_rect(osd.vp.x + x, osd.vp.y + y, width, height);

    return true;
}

/* Set a new screen location and size (screen coordinates) */
bool osd_update_pos(int x, int y, int width, int height)
{
    if (osd.status == OSD_DISABLED)
        return false;

    if (width < 0)
        width = 0;
    else if (width > osd.maxwidth)
        width = osd.maxwidth;

    if (height < 0)
        height = 0;
    else if (height > osd.maxheight)
        height = osd.maxheight;

    if (x == osd.vp.x && y == osd.vp.y &&
        width == osd.vp.width && height == osd.vp.height)
        return false; /* No change */

    if (osd.status != OSD_VISIBLE)
    {
        /* Not visible - just update pos */
        osd_set_vp_pos(x, y, width, height);
        return false;
    }

    /* Visible area has changed */
    osd_lcd_erase();

    /* Update the smallest rectangle that encloses both the old and new
       regions to make the change free of flicker (they may overlap) */
    int xu = MIN(osd.vp.x, x);
    int yu = MIN(osd.vp.y, y);
    int wu = MAX(osd.vp.x + osd.vp.width, x + width) - xu;
    int hu = MAX(osd.vp.y + osd.vp.height, y + height) - yu;

    osd_set_vp_pos(x, y, width, height);
    osd_lcd_update_back_buffer();
    osd_lcd_draw();

    rb->lcd_update_rect(xu, yu, wu, hu);
    return true;
}

/* Call periodically to have the OSD timeout and hide itself */
void osd_monitor_timeout(void)
{
    if (osd.status <= OSD_HIDDEN)
        return; /* Already hidden/disabled */

    if (osd.timeout > 0 && TIME_AFTER(*rb->current_tick, osd.hide_tick))
        osd_show(OSD_HIDE);
}

/* Set the OSD timeout value. <= 0 = never timeout */
void osd_set_timeout(long timeout)
{
    if (osd.status == OSD_DISABLED)
        return;

    osd.timeout = timeout;
    osd_monitor_timeout();
}

/* Use the OSD viewport context */
struct viewport * osd_get_viewport(void)
{
    return &osd.vp;
}

/* Get the maximum dimensions calculated by osd_init() */
void osd_get_max_dims(int *maxwidth, int *maxheight)
{
    if (maxwidth)
        *maxwidth = osd.maxwidth;

    if (maxheight)
        *maxheight = osd.maxheight;
}

/* Is the OSD enabled? */
bool osd_enabled(void)
{
    return osd.status != OSD_DISABLED;
}


/** LCD update substitutes **/

/* Prepare LCD framebuffer for regular drawing */
void osd_lcd_update_prepare(void)
{
    if (osd.status == OSD_VISIBLE)
    {
        osd.status = OSD_ERASED;
        osd_lcd_erase();
    }
}

/* Update the whole screen */
void osd_lcd_update(void)
{
    if (osd.status == OSD_ERASED)
    {
        /* Save the screen image underneath and restore the OSD image */
        osd.status = OSD_VISIBLE;
        osd_lcd_update_back_buffer();
        osd_lcd_draw();
    }

    rb->lcd_update();
}

/* Update a part of the screen */
void osd_lcd_update_rect(int x, int y, int width, int height)
{
    if (osd.status == OSD_ERASED)
    {
        /* Save the screen image underneath and restore the OSD image */
        osd.status = OSD_VISIBLE;
        osd_lcd_update_back_buffer();
        osd_lcd_draw();
    }

    rb->lcd_update_rect(x, y, width, height);
}
