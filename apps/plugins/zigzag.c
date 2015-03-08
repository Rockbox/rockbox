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

#define ABS(x) (x<0?-x:x)
#define SIGN(x) (x<0?-1:1)

/* keep this a power of two so the modulo is fast */
#define DIRECTION_SWITCH_INTERVAL 16
#define SECTION_WIDTH 32

#define BACKGROUND_COLOR LCD_RGBPACK(240, 240, 240)

#define BALL_COLOR LCD_RGBPACK(10, 10, 10)

#define TOP_COLOR LCD_RGBPACK(80, 80, 200)
#define RIGHTWALL_COLOR LCD_RGBPACK(20, 20, 180)
#define LEFTWALL_COLOR LCD_RGBPACK(100, 100, 160)

#define FRACBITS 8

#define PLAYER_Y (.8 * LCD_HEIGHT)

struct line_t {
    int left, right;
};

struct line_t world[LCD_HEIGHT];

/* direction the track is moving in, fixed-point with FRACBITS fractional bits */
long direction;
int distance_into;

/* at 100fps, this'll take a couple billion years to overflow */
unsigned long long current_frame;

/* for convenience purposes */
/* the left and right coordinates of the topmost line */
/* fixed-point */
long left;
long right;

int ball_x, ball_y;

/* MT19937 is a bit overkill */

#define RAND_A1 12
#define RAND_A2 25
#define RAND_A3 27

#define RAND_C1 4101842887655102017LL
#define RAND_C2 2685821657736338717LL

static uint64_t rand_state = RAND_C1;

uint64_t myrand64(void)
{
    /* Xorshift* generator */
    rand_state ^= rand_state >> RAND_A1;
    rand_state ^= rand_state << RAND_A2;
    rand_state ^= rand_state >> RAND_A3;
    return rand_state * RAND_C2;
}

unsigned int myrand(void)
{
    static uint64_t rand_temp;
    static int bytes_left = 0;

    if(bytes_left < 4)
    {
        rand_temp = myrand64();
        bytes_left = 8;
    }
    unsigned int ret = rand_temp & 0xFFFFFFFF;
    rand_temp >>= 32;
    bytes_left -= 4;
    return ret;
}

void mysrand64(uint64_t seed)
{
    if(seed == RAND_C1)
        seed = RAND_C1 + 1;
    rand_state = RAND_C1 ^ seed;
}

void mysrand(unsigned int seed)
{
    mysrand64(seed);
}

void gen_world(void)
{
    current_frame = 0;
    left = (LCD_WIDTH/2 - SECTION_WIDTH/2) << FRACBITS;
    right = (LCD_WIDTH/2 + SECTION_WIDTH/2) << FRACBITS;
    direction = 2 << FRACBITS;
    distance_into = 0;
    for(int i = (int)ARRAYLEN(world) - 1; i >= 0; --i)
    {
        if(distance_into == 0)
        {
            direction = (myrand() % 2) ? direction : -direction;
        }

        left += direction;
        right += direction;
        world[i].left = left >> FRACBITS;
        world[i].right = right >> FRACBITS;
        distance_into = (distance_into + 1) % DIRECTION_SWITCH_INTERVAL;

        if(left < 0)
        {
            direction = -direction;
            left += direction;
            right += direction;
        }
        if(right > LCD_WIDTH << FRACBITS)
        {
            direction = -direction;

            left += direction;
            right += direction;
        }
    }
}

/* scrolls down 1px */
void scroll(void)
{
    rb->memmove(world + 1, world, sizeof(struct line_t) * (ARRAYLEN(world) - 1));
    if(distance_into == 0)
    {
        direction = (myrand() % 2) ? direction : -direction;
    }

    left += direction;
    right += direction;
    world[0].left = left >> FRACBITS;
    world[0].right = right >> FRACBITS;
    distance_into = (distance_into + 1) % DIRECTION_SWITCH_INTERVAL;
    ++current_frame;

    if(left < 0)
    {
        direction = -direction;
        left += direction;
        right += direction;
    }
    if(right > LCD_WIDTH << FRACBITS)
    {
        direction = -direction;
        left += direction;
        right += direction;
    }
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
    rb->lcd_set_foreground(BALL_COLOR);
    rb->lcd_drawpixel(ball_x, ball_y);
    rb->lcd_update();
}

enum plugin_status plugin_start(const void *param)
{
    (void) param;
    mysrand(*rb->current_tick);
    gen_world();
    render();
    ball_x = LCD_WIDTH/2;
    ball_y = PLAYER_Y;
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
