/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Robert E. Hak <rhak@ramapo.edu>
 *
 * Windows Copyright (C) 2002 by Felix Arends
 * X11 Copyright (C) 2002 by Daniel Stenberg <daniel@haxx.se>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "lcd-common.h"

#include "lcd.h"

#ifdef WIN32 
#include "lcd-win32.h"
#else
#include "lcd-x11.h"
#endif

void lcd_blit(const fb_data* p_data, int x, int y, int width, int height,
              int stride)
{
    (void)p_data;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)stride;
}
    
void lcd_set_flip(bool yesno)
{
    (void)yesno;
}

void lcd_set_invert_display(bool invert)
{
    (void)invert;
}

void lcd_remote_set_invert_display(bool invert)
{
    (void)invert;
}

#ifdef HAVE_REMOTE_LCD
void lcd_remote_set_contrast(int val)
{
    (void)val;
}
void lcd_remote_backlight_on(int val)
{
    (void)val;
}
void lcd_remote_backlight_off(int val)
{
    (void)val;
}

void lcd_remote_set_flip(bool yesno)
{
    (void)yesno;
}
#endif
