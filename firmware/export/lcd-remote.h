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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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

#ifdef HAVE_REMOTE_LCD /* Not for the players with *only* a remote LCD (m3) */

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
#define REMOTETYPE_UNPLUGGED 0
#define REMOTETYPE_H100_LCD 1
#define REMOTETYPE_H300_LCD 2
#define REMOTETYPE_H300_NONLCD 3
    int remote_type(void);
#endif

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
#define LCD_REMOTE_STRIDE(w, h) (h)
#define LCD_REMOTE_FBSTRIDE(w, h) ((h+7)/8)
#define LCD_REMOTE_FBHEIGHT LCD_REMOTE_FBSTRIDE(LCD_REMOTE_WIDTH, LCD_REMOTE_HEIGHT)
#define LCD_REMOTE_NBELEMS(w, h) ((((w-1)*LCD_REMOTE_FBSTRIDE(w, h)) + h)  / sizeof(fb_remote_data))
#elif LCD_REMOTE_DEPTH == 2
#if LCD_REMOTE_PIXELFORMAT == VERTICAL_INTERLEAVED
#define LCD_REMOTE_STRIDE(w, h) (h)
#define LCD_REMOTE_FBSTRIDE(w, h) ((h+7)/8)
#define LCD_REMOTE_FBHEIGHT LCD_REMOTE_FBSTRIDE(LCD_REMOTE_WIDTH, LCD_REMOTE_HEIGHT)
#define LCD_REMOTE_NBELEMS(w, h) ((((w-1)*LCD_REMOTE_FBSTRIDE(w, h)) + h)  / sizeof(fb_remote_data))
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

#ifndef LCD_REMOTE_NBELEMS
/* At this time (2020) known remote screens only have vertical stride */
#define LCD_REMOTE_NBELEMS(w, h) (((w-1)*STRIDE_REMOTE(w, h)) + h) / sizeof(fb_remote_data))
#define LCD_REMOTE_STRIDE(w, h) STRIDE_REMOTE(w, h)
#define LCD_REMOTE_FBSTRIDE(w, h) STRIDE_REMOTE(w, h)
#endif

#ifndef LCD_REMOTE_NATIVE_STRIDE
#define LCD_REMOTE_NATIVE_STRIDE(s) (s)
#endif

extern struct viewport* lcd_remote_current_viewport;
#define FBREMOTEADDR(x,y) (fb_remote_data *)(lcd_remote_current_viewport->buffer->get_address_fn(x, y))
#define FRAMEBUFFER_REMOTE_SIZE (sizeof(fb_remote_data)*LCD_REMOTE_FBWIDTH*LCD_REMOTE_FBHEIGHT)

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

/* common functions */
void lcd_remote_init(void);
void lcd_remote_write_command(int cmd);
void lcd_remote_write_command_ex(int cmd, int data);
void lcd_remote_write_data(const fb_remote_data *data, int count);
extern void lcd_remote_set_framebuffer(fb_remote_data *fb);

extern void lcd_remote_bitmap_part(const fb_remote_data *src, int src_x,
                                   int src_y, int stride, int x, int y,
                                   int width, int height);
extern void lcd_remote_bitmap(const fb_remote_data *src, int x, int y,
                              int width, int height);
extern void lcd_remote_nine_segment_bmp(const struct bitmap* bm, int x, int y,
                                        int width, int height);

/* Low-level drawing function types */
typedef void lcd_remote_pixelfunc_type(int x, int y);
typedef void lcd_remote_blockfunc_type(fb_remote_data *address, unsigned mask,
                                       unsigned bits);

/* low level drawing function pointer arrays */
#if LCD_REMOTE_DEPTH > 1
    extern lcd_remote_pixelfunc_type* const *lcd_remote_pixelfuncs;
    extern lcd_remote_blockfunc_type* const *lcd_remote_blockfuncs;
#else
    extern lcd_remote_pixelfunc_type* const lcd_remote_pixelfuncs[8];
    extern lcd_remote_blockfunc_type* const lcd_remote_blockfuncs[8];
#endif

#endif /* HAVE_LCD_REMOTE */

#ifdef HAVE_REMOTE_LCD_TICKING
    void lcd_remote_emireduce(bool state);
#endif

void lcd_remote_init_device(void);
void lcd_remote_on(void);
void lcd_remote_off(void);

extern bool remote_initialized;
bool remote_detect(void);

extern void lcd_remote_init(void);
extern int  lcd_remote_default_contrast(void);
extern void lcd_remote_set_contrast(int val);

extern struct viewport* lcd_remote_init_viewport(struct viewport* vp);
extern struct viewport* lcd_remote_set_viewport(struct viewport* vp);
extern struct viewport* lcd_remote_set_viewport_ex(struct viewport* vp, int flags);

extern void lcd_remote_clear_display(void);
extern void lcd_remote_clear_viewport(void);
extern void lcd_remote_puts(int x, int y, const unsigned char *str);
extern void lcd_remote_putsf(int x, int y, const unsigned char *fmt, ...);
extern void lcd_remote_putc(int x, int y, unsigned short ch);
extern bool lcd_remote_puts_scroll(int x, int y, const unsigned char *str);
extern bool lcd_remote_putsxy_scroll_func(int x, int y, const unsigned char *string,
                                          void (*scroll_func)(struct scrollinfo *),
                                          void *data, int x_offset);

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

extern void lcd_remote_drawpixel(int x, int y);
extern void lcd_remote_drawline(int x1, int y1, int x2, int y2);
extern void lcd_remote_hline(int x1, int x2, int y);
extern void lcd_remote_vline(int x, int y1, int y2);
extern void lcd_remote_drawrect(int x, int y, int width, int height);
extern void lcd_remote_fillrect(int x, int y, int width, int height);
extern void lcd_remote_draw_border_viewport(void);
extern void lcd_remote_fill_viewport(void);
extern void lcd_remote_putsxy(int x, int y, const unsigned char *str);
extern void lcd_remote_putsxyf(int x, int y, const unsigned char *fmt, ...);

extern void lcd_remote_bidir_scroll(int threshold);
extern void lcd_remote_scroll_step(int pixels);

extern void lcd_remote_bmp_part(const struct bitmap* bm, int src_x, int src_y,
                                int x, int y, int width, int height);
extern void lcd_remote_bmp(const struct bitmap* bm, int x, int y);

#endif /* __LCD_REMOTE_H__ */
