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
*
* This is a generic framework to display 129 shades of grey on low-depth
* bitmap LCDs (Archos b&w, Iriver & Ipod 4-grey) within plugins.
*
* Copyright (C) 2008 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef __GREY_H__
#define __GREY_H__

#include "plugin.h"

#if defined(HAVE_LCD_BITMAP) && (LCD_DEPTH < 4)

/* The greyscale lib uses 8 bit brightness values natively on input. */
#define GREY_BRIGHTNESS(y) (y)

/* Some predefined levels for convenience: */
#define GREY_BLACK     GREY_BRIGHTNESS(0)
#define GREY_DARKGRAY  GREY_BRIGHTNESS(85)
#define GREY_LIGHTGRAY GREY_BRIGHTNESS(170)
#define GREY_WHITE     GREY_BRIGHTNESS(255)

/* Greyscale library management structure declaration. You need one of these
 * in every plugin using the library, depending on whether the structure should
 * use IRAM or not. */
#define GREY_INFO_STRUCT struct _grey_info _grey_info;
#define GREY_INFO_STRUCT_IRAM struct _grey_info _grey_info IBSS_ATTR;

/* Features you can request on library init (ORed together): */
#define GREY_BUFFERED  0x0001
#define GREY_RAWMAPPED 0x0002

/* Library initialisation and release */
bool grey_init(struct plugin_api* newrb, unsigned char *gbuf, long gbuf_size,
               unsigned features, int width, int height, long *buf_taken);
void grey_release(void);

/* Special functions */
void grey_show(bool enable);
void grey_deferred_lcd_update(void);

/* Update functions */
void grey_update(void);
void grey_update_rect(int x, int y, int width, int height);

/* Parameter handling */
void grey_set_position(int x, int y);
void grey_set_drawmode(int mode);
int  grey_get_drawmode(void);
void grey_set_foreground(unsigned brightness);
unsigned grey_get_foreground(void);
void grey_set_background(unsigned brightness);
unsigned grey_get_background(void);
void grey_set_drawinfo(int mode, unsigned fg_brightness, unsigned bg_brightness);
void grey_setfont(int newfont);
int  grey_getstringsize(const unsigned char *str, int *w, int *h);

/* Whole display */
void grey_clear_display(void);
void grey_ub_clear_display(void);

/* Pixel */
void grey_drawpixel(int x, int y);

/* Lines */
void grey_drawline(int x1, int y1, int x2, int y2);
void grey_hline(int x1, int x2, int y);
void grey_vline(int x, int y1, int y2);
void grey_drawrect(int x, int y, int nx, int ny);

/* Filled primitives */
void grey_fillrect(int x, int y, int nx, int ny);
void grey_filltriangle(int x1, int y1, int x2, int y2, int x3, int y3);

/* Bitmaps */
void grey_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                           int stride, int x, int y, int width, int height);
void grey_mono_bitmap(const unsigned char *src, int x, int y, int width,
                      int height);
void grey_gray_bitmap_part(const unsigned char *src, int src_x, int src_y,
                           int stride, int x, int y, int width, int height);
void grey_gray_bitmap(const unsigned char *src, int x, int y, int width,
                      int height);
void grey_ub_gray_bitmap_part(const unsigned char *src, int src_x, int src_y,
                              int stride, int x, int y, int width, int height);
void grey_ub_gray_bitmap(const unsigned char *src, int x, int y, int width,
                         int height);

/* Text */
void grey_putsxyofs(int x, int y, int ofs, const unsigned char *str);
void grey_putsxy(int x, int y, const unsigned char *str);

/* Scrolling */
void grey_scroll_left(int count);
void grey_scroll_right(int count);
void grey_scroll_up(int count);
void grey_scroll_down(int count);
void grey_ub_scroll_left(int count);
void grey_ub_scroll_right(int count);
void grey_ub_scroll_up(int count);
void grey_ub_scroll_down(int count);

/*** Internal stuff ***/

/* standard gamma (s23p8) */
#ifdef SIMULATOR                /* Standard PC gamma */
#define _GREY_GAMMA ((200<<8)/100)
#else   /* Target LCDs have a smaller contrast range */
#define _GREY_GAMMA ((180<<8)/100)  
#endif

/* flag definitions */
#define _GREY_RUNNING          0x8000  /* greyscale overlay is running */
#define _GREY_DEFERRED_UPDATE  0x4000  /* lcd_update() requested */

/* fast unsigned multiplication (16x16bit->32bit or 32x32bit->32bit,
 * whichever is faster for the architecture) */
#ifdef CPU_ARM
#define _GREY_MULUQ(a, b) ((uint32_t) (((uint32_t) (a)) * ((uint32_t) (b))))
#else
#define _GREY_MULUQ(a, b) ((uint32_t) (((uint16_t) (a)) * ((uint16_t) (b))))
#endif

#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#define _GREY_BSHIFT 0
#else /* vertical packing or vertical interleaved */
#if (LCD_DEPTH == 1) || (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
#define _GREY_BSHIFT 3
#elif LCD_DEPTH == 2
#define _GREY_BSHIFT 2
#endif
#endif /* LCD_PIXELFORMAT */

#define _GREY_BSIZE (1<<_GREY_BSHIFT)
#define _GREY_BMASK (_GREY_BSIZE-1)

/* The greyscale buffer management structure */
struct _grey_info
{
    int x;
    int y;
    int width;
    int height;
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
    int bx;         /* 8-pixel units */
    int bwidth;     /* 8-pixel units */
#else /* vertical packing or vertical interleaved */
    int by;         /* 4-pixel or 8-pixel units */
    int bheight;    /* 4-pixel or 8-pixel units */
#endif
    unsigned long flags;       /* various flags, see #defines */
    struct plugin_api *rb;     /* plugin API pointer */
    unsigned char *values;     /* start of greyscale pixel values */
    unsigned char *phases;     /* start of greyscale pixel phases */
    unsigned char *buffer;     /* start of chunky pixel buffer (for buffered mode) */
    unsigned char gvalue[256]; /* calculated brightness -> greyvalue table */
    int fg_brightness;         /* current foreground brightness */
    int bg_brightness;         /* current background brightness */
    int drawmode;              /* current draw mode */
    int curfont;               /* current selected font */
};

/* Global variable, defined in the plugin */
extern struct _grey_info _grey_info;

#endif /* HAVE_LCD_BITMAP && (LCD_DEPTH < 4) */
#endif /* __GREY_H__ */
