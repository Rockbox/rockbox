/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
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

#ifndef __LCD_H__
#define __LCD_H__

#include <stdbool.h>
#include <stddef.h>
#include "cpu.h"
#include "config.h"
#include "events.h"


/* Frame buffer stride
 *
 * Stride describes the amount that you need to increment to get to the next
 *  line.  For screens that have the pixels in contiguous horizontal strips
 *  stride should be equal to the image width.
 *
 *      For example, if the screen pixels are layed out as follows:
 *
 *                  width0  width1  width2                      widthX-1
 *                  ------  ------  ------  ------------------  --------
 *      height0 |   pixel0  pixel1  pixel2  ---------------->   pixelX-1
 *      height1 |   pixelX
 *
 *      then you need to add X pixels to get to the next line. (the next line
 *      in this case is height1).
 *
 * Similarly, if the screen has the pixels in contiguous vertical strips
 *  the stride would be equal to the image height.
 *
 *      For example if the screen pixels are layed out as follows:
 *
 *                      width0  width1
 *                      ------  ------
 *      height0     |   pixel0  pixelY
 *      height1     |   pixel1
 *      height2     |   pixel2
 *         |        |     |
 *        \|/       |    \|/
 *      heightY-1   |   pixelY-1
 *
 *      then you would need to add Y pixels to get to the next line (the next
 *      line in this case is from width0 to width1).
 *
 * The remote might have a different stride than the main screen so the screen
 *  number needs to be passed to the STRIDE macro so that the appropriate height
 *  or width can be passed to the lcd_bitmap, or lcd_remote_bitmap calls.
 *
 * STRIDE_REMOTE and STRIDE_MAIN should never be used when it is not clear whether
 *  lcd_remote_bitmap calls or lcd_bitmap calls are being made (for example the
 *  screens api).
 *
 * Screen should always use the screen_type enum that is at the top of this
 *  header.
 */
enum screen_type {
    SCREEN_MAIN
#ifdef HAVE_REMOTE_LCD
    ,SCREEN_REMOTE
#endif
};

struct scrollinfo;

#if defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
#define STRIDE_MAIN(w, h)   (h)
#else
#define STRIDE_MAIN(w, h)   (w)
#endif

#define STRIDE_REMOTE(w, h) (w)

#define STRIDE(screen, w, h) (screen==SCREEN_MAIN?STRIDE_MAIN((w), \
                                        (h)):STRIDE_REMOTE((w),(h)))

#if LCD_DEPTH <=8
#if (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED) \
 || (LCD_PIXELFORMAT == HORIZONTAL_INTERLEAVED)
        typedef unsigned short fb_data;
#define FB_DATA_SZ 2
#else
        typedef unsigned char fb_data;
#define FB_DATA_SZ 1
#endif
#elif LCD_DEPTH <= 16
    typedef unsigned short fb_data;
#define FB_DATA_SZ 2
#elif LCD_DEPTH <= 24
    struct _fb_pixel {
        unsigned char b, g, r;
    };
    typedef struct _fb_pixel fb_data;
#define FB_DATA_SZ 3
#else /* LCD_DEPTH > 24 */
#if (LCD_PIXELFORMAT == XRGB8888)
        struct _fb_pixel {
            unsigned char b, g, r, x;
        };
        typedef struct _fb_pixel fb_data;
#else
        typedef unsigned long fb_data;
#endif
#define FB_DATA_SZ 4
#endif /* LCD_DEPTH */

#ifdef HAVE_REMOTE_LCD
#if LCD_REMOTE_DEPTH <= 8
#if (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED) \
            || (LCD_REMOTE_PIXELFORMAT == HORIZONTAL_INTERLEAVED)
            typedef unsigned short fb_remote_data;
#define FB_RDATA_SZ 2
#else
            typedef unsigned char fb_remote_data;
#define FB_RDATA_SZ 1
#endif
#elif LCD_DEPTH <= 16
        typedef unsigned short fb_remote_data;
#define FB_RDATA_SZ 2
#else
        typedef unsigned long fb_remote_data;
#define FB_RDATA_SZ 4
#endif
#endif

#if defined(HAVE_LCD_MODES)
    void lcd_set_mode(int mode);
#define LCD_MODE_RGB565 0x00000001
#define LCD_MODE_YUV    0x00000002
#define LCD_MODE_PAL256 0x00000004

