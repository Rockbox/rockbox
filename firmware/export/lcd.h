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
#define MAX_PATH 260
#else
#include "file.h"  /* for MAX_PATH; FIXME: Why does this not work for sims? */
#endif

/* common functions */
extern void lcd_write_command(int byte);
extern void lcd_write_command_ex(int cmd, int data1, int data2);
extern void lcd_write_data(const unsigned char* p_bytes, int count);
extern void lcd_init(void);
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
extern void lcd_blit(const unsigned char* data, int x, int by, int width,
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

#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)

/* Memory copy of display bitmap */
#if LCD_DEPTH == 1
extern unsigned char lcd_framebuffer[LCD_HEIGHT/8][LCD_WIDTH];
#elif LCD_DEPTH == 2
#define MAX_LEVEL 3
extern unsigned char lcd_framebuffer[LCD_HEIGHT/4][LCD_WIDTH];
#endif

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

/* low level drawing function pointer arrays */
extern lcd_pixelfunc_type* lcd_pixelfuncs[8];
extern lcd_blockfunc_type* lcd_blockfuncs[8];

extern void lcd_drawpixel(int x, int y);
extern void lcd_drawline(int x1, int y1, int x2, int y2);
extern void lcd_hline(int x1, int x2, int y);
extern void lcd_vline(int x, int y1, int y2);
extern void lcd_drawrect(int x, int y, int width, int height);
extern void lcd_fillrect(int x, int y, int width, int height);
extern void lcd_bitmap_part(const unsigned char *src, int src_x, int src_y,
                            int stride, int x, int y, int width, int height);
extern void lcd_bitmap(const unsigned char *src, int x, int y, int width,
                       int height);
extern void lcd_putsxy(int x, int y, const unsigned char *string);

extern void lcd_invertscroll(int x, int y);
extern void lcd_bidir_scroll(int threshold);
extern void lcd_scroll_step(int pixels);

#if LCD_DEPTH > 1
extern void lcd_set_foreground(int brightness);
extern int lcd_get_foreground(void);
extern void lcd_set_background(int brightness);
extern int lcd_get_background(void);
extern void lcd_mono_bitmap_part(const unsigned char *src, int src_x, int src_y,
                            int stride, int x, int y, int width, int height);
extern void lcd_mono_bitmap(const unsigned char *src, int x, int y, int width,
                       int height);
#else /* LCD_DEPTH == 1 */
#define lcd_mono_bitmap lcd_bitmap
#define lcd_mono_bitmap_part lcd_bitmap_part
#endif

#endif /* CHARCELLS / BITMAP */

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
