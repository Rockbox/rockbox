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

#define STYLE_DEFAULT 0
#define STYLE_INVERT  1

#ifdef SIMULATOR
#define lcd_icon(x,y) sim_lcd_icon(x,y)
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#else
#include "file.h"  /* for MAX_PATH; FIXME: Why does this not work for sims? */
#endif

#if LCD_DEPTH <=8
typedef unsigned char fb_data;
#elif LCD_DEPTH <= 16
typedef unsigned short fb_data;
#else
typedef unsigned long fb_data;
#endif

/* common functions */
extern void lcd_write_command(int byte);
extern void lcd_write_command_ex(int cmd, int data1, int data2);
extern void lcd_write_data(const fb_data* p_bytes, int count);
extern void lcd_init(void);
#ifdef SIMULATOR
/* Define a dummy device specific init for the sims */
#define lcd_init_device()
#else
extern void lcd_init_device(void);
#endif
extern void lcd_backlight(bool on);
extern int  lcd_default_contrast(void);
extern void lcd_set_contrast(int val);

extern void lcd_clear_display(void);
extern void lcd_puts(int x, int y, const unsigned char *string);
extern void lcd_puts_style(int x, int y, const unsigned char *string, int style);
extern void lcd_putc(int x, int y, unsigned short ch);
extern void lcd_stop_scroll(void);
extern void lcd_scroll_speed(int speed);
extern void lcd_scroll_delay(int ms);
extern void lcd_puts_scroll(int x, int y, const unsigned char* string);
extern void lcd_puts_scroll_style(int x, int y, const unsigned char* string,
                                  int style);
extern void lcd_icon(int icon, bool enable);

#if defined(SIMULATOR) || defined(HAVE_LCD_BITMAP)
/* performance function */
extern void lcd_blit(const fb_data* data, int x, int by, int width,
                     int bheight, int stride);

extern void lcd_update(void);
/* update a fraction of the screen */
extern void lcd_update_rect(int x, int y, int width, int height);

#ifdef HAVE_REMOTE_LCD
extern void lcd_remote_update(void);
/* update a fraction of the screen */
extern void lcd_remote_update_rect(int x, int y, int width, int height);
#endif

#else
  #define lcd_update()
  #define lcd_update_rect(x,y,w,h)
#endif

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

extern void lcd_double_height(bool on);
extern void lcd_define_hw_pattern(int which,const char *pattern,int length);
extern void lcd_define_pattern(int which,const char *pattern);
unsigned char lcd_get_locked_pattern(void);
void lcd_unlock_pattern(unsigned char pat);
void lcd_put_cursor(int x, int y, char cursor_char);
void lcd_remove_cursor(void);
extern void lcd_bidir_scroll(int threshold);
#define JUMP_SCROLL_ALWAYS 5
extern void lcd_jump_scroll(int mode); /* 0=off, 1=once, ..., ALWAYS */
extern void lcd_jump_scroll_delay(int ms);
#endif

/* Draw modes */
#define DRMODE_COMPLEMENT 0
#define DRMODE_BG         1
#define DRMODE_FG         2
#define DRMODE_SOLID      3
#define DRMODE_INVERSEVID 4 /* used as bit modifier for basic modes */

/* Low-level drawing function types */
typedef void lcd_pixelfunc_type(int x, int y);
typedef void lcd_blockfunc_type(unsigned char *address, unsigned mask, unsigned bits);
#if LCD_DEPTH >= 8
typedef void lcd_fastpixelfunc_type(fb_data *address);
#endif

#ifdef HAVE_LCD_BITMAP

#ifdef HAVE_LCD_COLOR
#if LCD_DEPTH == 16
#define LCD_MAX_RED    31
#define LCD_MAX_GREEN  63
#define LCD_MAX_BLUE   31
#define _RGBPACK(r, g, b) ( ((((r) * 31 + 127) / 255) << 11) \
                           |((((g) * 63 + 127) / 255) << 5) \
                           | (((b) * 31 + 127) / 255))
#if (LCD_PIXELFORMAT == RGB565SWAPPED)
#define LCD_RGBPACK(r, g, b) ( ((_RGBPACK((r), (g), (b)) & 0xff00) >> 8) \
                              |((_RGBPACK((r), (g), (b)) & 0x00ff) << 8))
#else
#define LCD_RGBPACK(r, g, b) _RGBPACK((r), (g), (b))
#endif
#elif LCD_DEPTH == 18
#define LCD_MAX_RED    63
#define LCD_MAX_GREEN  63
#define LCD_MAX_BLUE   63
#define LCD_RGBPACK(r, g, b) ( ((((r) * 63 + 127) / 255) << 12) \
                           |((((g) * 63 + 127) / 255) << 6) \
                           | (((b) * 63 + 127) / 255))
#else
/* other colour depths */
#endif 

#define LCD_BLACK      LCD_RGBPACK(0, 0, 0)
#define LCD_DARKGRAY   LCD_RGBPACK(85, 85, 85)
#define LCD_LIGHTGRAY  LCD_RGBPACK(170, 170, 170)
#define LCD_WHITE      LCD_RGBPACK(255, 255, 255)
#define LCD_DEFAULT_FG LCD_BLACK
#define LCD_DEFAULT_BG LCD_RGBPACK(182, 198, 229) /* rockbox blue */

