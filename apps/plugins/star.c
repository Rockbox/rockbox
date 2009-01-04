/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Frederic Dang Ngoc
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
#include "plugin.h"
#ifdef HAVE_LCD_BITMAP

PLUGIN_HEADER

/* size of a level in file */
#define STAR_LEVEL_SIZE    ((STAR_WIDTH + 1) * STAR_HEIGHT + 1)

/* size of the game board */
#define STAR_WIDTH      16
#define STAR_HEIGHT      9

/* number of level */
#define STAR_LEVEL_COUNT 20

/* values of object in the board */
#define STAR_VOID       '.'
#define STAR_WALL       '*'
#define STAR_STAR       'o'
#define STAR_BALL       'X'
#define STAR_BLOCK      'x'

/* sleep time between two frames */
#if (LCD_HEIGHT * LCD_WIDTH >= 70000) && defined(IPOD_ARCH)
/* iPod 5G LCD is *slow* */
#define STAR_SLEEP rb->yield();
#elif (LCD_HEIGHT * LCD_WIDTH >= 30000)
#define STAR_SLEEP rb->sleep(0);
#else
#define STAR_SLEEP rb->sleep(1);
#endif

/* value of ball and block control */
#define STAR_CONTROL_BALL  0
#define STAR_CONTROL_BLOCK 1

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define STAR_QUIT           BUTTON_OFF
#define STAR_LEFT           BUTTON_LEFT
#define STAR_RIGHT          BUTTON_RIGHT
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_ON
#define STAR_TOGGLE_CONTROL2 BUTTON_PLAY
#define STAR_LEVEL_UP       BUTTON_F3
#define STAR_LEVEL_DOWN     BUTTON_F1
#define STAR_LEVEL_REPEAT   BUTTON_F2
#define STAR_MENU_RUN       BUTTON_PLAY
#define STAR_MENU_RUN2      BUTTON_RIGHT
#define STAR_MENU_RUN3      BUTTON_ON

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define STAR_QUIT           BUTTON_OFF
#define STAR_LEFT           BUTTON_LEFT
#define STAR_RIGHT          BUTTON_RIGHT
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_ON
#define STAR_TOGGLE_CONTROL2 BUTTON_SELECT
#define STAR_LEVEL_UP       BUTTON_F3
#define STAR_LEVEL_DOWN     BUTTON_F1
#define STAR_LEVEL_REPEAT   BUTTON_F2
#define STAR_MENU_RUN       BUTTON_SELECT
#define STAR_MENU_RUN2      BUTTON_RIGHT
#define STAR_MENU_RUN3      BUTTON_ON

#elif CONFIG_KEYPAD == ONDIO_PAD
#define STAR_QUIT           BUTTON_OFF
#define STAR_LEFT           BUTTON_LEFT
#define STAR_RIGHT          BUTTON_RIGHT
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL_PRE BUTTON_MENU
#define STAR_TOGGLE_CONTROL (BUTTON_MENU | BUTTON_REL)
#define STAR_LEVEL_UP       (BUTTON_MENU | BUTTON_RIGHT)
#define STAR_LEVEL_DOWN     (BUTTON_MENU | BUTTON_LEFT)
#define STAR_LEVEL_REPEAT   (BUTTON_MENU | BUTTON_UP)
#define STAR_MENU_RUN       BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define STAR_QUIT           BUTTON_OFF
#define STAR_LEFT           BUTTON_LEFT
#define STAR_RIGHT          BUTTON_RIGHT
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_MODE
#define STAR_TOGGLE_CONTROL2 BUTTON_SELECT
#define STAR_LEVEL_UP       (BUTTON_ON | BUTTON_RIGHT)
#define STAR_LEVEL_DOWN     (BUTTON_ON | BUTTON_LEFT)
#define STAR_LEVEL_REPEAT   (BUTTON_ON | BUTTON_SELECT)
#define STAR_MENU_RUN       BUTTON_RIGHT
#define STAR_MENU_RUN2      BUTTON_SELECT

#define STAR_RC_QUIT BUTTON_RC_STOP
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)

#define STAR_QUIT           (BUTTON_SELECT | BUTTON_MENU)
#define STAR_LEFT           BUTTON_LEFT
#define STAR_RIGHT          BUTTON_RIGHT
#define STAR_UP             BUTTON_MENU
#define STAR_DOWN           BUTTON_PLAY
#define STAR_TOGGLE_CONTROL_PRE BUTTON_SELECT
#define STAR_TOGGLE_CONTROL (BUTTON_SELECT | BUTTON_REL)
#define STAR_LEVEL_UP       (BUTTON_SELECT | BUTTON_RIGHT)
#define STAR_LEVEL_DOWN     (BUTTON_SELECT | BUTTON_LEFT)
#define STAR_LEVEL_REPEAT   (BUTTON_SELECT | BUTTON_PLAY)
#define STAR_MENU_RUN       BUTTON_RIGHT
#define STAR_MENU_RUN2      BUTTON_SELECT

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)

