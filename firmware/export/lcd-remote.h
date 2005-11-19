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

#ifdef HAVE_REMOTE_LCD

#define STYLE_DEFAULT 0
#define STYLE_INVERT  1

extern void lcd_remote_init(void);
extern void lcd_remote_backlight_on(void);
extern void lcd_remote_backlight_off(void);
extern int  lcd_remote_default_contrast(void);
extern void lcd_remote_set_contrast(int val);
extern void lcd_remote_emireduce(bool state);

extern void lcd_remote_clear_display(void);
extern void lcd_remote_puts(int x, int y, const unsigned char *string);
extern void lcd_remote_puts_style(int x, int y, const unsigned char *string,
                                  int style);
extern void lcd_remote_putc(int x, int y, unsigned short ch);
extern void lcd_remote_stop_scroll(void);
extern void lcd_remote_scroll_speed(int speed);
extern void lcd_remote_scroll_delay(int ms);
extern void lcd_remote_puts_scroll(int x, int y, const unsigned char* string);
extern void lcd_remote_puts_scroll_style(int x, int y,
                                         const unsigned char* string, int style);
                                                               
extern void lcd_remote_update(void);
extern void lcd_remote_update_rect(int x, int y, int width, int height);

/* Memory copy of display bitmap */
extern unsigned char lcd_remote_framebuffer[LCD_REMOTE_HEIGHT/8][LCD_REMOTE_WIDTH];

extern void lcd_remote_set_invert_display(bool yesno);
extern void lcd_remote_set_flip(bool yesno);
extern void lcd_remote_roll(int pixels);

extern void lcd_remote_set_drawmode(int mode);
extern int  lcd_remote_get_drawmode(void);
extern void lcd_remote_setmargins(int xmargin, int ymargin);
extern int  lcd_remote_getxmargin(void);
extern int  lcd_remote_getymargin(void);
extern void lcd_remote_setfont(int font);
extern int  lcd_remote_getstringsize(const unsigned char *str, int *w, int *h);

extern void lcd_remote_drawpixel(int x, int y);
extern void lcd_remote_drawline(int x1, int y1, int x2, int y2);
extern void lcd_remote_hline(int x1, int x2, int y);
extern void lcd_remote_vline(int x, int y1, int y2);
extern void lcd_remote_drawrect(int x, int y, int width, int height);
extern void lcd_remote_fillrect(int x, int y, int width, int height);
extern void lcd_remote_bitmap_part(const unsigned char *src, int src_x,
                                   int src_y, int stride, int x, int y,
                                   int width, int height);
extern void lcd_remote_bitmap(const unsigned char *src, int x, int y,
                              int width, int height);
extern void lcd_remote_putsxy(int x, int y, const unsigned char *string);

extern void lcd_remote_invertscroll(int x, int y);
extern void lcd_remote_bidir_scroll(int threshold);
extern void lcd_remote_scroll_step(int pixels);

#define lcd_remote_mono_bitmap lcd_remote_bitmap
#define lcd_remote_mono_bitmap_part lcd_remote_bitmap_part

#endif
#endif
