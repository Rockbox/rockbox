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
#include "grey.h"
#include "osd.h"

#if 1
#undef DEBUGF
#define DEBUGF(...)
#endif

#if defined(SIMULATOR) && LCD_DEPTH < 4
/* Sim isn't using --ffunction-sections thus greylib references will happen
   here even if not using this with greylib on a grayscale display, which
   demands that a struct _grey_info exist. */
#ifndef _WIN32
__attribute__((weak))
#endif /* _WIN32 */
    struct _grey_info _grey_info;
#endif /* defined(SIMULATOR) && LCD_DEPTH < 4 */

/* At this time: assumes use of the default viewport for normal drawing */

/* If multiple OSD's are wanted, could convert to caller-allocated */
struct osd
{
    enum osd_status
    {
        OSD_DISABLED = 0,       /* Disabled entirely */
        OSD_HIDDEN,             /* Hidden from view */
        OSD_VISIBLE,            /* Visible on screen */
        OSD_ERASED,             /* Erased in preparation for regular drawing */
    } status;                   /* View status */
    struct viewport vp;         /* Clipping viewport */
    struct frame_buffer_t framebuf; /* Holds framebuffer reference */
    int lcd_bitmap_stride;      /* Stride of LCD bitmap */
    void *lcd_bitmap_data;      /* Backbuffer framebuffer data */
    int back_bitmap_stride;     /* Stride of backbuffer bitmap */
    void *back_bitmap_data;     /* LCD framebuffer data */
    int maxwidth;               /* How wide may it be at most? */
    int maxheight;              /* How high may it be at most? */
    long timeout;               /* Current popup stay duration */
    long hide_tick;             /* Tick when it should be hidden */
    osd_draw_cb_fn_t draw_cb;   /* Draw update callback */
    /* Functions to factilitate interface compatibility of OSD types */
    void * (*init_buffers)(struct osd *osd, unsigned flags, void *buf,
                           size_t *bufsize);
    void (*set_viewport_pos)(struct viewport *vp, int x, int y, int width,
                             int height);
    void (*lcd_update)(void);
    void (*lcd_update_rect)(int x, int y, int width, int height);
    struct viewport *(*lcd_set_viewport)(struct viewport *vp);
    void (*lcd_set_framebuffer)(void *buf);
    void (*lcd_framebuffer_set_pos)(int x, int y, int width, int height);
    void (*lcd_bitmap_part)(const void *src, int src_x, int src_y,
                            int stride, int x, int y, int width, int height);
};

static struct osd native_osd;
#if LCD_DEPTH < 4
static struct osd grey_osd;
#endif