#define STAR_QUIT           BUTTON_POWER
#define STAR_LEFT           BUTTON_LEFT
#define STAR_RIGHT          BUTTON_RIGHT
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_SELECT
#define STAR_LEVEL_UP_PRE   BUTTON_REC
#define STAR_LEVEL_UP       (BUTTON_REC|BUTTON_REL)
#define STAR_LEVEL_DOWN_PRE BUTTON_REC
#define STAR_LEVEL_DOWN     (BUTTON_REC|BUTTON_REPEAT)
#define STAR_LEVEL_REPEAT   BUTTON_PLAY
#define STAR_MENU_RUN       BUTTON_RIGHT
#define STAR_MENU_RUN2      BUTTON_SELECT

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)

#define STAR_QUIT           BUTTON_POWER
#define STAR_LEFT           BUTTON_LEFT
#define STAR_RIGHT          BUTTON_RIGHT
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_SELECT
#define STAR_LEVEL_UP       BUTTON_VOL_UP
#define STAR_LEVEL_DOWN     BUTTON_VOL_DOWN
#define STAR_LEVEL_REPEAT   BUTTON_A
#define STAR_MENU_RUN       BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD) || \
(CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
(CONFIG_KEYPAD == SANSA_M200_PAD) || \
(CONFIG_KEYPAD == SANSA_FUZE_PAD)

#define STAR_QUIT BUTTON_POWER
#define STAR_LEFT           BUTTON_LEFT
#define STAR_RIGHT          BUTTON_RIGHT
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_SELECT
#define STAR_LEVEL_UP      (BUTTON_SELECT | BUTTON_RIGHT)
#define STAR_LEVEL_DOWN    (BUTTON_SELECT | BUTTON_LEFT)
#define STAR_LEVEL_REPEAT  (BUTTON_SELECT | BUTTON_DOWN)
#define STAR_MENU_RUN       BUTTON_SELECT


#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)

#define STAR_QUIT           BUTTON_POWER
#define STAR_LEFT           BUTTON_LEFT
#define STAR_RIGHT          BUTTON_RIGHT
#define STAR_UP             BUTTON_SCROLL_UP
#define STAR_DOWN           BUTTON_SCROLL_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_REW
#define STAR_LEVEL_UP       (BUTTON_PLAY | BUTTON_SCROLL_UP)
#define STAR_LEVEL_DOWN     (BUTTON_PLAY | BUTTON_SCROLL_DOWN)
#define STAR_LEVEL_REPEAT   (BUTTON_PLAY | BUTTON_RIGHT)
#define STAR_MENU_RUN       BUTTON_FF

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)

#define STAR_QUIT           BUTTON_BACK
#define STAR_LEFT           BUTTON_LEFT
#define STAR_RIGHT          BUTTON_RIGHT
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_SELECT
#define STAR_LEVEL_UP       BUTTON_VOL_UP
#define STAR_LEVEL_DOWN     BUTTON_VOL_DOWN
#define STAR_LEVEL_REPEAT   BUTTON_MENU
#define STAR_MENU_RUN       BUTTON_SELECT

#elif (CONFIG_KEYPAD == MROBE100_PAD)

#define STAR_QUIT           BUTTON_POWER
#define STAR_LEFT           BUTTON_LEFT
#define STAR_RIGHT          BUTTON_RIGHT
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_SELECT
#define STAR_LEVEL_UP       BUTTON_PLAY
#define STAR_LEVEL_DOWN     BUTTON_MENU
#define STAR_LEVEL_REPEAT   BUTTON_DISPLAY
#define STAR_MENU_RUN       BUTTON_SELECT

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD

#define STAR_QUIT           BUTTON_RC_REC
#define STAR_LEFT           BUTTON_RC_REW
#define STAR_RIGHT          BUTTON_RC_FF
#define STAR_UP             BUTTON_RC_VOL_UP
#define STAR_DOWN           BUTTON_RC_VOL_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_RC_MODE
#define STAR_LEVEL_UP       (BUTTON_RC_PLAY|BUTTON_RC_VOL_UP)
#define STAR_LEVEL_DOWN     (BUTTON_RC_PLAY|BUTTON_RC_VOL_DOWN)
#define STAR_LEVEL_REPEAT   (BUTTON_RC_PLAY|BUTTON_RC_MENU)
#define STAR_MENU_RUN       BUTTON_RC_FF

