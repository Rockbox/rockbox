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
#include "sh7034.h"
#include "config.h"

#define STYLE_DEFAULT 0
#define STYLE_INVERT  1

/* common functions */
extern void lcd_init(void);
extern void lcd_clear_display(void);
extern void lcd_backlight(bool on);
extern void lcd_puts(int x, int y, const unsigned char *string);
extern void lcd_puts_style(int x, int y, const unsigned char *string, int style);
extern void lcd_putc(int x, int y, unsigned short ch);

extern void lcd_puts_scroll(int x, int y, const unsigned char* string );
extern void lcd_puts_scroll_style(int x, int y, const unsigned char* string,
                                  int style);
extern void lcd_icon(int icon, bool enable);
extern void lcd_stop_scroll(void);
extern void lcd_scroll_speed( int speed );
extern void lcd_scroll_delay( int ms );
extern void lcd_set_contrast(int val);
extern void lcd_write_command( int byte );
extern void lcd_write_data( const unsigned char* p_bytes, int count );
extern int  lcd_default_contrast(void);

#if defined(SIMULATOR) || defined(HAVE_LCD_BITMAP)
extern void lcd_update(void);
/* performance function */
extern void lcd_blit (const unsigned char* p_data, int x, int y, int width,
                      int height, int stride);

/* update a fraction of the screen */
extern void lcd_update_rect(int x, int y, int width, int height);
#else
  #define lcd_update()
  #define lcd_update_rect(x,y,w,h)
#endif

#if defined(SIMULATOR)
#include "sim_icons.h"
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

extern void lcd_define_hw_pattern (int which,const char *pattern,int length);
extern void lcd_define_pattern (int which,const char *pattern);
extern void lcd_double_height (bool on);
#define JUMP_SCROLL_ALWAYS 5
extern void lcd_jump_scroll (int mode); /* 0=off, 1=once, ..., ALWAYS */
extern void lcd_jump_scroll_delay( int ms );
unsigned char lcd_get_locked_pattern(void);
void lcd_unlock_pattern(unsigned char pat);
void lcd_allow_bidirectional_scrolling(bool on);
extern void lcd_bidir_scroll(int threshold);
void lcd_put_cursor(int x, int y, char cursor_char);
void lcd_remove_cursor(void);
#endif

#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)
#if defined(HAVE_LCD_CHARCELLS) && defined(SIMULATOR)
#endif

#define DRAW_PIXEL(x,y) lcd_framebuffer[(y)/8][(x)] |= (1<<((y)&7))
#define CLEAR_PIXEL(x,y) lcd_framebuffer[(y)/8][(x)] &= ~(1<<((y)&7))
#define INVERT_PIXEL(x,y) lcd_framebuffer[(y)/8][(x)] ^= (1<<((y)&7))

/*
 * Memory copy of display bitmap
 */
extern unsigned char lcd_framebuffer[LCD_HEIGHT/8][LCD_WIDTH];

extern void lcd_setmargins(int xmargin, int ymargin);
extern int  lcd_getxmargin(void);
extern int  lcd_getymargin(void);
extern void lcd_bitmap (const unsigned char *src, int x, int y, int nx, int ny,
			bool clear);
extern void lcd_clearrect (int x, int y, int nx, int ny);
extern void lcd_fillrect (int x, int y, int nx, int ny);
extern void lcd_drawrect (int x, int y, int nx, int ny);
extern void lcd_invertrect (int x, int y, int nx, int ny);
extern void lcd_invertscroll(int x, int y);
extern void lcd_drawline( int x1, int y1, int x2, int y2 );
extern void lcd_clearline( int x1, int y1, int x2, int y2 );
extern void lcd_drawpixel(int x, int y);
extern void lcd_clearpixel(int x, int y);
extern void lcd_invertpixel(int x, int y);
extern void lcd_roll(int pixels);
extern void lcd_set_invert_display(bool yesno);
extern void lcd_set_flip(bool yesno);
extern void lcd_bidir_scroll(int threshold);
extern void lcd_scroll_step(int pixels);
extern void lcd_setfont(int font);
extern void lcd_putsxy(int x, int y, const unsigned char *string);
extern int  lcd_getstringsize(const unsigned char *str, int *w, int *h);

#endif /* CHARCELLS / BITMAP */

#endif /* __LCD_H__ */