/* Framebuffer allocation macros */
#if LCD_DEPTH == 1
#  if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#    define _OSD_WIDTH2BYTES(w)  (((w)+7)/8)
#    define _OSD_BYTES2WIDTH(b)  ((b)*8)
#  elif LCD_PIXELFORMAT == VERTICAL_PACKING
#    define _OSD_HEIGHT2BYTES(h) (((h)+7)/8)
#    define _OSD_BYTES2HEIGHT(b) ((b)*8)
#  else
#    error Unknown 1-bit format; please define macros
#  endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 2
#  if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#    define _OSD_WIDTH2BYTES(w)  (((w)+3)/4)
#    define _OSD_BYTES2WIDTH(b)  ((b)*4)
#  elif LCD_PIXELFORMAT == VERTICAL_PACKING
#    define _OSD_HEIGHT2BYTES(h) (((h)+3)/4)
#    define _OSD_BYTES2HEIGHT(b) ((b)*4)
#  elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
#    define _OSD_HEIGHT2BYTES(h) (((h)+7)/8*2)
#    define _OSD_BYTES2HEIGHT(b) ((b)/2*8)
#  else
#    error Unknown 2-bit format; please define macros
#  endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 16
#  if defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
#    define _OSD_HEIGHT2BYTES(h)   ((h)*2)
#    define _OSD_BYTES2HEIGHT(b)   ((b)/2)
#  else /* !defined(LCD_STRIDEFORMAT) || LCD_STRIDEFORMAT != VERTICAL_STRIDE */
#    define _OSD_WIDTH2BYTES(w)    ((w)*2)
#    define _OSD_BYTES2WIDTH(b)    ((b)/2)
#  endif /* end stride type selection */
#elif LCD_DEPTH == 24
#    define _OSD_WIDTH2BYTES(w)    ((w)*3)
#    define _OSD_BYTES2WIDTH(b)    ((b)/3)
#elif LCD_DEPTH == 32
#    define _OSD_WIDTH2BYTES(w)    ((w)*4)
#    define _OSD_BYTES2WIDTH(b)    ((b)/4)
#else /* other LCD depth */
#  error Unknown LCD depth; please define macros
#endif /* LCD_DEPTH */
/* Set defaults if not defined differently */
#ifndef _OSD_WIDTH2BYTES
#  define _OSD_WIDTH2BYTES(w)    (w)
#endif
#ifndef _OSD_BYTES2WIDTH
#  define _OSD_BYTES2WIDTH(b)    (b)
#endif
#ifndef _OSD_HEIGHT2BYTES
#  define _OSD_HEIGHT2BYTES(h)   (h)
#endif
#ifndef _OSD_BYTES2HEIGHT
#  define _OSD_BYTES2HEIGHT(b)   (b)
#endif
#ifndef _OSD_BUFSIZE
#  define _OSD_BUFSIZE(w, h)     (_OSD_WIDTH2BYTES(w)*_OSD_HEIGHT2BYTES(h))
#endif

static void _osd_destroy(struct osd *osd);
static bool _osd_show(struct osd *osd, unsigned flags);


/** Native LCD routines **/

/* Create a bitmap framebuffer from a buffer */
static void * _osd_lcd_init_buffers(struct osd *osd, unsigned flags,
                                    void *buf, size_t *bufsize)
{
    /* Used as dest, the LCD functions cannot deal with alternate
       strides as of now - the stride guides the calulations. If
       that is no longer the case, then width or height can be
       used instead (and less memory needed for a small surface!).
       IOW: crappiness means one dimension is non-negotiable.
     */
    DEBUGF("OSD: in(buf=%p bufsize=%lu)\n", buf,
           (unsigned long)*bufsize);

    rb->viewport_set_fullscreen(&osd->vp, SCREEN_MAIN);

#if defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
    int colbytes = _OSD_HEIGHT2BYTES(LCD_HEIGHT);
    int bytecols = *bufsize / colbytes;
    int w = _OSD_BYTES2WIDTH(bytecols);
    int h = _OSD_BYTES2HEIGHT(colbytes);

    if (flags & OSD_INIT_MAJOR_HEIGHT)
    {
        if (w == 0 || ((flags & OSD_INIT_MINOR_MIN) && w < osd->maxwidth))
        {
            DEBUGF("OSD: not enough buffer\n");
            return NULL; /* not enough buffer */
        }

        if ((flags & OSD_INIT_MINOR_MAX) && w > osd->maxwidth)
            w = osd->maxwidth;
    }
    else /* OSD_INIT_MAJOR_WIDTH implied */
    {
        if (w == 0 || w < osd->maxwidth)
        {
            DEBUGF("OSD: not enough buffer\n");
            return NULL; /* not enough buffer */
        }
        else if (w > osd->maxwidth)
        {
            w = osd->maxwidth;
        }
    }

    w = _OSD_BYTES2WIDTH(_OSD_WIDTH2BYTES(w));
    osd->lcd_bitmap_stride = _OSD_BYTES2HEIGHT(_OSD_HEIGHT2BYTES(LCD_HEIGHT));
    osd->back_bitmap_stride = h;
#else /* !defined(LCD_STRIDEFORMAT) || LCD_STRIDEFORMAT != VERTICAL_STRIDE */
    int rowbytes = _OSD_WIDTH2BYTES(LCD_WIDTH);
    int byterows = *bufsize / rowbytes;
    int w = _OSD_BYTES2WIDTH(rowbytes);
    int h = _OSD_BYTES2HEIGHT(byterows);

    if (flags & OSD_INIT_MAJOR_HEIGHT)
    {
        if (h == 0 || h < osd->maxheight)
        {
            DEBUGF("OSD: not enough buffer\n");
            return NULL;
        }
        else if (h > osd->maxheight)
        {
            h = osd->maxheight;
        }
    }
    else /* OSD_INIT_MAJOR_WIDTH implied */
    {
        if (h == 0 || ((flags & OSD_INIT_MINOR_MIN) && h < osd->maxheight))
        {
            DEBUGF("OSD: not enough buffer\n");
            return NULL;
        }

        if ((flags & OSD_INIT_MINOR_MAX) && h > osd->maxheight)
            h = osd->maxheight;
    }

    h = _OSD_BYTES2HEIGHT(_OSD_HEIGHT2BYTES(h));
    osd->lcd_bitmap_stride = _OSD_BYTES2WIDTH(_OSD_WIDTH2BYTES(LCD_WIDTH));
    osd->back_bitmap_stride = w;
#endif /* end stride type selection */

    /* vp is currently initialized to the default framebuffer */
    osd->lcd_bitmap_data = osd->vp.buffer->data;
    osd->back_bitmap_data = buf;

    osd->maxwidth = w;
    osd->maxheight = h;
    *bufsize = _OSD_BUFSIZE(w, h);

    DEBUGF("OSD: addr(fb=%p bb=%p)\n", osd->lcd_bitmap_data,
           osd->back_bitmap_data);
    DEBUGF("OSD: w=%d h=%d bufsz=%lu\n", w, h, (unsigned long)*bufsize);

    return buf;
}

