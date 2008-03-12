/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: lcd-remote-target.h 11967 2007-01-09 23:29:07Z linus $
 *
 * Copyright (C) 2007 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef LCD_REMOTE_TARGET_H
#define LCD_REMOTE_TARGET_H

#define REMOTE_INIT_LCD   1
#define REMOTE_DEINIT_LCD 2

#ifdef HAVE_REMOTE_LCD_TICKING
void lcd_remote_emireduce(bool state);
#endif
void lcd_remote_powersave(bool on);
void lcd_remote_set_invert_display(bool yesno);
void lcd_remote_set_flip(bool yesno);

bool remote_detect(void);
void lcd_remote_init_device(void);
void lcd_remote_on(void);
void lcd_remote_off(void);
void lcd_remote_update(void);
void lcd_remote_update_rect(int, int, int, int);

extern bool remote_initialized;

#endif