#if HAVE_LCD_MODES & LCD_MODE_PAL256
        void lcd_blit_pal256(unsigned char *src, int src_x, int src_y, int x, int y,
                            int width, int height);
        void lcd_pal256_update_pal(fb_data *palette);
#endif
#endif

struct frame_buffer_t {
    union
    {
        void           *data;
        char           *ch_ptr;
        fb_data        *fb_ptr;
#ifdef HAVE_REMOTE_LCD
        fb_remote_data *fb_remote_ptr;
#endif
    };
    void   *(*get_address_fn)(int x, int y);
    ptrdiff_t stride;
    size_t    elems;
};

#define VP_FLAG_ALIGN_RIGHT  0x01
#define VP_FLAG_ALIGN_CENTER 0x02

#define VP_FLAG_ALIGNMENT_MASK \
        (VP_FLAG_ALIGN_RIGHT|VP_FLAG_ALIGN_CENTER)

#define VP_IS_RTL(vp) (((vp)->flags & VP_FLAG_ALIGNMENT_MASK) == VP_FLAG_ALIGN_RIGHT)

#define VP_FLAG_VP_DIRTY 0x4000
#define VP_FLAG_CLEAR_FLAG 0x8000
#define VP_FLAG_VP_SET_CLEAN (VP_FLAG_CLEAR_FLAG | VP_FLAG_VP_DIRTY)

struct viewport {
    int x;
    int y;
    int width;
    int height;
    int flags;
    int font;
    int drawmode;
    struct frame_buffer_t *buffer;
    /* needed for even for mono displays to support greylib */
    unsigned fg_pattern;
    unsigned bg_pattern;
};

/* common functions */
extern void lcd_write_command(int byte);
extern void lcd_write_command_e(int cmd, int data);
extern void lcd_write_command_ex(int cmd, int data1, int data2);
extern void lcd_write_data(const fb_data* p_bytes, int count);
extern void lcd_init(void) INIT_ATTR;
extern void lcd_init_device(void) INIT_ATTR;

extern void lcd_backlight(bool on);
extern int  lcd_default_contrast(void);
extern void lcd_set_contrast(int val);
extern int  lcd_getwidth(void);
extern int  lcd_getheight(void);
extern int  lcd_getstringsize(const unsigned char *str, int *w, int *h);

extern struct viewport* lcd_init_viewport(struct viewport* vp);
extern struct viewport* lcd_set_viewport(struct viewport* vp);
extern struct viewport* lcd_set_viewport_ex(struct viewport* vp, int flags);

extern void lcd_update(void);
extern void lcd_update_viewport(void);
extern void lcd_update_viewport_rect(int x, int y, int width, int height);
extern void lcd_clear_viewport(void);
extern void lcd_clear_display(void);
extern void lcd_putsxy(int x, int y, const unsigned char *string);
extern void lcd_putsxyf(int x, int y, const unsigned char *fmt, ...);
extern void lcd_putsxy_style_offset(int x, int y, const unsigned char *str,
                                    int style, int offset);
extern void lcd_puts(int x, int y, const unsigned char *string);
extern void lcd_putsf(int x, int y, const unsigned char *fmt, ...);
extern void lcd_putc(int x, int y, unsigned long ucs);
extern bool lcd_puts_scroll(int x, int y, const unsigned char* string);
extern bool lcd_putsxy_scroll_func(int x, int y, const unsigned char *string,
                                   void (*scroll_func)(struct scrollinfo *),
                                   void *data, int x_offset);

/* performance function */
#if defined(HAVE_LCD_COLOR)
#if MEMORYSIZE > 2
#define LCD_YUV_DITHER 0x1
    extern void lcd_yuv_set_options(unsigned options);
    extern void lcd_blit_yuv(unsigned char * const src[3],
                         int src_x, int src_y, int stride,
                         int x, int y, int width, int height);
#endif /* MEMORYSIZE > 2 */
#else
    extern void lcd_blit_mono(const unsigned char *data, int x, int by, int width,
                          int bheight, int stride);
    extern void lcd_blit_grey_phase(unsigned char *values, unsigned char *phases,
                                int bx, int by, int bwidth, int bheight,
                                int stride);
#endif


