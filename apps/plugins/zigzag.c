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

#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"

/* keep this a power of two so the modulo is fast */
#define DIRECTION_SWITCH_INTERVAL 16
#define SECTION_WIDTH 32

#define BACKGROUND_COLOR LCD_RGBPACK(240, 240, 240)

#define TOP_COLOR LCD_RGBPACK(80, 80, 200)
#define RIGHTWALL_COLOR LCD_RGBPACK(20, 20, 180)
#define LEFTWALL_COLOR LCD_RGBPACK(100, 100, 160)

struct line_t {
    int left, right;
};

struct line_t world[LCD_HEIGHT];

/* direction the track is moving in */
int direction;
int distance_into;

/* for convenience purposes */
/* the left and right coordinates of the topmost line */
int left;
int right;

void gen_world(void)
{
    left = LCD_WIDTH/2 - SECTION_WIDTH/2;
    right = LCD_WIDTH/2 + SECTION_WIDTH/2;
    direction = 1;
    distance_into = 0;
    for(int i = (int)ARRAYLEN(world) - 1; i >= 0; --i)
    {
        if(distance_into == 0)
        {
            direction = (rb->rand() % 2) ? -1 : 1;
        }
        left += direction;
        right += direction;
        world[i].left = left;
        world[i].right = right;
        distance_into = (distance_into + 1) % DIRECTION_SWITCH_INTERVAL;
    }
}

/* scrolls down 1px */
void scroll(void)
{
    rb->memmove(world + 1, world, sizeof(struct line_t) * (ARRAYLEN(world) - 1));
    if(distance_into == 0)
    {
        direction = (rb->rand() % 2) ? -1 : 1;
    }

    if(left < 0)
        direction = -direction;
    if(right > LCD_WIDTH)
        direction = -direction;

    left += direction;
    right += direction;
    world[0].left = left;
    world[0].right = right;
    distance_into = (distance_into + 1) % DIRECTION_SWITCH_INTERVAL;
}

void render(void)
{
    rb->lcd_set_background(BACKGROUND_COLOR);
    rb->lcd_clear_display();
    int wallleft = world[0].left;
    int wallright = world[0].right;
    for(unsigned int i = 0; i < ARRAYLEN(world); ++i)
    {
        rb->lcd_set_foreground(TOP_COLOR);
        rb->lcd_hline(world[i].left, world[i].right, i);
        rb->lcd_set_foreground(LEFTWALL_COLOR);
        rb->lcd_hline(wallleft, world[i].left, i);
        rb->lcd_set_foreground(RIGHTWALL_COLOR);
        rb->lcd_hline(world[i].right, wallright, i);
        if(world[i].left < wallleft)
            wallleft = world[i].left;
        if(world[i].right > wallright)
            wallright = world[i].right;
    }
    rb->lcd_update();
}

enum plugin_status plugin_start(const void *param)
{
    (void) param;
    rb->srand(*rb->current_tick);
    gen_world();
    render();
    while(1)
    {
        const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
        int button;
        switch(button = pluginlib_getaction(0, plugin_contexts, ARRAYLEN(plugin_contexts)))
        {
        default:
            exit_on_usb(button);
        }
        scroll();
        render();
        rb->yield();
    }
}
