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

/* common functions */
extern void lcd_init(void);
extern void lcd_clear_display(void);
extern void lcd_backlight(bool on);
extern void lcd_puts(int x, int y, unsigned char *string);
extern void lcd_putc(int x, int y, unsigned char ch);
extern void lcd_scroll_pause(void);
extern void lcd_scroll_pause_line(int line);
extern void lcd_scroll_resume(void);
extern void lcd_scroll_resume_line(int line);
extern void lcd_puts_scroll(int x, int y, unsigned char* string );
extern void lcd_icon(int icon, bool enable);
extern void lcd_stop_scroll(void);
extern void lcd_stop_scroll_line(int line);
extern void lcd_scroll_speed( int speed );
extern void lcd_set_contrast(int val);
extern void lcd_write( bool command, int byte );

#if defined(SIMULATOR) || defined(HAVE_LCD_BITMAP)
extern void lcd_update(void);

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

extern void lcd_define_pattern (int which,char *pattern,int length);
extern void lcd_double_height (bool on);

#endif

#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)

#if !defined(HAVE_LCD_BITMAP) && defined(SIMULATOR)
#define LCD_WIDTH       16*6  /* Display width in pixels */
#define LCD_HEIGHT      32    /* Display height in pixels */
#else
#define LCD_WIDTH       112   /* Display width in pixels */
#define LCD_HEIGHT      64    /* Display height in pixels */
#endif

#define DRAW_PIXEL(x,y) lcd_framebuffer[(x)][(y)/8] |= (1<<((y)&7))
#define CLEAR_PIXEL(x,y) lcd_framebuffer[(x)][(y)/8] &= ~(1<<((y)&7))
#define INVERT_PIXEL(x,y) lcd_framebuffer[(x)][(y)/8] ^= (1<<((y)&7))

/*
 * Memory copy of display bitmap
 */
extern unsigned char lcd_framebuffer[LCD_WIDTH][LCD_HEIGHT/8];

extern void lcd_setmargins(int xmargin, int ymargin);
extern int  lcd_getxmargin(void);
extern int  lcd_getymargin(void);
extern void lcd_bitmap (unsigned char *src, int x, int y, int nx, int ny,
			bool clear);
extern void lcd_clearrect (int x, int y, int nx, int ny);
extern void lcd_fillrect (int x, int y, int nx, int ny);
extern void lcd_drawrect (int x, int y, int nx, int ny);
extern void lcd_invertrect (int x, int y, int nx, int ny);
extern void lcd_drawline( int x1, int y1, int x2, int y2 );
extern void lcd_clearline( int x1, int y1, int x2, int y2 );
extern void lcd_drawpixel(int x, int y);
extern void lcd_clearpixel(int x, int y);
extern void lcd_invertpixel(int x, int y);
extern void lcd_roll(int pixels);

extern void lcd_setfont(int font);
extern void lcd_putsxy(int x, int y, unsigned char *string);
extern int  lcd_getstringsize(unsigned char *str, int *w, int *h);

#endif /* CHARCELLS / BITMAP */


#endif /* __LCD_H__ */