#elif (CONFIG_KEYPAD == COWOND2_PAD)

#define STAR_QUIT           BUTTON_POWER
#define STAR_QUIT_NAME      "[POWER]"
#define STAR_MENU_RUN       BUTTON_MENU

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD

#define STAR_QUIT           BUTTON_BACK
#define STAR_LEFT           BUTTON_LEFT
#define STAR_RIGHT          BUTTON_RIGHT
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_PLAY
#define STAR_LEVEL_UP       (BUTTON_CUSTOM | BUTTON_UP)
#define STAR_LEVEL_DOWN     (BUTTON_CUSTOM | BUTTON_DOWN)
#define STAR_LEVEL_REPEAT   (BUTTON_CUSTOM | BUTTON_RIGHT)
#define STAR_MENU_RUN       BUTTON_MENU

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
//#ifndef STAR_QUIT
//#define STAR_QUIT           BUTTON_TOPLEFT
//#define STAR_QUIT_NAME           "[TOPLEFT]"
//#endif
#ifndef STAR_MENU_RUN
#define STAR_MENU_RUN       BUTTON_TOPRIGHT
#endif
#ifndef STAR_LEFT
#define STAR_LEFT           BUTTON_MIDLEFT
#endif
#ifndef STAR_RIGHT
#define STAR_RIGHT          BUTTON_MIDRIGHT
#endif
#ifndef STAR_UP
#define STAR_UP             BUTTON_TOPMIDDLE
#endif
#ifndef STAR_DOWN
#define STAR_DOWN           BUTTON_BOTTOMMIDDLE
#endif
#ifndef STAR_TOGGLE_CONTROL
#define STAR_TOGGLE_CONTROL BUTTON_CENTER
#define STAR_TOGGLE_CONTROL_NAME "[CENTER]"
#endif
#ifndef STAR_LEVEL_UP
#define STAR_LEVEL_UP       BUTTON_TOPLEFT
#define STAR_LEVEL_UP_NAME  "[TOPLEFT]"
#endif
#ifndef STAR_LEVEL_DOWN
#define STAR_LEVEL_DOWN     BUTTON_BOTTOMLEFT
#define STAR_LEVEL_DOWN_NAME "[BOTTOMLEFT]"
#endif
#ifndef STAR_LEVEL_REPEAT
#define STAR_LEVEL_REPEAT   BUTTON_BOTTOMRIGHT
#define STAR_LEVEL_REPEAT_NAME "[BOTTOMRIGHT]"
#endif
#endif

/* function returns because of USB? */
static bool usb_detected = false;

/* position of the ball */
static int ball_x, ball_y;

/* position of the block */
static int block_x, block_y;

/* number of stars to get to finish the level */
static int star_count;

/* the object we control : ball or block */
static int control;

/* the current board */
static char board[STAR_HEIGHT][STAR_WIDTH];

#include "pluginbitmaps/star_tiles.h"

#define TILE_WIDTH    BMPWIDTH_star_tiles
#define TILE_HEIGHT   (BMPHEIGHT_star_tiles/5)
#define STAR_OFFSET_X ((LCD_WIDTH - STAR_WIDTH * TILE_WIDTH) / 2)
#define STAR_OFFSET_Y ((LCD_HEIGHT - STAR_HEIGHT * TILE_HEIGHT - MAX(TILE_HEIGHT, 8)) / 2)

#define WALL  0
#define SPACE 1
#define BLOCK 2
#define STAR  3
#define BALL  4

#define MENU_START 0

/* char font size */
static int char_width = -1;
static int char_height = -1;

static const struct plugin_api* rb;