/* update a fraction of the screen */
extern void lcd_update_rect(int x, int y, int width, int height);

#ifdef HAVE_REMOTE_LCD
    extern void lcd_remote_update(void);
    /* update a fraction of the screen */
    extern void lcd_remote_update_rect(int x, int y, int width, int height);
#endif /* HAVE_REMOTE_LCD */

/* Bitmap formats */
enum
{
    FORMAT_MONO,
    FORMAT_NATIVE,
    FORMAT_ANY   /* For passing to read_bmp_file() */
};


/* Draw modes */
#define DRMODE_COMPLEMENT 0
#define DRMODE_BG         1
#define DRMODE_FG         2
#define DRMODE_SOLID      3
#define DRMODE_INVERSEVID 4 /* used as bit modifier for basic modes */
/* Internal drawmode modifiers. DO NOT use with set_drawmode() */
#define DRMODE_INT_BD     8
#define DRMODE_INT_IMG   16

/* Low-level drawing function types */
typedef void lcd_pixelfunc_type(int x, int y);
typedef void lcd_blockfunc_type(fb_data *address, unsigned mask, unsigned bits);
#if LCD_DEPTH >= 8
    typedef void lcd_fastpixelfunc_type(fb_data *address);
#endif

#if defined(HAVE_LCD_COLOR) && defined(LCD_REMOTE_DEPTH) && \
 LCD_REMOTE_DEPTH > 1
/* Just return color for screens use */
    static inline unsigned lcd_color_to_native(unsigned color)
    { return color; }
#define SCREEN_COLOR_TO_NATIVE(screen, color) (screen)->color_to_native(color)
#else
#define SCREEN_COLOR_TO_NATIVE(screen, color) (color)
#endif

#ifdef HAVE_LCD_COLOR
#if LCD_PIXELFORMAT == RGB565 || LCD_PIXELFORMAT == RGB565SWAPPED
#define LCD_MAX_RED    31
#define LCD_MAX_GREEN  63
#define LCD_MAX_BLUE   31
#define LCD_RED_BITS   5
#define LCD_GREEN_BITS 6
#define LCD_BLUE_BITS  5

/* pack/unpack native RGB values */
#define _RGBPACK_LCD(r, g, b)    ( ((r) << 11) | ((g) << 5) | (b) )
#define _RGB_UNPACK_RED_LCD(x)   ( (((x) >> 11)       ) )
#define _RGB_UNPACK_GREEN_LCD(x) ( (((x) >>  5) & 0x3f) )
#define _RGB_UNPACK_BLUE_LCD(x)  ( (((x)      ) & 0x1f) )

/* pack/unpack 24-bit RGB values */
#define _RGBPACK(r, g, b)    _RGBPACK_LCD((r) >> 3, (g) >> 2, (b) >> 3)
#define _RGB_UNPACK_RED(x)   ( (((x) >> 8) & 0xf8) | (((x) >> 13) & 0x07) )
#define _RGB_UNPACK_GREEN(x) ( (((x) >> 3) & 0xfc) | (((x) >>  9) & 0x03) )
#define _RGB_UNPACK_BLUE(x)  ( (((x) << 3) & 0xf8) | (((x) >>  2) & 0x07) )

#if (LCD_PIXELFORMAT == RGB565SWAPPED)
/* RGB3553 */
#define _LCD_UNSWAP_COLOR(x)     swap16(x)
#define LCD_RGBPACK_LCD(r, g, b) ( (((r) <<   3)      ) | \
                                   (((g) >>   3)      ) | \
                                   (((g) & 0x07) << 13) | \
                                   (((b) <<   8)      ) )
#define LCD_RGBPACK(r, g, b)     ( (((r) >>   3) <<  3) | \
                                   (((g) >>   5)      ) | \
                                   (((g) & 0x1c) << 11) | \
                                   (((b) >>   3) <<  8) )
/* swap color once - not currenly used in static inits */
#define _SWAPUNPACK(x, _unp_) \
            ({ typeof (x) _x_ = swap16(x); _unp_(_x_); })
