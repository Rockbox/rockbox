/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 1999 Mattis Wadman (nappe@sudac.org)
 *
 * Heavily modified for embedded use by Björn Stenberg (bjorn@haxx.se)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"

#ifdef HAVE_LCD_BITMAP
#include <stdbool.h>
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include <string.h>

#ifdef SIMULATOR
#include <stdio.h>
#endif

#define TETRIS_TITLE       "Tetris!"
#define TETRIS_TITLE_FONT  1
#define TETRIS_TITLE_XLOC  43
#define TETRIS_TITLE_YLOC  15

int start_x = 2;
int start_y = 1;
int max_x = 28;
int max_y = 48;
int current_x = 0;
int current_y = 0;
int current_f = 0;
int current_b = 0;
int level = 0;
short lines = 0;
int score = 0;
int next_b = 0;
int next_f = 0;
char virtual[LCD_WIDTH*LCD_HEIGHT];
short level_speeds[10] = {1000,900,800,700,600,500,400,300,250,200};
int blocks = 7;
int block_frames[7] = {1,2,2,2,4,4,4};

/*
 block_data is built up the following way

 first array index specifies the block number
 second array index specifies the rotation of the block
 third array index specifies:
     0: x-coordinates of pixels
     1: y-coordinates of pixels
 fourth array index specifies the coordinate of a pixel

 each block consists of four pixels whose relative coordinates are given
 with block_data
*/

int block_data[7][4][2][4] =
{
    {
        {{0,1,0,1},{0,0,1,1}}
    },
    {
        {{0,1,1,2},{1,1,0,0}},
        {{0,0,1,1},{0,1,1,2}}
    },
    {
        {{0,1,1,2},{0,0,1,1}},
        {{1,1,0,0},{0,1,1,2}}
    },
    {
        {{1,1,1,1},{0,1,2,3}},
        {{0,1,2,3},{2,2,2,2}}
    },
    {
        {{1,1,1,2},{2,1,0,0}},
        {{0,1,2,2},{1,1,1,2}},
        {{0,1,1,1},{2,2,1,0}},
        {{0,0,1,2},{0,1,1,1}}
    },
    {
        {{0,1,1,1},{0,0,1,2}},
        {{0,1,2,2},{1,1,1,0}},
        {{1,1,1,2},{0,1,2,2}},
        {{0,0,1,2},{2,1,1,1}}
    },
    {
        {{1,0,1,2},{0,1,1,1}},
        {{2,1,1,1},{1,0,1,2}},
        {{1,0,1,2},{2,1,1,1}},
        {{0,1,1,1},{1,0,1,2}}
    }
};

/* not even pseudo random :) */
int t_rand(int range)
{
    static int count;
    count++;
    return count % range;
}

void draw_frame(int fstart_x,int fstop_x,int fstart_y,int fstop_y)
{
    lcd_drawline(fstart_x, fstart_y, fstop_x, fstart_y);
    lcd_drawline(fstart_x, fstop_y, fstop_x, fstop_y);

    lcd_drawline(fstart_x, fstart_y, fstart_x, fstop_y);
    lcd_drawline(fstop_x, fstart_y, fstop_x, fstop_y);
}

void draw_block(int x,int y,int block,int frame,bool clear)
{
    int i;
    for(i=0;i < 4;i++) {
        if (clear)
        {
            lcd_clearpixel(start_x+x+block_data[block][frame][0][i] * 2,
                           start_y+y+block_data[block][frame][1][i] * 2);
            lcd_clearpixel(start_x+x+block_data[block][frame][0][i] * 2 + 1,
                           start_y+y+block_data[block][frame][1][i] * 2);
            lcd_clearpixel(start_x+x+block_data[block][frame][0][i] * 2,
                           start_y+y+block_data[block][frame][1][i] * 2 + 1);
            lcd_clearpixel(start_x+x+block_data[block][frame][0][i] * 2 + 1,
                           start_y+y+block_data[block][frame][1][i] * 2 + 1);
        }
        else
        {
            lcd_drawpixel(start_x+x+block_data[block][frame][0][i] * 2,
                          start_y+y+block_data[block][frame][1][i] * 2);
            lcd_drawpixel(start_x+x+block_data[block][frame][0][i] * 2 + 1,
                          start_y+y+block_data[block][frame][1][i] * 2);
            lcd_drawpixel(start_x+x+block_data[block][frame][0][i] * 2,
                          start_y+y+block_data[block][frame][1][i] * 2 + 1);
            lcd_drawpixel(start_x+x+block_data[block][frame][0][i] * 2 + 1,
                          start_y+y+block_data[block][frame][1][i] * 2 + 1);
        }
    }
}