/* this arrays contains a group of levels loaded into memory */
static unsigned char* levels =
"****************\n"
"*X**........o*x*\n"
"*..........o.***\n"
"*.......**o....*\n"
"*...**.o......**\n"
"**.o..o.....o..*\n"
"*.o......o**.o.*\n"
"*.....**o.....o*\n"
"****************\n"
"\n"
".*..*.*.*...*.**\n"
"*...o.........X*\n"
"...*o..*o...o...\n"
"*.*.o.....o..*.*\n"
"......*...o...*.\n"
"*....*x*..o....*\n"
"...*..*.*...*oo*\n"
"*.............*.\n"
".*..*........*..\n"
"\n"
"****************\n"
"*...........o*x*\n"
"*...**......**X*\n"
"*...*o.........*\n"
"*.o.....o**...o*\n"
"*.*o..o..o*..o**\n"
"*.**o.*o..o.o***\n"
"*o....**o......*\n"
"****************\n"
"\n"
"****************\n"
"*............*x*\n"
"*.....*........*\n"
"**o*o.o*o*o*o*o*\n"
"*.*.*o.o*.*.*.**\n"
"**o*o*o.o*o*o*o*\n"
"*.....*........*\n"
"*...*.......*X.*\n"
"****************\n"
"\n"
".**************.\n"
"*X..*...*..*...*\n"
"*..*o.*.o..o.*.*\n"
"**......*..*...*\n"
"*o.*o*........**\n"
"**.....*.o.*...*\n"
"*o*..*.*.*...*x*\n"
"*...*....o*..*o*\n"
".**************.\n"
"\n"
"....************\n"
"...*...o...*o.o*\n"
"..*....o....*.**\n"
".*.....o.......*\n"
"*X.....o.......*\n"
"**.....o..*...**\n"
"*......o....*..*\n"
"*x.*...o..**o..*\n"
"****************\n"
"\n"
"****************\n"
"*..............*\n"
".**.***..*o.**o*\n"
".*o..*o.*.*.*.*.\n"
"..*..*..***.**..\n"
".**..*..*o*.*o**\n"
"*..............*\n"
"*..X*o....x..*o*\n"
"****************\n"
"\n"
"***************.\n"
"*..o**.........*\n"
"*..*o..**.o....*\n"
"*..o**.*.*o....*\n"
"**.....**..*o*.*\n"
"**.*.......o*o.*\n"
"*oxo*...o..*X*.*\n"
"**.............*\n"
".***************\n"
"\n"
"..*.***********.\n"
".*o*o......*..X*\n"
"*o.o*....o....*.\n"
".*.*..o**..o*..*\n"
"*..*o.*oxo....o*\n"
"*.....o**.....*.\n"
"*o*o.........*..\n"
"*...........*...\n"
".***********....\n"
"\n"
"....***********.\n"
"*****.o........*\n"
"*...x.***o.o*.o*\n"
"*.o...*o.*o...*.\n"
"*.....*..o..*.o*\n"
"*o*o..*.o*..*X*.\n"
".*o...***..***..\n"
"*.........*.*.*.\n"
".*********..*..*\n"
"\n"
"****************\n"
"*......*......X*\n"
"*..*oo.....oo.**\n"
"**...o...**...o*\n"
"*o....*o*oo..***\n"
"**.**....**....*\n"
"*o..o*.o....x.o*\n"
"**o***....*...**\n"
"***************.\n"
"\n"
"**.....**..****.\n"
"*X*****o.***.o**\n"
"*....oo.....o..*\n"
"*.**..**o..*o*.*\n"
"*.*.o.*.*o.**..*\n"
"*.**..**...*x*.*\n"
"*.....o........*\n"
"*........o.....*\n"
"****************\n"
"\n"
".**************.\n"
"*.X*........o.**\n"
"*.*...*o...o**.*\n"
"*.......o....*.*\n"
"*.o..........*o*\n"
"*.*......o.....*\n"
"**......o.o..*o*\n"
"*x..*....o.*.*.*\n"
".**************.\n"
"\n"
"****************\n"
"*o*o........o*o*\n"
"*.o*X......**..*\n"
"*.x........o...*\n"
"*........o*....*\n"
"*......o.......*\n"
"*.o*........*..*\n"
"*o*o........o*o*\n"
"****************\n"
"\n"
".******.********\n"
"*.....o*.....o.*\n"
"*.*.o.*..*...o.*\n"
"*..X*...*oo.*o.*\n"
".*.*...*.o..x*.*\n"
"*o.......*..*o.*\n"
".*......o.....*.\n"
"*o............o*\n"
"****************\n"
"\n"
"****************\n"
"**.x*o.o......o*\n"
"*o.Xo*o.......**\n"
"**.***........**\n"
"**.....o*o*....*\n"
"*oo.......o*o..*\n"
"**.o....****o..*\n"
"**o*..*........*\n"
"****************\n"
"\n"
"****************\n"
"*.o*........*X.*\n"
"*.*..o*oo*o..*.*\n"
"*....*o**o*.o..*\n"
"*.o*.......o*..*\n"
"*..o*o....o*...*\n"
"*.*..*.**o*..*.*\n"
"*....o.*o...x..*\n"
"****************\n"
"\n"
"****************\n"
"*.o....o..x*...*\n"
"*..*o*o...*o...*\n"
"*...*o*....*o..*\n"
"*...o..*...o*o.*\n"
"*.*o*...*.o*...*\n"
"*.o*o.*.o.*....*\n"
"*o*X..*.....*..*\n"
"****************\n"
"\n"
"****************\n"
"*o...**.....**o*\n"
"*.*..*......*o.*\n"
"*.o*...o**..o..*\n"
"*.*....*o......*\n"
"*....*...o*....*\n"
"*.**.o*.**o..*x*\n"
"*.o*.*o.....**X*\n"
"****************\n"
"\n"
"****************\n"
"*...o*o........*\n"
"**o..o*.**o...**\n"
"*.*.*.o...*..*.*\n"
"*.x.*..**..*.Xo*\n"
"*.*..*...o.*.*.*\n"
"**...o**.*o..o**\n"
"*........o*o...*\n"
"****************";