/* Set viewport coordinates */
static void _osd_lcd_viewport_set_pos(
    struct viewport *vp, int x, int y, int width, int height)
{
    vp->x = x;
    vp->y = y;
    vp->width = width;
    vp->height = height;
}


#if LCD_DEPTH < 4
/** Greylib LCD routines **/

/* Create a greylib bitmap framebuffer from a buffer */
static void * _osd_grey_init_buffers(struct osd *osd, unsigned flags,
                                     void *buf, size_t *bufsize)
{
    int w, h;

    DEBUGF("OSD (grey): in(buf=%p bufsize=%lu)\n", buf,
           (unsigned long)*bufsize);

    grey_viewport_set_fullscreen(&osd->vp, SCREEN_MAIN);

    if (flags & OSD_INIT_MAJOR_HEIGHT)
    {
        h = osd->maxheight;
        w = *bufsize / h;

        if (w == 0 || ((flags & OSD_INIT_MINOR_MIN) && w < osd->maxwidth))
        {
            DEBUGF("OSD (grey): Not enough buffer\n");
            return NULL;
        }

        if ((flags & OSD_INIT_MINOR_MAX) && w > osd->maxwidth)
            w = osd->maxwidth;
    }
    else /* OSD_INIT_MAJOR_WIDTH implied */
    {
        w = osd->maxwidth;
        h = *bufsize / w;

        if (h == 0 || ((flags & OSD_INIT_MINOR_MIN) && h < osd->maxheight))
        {
            DEBUGF("OSD (grey): Not enough buffer\n");
            return NULL;
        }

        if ((flags & OSD_INIT_MINOR_MAX) && h > osd->maxheight)
            h = osd->maxheight;
    }

    /* Have to peek into _grey_info a bit */
    osd->lcd_bitmap_stride = _grey_info.width;
    osd->lcd_bitmap_data = _grey_info.buffer;
    osd->back_bitmap_stride = w;
    osd->back_bitmap_data = buf;

    osd->maxwidth = w;
    osd->maxheight = h;
    *bufsize = w * h;

    DEBUGF("OSD (grey): addr(fb=%p bb=%p)\n", osd->lcd_bitmap_data,
           osd->back_bitmap_data);
    DEBUGF("OSD (grey): w=%d h=%d bufsz=%lu\n", w, h, (unsigned long)*bufsize);

    return buf;
}
#endif /* LCD_DEPTH < 4*/


