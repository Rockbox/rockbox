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

#include "types.h"
#include "lcd.h"
#include "button.h"
#include "kernel.h"

#ifdef SIMULATOR
#include <stdio.h>
#endif

int start_x = 1;
int start_y = 1;
int max_x = 14;
int max_y = 24;
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
int rand(int range)
{
    static int count;
    count++;
    return count % range;
}

void draw_frame(int fstart_x,int fstop_x,int fstart_y,int fstop_y)
{
    int i;
    for (i=0; fstart_x+i-1 < fstop_x; i++)
    {
        lcd_drawpixel(fstart_x+i,fstart_y);
        lcd_drawpixel(fstart_x+i,fstop_y);
    }
    for (i=1; fstart_y+i < fstop_y; i++)
    {
        lcd_drawpixel(fstart_x,fstart_y+i);
        lcd_drawpixel(fstop_x,fstart_y+i);
    }
    lcd_drawpixel(fstart_x,fstart_y);
    lcd_drawpixel(fstop_x,fstart_y);
    lcd_drawpixel(fstart_x,fstop_y);
    lcd_drawpixel(fstop_x,fstop_y);
}

void draw_block(int x,int y,int block,int frame,int clear)
{
    int i;
    for(i=0;i < 4;i++)
        if ( (clear ? 0 : block+1) ) 
            lcd_drawpixel(start_x+x+block_data[block][frame][0][i],
                          start_y+y+block_data[block][frame][1][i]);
        else
            lcd_clearpixel(start_x+x+block_data[block][frame][0][i],
                           start_y+y+block_data[block][frame][1][i]);
}

void to_virtual()
{
    int i;
    for(i=0;i < 4;i++)
        *(virtual+
          ((current_y+block_data[current_b][current_f][1][i])*max_x)+
          (current_x+block_data[current_b][current_f][0][i])) = current_b+1;
}

int valid_position(int x,int y,int block,int frame)
{
    int i;
    for(i=0;i < 4;i++)
        if( (*(virtual+((y+block_data[block][frame][1][i])*max_x)+x+
               block_data[block][frame][0][i]) != 0) || 
            (x+block_data[block][frame][0][i] < 0) || 
            (x+block_data[block][frame][0][i] > max_x-1) ||
            (y+block_data[block][frame][1][i] < 0) || 
            (y+block_data[block][frame][1][i] > max_y-1))
            return FALSE;
    return TRUE;
}

void from_virtual()
{
    int x,y;
    for(y=0;y < max_y;y++)
        for(x=0;x < max_x;x++)
            if(*(virtual+(y*max_x)+x))
                lcd_drawpixel(start_x+x,start_y+y);
            else
                lcd_clearpixel(start_x+x,start_y+y);
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
        draw_block(current_x,current_y,current_b,last_frame,TRUE);
        current_x += x;
        current_y += y;
        draw_block(current_x,current_y,current_b,current_f,FALSE);
        lcd_update();
    }
    else
        current_f = last_frame;
}

void new_block()
{
    current_b = next_b;
    current_f = next_f;
    current_x = (int)((max_x)/2)-1;
    current_y = 0;
    next_b = rand(blocks);
    next_f = rand(block_frames[next_b]);
    draw_block(max_x+2,start_y-1,current_b,current_f,TRUE);
    draw_block(max_x+2,start_y-1,next_b,next_f,FALSE);
    if(!valid_position(current_x,current_y,current_b,current_f))
    {
        draw_block(current_x,current_y,current_b,current_f,FALSE);
        lcd_update();
    }
    else
        draw_block(current_x,current_y,current_b,current_f,FALSE);
}

int check_lines()
{
    int x,y,line,i;
    int lines = 0;
    for(y=0;y < max_y;y++)
    {
        line = TRUE;
        for(x=0;x < max_x;x++)
            if(virtual[y*max_x+x] == 0)
                line = FALSE;
        if(line)
        {
            lines++;
            for(i=y;i > 1;i--)
		for (x=0;x<max_x;x++)
		    virtual[i*max_x] = virtual[((i-1)*max_x)];
	    for (x=0;x<max_x;x++)
		virtual[max_x] = 0;
        }
    }
    return lines;
}

void move_down()
{
    int l;
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
        new_block();
        move_block(0,0,0);
    }
    else
        move_block(0,1,0);
} 

void game_loop(void)
{
    while(1)
    {
	int b=0;
	int count = 0;
	/*  while(count*20 < level_speeds[level]) */
	{
	    b = button_get();
	    if ( b & BUTTON_OFF )
              return; /* get out of here */

	    if ( b & BUTTON_LEFT ) {
		move_block(-1,0,0);
	    }
	    if ( b & BUTTON_RIGHT ) {
		move_block(1,0,0);
	    }
	    if ( b & BUTTON_UP ) {
		move_block(0,0,1);
	    }
	    if ( b & BUTTON_DOWN ) {
		move_down();
	    }
	    count++;
	    sleep(1);
	}
	move_down();
    }
}

void tetris(void)
{
    draw_frame(start_x-1,start_x+max_x,start_y-1,start_y+max_y);
    lcd_puts(10, 32, "Tetris!", 2);
    lcd_update();

    next_b = rand(blocks);
    next_f = rand(block_frames[next_b]);
    new_block();
    game_loop();
}
