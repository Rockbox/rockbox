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
#define REMOTE_DRAW_PIXEL(x,y)		lcd_remote_framebuffer[(y)/8][(x)] |= (1<<((y)&7))
#define REMOTE_CLEAR_PIXEL(x,y)		lcd_remote_framebuffer[(y)/8][(x)] &= ~(1<<((y)&7))
#define REMOTE_INVERT_PIXEL(x,y)		lcd_remote_framebuffer[(y)/8][(x)] ^= (1<<((y)&7))

extern unsigned char lcd_remote_framebuffer[LCD_REMOTE_HEIGHT/8][LCD_REMOTE_WIDTH];

extern void lcd_remote_init(void);
extern void lcd_remote_clear_display(void);
extern void lcd_remote_backlight_on(void);
extern void lcd_remote_backlight_off(void);
extern void lcd_remote_set_contrast(int val);
extern void lcd_remote_set_invert_display(bool yesno);
extern int  lcd_remote_default_contrast(void);
extern void lcd_remote_bitmap (const unsigned char *src, int x, int y,
                               int nx, int ny, bool clear);
extern void lcd_remote_update(void);

#endif
#endif