/** Common LCD routines **/

/* Draw the OSD image portion using the callback */
static void _osd_draw_osd_rect(struct osd *osd, int x, int y,
                               int width, int height)
{
    osd->lcd_set_viewport(&osd->vp);
    osd->draw_cb(x, y, width, height);
    osd->lcd_set_viewport(NULL);
}

/* Draw the OSD image using the callback */
static void _osd_draw_osd(struct osd *osd)
{
    _osd_draw_osd_rect(osd, 0, 0, osd->vp.width, osd->vp.height);
}

static void _osd_update_viewport(struct osd *osd)
{
    osd->lcd_update_rect(osd->vp.x, osd->vp.y, osd->vp.width,
                         osd->vp.height);
}

/* Sync the backbuffer to the framebuffer image */
static void _osd_update_back_buffer(struct osd *osd)
{
    /* Assume it's starting with default viewport for now */
    osd->lcd_set_framebuffer(osd->back_bitmap_data);
#if LCD_DEPTH < 4
    if (osd->lcd_framebuffer_set_pos)
        osd->lcd_framebuffer_set_pos(0, 0, osd->maxwidth, osd->maxheight);
#endif /* LCD_DEPTH < 4 */
    osd->lcd_bitmap_part(osd->lcd_bitmap_data, osd->vp.x, osd->vp.y,
                         osd->lcd_bitmap_stride, 0, 0, osd->vp.width,
                         osd->vp.height);
    /* Assume it was on default framebuffer for now */
    osd->lcd_set_framebuffer(NULL);
}

/* Erase the OSD to restore the framebuffer image */
static void _osd_erase_osd(struct osd *osd)
{
    osd->lcd_bitmap_part(osd->back_bitmap_data, 0, 0, osd->back_bitmap_stride,
                         osd->vp.x, osd->vp.y, osd->vp.width, osd->vp.height);
}

/* Initialized the OSD and set its backbuffer */
static bool _osd_init(struct osd *osd, unsigned flags, void *backbuf,
                      size_t backbuf_size, osd_draw_cb_fn_t draw_cb,
                      int *width, int *height, size_t *bufused)
{
    _osd_destroy(osd);

    if (!draw_cb)
        return false;

    if (!backbuf)
        return false;

    void *backbuf_orig = backbuf; /* Save in case of ptr advance */
    ALIGN_BUFFER(backbuf, backbuf_size, MAX(FB_DATA_SZ, 4));

    if (!backbuf_size)
        return false;

    if (flags & OSD_INIT_MAJOR_HEIGHT)
    {
        if (!height || *height <= 0)
            return false;

        if ((flags & (OSD_INIT_MINOR_MIN | OSD_INIT_MINOR_MAX)) &&
            (!width || *width <= 0))
        {
            return false;
        }
    }
    else
    {
        if (!width || *width <= 0)
            return false;

        if ((flags & (OSD_INIT_MINOR_MIN | OSD_INIT_MINOR_MAX)) &&
            (!height || *height <= 0))
        {
            return false;
        }
    }

    /* Store requested sizes in max(width|height) */
    if (width)
        osd->maxwidth = *width;
    else
        osd->maxwidth = LCD_WIDTH;

    if (height)
        osd->maxheight = *height;
    else
        osd->maxheight = LCD_HEIGHT;

    if (!osd->init_buffers(osd, flags, backbuf, &backbuf_size))
    {
        osd->maxwidth = osd->maxheight = 0;
        return false;
    }

    osd->draw_cb = draw_cb;

    if (bufused)
        *bufused = backbuf_size + (backbuf_orig - backbuf);

    if (width)
        *width = osd->maxwidth;

    if (height)
        *height = osd->maxheight;

    /* Set the default position to the whole thing */
    osd->set_viewport_pos(&osd->vp, 0, 0, osd->maxwidth, osd->maxheight);

    osd->status = OSD_HIDDEN; /* Ready when you are */

    return true;
}