#define RGB_UNPACK_RED(x)        _SWAPUNPACK((x), _RGB_UNPACK_RED)
#define RGB_UNPACK_GREEN(x)      _SWAPUNPACK((x), _RGB_UNPACK_GREEN)
#define RGB_UNPACK_BLUE(x)       _SWAPUNPACK((x), _RGB_UNPACK_BLUE)
#define RGB_UNPACK_RED_LCD(x)    _SWAPUNPACK((x), _RGB_UNPACK_RED_LCD)
#define RGB_UNPACK_GREEN_LCD(x)  _SWAPUNPACK((x), _RGB_UNPACK_GREEN_LCD)
#define RGB_UNPACK_BLUE_LCD(x)   _SWAPUNPACK((x), _RGB_UNPACK_BLUE_LCD)
#else /* LCD_PIXELFORMAT == RGB565 */
/* RGB565 */
#define _LCD_UNSWAP_COLOR(x)     (x)
#define LCD_RGBPACK(r, g, b)     _RGBPACK((r), (g), (b))
#define LCD_RGBPACK_LCD(r, g, b) _RGBPACK_LCD((r), (g), (b))
#define RGB_UNPACK_RED(x)        _RGB_UNPACK_RED(x)
#define RGB_UNPACK_GREEN(x)      _RGB_UNPACK_GREEN(x)
#define RGB_UNPACK_BLUE(x)       _RGB_UNPACK_BLUE(x)
#define RGB_UNPACK_RED_LCD(x)    _RGB_UNPACK_RED_LCD(x)
#define RGB_UNPACK_GREEN_LCD(x)  _RGB_UNPACK_GREEN_LCD(x)
#define RGB_UNPACK_BLUE_LCD(x)   _RGB_UNPACK_BLUE_LCD(x)
#endif /* RGB565* */

#elif (LCD_PIXELFORMAT == RGB888) || (LCD_PIXELFORMAT == XRGB8888)
#define LCD_MAX_RED    255
#define LCD_MAX_GREEN  255
#define LCD_MAX_BLUE   255
#define LCD_RED_BITS   8
#define LCD_GREEN_BITS 8
#define LCD_BLUE_BITS  8

/* pack/unpack native RGB values */
#define _RGBPACK(r, g, b)        ( r << 16 | g << 8 | b )
#define _RGB_UNPACK_RED(x)       ((x >> 16) & 0xff)
#define _RGB_UNPACK_GREEN(x)     ((x >>  8) & 0xff)
#define _RGB_UNPACK_BLUE(x)      ((x >>  0) & 0xff)

#define _LCD_UNSWAP_COLOR(x)     (x)
#define LCD_RGBPACK(r, g, b)     _RGBPACK((r), (g), (b))
#define LCD_RGBPACK_LCD(r, g, b) _RGBPACK((r), (g), (b))
#define RGB_UNPACK_RED(x)        _RGB_UNPACK_RED(x)
#define RGB_UNPACK_GREEN(x)      _RGB_UNPACK_GREEN(x)
#define RGB_UNPACK_BLUE(x)       _RGB_UNPACK_BLUE(x)
#define RGB_UNPACK_RED_LCD(x)    _RGB_UNPACK_RED(x)
#define RGB_UNPACK_GREEN_LCD(x)  _RGB_UNPACK_GREEN(x)
#define RGB_UNPACK_BLUE_LCD(x)   _RGB_UNPACK_BLUE(x)

#else
/* other colour depths */
#endif

#define LCD_BLACK      LCD_RGBPACK(0, 0, 0)
#define LCD_DARKGRAY   LCD_RGBPACK(85, 85, 85)
#define LCD_LIGHTGRAY  LCD_RGBPACK(170, 170, 170)
#define LCD_WHITE      LCD_RGBPACK(255, 255, 255)
#define LCD_DEFAULT_FG LCD_WHITE
#define LCD_DEFAULT_BG LCD_BLACK
#define LCD_DEFAULT_LS LCD_WHITE

#elif LCD_DEPTH > 1 /* greyscale */

#define LCD_MAX_LEVEL ((1 << LCD_DEPTH) - 1)
#define LCD_BRIGHTNESS(y) (((y) * LCD_MAX_LEVEL + 127) / 255)

#define LCD_BLACK      LCD_BRIGHTNESS(0)
#define LCD_DARKGRAY   LCD_BRIGHTNESS(85)
#define LCD_LIGHTGRAY  LCD_BRIGHTNESS(170)
#define LCD_WHITE      LCD_BRIGHTNESS(255)
#define LCD_DEFAULT_FG LCD_BLACK
#define LCD_DEFAULT_BG LCD_WHITE

