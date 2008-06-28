/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Kjell Ericson
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

#define ICON_HEIGHT 12
#define CHAR_HEIGHT 8
#define CHAR_WIDTH 6
#define CHAR_PIXEL 2
#define BORDER_MARGIN 1

extern bool sim_lcd_framebuffer[SIM_LCD_HEIGHT][SIM_LCD_WIDTH];

void lcd_print_icon(int x, int icon_line, bool enable, char **icon);
void lcd_print_char(int x, int y, unsigned char ch);
void sim_lcd_update_rect(int x, int y, int width, int height);
void sim_lcd_define_pattern(int pat, const char *pattern);