static void _osd_destroy(struct osd *osd)
{
    _osd_show(osd, OSD_HIDE);

    /* Set to essential defaults */
    osd->status = OSD_DISABLED;
    osd->set_viewport_pos(&osd->vp, 0, 0, 0, 0);
    osd->maxwidth = osd->maxheight = 0;
    osd->timeout = 0;
}

/* Redraw the entire OSD */
static bool _osd_update(struct osd *osd)
{
    if (osd->status != OSD_VISIBLE)
        return false;

    _osd_draw_osd(osd);
    _osd_update_viewport(osd);
    return true;
}

/* Show/Hide the OSD on screen */
static bool _osd_show(struct osd *osd, unsigned flags)
{
    if (flags & OSD_SHOW)
    {
        switch (osd->status)
        {
        case OSD_DISABLED:
            break; /* No change */

        case OSD_HIDDEN:
            _osd_update_back_buffer(osd);
            osd->status = OSD_VISIBLE;
            _osd_update(osd);
            osd->hide_tick = *rb->current_tick + osd->timeout;
            break;

        case OSD_VISIBLE:
            if (flags & OSD_UPDATENOW)
                _osd_update(osd);
            /* Fall-through */
        case OSD_ERASED:
            osd->hide_tick = *rb->current_tick + osd->timeout;
            return true;
        }
    }
    else
    {
        switch (osd->status)
        {
        case OSD_DISABLED:
        case OSD_HIDDEN:
            break;

        case OSD_VISIBLE:
            _osd_erase_osd(osd);
            _osd_update_viewport(osd);
        /* Fall-through */
        case OSD_ERASED:
            osd->status = OSD_HIDDEN;
            return true;
        }
    }

    return false;
}

/* Redraw part of the OSD (viewport-relative coordinates) */
static bool _osd_update_rect(struct osd *osd, int x, int y, int width,
                             int height)
{
    if (osd->status != OSD_VISIBLE)
        return false;

    _osd_draw_osd_rect(osd, x, y, width, height);

    int vp_x = osd->vp.x;
    int vp_w = osd->vp.width;

    if (x + width > vp_w)
        width = vp_w - x;

    if (x < 0)
    {
        width += x;
        x = 0;
    }

    if (width <= 0)
        return false;

    int vp_y = osd->vp.y;
    int vp_h = osd->vp.height;

    if (y + height > vp_h)
        height = vp_h - y;

    if (y < 0)
    {
        height += y;
        y = 0;
    }

    if (height <= 0)
        return false;

    osd->lcd_update_rect(vp_x + x, vp_y + y, width, height);

    return true;
}

/* Set a new screen location and size (screen coordinates) */
static bool _osd_update_pos(struct osd *osd, int x, int y, int width,
                            int height)
{
    if (osd->status == OSD_DISABLED)
        return false;

    if (width < 0)
        width = 0;
    else if (width > osd->maxwidth)
        width = osd->maxwidth;

    if (height < 0)
        height = 0;
    else if (height > osd->maxheight)
        height = osd->maxheight;

    int vp_x = osd->vp.x;
    int vp_y = osd->vp.y;
    int vp_w = osd->vp.width;
    int vp_h = osd->vp.height;

    if (x == vp_x && y == vp_y && width == vp_w && height == vp_h)
        return false; /* No change */

    if (osd->status != OSD_VISIBLE)
    {
        /* Not visible - just update pos */
        osd->set_viewport_pos(&osd->vp, x, y, width, height);
        return false;
    }

    /* Visible area has changed */
    _osd_erase_osd(osd);

    /* Update the smallest rectangle that encloses both the old and new
       regions to make the change free of flicker (they may overlap) */
    int xu = MIN(vp_x, x);
    int yu = MIN(vp_y, y);
    int wu = MAX(vp_x + vp_w, x + width) - xu;
    int hu = MAX(vp_y + vp_h, y + height) - yu;

    osd->set_viewport_pos(&osd->vp, x, y, width, height);
    _osd_update_back_buffer(osd);
    _osd_draw_osd(osd);
    osd->lcd_update_rect(xu, yu, wu, hu);

    return true;
}