#endif /* HAVE_LCD_COLOR */

/* Framebuffer conversion macros: Convert from and to the native display data
 * format (fb_data).
 *
 * FB_RGBPACK:          Convert the three r,g,b values to fb_data. r,g,b are
 *                      assumed to in 8-bit format.
 * FB_RGBPACK_LCD       Like FB_RGBPACK, except r,g,b shall be in display-native
 *                      bit format (e.g. 5-bit r for RGB565)
 * FB_UNPACK_RED        Extract the red component of fb_data into 8-bit red value.
 * FB_UNPACK_GREEN      Like FB_UNPACK_RED, just for the green component.
 * FB_UNPACK_BLIE       Like FB_UNPACK_RED, just for the green component.
 * FB_SCALARPACK        Similar to FB_RGBPACK, except that the channels are already
 *                      combined into a single scalar value. Again, 8-bit per channel.
 * FB_SCALARPACK_LCD    Like FB_SCALARPACK, except the channels shall be in
 *                      display-native format (i.e. the scalar is 16bits on RGB565)
 * FB_UNPACK_SCALAR_LCD Converts an fb_data to a scalar value in display-native
 *                      format, so it's the reverse of FB_SCALARPACK_LCD
 */
#if LCD_DEPTH >= 24
    static inline fb_data scalar_to_fb(unsigned p)
    {
        union { fb_data st; unsigned sc; } convert;
        convert.sc = p; return convert.st;
    }
    static inline unsigned fb_to_scalar(fb_data p)
    {
        union { fb_data st; unsigned sc; } convert;
        convert.st = p; return convert.sc;
    }
#define FB_RGBPACK(r_, g_, b_)      ((fb_data){.r = r_, .g = g_, .b = b_})
#define FB_RGBPACK_LCD(r_, g_, b_)  FB_RGBPACK(r_, g_, b_)
#define FB_UNPACK_RED(fb)           ((fb).r)
#define FB_UNPACK_GREEN(fb)         ((fb).g)
#define FB_UNPACK_BLUE(fb)          ((fb).b)
#define FB_SCALARPACK(c)            scalar_to_fb(c)
#define FB_SCALARPACK_LCD(c)        scalar_to_fb(c)
#define FB_UNPACK_SCALAR_LCD(fb)    fb_to_scalar(fb)
#elif defined(HAVE_LCD_COLOR)
#define FB_RGBPACK(r_, g_, b_)      LCD_RGBPACK(r_, g_, b_)
#define FB_RGBPACK_LCD(r_, g_, b_)  LCD_RGBPACK_LCD(r_, g_, b_)
#define FB_UNPACK_RED(fb)           RGB_UNPACK_RED(fb)
#define FB_UNPACK_GREEN(fb)         RGB_UNPACK_GREEN(fb)
#define FB_UNPACK_BLUE(fb)          RGB_UNPACK_BLUE(fb)
#define FB_SCALARPACK(c)            LCD_RGBPACK(RGB_UNPACK_RED(c), RGB_UNPACK_GREEN(c), RGB_UNPACK_BLUE(c))
#define FB_SCALARPACK_LCD(c)        (c)
#define FB_UNPACK_SCALAR_LCD(fb)    (fb)
#else
#define FB_SCALARPACK(c)            (c)
#define FB_SCALARPACK_LCD(c)        (c)
#define FB_UNPACK_SCALAR_LCD(fb)    (fb)
#endif