void to_virtual(void)
{
    int i;
    for(i = 0; i < 4; i++)
    {
        *(virtual + 
          (current_y + block_data[current_b][current_f][1][i] * 2) * max_x +
          current_x + block_data[current_b][current_f][0][i] * 2) = current_b + 1;
        *(virtual +
          (current_y + block_data[current_b][current_f][1][i] * 2 + 1) * max_x +
          current_x + block_data[current_b][current_f][0][i] * 2) = current_b + 1;
        *(virtual +
          (current_y + block_data[current_b][current_f][1][i] * 2) * max_x +
          current_x + block_data[current_b][current_f][0][i] * 2 + 1) = current_b + 1;
        *(virtual +
          (current_y + block_data[current_b][current_f][1][i] * 2 + 1) * max_x +
          current_x + block_data[current_b][current_f][0][i] * 2 + 1) = current_b + 1;
    }
}

bool block_touch (int x, int y)
{
    if (*(virtual + y * max_x + x) != 0 ||
        *(virtual + y * max_x + x + 1) != 0  ||
        *(virtual + (y + 1) * max_x + x) != 0  ||
        *(virtual + (y + 1) * max_x + x + 1) != 0)
        return true;
    return false;
}

bool gameover(void)
{
    int i;
    int frame, block, y, x;

    x = current_x;
    y = current_y;
    block = current_b;
    frame = current_f;

    for(i=0;i < 4; i++){
        /* Do we have blocks touching? */
        if(block_touch(x + block_data[block][frame][0][i] * 2, y + block_data[block][frame][1][i] * 2)) 
        {
            /* Are we at the top of the frame? */
            if(y + block_data[block][frame][1][i] * 2 < start_y)
            {
                /* Game over ;) */
                return true;
            }
        }
    }
    return false;
}

bool valid_position(int x,int y,int block,int frame)
{
    int i;
    for(i=0;i < 4;i++)
        if ((y + block_data[block][frame][1][i] * 2 > max_y - 2) ||
            (x + block_data[block][frame][0][i] * 2 > max_x - 2) ||
            (y + block_data[block][frame][1][i] * 2 < 0) ||
            (x + block_data[block][frame][0][i] * 2 < 0) ||
            block_touch (x + block_data[block][frame][0][i] * 2, y + block_data[block][frame][1][i] * 2))
            return false;
    return true;
}

void from_virtual(void)
{
    int x,y;
    for(y = 0; y < max_y; y++)
        for(x = 1; x < max_x - 1; x ++)
            if(*(virtual + (y * max_x) + x) != 0)
                lcd_drawpixel(start_x + x, start_y + y);
            else
                lcd_clearpixel(start_x + x, start_y + y);
}

void move_block(int x,int y,int f)
{
    int last_frame = current_f;
    if(f != 0)
    {
        current_f += f;
        if(current_f > block_frames[current_b]-1)
            current_f = 0;
        if(current_f < 0)
            current_f = block_frames[current_b]-1;
    }

    if(valid_position(current_x+x,current_y+y,current_b,current_f))
    {
        draw_block(current_x,current_y,current_b,last_frame,true);
        current_x += x;
        current_y += y;
        draw_block(current_x,current_y,current_b,current_f,false);
        lcd_update();
    }
    else
        current_f = last_frame;
}