/* Call periodically to have the OSD timeout and hide itself */
static void _osd_monitor_timeout(struct osd *osd)
{
    if (osd->status <= OSD_HIDDEN)
        return; /* Already hidden/disabled */

    if (osd->timeout > 0 && TIME_AFTER(*rb->current_tick, osd->hide_tick))
        _osd_show(osd, OSD_HIDE);
}

/* Set the OSD timeout value. <= 0 = never timeout */
static void _osd_set_timeout(struct osd *osd, long timeout)
{
    if (osd->status == OSD_DISABLED)
        return;

    osd->timeout = timeout;
    _osd_monitor_timeout(osd);
}

/* Use the OSD viewport context */
static inline struct viewport * _osd_get_viewport(struct osd *osd)
{
    return &osd->vp;
}

/* Get the maximum dimensions calculated by osd_init() */
static void _osd_get_max_dims(struct osd *osd,
                              int *maxwidth, int *maxheight)
{
    if (maxwidth)
        *maxwidth = osd->maxwidth;

    if (maxheight)
        *maxheight = osd->maxheight;
}

/* Is the OSD enabled? */
static inline bool _osd_enabled(struct osd *osd)
{
    return osd->status != OSD_DISABLED;
}


/** LCD update substitutes **/

/* Prepare LCD framebuffer for regular drawing */
static inline void _osd_lcd_update_prepare(struct osd *osd)
{
    if (osd->status == OSD_VISIBLE)
    {
        osd->status = OSD_ERASED;
        _osd_erase_osd(osd);
    }
}

/* Update the whole screen */
static inline void _osd_lcd_update(struct osd *osd)
{
    if (osd->status == OSD_ERASED)
    {
        /* Save the screen image underneath and restore the OSD image */
        osd->status = OSD_VISIBLE;
        _osd_update_back_buffer(osd);
        _osd_draw_osd(osd);
    }

    osd->lcd_update();
}

/* Update a part of the screen */
static void _osd_lcd_update_rect(struct osd *osd,
        int x, int y, int width, int height)
{
    if (osd->status == OSD_ERASED)
    {
        /* Save the screen image underneath and restore the OSD image */
        osd->status = OSD_VISIBLE;
        _osd_update_back_buffer(osd);
        _osd_draw_osd(osd);
    }

    osd->lcd_update_rect(x, y, width, height);
}

static void _osd_lcd_viewport_set_buffer(void *buffer)
{
    if (buffer)
    {
        native_osd.framebuf.data = buffer;
        native_osd.framebuf.elems = native_osd.maxheight * native_osd.maxwidth;
        native_osd.framebuf.get_address_fn = NULL; /*Default iterator*/

        if (buffer == native_osd.back_bitmap_data)
            native_osd.framebuf.stride = (native_osd.back_bitmap_stride);
        else
            native_osd.framebuf.stride = (native_osd.lcd_bitmap_stride);

        rb->viewport_set_buffer(NULL, &native_osd.framebuf, SCREEN_MAIN);
    }
    else
        rb->viewport_set_buffer(NULL, NULL, SCREEN_MAIN);
}

/* Native LCD, public */
bool osd_init(unsigned flags, void *backbuf, size_t backbuf_size,
              osd_draw_cb_fn_t draw_cb, int *width, int *height,
              size_t *bufused)
{
    native_osd.init_buffers = _osd_lcd_init_buffers;
    native_osd.set_viewport_pos = _osd_lcd_viewport_set_pos;
    native_osd.lcd_update = rb->lcd_update;
    native_osd.lcd_update_rect = rb->lcd_update_rect;
    native_osd.lcd_set_viewport = rb->lcd_set_viewport;
    native_osd.lcd_set_framebuffer = (void *)_osd_lcd_viewport_set_buffer;
#if LCD_DEPTH < 4
    native_osd.lcd_framebuffer_set_pos = NULL;
#endif /* LCD_DEPTH < 4 */
    native_osd.lcd_bitmap_part = (void *)rb->lcd_bitmap_part;

    return _osd_init(&native_osd, flags, backbuf, backbuf_size, draw_cb,
                     width, height, bufused);
}

