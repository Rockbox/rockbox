/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "plugin.h"

#define SKY_COLOR LCD_RGBPACK(0, 0, 255)
#define GRASS_COLOR LCD_RGBPACK(0, 255, 0)
#define ROAD_COLOR1 LCD_RGBPACK(120, 120, 120)
#define ROAD_COLOR2 LCD_RGBPACK(80, 80, 80)

#define GROUND_LEVEL (LCD_HEIGHT*.7)

#define SCALE_STEP 2

unsigned road_colors[] = {ROAD_COLOR1, ROAD_COLOR2};

enum plugin_status plugin_start(const void *param)
{
    (void) param;
    int next_colidx=0;
    for(int i=0;;++i)
    {
        rb->lcd_set_background(SKY_COLOR);
        rb->lcd_set_foreground(GRASS_COLOR);
        int road_width=4;
        int road_colidx=next_colidx;
        int road_color=road_colors[road_colidx];
        int road_left=(LCD_WIDTH/2)-(road_width/2);
        int road_right=(LCD_WIDTH/2)+(road_width/2);
        for(int y=0;y<LCD_WIDTH;++y)
        {
            if(y<GROUND_LEVEL)
            {
                /* sky */
                rb->lcd_set_drawmode(DRMODE_BG | DRMODE_INVERSEVID);
                rb->lcd_hline(0, LCD_WIDTH-1, y);
                rb->lcd_set_drawmode(DRMODE_SOLID);
            }
            else
            {
                /* grass+road */
                rb->lcd_hline(0, road_left, y);
                rb->lcd_set_foreground(road_color);
                rb->lcd_hline(road_left, road_right, y);
                rb->lcd_set_foreground(GRASS_COLOR);
                rb->lcd_hline(road_right, LCD_WIDTH-1, y);
                road_left-=(SCALE_STEP/2);
                road_right+=(SCALE_STEP/2);
                road_width+=SCALE_STEP;
            }
        }
        rb->lcd_update();
    }
}
