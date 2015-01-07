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

#include "lib/xlcd.h"

#define SKY_COLOR LCD_RGBPACK(0, 0, 255)
#define GRASS_COLOR LCD_RGBPACK(0, 255, 0)
#define ROAD_COLOR1 LCD_RGBPACK(120, 120, 120)
#define ROAD_COLOR2 LCD_RGBPACK(80, 80, 80)

#define BORDER_COLOR1 LCD_RGBPACK(240, 240, 240)
#define BORDER_COLOR2 LCD_RGBPACK(240, 0, 0)

#define GROUND_LEVEL (LCD_HEIGHT/2)

#define SCALE_FACTOR 16

#define ROAD_WIDTH 48

/* z_map[0] represents the z of the horizon */
/* z_map[n-1] represents the z at the bottom of the screen */
int z_map[LCD_HEIGHT-GROUND_LEVEL];
int road_width[LCD_HEIGHT-GROUND_LEVEL];

void render(void)
{
    rb->lcd_set_background(SKY_COLOR);
    rb->lcd_clear_display();
    for(int y=0;y<LCD_HEIGHT;++y)
    {
        if(y<GROUND_LEVEL)
        {
            /* sky */
        }
        else
        {
            rb->lcd_set_foreground(GRASS_COLOR);
            rb->lcd_hline(0, (LCD_WIDTH/2)-(road_width[y-GROUND_LEVEL]/2), y);
            rb->lcd_hline((LCD_WIDTH/2)+(road_width[y-GROUND_LEVEL]/2), LCD_WIDTH-1, y);

            rb->lcd_set_foreground(ROAD_COLOR1);
            rb->lcd_hline((LCD_WIDTH/2)-(road_width[y-GROUND_LEVEL]/2), (LCD_WIDTH/2)+(road_width[y-GROUND_LEVEL]/2), y);

        }
    }
}

enum plugin_status plugin_start(const void *param)
{
    (void) param;
    int next_colidx=0;
    for(int i=0;i<ARRAYLEN(z_map);++i)
    {
        z_map[LCD_HEIGHT-GROUND_LEVEL-i] = (i + GROUND_LEVEL - (LCD_HEIGHT / 2));
        if(z_map[LCD_HEIGHT-GROUND_LEVEL-i]/4)
            road_width[LCD_HEIGHT-GROUND_LEVEL-i] = ROAD_WIDTH / (z_map[LCD_HEIGHT-GROUND_LEVEL-i]/4);
        else
            road_width[LCD_HEIGHT-GROUND_LEVEL-i] = LCD_WIDTH;
    }
    for(;;)
    {
        render();
        rb->lcd_update();
    }
}