/* Frame buffer dimensions */
#if LCD_DEPTH == 1
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#define LCD_FBSTRIDE(w, h) ((w+7)/8)
#define LCD_FBWIDTH LCD_FBSTRIDE(LCD_WIDTH, LCD_HEIGHT)
#define LCD_NBELEMS(w, h) ((((h-1)*LCD_FBSTRIDE(w, h)) + w) / sizeof(fb_data))
#else /* LCD_PIXELFORMAT == VERTICAL_PACKING */
#define LCD_FBSTRIDE(w, h) ((h+7)/8)
#define LCD_FBHEIGHT LCD_FBSTRIDE(LCD_WIDTH, LCD_HEIGHT)
#define LCD_NBELEMS(w, h) ((((w-1)*LCD_FBSTRIDE(w, h)) + h) / sizeof(fb_data))
#endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#define LCD_FBSTRIDE(w, h) ((w+3)>>2)
#define LCD_NATIVE_STRIDE(s) LCD_FBSTRIDE(s, s)
#define LCD_FBWIDTH LCD_FBSTRIDE(LCD_WIDTH, LCD_HEIGHT)
#define LCD_NBELEMS(w, h) ((((h-1)*LCD_FBSTRIDE(w, h)) + w) / sizeof(fb_data))
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
#define LCD_FBSTRIDE(w, h) ((h+3)/4)
#define LCD_FBHEIGHT LCD_FBSTRIDE(LCD_WIDTH, LCD_HEIGHT)
#define LCD_NBELEMS(w, h) ((((w-1)*LCD_FBSTRIDE(w, h)) + h) / sizeof(fb_data))
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
#define LCD_FBSTRIDE(w, h) ((h+7)/8)
#define LCD_FBHEIGHT LCD_FBSTRIDE(LCD_WIDTH, LCD_HEIGHT)
#define LCD_NBELEMS(w, h) ((((w-1)*LCD_FBSTRIDE(w, h)) + h) / sizeof(fb_data))
#endif /* LCD_PIXELFORMAT */
#endif /* LCD_DEPTH */
/* Set defaults if not defined different yet. The defaults apply to both
 * dimensions for LCD_DEPTH >= 8 */
#ifndef LCD_FBWIDTH
#define LCD_FBWIDTH LCD_WIDTH
#endif
#ifndef LCD_FBHEIGHT
#define LCD_FBHEIGHT LCD_HEIGHT
#endif

#ifndef LCD_NATIVE_STRIDE
/* 2-bit Horz is the only display that actually defines this */
#define LCD_NATIVE_STRIDE(s) (s)
#endif

#ifndef LCD_NBELEMS
#if defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
#define LCD_NBELEMS(w, h) (((w-1)*STRIDE_MAIN(w, h)) + h)
#else
#define LCD_NBELEMS(w, h) (((h-1)*STRIDE_MAIN(w, h)) + w)
#endif
#define LCD_FBSTRIDE(w, h) STRIDE_MAIN(w, h)
#endif

#ifndef LCD_STRIDE
    #define LCD_STRIDE(w, h) STRIDE_MAIN(w, h)
#endif

extern struct viewport* lcd_current_viewport;

#define FBADDR(x,y) ((fb_data*) lcd_current_viewport->buffer->get_address_fn(x, y))

#define FRAMEBUFFER_SIZE (sizeof(fb_data)*LCD_FBWIDTH*LCD_FBHEIGHT)

/** Port-specific functions. Enable in port config file. **/
#ifdef HAVE_REMOTE_LCD_AS_MAIN
void lcd_on(void);
void lcd_off(void);
void lcd_poweroff(void);
#endif

#ifdef HAVE_LCD_ENABLE
/* Enable/disable the main display. */
extern void lcd_enable(bool on);
#endif /* HAVE_LCD_ENABLE */

#ifdef HAVE_LCD_SLEEP
/* Put the LCD into a power saving state deeper than lcd_enable(false). */
extern void lcd_sleep(void);
#endif /* HAVE_LCD_SLEEP */
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
/* Register a hook that is called when the lcd is powered and after the
 * framebuffer data is synchronized */
/* Sansa Clip has these function in it's lcd driver, since it's the only
 * 1-bit display featuring lcd_active, so far */

enum {
    LCD_EVENT_ACTIVATION = (EVENT_CLASS_LCD|1),
};

extern bool lcd_active(void);
#endif

#ifdef HAVE_LCD_SHUTDOWN
extern void lcd_shutdown(void);
#endif

#define FORMAT_TRANSPARENT 0x40000000
#define FORMAT_DITHER      0x20000000
#define FORMAT_REMOTE      0x10000000
#define FORMAT_RESIZE      0x08000000
#define FORMAT_KEEP_ASPECT 0x04000000
#define FORMAT_RETURN_SIZE 0x02000000

#define TRANSPARENT_COLOR LCD_RGBPACK(255,0,255)
#define REPLACEWITHFG_COLOR LCD_RGBPACK(0,255,255)