#elif LCD_DEPTH > 1 /* greyscale */
#define LCD_MAX_LEVEL ((1 << LCD_DEPTH) - 1)
#define LCD_BRIGHTNESS(y) (((y) * LCD_MAX_LEVEL + 127) / 255)

#define LCD_BLACK      LCD_BRIGHTNESS(0)
#define LCD_DARKGRAY   LCD_BRIGHTNESS(85)
#define LCD_LIGHTGRAY  LCD_BRIGHTNESS(170)
#define LCD_WHITE      LCD_BRIGHTNESS(255)
#define LCD_DEFAULT_FG LCD_BLACK
#define LCD_DEFAULT_BG LCD_WHITE
#endif

/* Memory copy of display bitmap */
#if LCD_DEPTH == 1
extern fb_data lcd_framebuffer[LCD_HEIGHT/8][LCD_WIDTH];
#elif LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#define FB_WIDTH ((LCD_WIDTH+3)/4)
extern fb_data lcd_framebuffer[LCD_HEIGHT][FB_WIDTH];
#else
extern fb_data lcd_framebuffer[LCD_HEIGHT/4][LCD_WIDTH];
#endif
#elif LCD_DEPTH == 16
extern fb_data lcd_framebuffer[LCD_HEIGHT][LCD_WIDTH];
#elif LCD_DEPTH == 18
extern fb_data lcd_framebuffer[LCD_HEIGHT][LCD_WIDTH];
#endif

#if (CONFIG_BACKLIGHT==BL_IRIVER_H300) || (CONFIG_BACKLIGHT==BL_IPOD3G)
extern void lcd_enable(bool on);
#endif

/* Bitmap formats */
enum
{
    FORMAT_MONO,
    FORMAT_NATIVE,
    FORMAT_ANY   /* For passing to read_bmp_file() */
};

#define FORMAT_TRANSPARENT 0x40000000

#define TRANSPARENT_COLOR LCD_RGBPACK(255,0,255)

struct bitmap {
    int width;
    int height;
#if LCD_DEPTH > 1
    int format;
    unsigned char *maskdata;
#endif
    unsigned char *data;
};

extern void lcd_set_invert_display(bool yesno);
extern void lcd_set_flip(bool yesno);
extern void lcd_roll(int pixels);

extern void lcd_set_drawmode(int mode);
extern int  lcd_get_drawmode(void);
extern void lcd_setmargins(int xmargin, int ymargin);
extern int  lcd_getxmargin(void);
extern int  lcd_getymargin(void);
extern void lcd_setfont(int font);
extern int  lcd_getstringsize(const unsigned char *str, int *w, int *h);

extern void lcd_puts_offset(int x, int y, const unsigned char *str, int offset);
extern void lcd_puts_style_offset(int x, int y, const unsigned char *str, 
                                  int style, int offset);
extern void lcd_puts_scroll_offset(int x, int y, const unsigned char *string,
                                  int offset);
extern void lcd_puts_scroll_style_offset(int x, int y, const unsigned char *string,
                                  int style, int offset);                                  

/* low level drawing function pointer arrays */
extern lcd_pixelfunc_type* const lcd_pixelfuncs[8];
extern lcd_blockfunc_type* const lcd_blockfuncs[8];
#if LCD_DEPTH >= 8
extern lcd_fastpixelfunc_type* const * lcd_fastpixelfuncs;
#endif

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
extern void lcd_putsxy(int x, int y, const unsigned char *string);

extern void lcd_invertscroll(int x, int y);
extern void lcd_bidir_scroll(int threshold);
extern void lcd_scroll_step(int pixels);

#if LCD_DEPTH > 1
extern void     lcd_set_foreground(unsigned foreground);
extern unsigned lcd_get_foreground(void);
extern void     lcd_set_background(unsigned background);
extern unsigned lcd_get_background(void);
extern void     lcd_set_drawinfo(int mode, unsigned foreground,
                                 unsigned background);
#ifdef HAVE_LCD_COLOR
void lcd_set_backdrop(fb_data* backdrop);
fb_data* lcd_get_backdrop(void);
#endif

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
#endif

#endif /* HAVE_LCD_BITMAP */

/* internal usage, but in multiple drivers */
#ifdef HAVE_LCD_BITMAP 
#define SCROLL_SPACING 3

struct scrollinfo {
    char line[MAX_PATH + LCD_WIDTH/2 + SCROLL_SPACING + 2];
    int len;    /* length of line in chars */
    int width;  /* length of line in pixels */
    int offset;
    int startx;
    bool backward; /* scroll presently forward or backward? */
    bool bidir;
    bool invert; /* invert the scrolled text */
    long start_tick;
};
#else /* !HAVE_LCD_BITMAP */

struct scrollinfo {
    int mode;
    char text[MAX_PATH];
    int textlen;
    int offset;
    int turn_offset;
    int startx;
    int starty;
    long scroll_start_tick;
    int direction; /* +1 for right or -1 for left*/
    int jump_scroll;
    int jump_scroll_steps;
};
#endif

#endif /* __LCD_H__ */
