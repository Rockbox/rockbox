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
extern void lcd_puts(int x, int y, char *string);

#if defined(SIMULATOR) || defined(HAVE_LCD_BITMAP)
  extern void lcd_update(void);
#else
  #define lcd_update()
#endif

#ifdef HAVE_LCD_CHARCELLS
#    define LCD_ICON_BATTERY         0
#      define LCD_BATTERY_FRAME   0x02
#      define LCD_BATTERY_BAR1    0x08
#      define LCD_BATTERY_BAR2    0x04
#      define LCD_BATTERY_BAR3    0x10
#    define LCD_ICON_USB             2
#      define LCD_USB_LOGO        0xFF
#    define LCD_ICON_PLAY            3
#      define LCD_PLAY_ICON       0xFF
#    define LCD_ICON_RECORD          4
#      define LCD_RECORD_ICON     0x10
#    define LCD_ICON_STOP            5
#      define LCD_STOP_ICON       0x0F
#    define LCD_ICON_AUDIO           5
#      define LCD_AUDIO_ICON      0xF0
#    define LCD_ICON_REVERSE         6
#      define LCD_REVERSE_ICON    0xFF
#    define LCD_ICON_SINGLE          7
#      define LCD_SINGLE_ICON     0xFF
#    define LCD_ICON_VOLUME0         9
#      define LCD_VOLUME_ICON     0x04   
#      define LCD_VOLUME_BAR1     0x02
#      define LCD_VOLUME_BAR2     0x01
#    define LCD_ICON_VOLUME1        10
#      define LCD_VOLUME_BAR3     0x08
#      define LCD_VOLUME_BAR4     0x04
#      define LCD_VOLUME_BAR5     0x01
#    define LCD_ICON_PARAM          10
#      define LCD_PARAM_SYMBOL    0xF0

extern void lcd_define_pattern (int which,char *pattern,int length);

#endif
#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)

#if defined(HAVE_LCD_CHARCELLS) && defined(SIMULATOR)
#include <chardef.h>
#endif

#define LCD_WIDTH       112   /* Display width in pixels */
#define LCD_HEIGHT      64    /* Display height in pixels */

extern void lcd_putsxy(int x, int y, char *string, int font);
extern void lcd_setfont(int font);
extern void lcd_getfontsize(int font, int *width, int *height);
extern void lcd_setmargins(int xmargin, int ymargin);
extern void lcd_bitmap (unsigned char *src, int x, int y, int nx, int ny,
			bool clear);
extern void lcd_clearrect (int x, int y, int nx, int ny);
extern void lcd_fillrect (int x, int y, int nx, int ny);
extern void lcd_drawrect (int x, int y, int nx, int ny);
extern void lcd_invertrect (int x, int y, int nx, int ny);
extern void lcd_drawline( int x1, int y1, int x2, int y2 );
extern void lcd_drawpixel(int x, int y);
extern void lcd_clearpixel(int x, int y);


#if defined(HAVE_LCD_CHARCELLS) && defined(SIMULATOR)
#include <charundef.h>
#endif

#endif /* CHARCELLS / BITMAP */

#endif /* __LCD_H__ */
