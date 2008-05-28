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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __LCD_H__
#define __LCD_H__

#include <stdbool.h>
#include "cpu.h"
#include "config.h"

struct viewport {
    int x;
    int y;
    int width;
    int height;
#ifdef HAVE_LCD_BITMAP
    int font;
    int drawmode;
#endif
    int xmargin;  /* During the transition only - to be removed */
    int ymargin;  /* During the transition only - to be removed */
#if LCD_DEPTH > 1
    unsigned fg_pattern;
    unsigned bg_pattern;
#ifdef HAVE_LCD_COLOR
    unsigned lss_pattern;
    unsigned lse_pattern;
    unsigned lst_pattern;
#endif
#endif
};

#define STYLE_DEFAULT    0x00000000
#define STYLE_COLORED    0x10000000
#define STYLE_INVERT     0x20000000
#define STYLE_COLORBAR   0x40000000
#define STYLE_GRADIENT   0x80000000
#define STYLE_MODE_MASK  0xF0000000
#define STYLE_COLOR_MASK 0x0000FFFF
#ifdef HAVE_LCD_COLOR
#define STYLE_CURLN_MASK 0x0000FF00
#define STYLE_MAXLN_MASK 0x000000FF
#define CURLN_PACK(x)    (((x)<<8) & STYLE_CURLN_MASK)
#define CURLN_UNPACK(x)  ((unsigned char)(((x)&STYLE_CURLN_MASK) >> 8))
#define NUMLN_PACK(x)    ((x) & STYLE_MAXLN_MASK)
#define NUMLN_UNPACK(x)  ((unsigned char)((x) & STYLE_MAXLN_MASK))
#endif

#ifdef SIMULATOR
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#else
#include "file.h"  /* for MAX_PATH; FIXME: Why does this not work for sims? */
#endif /* SIMULATOR */

#ifdef HAVE_LCD_BITMAP
#if LCD_DEPTH <=8
#if (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED) \
 || (LCD_PIXELFORMAT == HORIZONTAL_INTERLEAVED)
typedef unsigned short fb_data;
#else
typedef unsigned char fb_data;
#endif
#elif LCD_DEPTH <= 16
typedef unsigned short fb_data;
#else /* LCD_DEPTH > 16 */
typedef unsigned long fb_data;
#endif /* LCD_DEPTH */

#else /* LCD_CHARCELLS */
typedef unsigned char fb_data;
#endif

/* common functions */
extern void lcd_write_command(int byte);
extern void lcd_write_command_e(int cmd, int data);
extern void lcd_write_command_ex(int cmd, int data1, int data2);
extern void lcd_write_data(const fb_data* p_bytes, int count);
extern void lcd_init(void);

#ifdef SIMULATOR
/* Define a dummy device specific init for the sims */
#define lcd_init_device()
#else
extern void lcd_init_device(void);
#endif /* SIMULATOR */

extern void lcd_backlight(bool on);
extern int  lcd_default_contrast(void);
extern void lcd_set_contrast(int val);
extern void lcd_setmargins(int xmargin, int ymargin);
extern int  lcd_getxmargin(void);
extern int  lcd_getymargin(void);
extern int  lcd_getwidth(void);
extern int  lcd_getheight(void);
extern int  lcd_getstringsize(const unsigned char *str, int *w, int *h);

extern void lcd_set_viewport(struct viewport* vp);
extern void lcd_update(void);
extern void lcd_update_viewport(void);
extern void lcd_clear_viewport(void);
extern void lcd_clear_display(void);
extern void lcd_putsxy(int x, int y, const unsigned char *string);
extern void lcd_puts(int x, int y, const unsigned char *string);
extern void lcd_puts_style(int x, int y, const unsigned char *string, int style);
extern void lcd_puts_offset(int x, int y, const unsigned char *str, int offset);
extern void lcd_puts_scroll_offset(int x, int y, const unsigned char *string,
                                  int offset);
extern void lcd_putc(int x, int y, unsigned long ucs);
extern void lcd_stop_scroll(void);
extern void lcd_bidir_scroll(int threshold);
extern void lcd_scroll_speed(int speed);
extern void lcd_scroll_delay(int ms);
extern void lcd_puts_scroll(int x, int y, const unsigned char* string);
extern void lcd_puts_scroll_style(int x, int y, const unsigned char* string,
                                  int style);

#ifdef HAVE_LCD_BITMAP

/* performance function */
#if defined(HAVE_LCD_COLOR)
#define LCD_YUV_DITHER 0x1
extern void lcd_yuv_set_options(unsigned options);
extern void lcd_blit_yuv(unsigned char * const src[3],
                         int src_x, int src_y, int stride,
                         int x, int y, int width, int height);
#else
extern void lcd_blit_mono(const unsigned char *data, int x, int by, int width,
                          int bheight, int stride);
extern void lcd_blit_grey_phase(unsigned char *values, unsigned char *phases,
                                int bx, int by, int bwidth, int bheight,
                                int stride);
