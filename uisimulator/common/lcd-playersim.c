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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "hwcompat.h"

#include "lcd.h"
#include "lcd-charcell.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "system.h"

#include "font-player.h"
#include "lcd-playersim.h"

/*** definitions ***/

bool sim_lcd_framebuffer[SIM_LCD_HEIGHT][SIM_LCD_WIDTH];

static int double_height = 1;

void lcd_print_icon(int x, int icon_line, bool enable, char **icon)
{
    int row, col;
    int y = (ICON_HEIGHT+(CHAR_HEIGHT*2+2)*CHAR_PIXEL) * icon_line;

    y += BORDER_MARGIN;
    x += BORDER_MARGIN;

    for (row = 0; icon[row]; row++)
    {
        for (col = 0; icon[row][col]; col++)
        {
            switch (icon[row][col])
            {
              case '*':
                sim_lcd_framebuffer[y+row][x+col] = enable;
                break;

              case ' ':
                sim_lcd_framebuffer[y+row][x+col] = false;
                break;
            }
        }
    }
    sim_lcd_update_rect(x, y, col, row);
    /* icon drawing updates immediately */
}

void lcd_print_char(int x, int y, unsigned char ch)
{
    int xpos = x * CHAR_WIDTH*CHAR_PIXEL;
    int ypos = y * CHAR_HEIGHT*CHAR_PIXEL + ICON_HEIGHT;
    int row, col, r, c;

    if (double_height > 1 && y == 1)
        return;  /* only one row available if text is double height */
        
    for (row = 0; row < 7; row ++)
    {
        unsigned fontbitmap = (*font_player)[ch][row];
        int height = (row == 3) ? 1 : double_height;
        
        y = ypos + row * CHAR_PIXEL * double_height;
        for (col = 0; col < 5; col++)
        {
            bool fontbit = fontbitmap & (0x10 >> col);

            x = xpos + col * CHAR_PIXEL;
            for (r = 0; r < height * CHAR_PIXEL; r++)
                for (c = 0; c < CHAR_PIXEL; c++)
                    sim_lcd_framebuffer[y+r][x+c] = fontbit;
        }
    }
    if (double_height > 1)
    {
        y = ypos + 15*CHAR_PIXEL;
        for (r = 0; r < CHAR_PIXEL; r++)
            for (c = 0; c < 5*CHAR_PIXEL; c++)
                sim_lcd_framebuffer[y+r][xpos+c] = false;
    }
}

void lcd_double_height(bool on)
{
    int newval = (is_new_player() && on) ? 2 : 1;
    
    if (newval != double_height)
    {
        double_height = newval;
        lcd_update();
    }
}

void sim_lcd_define_pattern(int pat, const char *pattern)
{
    if (pat < lcd_pattern_count)
        memcpy((*font_player)[pat], pattern, 7);
}