/**
 * Display text.
 */
void star_display_text(char *str, bool waitkey)
{
    int chars_by_line;
    int lines_by_screen;
    int chars_for_line;
    int current_line = 0;
    int first_char_index = 0;
    char *ptr_char;
    char *ptr_line;
    int i;
    char line[255];
    int key;
    bool go_on;

    rb->lcd_clear_display();

    chars_by_line = LCD_WIDTH / char_width;
    lines_by_screen = LCD_HEIGHT / char_height;

    do
    {
        ptr_char = str + first_char_index;
        chars_for_line = 0;
        i = 0;
        ptr_line = line;
        while (i < chars_by_line)
        {
            switch (*ptr_char)
            {
                case '\t':
                case ' ':
                    *(ptr_line++) = ' ';
                case '\n':
                case '\0':
                    chars_for_line = i;
                    break;

                default:
                    *(ptr_line++) = *ptr_char;
            }
            if (*ptr_char == '\n' || *ptr_char == '\0')
                break;
            ptr_char++;
            i++;
        }

        if (chars_for_line == 0)
            chars_for_line = i;

        line[chars_for_line] = '\0';

        /* test if we have cut a word. If it is the case we don't have to */
        /* skip the space */
        if (i == chars_by_line && chars_for_line == chars_by_line)
            first_char_index += chars_for_line;
        else
            first_char_index += chars_for_line + 1;

        /* print the line on the screen */
        rb->lcd_putsxy(0, current_line * char_height, line);

        /* if the number of line showed on the screen is equals to the */
        /* maximum number of line we can show, we wait for a key pressed to */
        /* clear and show the remaining text. */
        current_line++;
        if (current_line == lines_by_screen || *ptr_char == '\0')
        {
            current_line = 0;
            rb->lcd_update();
            go_on = false;
            while (waitkey && !go_on)
            {
                key = rb->button_get(true);
                switch (key)
                {
                    case STAR_QUIT:
                    case STAR_LEFT:
                    case STAR_DOWN:
                        go_on = true;
                        break;

                    default:
                        if (rb->default_event_handler(key) == SYS_USB_CONNECTED)
                        {
                            usb_detected = true;
                            go_on = true;
                            break;
                        }
                }
            }
            rb->lcd_clear_display();
        }
    } while (*ptr_char != '\0');
}

/**
 * Do a pretty transition from one level to another.
 */
static void star_transition_update(void)
{
    const int center_x = LCD_WIDTH / 2;
    const int center_y = LCD_HEIGHT / 2;
    int x = 0;
    int y = 0;
#if LCD_WIDTH >= LCD_HEIGHT
#if defined(IPOD_VIDEO)
    const int step = 4;
#else
    const int step = 1;
#endif
    const int lcd_demi_width = LCD_WIDTH / 2;
    int var_y = 0;

    for (; x < lcd_demi_width ; x+=step)
    {
        var_y += LCD_HEIGHT;
        if (var_y > LCD_WIDTH)
        {
            var_y -= LCD_WIDTH;
            y+=step;
        }
        if( x )
        {
            rb->lcd_update_rect(center_x - x, center_y - y, x*2, step);
            rb->lcd_update_rect(center_x - x, center_y + y - step, x*2, step);
        }
        if( y )
        {
            rb->lcd_update_rect(center_x - x, center_y - y, step, y*2);
            rb->lcd_update_rect(center_x + x - step, center_y - y, step, y*2);
        }
        STAR_SLEEP
    }
#else
    int lcd_demi_height = LCD_HEIGHT / 2;
    int var_x = 0;

    for (; y < lcd_demi_height ; y++)
    {
        var_x += LCD_WIDTH;
        if (var_x > LCD_HEIGHT)
        {
            var_x -= LCD_HEIGHT;
            x++;
        }
        if( x )
        {
            rb->lcd_update_rect(center_x - x, center_y - y, x * 2, 1);
            rb->lcd_update_rect(center_x - x, center_y + y - 1, x * 2, 1);
        }
        if( y )
        {
            rb->lcd_update_rect(center_x - x, center_y - y, 1, y * 2);
            rb->lcd_update_rect(center_x + x - 1, center_y - y, 1, y * 2);
        }
        STAR_SLEEP
    }
#endif
    rb->lcd_update();
}

