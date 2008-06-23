/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Richard S. La Charité
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __LCD_REMOTE_H__
#define __LCD_REMOTE_H__

#include <stdbool.h>
#include "cpu.h"
#include "config.h"
#include "lcd.h"

#ifdef HAVE_REMOTE_LCD

#if defined(TARGET_TREE) && !defined(__PCTOOL__)
#include "lcd-remote-target.h"
#endif

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
#define REMOTETYPE_UNPLUGGED 0
#define REMOTETYPE_H100_LCD 1
#define REMOTETYPE_H300_LCD 2
#define REMOTETYPE_H300_NONLCD 3
int remote_type(void);
#endif

#define STYLE_DEFAULT    0x00000000
#define STYLE_INVERT     0x20000000

#if LCD_REMOTE_DEPTH <= 8
#if (LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED) \
 || (LCD_REMOTE_PIXELFORMAT == HORIZONTAL_INTERLEAVED)
typedef unsigned short fb_remote_data;
#else
typedef unsigned char fb_remote_data;
#endif
#elif LCD_DEPTH <= 16
typedef unsigned short fb_remote_data;
#else
typedef unsigned long fb_remote_data;
#endif

/* common functions */
void lcd_remote_init(void);
void lcd_remote_write_command(int cmd);
void lcd_remote_write_command_ex(int cmd, int data);
void lcd_remote_write_data(const fb_remote_data *data, int count);

/* Low-level drawing function types */
typedef void lcd_remote_pixelfunc_type(int x, int y);
typedef void lcd_remote_blockfunc_type(fb_remote_data *address, unsigned mask,
                                       unsigned bits);

#if LCD_REMOTE_DEPTH > 1 /* greyscale - 8 bit max */
#ifdef HAVE_LCD_COLOR
extern unsigned lcd_remote_color_to_native(unsigned color);
#endif

#define LCD_REMOTE_MAX_LEVEL ((1 << LCD_REMOTE_DEPTH) - 1)
/**
 * On 2 bit for example (y >> (8-DEPTH)) = (y >> 6) = y/64 gives:
 *  |000-063|064-127|128-191|192-255|
 *  |   0   |   1   |   2   |   3   |
 */
#define LCD_REMOTE_BRIGHTNESS(y) ((y) >> (8-LCD_REMOTE_DEPTH))

#define LCD_REMOTE_BLACK      LCD_REMOTE_BRIGHTNESS(0)
#define LCD_REMOTE_DARKGRAY   LCD_REMOTE_BRIGHTNESS(85)
#define LCD_REMOTE_LIGHTGRAY  LCD_REMOTE_BRIGHTNESS(170)
#define LCD_REMOTE_WHITE      LCD_REMOTE_BRIGHTNESS(255)
#define LCD_REMOTE_DEFAULT_FG LCD_REMOTE_BLACK
#define LCD_REMOTE_DEFAULT_BG LCD_REMOTE_WHITE
#endif

/* Frame buffer dimensions (format checks only cover existing targets!) */
#if LCD_REMOTE_DEPTH == 1
#define LCD_REMOTE_FBHEIGHT ((LCD_REMOTE_HEIGHT+7)/8)
#elif LCD_REMOTE_DEPTH == 2
#if LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED
#define LCD_REMOTE_FBHEIGHT ((LCD_REMOTE_HEIGHT+7)/8)
#endif
#endif /* LCD_REMOTE_DEPTH */
/* Set defaults if not defined different yet. The defaults apply to both
 * dimensions for LCD_REMOTE_DEPTH >= 8 */
#ifndef LCD_REMOTE_FBWIDTH
#define LCD_REMOTE_FBWIDTH LCD_REMOTE_WIDTH
#endif
#ifndef LCD_REMOTE_FBHEIGHT
#define LCD_REMOTE_FBHEIGHT LCD_REMOTE_HEIGHT
#endif
/* The actual framebuffer */
extern fb_remote_data lcd_remote_framebuffer[LCD_REMOTE_FBHEIGHT][LCD_REMOTE_FBWIDTH];


extern void lcd_remote_init(void);
extern int  lcd_remote_default_contrast(void);
extern void lcd_remote_set_contrast(int val);

extern void lcd_remote_set_viewport(struct viewport* vp);
extern void lcd_remote_clear_display(void);
extern void lcd_remote_clear_viewport(void);
extern void lcd_remote_puts(int x, int y, const unsigned char *str);
extern void lcd_remote_puts_style(int x, int y, const unsigned char *str,
                                  int style);
