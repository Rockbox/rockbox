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
#include "options.h"

#ifdef USE_GAMES

#include <stdbool.h>
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include <string.h>
#include "menu.h"
#include "screens.h"

#ifdef SIMULATOR
#include <stdio.h>
#endif
#include "sprintf.h"
#include "lang.h"
#define TETRIS_TITLE       "Tetris!"
#define TETRIS_TITLE_FONT  1
#define TETRIS_TITLE_XLOC  43
#define TETRIS_TITLE_YLOC  15

static const int start_x = 5;
static const int start_y = 5;
static const int max_x = 4 * 17;
static const int max_y = 3 * 10;
static const short level_speeds[10] = {1000,900,800,700,600,500,400,300,250,200};
static const int blocks = 7;
static const int block_frames[7] = {1,2,2,2,4,4,4};

static int current_x, current_y, current_f, current_b;
static int level, score;
static int next_b, next_f;
static short lines;
static char virtual[LCD_WIDTH * LCD_HEIGHT];

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

static const char block_data[7][4][2][4] =
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

static int t_rand(int range)
{
    return current_tick % range;
}

static void draw_frame(int fstart_x,int fstop_x,int fstart_y,int fstop_y)
{
    lcd_drawline(fstart_x, fstart_y, fstop_x, fstart_y);
    lcd_drawline(fstart_x, fstop_y, fstop_x, fstop_y);

    lcd_drawline(fstart_x, fstart_y, fstart_x, fstop_y);
    lcd_drawline(fstop_x, fstart_y, fstop_x, fstop_y);

    lcd_drawline(fstart_x - 1, fstart_y + 1, fstart_x - 1, fstop_y + 1);
    lcd_drawline(fstart_x - 1, fstop_y + 1, fstop_x - 1, fstop_y + 1);
}

static void draw_block(int x, int y, int block, int frame, bool clear)
{
    int i, a, b;
    for(i=0;i < 4;i++) {
        if (clear)
        {
            for (a = 0; a < 3; a++)
                for (b = 0; b < 4; b++)
                    lcd_clearpixel(start_x + x + block_data[block][frame][1][i] * 4 - b,
                                   start_y + y + block_data[block][frame][0][i] * 3 + a);
        }
        else
        {
            for (a = 0; a < 3; a++)
                for (b = 0; b < 4; b++)
                    lcd_drawpixel(start_x+x+block_data[block][frame][1][i] * 4 - b,
                                  start_y+y+block_data[block][frame][0][i] * 3 + a);
        }
    }
}

static void to_virtual(void)
{
    int i,a,b;

    for(i = 0; i < 4; i++)
        for (a = 0; a < 3; a++)
            for (b = 0; b < 4; b++)
                *(virtual + 
                (current_y + block_data[current_b][current_f][0][i] * 3 + a) * max_x +
                current_x + block_data[current_b][current_f][1][i] * 4 - b) = current_b + 1;
}

static bool block_touch (int x, int y)
{
    int a,b;
    for (a = 0; a < 4; a++)
        for (b = 0; b < 3; b++)
            if (*(virtual + (y + b) * max_x + (x - a)) != 0)
                return true;
    return false;
}

static bool gameover(void)
{
    int i;
    int frame, block, y, x;

    x = current_x;
    y = current_y;
    block = current_b;
    frame = current_f;

    for(i = 0; i < 4; i++){
        /* Do we have blocks touching? */
        if(block_touch(x + block_data[block][frame][1][i] * 4, y + block_data[block][frame][0][i] * 3)) 
        {
            /* Are we at the top of the frame? */
            if(x + block_data[block][frame][1][i] * 4 >= max_x - 16)
            {
                /* Game over ;) */
                return true;
            }
        }
    }
    return false;
}

static bool valid_position(int x, int y, int block, int frame)
{
    int i;
    for(i=0;i < 4;i++)
        if ((y + block_data[block][frame][0][i] * 3 > max_y - 3) ||
            (x + block_data[block][frame][1][i] * 4 > max_x - 4) ||
            (y + block_data[block][frame][0][i] * 3 < 0) ||
            (x + block_data[block][frame][1][i] * 4 < 4) ||
            block_touch (x + block_data[block][frame][1][i] * 4, y + block_data[block][frame][0][i] * 3))
            return false;
    return true;
}