#endif


/* update a fraction of the screen */
extern void lcd_update_rect(int x, int y, int width, int height);
extern void lcd_update_viewport_rect(int x, int y, int width, int height);

#ifdef HAVE_REMOTE_LCD
extern void lcd_remote_update(void);
/* update a fraction of the screen */
extern void lcd_remote_update_rect(int x, int y, int width, int height);
#endif /* HAVE_REMOTE_LCD */
#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_LCD_CHARCELLS

/* Icon definitions for lcd_icon() */
enum
{
    ICON_BATTERY = 0,
    ICON_BATTERY_1,
    ICON_BATTERY_2,
    ICON_BATTERY_3,
    ICON_USB,
    ICON_PLAY,
    ICON_RECORD,
    ICON_PAUSE,
    ICON_AUDIO,
    ICON_REPEAT,
    ICON_1,
    ICON_VOLUME,
    ICON_VOLUME_1,
    ICON_VOLUME_2,
    ICON_VOLUME_3,
    ICON_VOLUME_4,
    ICON_VOLUME_5,
    ICON_PARAM
};

void lcd_icon(int icon, bool enable);
void lcd_double_height(bool on);
void lcd_define_pattern(unsigned long ucs, const char *pattern);
unsigned long lcd_get_locked_pattern(void);
void lcd_unlock_pattern(unsigned long ucs);
void lcd_put_cursor(int x, int y, unsigned long cursor_ucs);
void lcd_remove_cursor(void);
#define JUMP_SCROLL_ALWAYS 5
extern void lcd_jump_scroll(int mode); /* 0=off, 1=once, ..., ALWAYS */
extern void lcd_jump_scroll_delay(int ms);
#endif /* HAVE_LCD_CHARCELLS */

/* Draw modes */
#define DRMODE_COMPLEMENT 0
#define DRMODE_BG         1
#define DRMODE_FG         2
#define DRMODE_SOLID      3
#define DRMODE_INVERSEVID 4 /* used as bit modifier for basic modes */

/* Low-level drawing function types */
typedef void lcd_pixelfunc_type(int x, int y);
typedef void lcd_blockfunc_type(fb_data *address, unsigned mask, unsigned bits);
#if LCD_DEPTH >= 8
typedef void lcd_fastpixelfunc_type(fb_data *address);
#endif

#ifdef HAVE_LCD_BITMAP

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

/* Frame buffer dimensions */
#if LCD_DEPTH == 1
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#define LCD_FBWIDTH ((LCD_WIDTH+7)/8)
#else /* LCD_PIXELFORMAT == VERTICAL_PACKING */
#define LCD_FBHEIGHT ((LCD_HEIGHT+7)/8)
#endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#define LCD_FBWIDTH ((LCD_WIDTH+3)/4)
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
#define LCD_FBHEIGHT ((LCD_HEIGHT+3)/4)
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
#define LCD_FBHEIGHT ((LCD_HEIGHT+7)/8)
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
/* The actual framebuffer */
extern fb_data lcd_framebuffer[LCD_FBHEIGHT][LCD_FBWIDTH];

/** Port-specific functions. Enable in port config file. **/
#ifdef HAVE_REMOTE_LCD_AS_MAIN
void lcd_on(void);
void lcd_off(void);
void lcd_poweroff(void);
#endif

#ifdef HAVE_LCD_ENABLE
/* Enable/disable the main display. */
extern void lcd_enable(bool on);
extern bool lcd_enabled(void);
/* Register a hook that is called when the lcd is powered and after the
 * framebuffer data is synchronized */
void lcd_set_enable_hook(void (*enable_hook)(void));
#endif /* HAVE_LCD_ENABLE */
void lcd_call_enable_hook(void);

#ifdef HAVE_LCD_SLEEP
/* Put the LCD into a power saving state deeper than lcd_enable(false). */
extern void lcd_sleep(void);
#endif /* HAVE_LCD_SLEEP */

#ifdef HAVE_LCD_SHUTDOWN
void lcd_shutdown(void);
#endif

/* Bitmap formats */
enum
{
    FORMAT_MONO,
    FORMAT_NATIVE,
    FORMAT_ANY   /* For passing to read_bmp_file() */
};

#define FORMAT_TRANSPARENT 0x40000000
#define FORMAT_DITHER      0x20000000
#define FORMAT_REMOTE      0x10000000

#define TRANSPARENT_COLOR LCD_RGBPACK(255,0,255)
#define REPLACEWITHFG_COLOR LCD_RGBPACK(0,255,255)

struct bitmap {
    int width;
    int height;
#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    int format;
    unsigned char *maskdata;
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

extern void lcd_puts_style_offset(int x, int y, const unsigned char *str,
                                  int style, int offset);
extern void lcd_puts_scroll_style_offset(int x, int y, const unsigned char *string,
                                  int style, int offset);

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

#endif /* HAVE_LCD_BITMAP */

#endif /* __LCD_H__ */