/**
 * Display information board of the current level.
 */
static void star_display_board_info(int current_level)
{
    int label_pos_y, tile_pos_y;
    char str_info[32];

    if (TILE_HEIGHT > char_height)
    {
        tile_pos_y = LCD_HEIGHT - TILE_HEIGHT;
        label_pos_y = tile_pos_y + (TILE_HEIGHT - char_height) / 2;
    }
    else
    {
        label_pos_y = LCD_HEIGHT - char_height;
        tile_pos_y = label_pos_y + (char_height - TILE_HEIGHT) / 2;
    }

    rb->snprintf(str_info, sizeof(str_info), "L:%02d", current_level + 1);
    rb->lcd_putsxy(STAR_OFFSET_X, label_pos_y, str_info);
    rb->snprintf(str_info, sizeof(str_info), "S:%02d", star_count);
    rb->lcd_putsxy(LCD_WIDTH/2 - 2 * char_width, label_pos_y, str_info);
    rb->lcd_putsxy(STAR_OFFSET_X + (STAR_WIDTH-1) * TILE_WIDTH - 2 * char_width,
                   label_pos_y, "C:");

    rb->lcd_bitmap_part(star_tiles, 0, control == STAR_CONTROL_BALL ?
                        BALL*TILE_HEIGHT : BLOCK*TILE_HEIGHT, TILE_WIDTH,
                        STAR_OFFSET_X + (STAR_WIDTH-1) * TILE_WIDTH,
                        tile_pos_y, TILE_WIDTH, TILE_HEIGHT);

    rb->lcd_update_rect(0, MIN(label_pos_y, tile_pos_y), LCD_WIDTH,
                        MAX(TILE_HEIGHT, char_height));
}


/**
 * Load a level into board array.
 */
static int star_load_level(int current_level)
{
    int x, y;
    char *ptr_tab;

    if (current_level < 0)
        current_level = 0;
    else if (current_level > STAR_LEVEL_COUNT-1)
        current_level = STAR_LEVEL_COUNT-1;


    ptr_tab = levels + current_level * STAR_LEVEL_SIZE;
    control = STAR_CONTROL_BALL;
    star_count = 0;

    rb->lcd_clear_display();

    for (y = 0 ; y < STAR_HEIGHT ; y++)
    {
        for (x = 0 ; x < STAR_WIDTH ; x++)
        {
            board[y][x] = *ptr_tab;
            switch (*ptr_tab)
            {
#   define DRAW_TILE( a )                                                 \
                    rb->lcd_bitmap_part( star_tiles, 0,                   \
                                         a*TILE_HEIGHT, TILE_WIDTH,       \
                                         STAR_OFFSET_X + x * TILE_WIDTH,  \
                                         STAR_OFFSET_Y + y * TILE_HEIGHT, \
                                         TILE_WIDTH, TILE_HEIGHT);
                case STAR_VOID:
                    DRAW_TILE( SPACE );
                    break;

                case STAR_WALL:
                    DRAW_TILE( WALL );
                    break;

                case STAR_STAR:
                    DRAW_TILE( STAR );
                    star_count++;
                    break;

                case STAR_BALL:
                    ball_x = x;
                    ball_y = y;
                    DRAW_TILE( BALL );
                    break;


                case STAR_BLOCK:
                    block_x = x;
                    block_y = y;
                    DRAW_TILE( BLOCK );
                    break;
            }
            ptr_tab++;
        }
        ptr_tab++;
    }
    star_display_board_info(current_level);
    star_transition_update();
    return 1;
}