static void from_virtual(void)
{
    int x,y;
    for(y = 0; y < max_y; y++)
        for(x = 1; x < max_x - 1; x++)
            if(*(virtual + (y * max_x) + x) != 0)
                lcd_drawpixel(start_x + x, start_y + y);
            else
                lcd_clearpixel(start_x + x, start_y + y);
}

static void move_block(int x,int y,int f)
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

    if(valid_position(current_x + x, current_y + y, current_b, current_f))
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

static void new_block(void)
{
    current_b = next_b;
    current_f = next_f;
    current_x = max_x - 16;
    current_y = (int)12;
    next_b = t_rand(blocks);
    next_f = t_rand(block_frames[next_b]);

    lcd_drawline (max_x + 7, start_y - 1, max_x + 29, start_y - 1);
    lcd_drawline (max_x + 29, start_y, max_x + 29, start_y + 14);
    lcd_drawline (max_x + 29, start_y + 14, max_x + 7, start_y + 14);
    lcd_drawline (max_x + 7, start_y + 14, max_x + 7, start_y - 1);
    lcd_drawline (max_x + 6, start_y + 15, max_x + 6, start_y);
    lcd_drawline (max_x + 6, start_y + 15, max_x + 28, start_y + 15);

    draw_block(max_x + 9, start_y - 4, current_b, current_f, true);
    draw_block(max_x + 9, start_y - 4, next_b, next_f, false);
    if(!valid_position(current_x, current_y, current_b, current_f))
    {
        draw_block(current_x, current_y, current_b, current_f, false);
        lcd_update();
    }
    else
        draw_block(current_x, current_y, current_b, current_f, false);
}

static int check_lines(void)
{
    int x,y,i,j;
    bool line;
    int lines = 0;
    for(x = 0; x < max_x; x++)
    {
        line = true;
        for(y = 0; y < max_y; y++)
        {
            if(*(virtual + y * max_x + x) == 0)
            {
                line = false;
                break;
            }
        }

        if(line)
        {
            lines++;
            /* move rows down */
            for(i = x; i < max_x - 1; i++)
                for (j = 0; j < max_y; j++)
		            *(virtual + j * max_x + i) = *(virtual + j * max_x + (i + 1));

            x--; /* re-check this line */
        }
    }

    return lines / 4;
}

static void move_down(void)
{
    int l;
    char s[25];

    if(!valid_position(current_x - 4, current_y, current_b, current_f))
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

        snprintf (s, sizeof(s), "%d %s %d", lines, 
                  str(LANG_TETRIS_LEVEL), level);
        lcd_putsxy (2, 42, s);

        new_block();
        move_block(0,0,0);
    }
    else
        move_block(-4,0,0);
} 

static bool game_loop(void)
{
    while(1)
    {
        int count = 0;
        while(count * 300 < level_speeds[level])
        {
            switch(button_get_w_tmo(HZ/10))
            {
                case BUTTON_OFF:
                    return false;
                
                case BUTTON_UP:
                case BUTTON_UP | BUTTON_REPEAT:
                    move_block(0,-3,0);
                    break;
                
                case BUTTON_DOWN:
                case BUTTON_DOWN | BUTTON_REPEAT:
                    move_block(0,3,0);
                    break;
                
                case BUTTON_RIGHT:
                case BUTTON_RIGHT | BUTTON_REPEAT:
                    move_block(0,0,1);
                    break;
                
                case BUTTON_LEFT:
                case BUTTON_LEFT | BUTTON_REPEAT:
                    move_down();
                    break;

                case SYS_USB_CONNECTED:
                    usb_screen();
                    return true;
            }
            
            count++;
        }
        
        if(gameover())
        {
            lcd_clearrect(0, 52, LCD_WIDTH, LCD_HEIGHT - 52);
            lcd_putsxy (2, 52, str(LANG_TETRIS_LOSE));
            lcd_update();
            sleep(HZ * 3);
            return false;
        }

        move_down();
    }

    return false;
}

static void init_tetris(void)
{
    memset(&virtual, 0, sizeof(virtual));

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

bool tetris(void)
{
    char buf[20];

    init_tetris();

    draw_frame(start_x, start_x + max_x - 1, start_y - 1, start_y + max_y);
    snprintf(buf, sizeof(buf), "0 %s 0", str(LANG_TETRIS_LEVEL));
    lcd_putsxy (2, 42, buf);
    lcd_update();

    next_b = t_rand(blocks);
    next_f = t_rand(block_frames[next_b]);
    new_block();
    return game_loop();
}

#endif