void new_block(void)
{
    current_b = next_b;
    current_f = next_f;
    current_x = (int)((max_x)/2)-1;
    current_y = 0;
    next_b = t_rand(blocks);
    next_f = t_rand(block_frames[next_b]);
    draw_block(max_x+2,start_y-1,current_b,current_f,true);
    draw_block(max_x+2,start_y-1,next_b,next_f,false);
    if(!valid_position(current_x,current_y,current_b,current_f))
    {
        draw_block(current_x,current_y,current_b,current_f,false);
        lcd_update();
    }
    else
        draw_block(current_x,current_y,current_b,current_f,false);
}

int check_lines(void)
{
    int x,y,i;
    bool line;
    int lines = 0;
    for(y = 0; y < max_y; y++)
    {
        line = true;
        for(x = 1; x < max_x - 1; x++)
            if(*(virtual + y * max_x + x) == 0)
            {
                line = false;
                break;
            }
        if(line)
        {
            lines++;
            for(i = y; i > 1; i--)
		        memcpy(virtual + i * max_x + 1, virtual + (i-1) * max_x + 1, max_x - 2);
		    memset (&virtual[max_x] + 1, 0, max_x - 2);
        }
    }
    return lines / 2;
}

void move_down(void)
{
    int l;
    char s[25];
    if(!valid_position(current_x,current_y+1,current_b,current_f))
    {
        to_virtual();
        l = check_lines();
        if(l)
        {
            lines += l;
            level = (int)lines/10;
            if(level > 9)
                level = 9;
            from_virtual();
            score += l*l;
        }
        sprintf (s, "%d Rows - Level %d", lines, level);
        lcd_putsxy (2, 52, s, 0);
        new_block();
        move_block(0,0,0);
    }
    else
        move_block(0,2,0);
} 

void game_loop(void)
{
    while(1)
    {
	int b=0;
	int count = 0;
	/*  while(count*20 < level_speeds[level]) */
	{
	    b = button_get(false);
	    if ( b & BUTTON_OFF )
              return; /* get out of here */

	    if ( b & BUTTON_LEFT ) {
		move_block(-2,0,0);
	    }
	    if ( b & BUTTON_RIGHT ) {
		move_block(2,0,0);
	    }
	    if ( b & BUTTON_UP ) {
		move_block(0,0,1);
	    }
	    if ( b & BUTTON_DOWN ) {
		move_down();
	    }
	    count++;
	    sleep(10);
	}
    if(gameover()) {
        int w, h;

        lcd_getfontsize(TETRIS_TITLE_FONT, &w, &h);
        lcd_clearrect(TETRIS_TITLE_XLOC, TETRIS_TITLE_YLOC, 
                      TETRIS_TITLE_XLOC+(w*sizeof(TETRIS_TITLE)), 
                      TETRIS_TITLE_YLOC-h);
        lcd_putsxy(TETRIS_TITLE_XLOC, TETRIS_TITLE_YLOC, "You lose!",
                   TETRIS_TITLE_FONT);
        lcd_update();
        sleep(2);
        return;
    }
	move_down();
    }
}

void init_tetris(void)
{
    memset(&virtual, 0, sizeof(virtual));
    start_x = 2;
    start_y = 1;
    max_x = 28;
    max_y = 48;
    current_x = 0;
    current_y = 0;
    current_f = 0;
    current_b = 0;
    level = 0;
    lines = 0;
    score = 0;
    next_b = 0;
    next_f = 0;
}

void tetris(void)
{
    init_tetris();

    draw_frame(start_x,start_x + max_x - 1, start_y - 1, start_y + max_y);
    lcd_putsxy(TETRIS_TITLE_XLOC, TETRIS_TITLE_YLOC, TETRIS_TITLE,
               TETRIS_TITLE_FONT);
    lcd_putsxy(TETRIS_TITLE_XLOC, TETRIS_TITLE_YLOC + 16, "*******",
               TETRIS_TITLE_FONT);
    lcd_putsxy (2, 52, "0 Rows - Level 0", 0);
    lcd_update();

    next_b = t_rand(blocks);
    next_f = t_rand(block_frames[next_b]);
    new_block();
    game_loop();
}

#endif