extern void lcd_remote_puts_offset(int x, int y, const unsigned char *str,
                                   int offset);
extern void lcd_remote_puts_style_offset(int x, int y, const unsigned char *str,
                                         int style, int offset);
extern void lcd_remote_putc(int x, int y, unsigned short ch);
extern void lcd_remote_stop_scroll(void);
extern void lcd_remote_scroll_speed(int speed);
extern void lcd_remote_scroll_delay(int ms);
extern void lcd_remote_puts_scroll(int x, int y, const unsigned char *str);
extern void lcd_remote_puts_scroll_style(int x, int y, const unsigned char *str,
                                         int style);
extern void lcd_remote_puts_scroll_offset(int x, int y,
                                          const unsigned char *str, int offset);
extern void lcd_remote_puts_scroll_style_offset(int x, int y,
                                                const unsigned char *string,
                                                int style, int offset);

extern void lcd_remote_update(void);
extern void lcd_remote_update_rect(int x, int y, int width, int height);
extern void lcd_remote_update_viewport(void);
extern void lcd_remote_update_viewport_rect(int x, int y, int width, int height);

extern void lcd_remote_set_invert_display(bool yesno);
extern void lcd_remote_set_flip(bool yesno);

extern void lcd_remote_set_drawmode(int mode);
extern int  lcd_remote_get_drawmode(void);
extern int  lcd_remote_getwidth(void);
extern int  lcd_remote_getheight(void);
extern void lcd_remote_setfont(int font);
extern int  lcd_remote_getfont(void);
extern int  lcd_remote_getstringsize(const unsigned char *str, int *w, int *h);

/* low level drawing function pointer arrays */
#if LCD_REMOTE_DEPTH > 1
extern lcd_remote_pixelfunc_type* const *lcd_remote_pixelfuncs;
extern lcd_remote_blockfunc_type* const *lcd_remote_blockfuncs;
#else
extern lcd_remote_pixelfunc_type* const lcd_remote_pixelfuncs[8];
extern lcd_remote_blockfunc_type* const lcd_remote_blockfuncs[8];
#endif

extern void lcd_remote_drawpixel(int x, int y);
extern void lcd_remote_drawline(int x1, int y1, int x2, int y2);
extern void lcd_remote_hline(int x1, int x2, int y);
extern void lcd_remote_vline(int x, int y1, int y2);
extern void lcd_remote_drawrect(int x, int y, int width, int height);
extern void lcd_remote_fillrect(int x, int y, int width, int height);
extern void lcd_remote_bitmap_part(const fb_remote_data *src, int src_x,
                                   int src_y, int stride, int x, int y,
                                   int width, int height);
extern void lcd_remote_bitmap(const fb_remote_data *src, int x, int y,
                              int width, int height);
extern void lcd_remote_putsxy(int x, int y, const unsigned char *str);

extern void lcd_remote_bidir_scroll(int threshold);
extern void lcd_remote_scroll_step(int pixels);

#if LCD_REMOTE_DEPTH > 1
extern void     lcd_remote_set_foreground(unsigned foreground);
extern unsigned lcd_remote_get_foreground(void);
extern void     lcd_remote_set_background(unsigned background);
extern unsigned lcd_remote_get_background(void);
extern void     lcd_remote_set_drawinfo(int mode, unsigned foreground,
                                        unsigned background);
void lcd_remote_set_backdrop(fb_remote_data* backdrop);
fb_remote_data* lcd_remote_get_backdrop(void);

extern void lcd_remote_mono_bitmap_part(const unsigned char *src, int src_x,
                                        int src_y, int stride, int x, int y,
                                        int width, int height);
extern void lcd_remote_mono_bitmap(const unsigned char *src, int x, int y,
                                   int width, int height);
extern void lcd_remote_bitmap_transparent_part(const fb_remote_data *src,
                                               int src_x, int src_y,
                                               int stride, int x, int y,
                                               int width, int height);
extern void lcd_bitmap_remote_transparent(const fb_remote_data *src, int x,
                                          int y, int width, int height);
#else /* LCD_REMOTE_DEPTH == 1 */
#define lcd_remote_mono_bitmap lcd_remote_bitmap
#define lcd_remote_mono_bitmap_part lcd_remote_bitmap_part
#endif /* LCD_REMOTE_DEPTH */

#endif
#endif /* __LCD_REMOTE_H__ */