static void star_animate_tile(int tile_no, int start_x, int start_y,
                              int delta_x, int delta_y)
{
    int i;

    if (delta_x != 0) /* horizontal */
    {
        for (i = 1 ; i <= TILE_WIDTH ; i++)
        {
            STAR_SLEEP
            rb->lcd_bitmap_part(star_tiles, 0, SPACE * TILE_HEIGHT, TILE_WIDTH,
                        start_x, start_y, TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_bitmap_part(star_tiles, 0, tile_no * TILE_HEIGHT, TILE_WIDTH,
                        start_x + delta_x * i, start_y, TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_update_rect(start_x + delta_x * i - (delta_x>0?1:0),
                                start_y, TILE_WIDTH + 1, TILE_HEIGHT);
        }
    }
    else /* vertical */
    {
        for (i = 1 ; i <= TILE_HEIGHT ; i++)
        {
            STAR_SLEEP
            rb->lcd_bitmap_part(star_tiles, 0, SPACE * TILE_HEIGHT, TILE_WIDTH,
                        start_x, start_y, TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_bitmap_part(star_tiles, 0, tile_no * TILE_HEIGHT, TILE_WIDTH,
                        start_x, start_y + delta_y * i, TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_update_rect(start_x, start_y + delta_y * i - (delta_y>0?1:0),
                                TILE_WIDTH, TILE_HEIGHT + 1);
        }
    }
}

/**
 * Run the game.
 */
static int star_run_game(int current_level)
{
    int move_x = 0;
    int move_y = 0;
    int key;
    int lastkey = BUTTON_NONE;

    int label_offset_y;

    label_offset_y = LCD_HEIGHT - char_height;

    if (!star_load_level(current_level))
        return 0;

    while (true)
    {
        move_x = 0;
        move_y = 0;

        while ((move_x == 0) && (move_y == 0))
        {
            key = rb->button_get(true);
            switch (key)
            {
#ifdef STAR_RC_QUIT
                case STAR_RC_QUIT:
#endif
                case STAR_QUIT:
                    return -1;

                case STAR_LEFT:
                    move_x = -1;
                    break;

                case STAR_RIGHT:
                    move_x = 1;
                    break;

                case STAR_UP:
                    move_y = -1;
                    break;

                case STAR_DOWN:
                    move_y = 1;
                    break;

                case STAR_LEVEL_DOWN:
#ifdef STAR_LEVEL_DOWN_PRE
                    if (lastkey != STAR_LEVEL_DOWN_PRE)
                        break;
#endif
                    if (current_level > 0)
                    {
                        current_level--;
                        star_load_level(current_level);
                    }
                    break;

                case STAR_LEVEL_REPEAT:
                    star_load_level(current_level);
                    break;

                case STAR_LEVEL_UP:
#ifdef STAR_LEVEL_UP_PRE
                    if (lastkey != STAR_LEVEL_UP_PRE)
                        break;
#endif
                    if (current_level < STAR_LEVEL_COUNT - 1)
                    {
                        current_level++;
                        star_load_level(current_level);
                    }
                    break;

                case STAR_TOGGLE_CONTROL:
#ifdef STAR_TOGGLE_CONTROL_PRE
                    if (lastkey != STAR_TOGGLE_CONTROL_PRE)
                        break;
#endif
#ifdef STAR_TOGGLE_CONTROL2
                case STAR_TOGGLE_CONTROL2:
#endif
                    if (control == STAR_CONTROL_BALL)
                        control = STAR_CONTROL_BLOCK;
                    else
                        control = STAR_CONTROL_BALL;
                    star_display_board_info(current_level);
                    break;

                default:
                    if (rb->default_event_handler(key) == SYS_USB_CONNECTED)
                    {
                        usb_detected = true;
                        return 0;
                    }
                    break;
            }
            if (key != BUTTON_NONE)
                lastkey = key;
        }

        if (control == STAR_CONTROL_BALL)
        {
            board[ball_y][ball_x] = STAR_VOID;
            while ((board[ball_y + move_y][ball_x + move_x] == STAR_VOID
                    || board[ball_y + move_y][ball_x + move_x] == STAR_STAR))

            {
                star_animate_tile(BALL, STAR_OFFSET_X + ball_x * TILE_WIDTH,
                                  STAR_OFFSET_Y + ball_y * TILE_HEIGHT,
                                  move_x, move_y);
                ball_x += move_x;
                ball_y += move_y;

                if (board[ball_y][ball_x] == STAR_STAR)
                {
                    board[ball_y][ball_x] = STAR_VOID;
                    star_count--;

                    star_display_board_info(current_level);
                }
            }
            board[ball_y][ball_x] = STAR_BALL;
        }
        else
        {
            board[block_y][block_x] = STAR_VOID;
            while (board[block_y + move_y][block_x + move_x] == STAR_VOID)
            {
                star_animate_tile(BLOCK, STAR_OFFSET_X + block_x * TILE_WIDTH,
                                  STAR_OFFSET_Y + block_y * TILE_HEIGHT,
                                  move_x, move_y);
                block_x += move_x;
                block_y += move_y;
            }
            board[block_y][block_x] = STAR_BLOCK;
        }

        if (star_count == 0)
        {
            current_level++;
            if (current_level == STAR_LEVEL_COUNT)
            {
                rb->lcd_clear_display();
                star_display_text("Congratulations!", true);
                rb->lcd_update();

                /* There is no such level as STAR_LEVEL_COUNT so it can't be the
                 * current_level */
                current_level--;
                return 1;
            }
            star_load_level(current_level);
        }
    }
}

/**
 * Display the choice menu.
 */
static int star_menu(void)
{
    int selection, level=1;
    bool menu_quit = false;
    struct viewport vp[NB_SCREENS];
    /* get the size of char */
    rb->lcd_getstringsize("a", &char_width, &char_height);

    MENUITEM_STRINGLIST(menu,"Star Menu",NULL,"Play","Choose Level",
                        "Information","Keys","Quit");
    FOR_NB_SCREENS(selection)
    {
        rb->viewport_set_defaults(&vp[selection], selection);
#if LCD_DEPTH > 1
        if (rb->screens[selection]->depth > 1)
        {
            vp->bg_pattern = LCD_BLACK;
            vp->fg_pattern = LCD_WHITE;
        }
#endif
    }
    while(!menu_quit)
    {
        switch(rb->do_menu(&menu, &selection, vp, true))
        {
            case 0:
                menu_quit = true;
                break;
            case 1:
                rb->set_int("Level", "", UNIT_INT, &level,
                            NULL, 1, 1, STAR_LEVEL_COUNT, NULL );
                break;
            case 2:
                star_display_text(
                    "INFO\n\n"
                    "Take all the stars to go to the next level. "
                    "You can toggle control with the block to "
                    "use it as a mobile wall. The block cannot "
                    "take stars.", true);
                    break;
            case 3:
#if CONFIG_KEYPAD == RECORDER_PAD
                star_display_text("KEYS\n\n"
                                  "[ON] Toggle Ctl.\n"
                                  "[OFF] Exit\n"
                                  "[F1] Prev. level\n"
                                  "[F2] Reset level\n"
                                  "[F3] Next level", true);
#elif CONFIG_KEYPAD == ONDIO_PAD
                star_display_text("KEYS\n\n"
                                  "[MODE] Toggle Ctl\n"
                                  "[OFF] Exit\n"
                                  "[M <] Prev. level\n"
                                  "[M ^] Reset level\n"
                                  "[M >] Next level", true);
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
                star_display_text("KEYS\n\n"
                                  "[MODE/NAVI] Toggle Ctrl\n"
                                  "[OFF] Exit\n"
                                  "[ON + LEFT] Prev. level\n"
                                  "[ON + NAVI] Reset level\n"
                                  "[ON + RIGHT] Next level", true);
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
                star_display_text("KEYS\n\n"
                                  "[SELECT] Toggle Ctl\n"
                                  "[S + MENU] Exit\n"
                                  "[S <] Prev. level\n"
                                  "[S + PLAY] Reset level\n"
                                  "[S >] Next level", true);
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
                star_display_text("KEYS\n\n"
                                  "[SELECT] Toggle Ctl\n"
                                  "[POWER] Exit\n"
                                  "[REC..] Prev. level\n"
                                  "[PLAY] Reset level\n"
                                  "[REC] Next level", true);
#elif CONFIG_KEYPAD == GIGABEAT_PAD
                star_display_text("KEYS\n\n"
                                  "[MENU] Toggle Control\n"
                                  "[A] Exit\n"
                                  "[PWR+DOWN] Prev. level\n"
                                  "[PWR+RIGHT] Reset level\n"
                                  "[PWR+UP] Next level", true);
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
                star_display_text("KEYS\n\n"
                                  "[REW] Toggle Ctl\n"
                                  "[POWER] Exit\n"
                                  "[PLAY+DOWN] Prev. level\n"
                                  "[PLAY+RIGHT] Reset level\n"
                                  "[PLAY+UP] Next level", true);
#endif
#ifdef HAVE_TOUCHSCREEN
                star_display_text("KEYS\n\n"
                                  STAR_TOGGLE_CONTROL_NAME " Toggle Control\n"
                                  STAR_QUIT_NAME " Exit\n"
                                  STAR_LEVEL_DOWN_NAME " Prev. level\n"
                                  STAR_LEVEL_REPEAT_NAME " Reset level\n"
                                  STAR_LEVEL_UP_NAME " Next level", true);
#endif
                break;
            default:
                menu_quit = true;
                break;
        }
    }

    if (selection == MENU_START)
    {
        rb->lcd_setfont(FONT_SYSFIXED);
        rb->lcd_getstringsize("a", &char_width, &char_height);
        level--;
        star_run_game(level);
    }
    return PLUGIN_OK;
}

/**
 * Main entry point
 */
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    (void)parameter;
    rb = api;

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_background( LCD_BLACK );
    rb->lcd_set_foreground( LCD_WHITE );
#endif

    /* display choice menu */
    return star_menu();
}

#endif