struct bitmap {
    int width;
    int height;
#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    int format;
    unsigned char *maskdata;
#endif
#ifdef HAVE_LCD_COLOR
    int alpha_offset; /* byte-offset of alpha channel in data */
#endif
    unsigned char *data;
};

extern void lcd_set_invert_display(bool yesno);
#ifdef HAVE_BACKLIGHT_INVERSION
    extern void lcd_set_backlight_inversion(bool yesno);
#endif /* HAVE_BACKLIGHT_INVERSION */
extern void lcd_set_flip(bool yesno);

extern void lcd_set_drawmode(int mode);
extern int  lcd_get_drawmode(void);
extern void lcd_setfont(int font);
extern int lcd_getfont(void);

/* low level drawing function pointer arrays */
#if LCD_DEPTH >= 8
    extern lcd_fastpixelfunc_type* const *lcd_fastpixelfuncs;
#elif LCD_DEPTH > 1
    extern lcd_pixelfunc_type* const *lcd_pixelfuncs;
    extern lcd_blockfunc_type* const *lcd_blockfuncs;
#else /* LCD_DEPTH == 1*/
    extern lcd_pixelfunc_type* const lcd_pixelfuncs[8];
    extern lcd_blockfunc_type* const lcd_blockfuncs[8];
#endif /* LCD_DEPTH */

extern void lcd_drawpixel(int x, int y);
extern void lcd_drawline(int x1, int y1, int x2, int y2);
extern void lcd_hline(int x1, int x2, int y);
extern void lcd_vline(int x, int y1, int y2);
extern void lcd_drawrect(int x, int y, int width, int height);
extern void lcd_fillrect(int x, int y, int width, int height);
extern void lcd_gradient_fillrect(int x, int y, int width, int height,
                                    unsigned start_rgb, unsigned end_rgb);
extern void lcd_gradient_fillrect_part(int x, int y, int width, int height,
            unsigned start_rgb, unsigned end_rgb, int src_height, int row_skip);
extern void lcd_draw_border_viewport(void);
extern void lcd_fill_viewport(void);
extern void lcd_bitmap_part(const fb_data *src, int src_x, int src_y,
                            int stride, int x, int y, int width, int height);
extern void lcd_bitmap(const fb_data *src, int x, int y, int width,
                       int height);

extern void lcd_scroll_step(int pixels);

#if LCD_DEPTH > 1
    extern void     lcd_set_foreground(unsigned foreground);
    extern unsigned lcd_get_foreground(void);
    extern void     lcd_set_background(unsigned background);
    extern unsigned lcd_get_background(void);
#ifdef HAVE_LCD_COLOR
    extern void     lcd_set_selector_start(unsigned selector);
    extern void     lcd_set_selector_end(unsigned selector);
    extern void     lcd_set_selector_text(unsigned selector_text);
#endif
    extern void     lcd_set_drawinfo(int mode, unsigned foreground,
                                    unsigned background);
    void lcd_set_backdrop(fb_data* backdrop);

    fb_data* lcd_get_backdrop(void);

    extern void lcd_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                                    int stride, int x, int y, int width, int height);
    extern void lcd_mono_bitmap(const unsigned char *src, int x, int y, int width,
                                int height);
    extern void lcd_bitmap_transparent_part(const fb_data *src,
                                            int src_x, int src_y,
                                            int stride, int x, int y, int width,
                                            int height);
    extern void lcd_bitmap_transparent(const fb_data *src, int x, int y,
                                        int width, int height);
#else /* LCD_DEPTH == 1 */
#define lcd_mono_bitmap lcd_bitmap
#define lcd_mono_bitmap_part lcd_bitmap_part
#endif /* LCD_DEPTH */
extern void lcd_bmp_part(const struct bitmap* bm, int src_x, int src_y,
                            int x, int y, int width, int height);
extern void lcd_bmp(const struct bitmap* bm, int x, int y);
extern void lcd_nine_segment_bmp(const struct bitmap* bm, int x, int y,
                                int width, int height);

/* TODO: Impement this for remote displays if ever needed */
#if defined(LCD_DPI) && (LCD_DPI > 0)
    /* returns the pixel density of the display */
    static inline int lcd_get_dpi(void) { return LCD_DPI; }
#else
    extern int lcd_get_dpi(void);
#endif /* LCD_DPI */

#endif /* __LCD_H__ */
