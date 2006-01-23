/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Greyscale framework
*
* This is a generic framework to use greyscale display within Rockbox
* plugins. It does not work for the player.
*
* Copyright (C) 2004-2005 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef __GRAY_H__
#define __GRAY_H__

#ifndef SIMULATOR /* not for simulator by now */
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP /* and also not for the Player */

#define GRAY_BRIGHTNESS(y) (y)

#define GRAY_BLACK     GRAY_BRIGHTNESS(0)
#define GRAY_DARKGRAY  GRAY_BRIGHTNESS(85)
#define GRAY_LIGHTGRAY GRAY_BRIGHTNESS(170)
#define GRAY_WHITE     GRAY_BRIGHTNESS(255)

/* Library initialisation and release */
int  gray_init(struct plugin_api* newrb, unsigned char *gbuf, long gbuf_size,
               bool buffered, int width, int bheight, int depth, long *buf_taken);
void gray_release(void);

/* Special functions */
void gray_show(bool enable);
void gray_deferred_lcd_update(void);

/* Update functions */
void gray_update(void);
void gray_update_rect(int x, int y, int width, int height);

/* Parameter handling */
void gray_set_position(int x, int by);
void gray_set_drawmode(int mode);
int  gray_get_drawmode(void);
void gray_set_foreground(unsigned brightness);
unsigned gray_get_foreground(void);
void gray_set_background(unsigned brightness);
unsigned gray_get_background(void);
void gray_set_drawinfo(int mode, unsigned fg_brightness, unsigned bg_brightness);
void gray_setfont(int newfont);
int  gray_getstringsize(const unsigned char *str, int *w, int *h);

/* Whole display */
void gray_clear_display(void);
void gray_ub_clear_display(void);

/* Pixel */
void gray_drawpixel(int x, int y);

/* Lines */
void gray_drawline(int x1, int y1, int x2, int y2);
void gray_hline(int x1, int x2, int y);
void gray_vline(int x, int y1, int y2);
void gray_drawrect(int x, int y, int nx, int ny);

/* Filled primitives */
void gray_fillrect(int x, int y, int nx, int ny);
void gray_filltriangle(int x1, int y1, int x2, int y2, int x3, int y3);

/* Bitmaps */
void gray_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                           int stride, int x, int y, int width, int height);
void gray_mono_bitmap(const unsigned char *src, int x, int y, int width,
                      int height);
void gray_gray_bitmap_part(const unsigned char *src, int src_x, int src_y,
                           int stride, int x, int y, int width, int height);
void gray_gray_bitmap(const unsigned char *src, int x, int y, int width,
                      int height);
void gray_ub_gray_bitmap_part(const unsigned char *src, int src_x, int src_y,
                              int stride, int x, int y, int width, int height);
void gray_ub_gray_bitmap(const unsigned char *src, int x, int y, int width,
                         int height);

/* Text */
void gray_putsxyofs(int x, int y, int ofs, const unsigned char *str);
void gray_putsxy(int x, int y, const unsigned char *str);

/* Scrolling */
void gray_scroll_left(int count);
void gray_scroll_right(int count);
void gray_scroll_up(int count);
void gray_scroll_down(int count);
void gray_ub_scroll_left(int count);
void gray_ub_scroll_right(int count);
void gray_ub_scroll_up(int count);
void gray_ub_scroll_down(int count);

/*** Internal stuff ***/

#define _PBLOCK_EXP 3
#define _PBLOCK (1 << _PBLOCK_EXP)

/* flag definitions */
#define _GRAY_RUNNING          0x0001  /* greyscale overlay is running */
#define _GRAY_DEFERRED_UPDATE  0x0002  /* lcd_update() requested */

/* unsigned 16 bit multiplication (a single instruction on the SH) */
#define MULU16(a, b) ((unsigned long) \
                     (((unsigned short) (a)) * ((unsigned short) (b))))

/* The grayscale buffer management structure */
struct _gray_info
{
    int x;
    int by;         /* 8-pixel units */
    int width;
    int height;
    int bheight;    /* 8-pixel units */
    int depth;      /* number_of_bitplanes  = (number_of_grayscales - 1) */
    int cur_plane;  /* for the timer isr */
    int drawmode;              /* current draw mode */
    int fg_brightness;         /* current foreground brightness */
    int bg_brightness;         /* current background brightness */
    long plane_size;
    unsigned long flags;       /* various flags, see #defines */
    unsigned long randmask;    /* mask for random value in _writepixel() */
    unsigned long *bitpattern; /* start of pattern table */
    unsigned char *plane_data; /* start of bitplane data */
    unsigned char *cur_buffer; /* start of current chunky pixel buffer */
    unsigned char *back_buffer;/* start of chunky pixel back buffer */
    int curfont;               /* current selected font */
};

/* Global variables */
extern struct plugin_api *_gray_rb;
extern struct _gray_info _gray_info;
extern short _gray_random_buffer;

#endif /* HAVE_LCD_BITMAP */
#endif /* !SIMULATOR */
#endif /* __GRAY_H__ */