void osd_destroy(void)
{
    return _osd_destroy(&native_osd);
}

bool osd_show(unsigned flags)
{
    return _osd_show(&native_osd, flags);
}

bool osd_update(void)
{
    return _osd_update(&native_osd);
}

bool osd_update_rect(int x, int y, int width, int height)
{
    return _osd_update_rect(&native_osd, x, y, width, height);
}

bool osd_update_pos(int x, int y, int width, int height)
{
    return _osd_update_pos(&native_osd, x, y, width, height);
}

void osd_monitor_timeout(void)
{
    _osd_monitor_timeout(&native_osd);
}

void osd_set_timeout(long timeout)
{
    _osd_set_timeout(&native_osd, timeout);
}

struct viewport * osd_get_viewport(void)
{
    return _osd_get_viewport(&native_osd);
}

void osd_get_max_dims(int *maxwidth, int *maxheight)
{
    _osd_get_max_dims(&native_osd, maxwidth, maxheight);
}

bool osd_enabled(void)
{
    return _osd_enabled(&native_osd);
}

void osd_lcd_update_prepare(void)
{
    _osd_lcd_update_prepare(&native_osd);
}


void osd_lcd_update(void)
{
    _osd_lcd_update(&native_osd);
}

void osd_lcd_update_rect(int x, int y, int width, int height)
{
    _osd_lcd_update_rect(&native_osd, x, y, width, height);
}

#if LCD_DEPTH < 4
/* Greylib LCD, public */
bool osd_grey_init(unsigned flags, void *backbuf, size_t backbuf_size,
                   osd_draw_cb_fn_t draw_cb, int *width, int *height,
                   size_t *bufused)
{
    grey_osd.init_buffers = _osd_grey_init_buffers;
    grey_osd.set_viewport_pos = grey_viewport_set_pos;
    grey_osd.lcd_update = grey_update;
    grey_osd.lcd_update_rect = grey_update_rect;
    grey_osd.lcd_set_viewport = grey_set_viewport;
    grey_osd.lcd_set_framebuffer = (void *)grey_set_framebuffer;
    grey_osd.lcd_framebuffer_set_pos = grey_framebuffer_set_pos;
    grey_osd.lcd_bitmap_part = (void *)grey_gray_bitmap_part;

    return _osd_init(&grey_osd, flags, backbuf, backbuf_size, draw_cb,
                     width, height, bufused);
}

void osd_grey_destroy(void)
{
    return _osd_destroy(&grey_osd);
}

bool osd_grey_show(unsigned flags)
{
    return _osd_show(&grey_osd, flags);
}

bool osd_grey_update(void)
{
    return _osd_update(&grey_osd);
}

bool osd_grey_update_rect(int x, int y, int width, int height)
{
    return _osd_update_rect(&grey_osd, x, y, width, height);
}

bool osd_grey_update_pos(int x, int y, int width, int height)
{
    return _osd_update_pos(&grey_osd, x, y, width, height);
}

void osd_grey_monitor_timeout(void)
{
    _osd_monitor_timeout(&grey_osd);
}

void osd_grey_set_timeout(long timeout)
{
    _osd_set_timeout(&grey_osd, timeout);
}

struct viewport * osd_grey_get_viewport(void)
{
    return _osd_get_viewport(&grey_osd);
}

void osd_grey_get_max_dims(int *maxwidth, int *maxheight)
{
    _osd_get_max_dims(&grey_osd, maxwidth, maxheight);
}

bool osd_grey_enabled(void)
{
    return _osd_enabled(&grey_osd);
}

void osd_grey_lcd_update_prepare(void)
{
    _osd_lcd_update_prepare(&grey_osd);
}

void osd_grey_lcd_update(void)
{
    _osd_lcd_update(&grey_osd);
}

void osd_grey_lcd_update_rect(int x, int y, int width, int height)
{
    _osd_lcd_update_rect(&grey_osd, x, y, width, height);
}
#endif /* LCD_DEPTH < 4 */
