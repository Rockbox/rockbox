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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "lcd.h"
#include "lcd-sdl.h"

void lcd_set_flip(bool yesno)
{
    (void)yesno;
}

void lcd_set_invert_display(bool invert)
{
    (void)invert;
}

int lcd_default_contrast(void)
{
    return 28;
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

void lcd_remote_set_invert_display(bool invert)
{
    (void)invert;
}
#endif
