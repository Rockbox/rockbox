/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Tom Ross
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

#include "pluginbitmaps/pegbox_header.h"
#include "pluginbitmaps/pegbox_pieces.h"

#if LCD_HEIGHT >= 80 /* enough space for a graphical menu */
#include "pluginbitmaps/pegbox_menu_top.h"
#include "pluginbitmaps/pegbox_menu_items.h"
#define MENU_X      (LCD_WIDTH-BMPWIDTH_pegbox_menu_items)/2
#define MENU_Y      BMPHEIGHT_pegbox_menu_top
#define ITEM_WIDTH  BMPWIDTH_pegbox_menu_items
#define ITEM_HEIGHT (BMPHEIGHT_pegbox_menu_items/9)
#endif

static const struct plugin_api* rb;

PLUGIN_HEADER

/* final game return status */
#define PB_END  3
#define PB_USB  2
#define PB_QUIT 1

#define DATA_FILE   PLUGIN_DIR "/games/pegbox.data"
#define SAVE_FILE   PLUGIN_DIR "/games/pegbox.save"

#define ROWS       8   /* Number of rows on each board */
#define COLS       12  /* Number of columns on each board */
#define NUM_LEVELS 15  /* Number of levels */

#define SPACE     0
#define WALL      1
#define TRIANGLE  2
#define CROSS     3
#define SQUARE    4
#define CIRCLE    5
#define HOLE      6
#define PLAYER    7

#if CONFIG_KEYPAD == RECORDER_PAD
#define PEGBOX_SAVE     BUTTON_ON
#define PEGBOX_QUIT     BUTTON_OFF
#define PEGBOX_RESTART  BUTTON_F2
#define PEGBOX_LVL_UP   BUTTON_F1
#define PEGBOX_LVL_DOWN BUTTON_F3
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SAVE_TEXT "ON"
#define QUIT_TEXT "OFF"
#define RESTART_TEXT "F2"
#define LVL_UP_TEXT "F1"
#define LVL_DOWN_TEXT "F3"

#elif CONFIG_KEYPAD == ONDIO_PAD
#define PEGBOX_SAVE     BUTTON_OFF
#define PEGBOX_QUIT     (BUTTON_MENU | BUTTON_LEFT)
#define PEGBOX_RESTART  (BUTTON_MENU | BUTTON_RIGHT)
#define PEGBOX_LVL_UP   (BUTTON_MENU | BUTTON_UP)
#define PEGBOX_LVL_DOWN (BUTTON_MENU | BUTTON_DOWN)
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SAVE_TEXT "OFF"
#define QUIT_TEXT "M+LEFT"
#define RESTART_TEXT "M+RIGHT"
#define LVL_UP_TEXT "M+UP"
#define LVL_DOWN_TEXT "M+DOWN"

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define PEGBOX_SAVE     BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_OFF
#define PEGBOX_RESTART  BUTTON_ON
#define PEGBOX_LVL_UP   BUTTON_MODE
#define PEGBOX_LVL_DOWN BUTTON_REC
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SAVE_TEXT "NAVI"
#define QUIT_TEXT "OFF"
#define RESTART_TEXT "ON"
#define LVL_UP_TEXT "AB"
#define LVL_DOWN_TEXT "REC"

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define PEGBOX_SAVE     (BUTTON_SELECT|BUTTON_RIGHT)
#define PEGBOX_QUIT     (BUTTON_SELECT|BUTTON_PLAY)
#define PEGBOX_RESTART  (BUTTON_SELECT|BUTTON_LEFT)
#define PEGBOX_LVL_UP   (BUTTON_SELECT|BUTTON_MENU)
#define PEGBOX_UP       BUTTON_MENU
#define PEGBOX_DOWN     BUTTON_PLAY
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SAVE_TEXT "SELECT+RIGHT"
#define QUIT_TEXT "SELECT+PLAY"
#define RESTART_TEXT "SELECT+LEFT"
#define LVL_UP_TEXT  "SELECT+MENU"
#define LVL_DOWN_TEXT "-"

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define PEGBOX_SAVE     BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  BUTTON_REC
#define PEGBOX_LVL_UP   BUTTON_PLAY
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SAVE_TEXT "SELECT"
#define QUIT_TEXT "OFF"
#define RESTART_TEXT "REC"
#define LVL_UP_TEXT "PLAY"
#define LVL_DOWN_TEXT "-"

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define PEGBOX_SAVE     BUTTON_MODE
#define PEGBOX_QUIT     BUTTON_PLAY
#define PEGBOX_RESTART  (BUTTON_EQ|BUTTON_MODE)
#define PEGBOX_LVL_UP   (BUTTON_EQ|BUTTON_UP)
#define PEGBOX_LVL_DOWN (BUTTON_EQ|BUTTON_DOWN)
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SAVE_TEXT "MODE"
#define QUIT_TEXT "PLAY"
#define RESTART_TEXT "EQ+MODE"
#define LVL_UP_TEXT "EQ+UP"
#define LVL_DOWN_TEXT "EQ+DOWN"

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define PEGBOX_SAVE     BUTTON_PLAY
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  (BUTTON_FF|BUTTON_REPEAT)
#define PEGBOX_LVL_UP   (BUTTON_FF|BUTTON_SCROLL_UP)
#define PEGBOX_LVL_DOWN (BUTTON_FF|BUTTON_SCROLL_DOWN)
#define PEGBOX_UP       BUTTON_SCROLL_UP
#define PEGBOX_DOWN     BUTTON_SCROLL_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SAVE_TEXT "PLAY"
#define QUIT_TEXT "OFF"
#define RESTART_TEXT "LONG FF"
#define LVL_UP_TEXT "FF+SCROLL_UP"
#define LVL_DOWN_TEXT "FF+SCROLL_DOWN"

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define PEGBOX_SAVE     BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  BUTTON_REC
#define PEGBOX_LVL_UP   BUTTON_SCROLL_BACK
#define PEGBOX_LVL_DOWN BUTTON_SCROLL_FWD
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SAVE_TEXT "SELECT"
#define QUIT_TEXT "POWER"
#define RESTART_TEXT "REC"
#define LVL_UP_TEXT "SCROLL BACK"
#define LVL_DOWN_TEXT "SCROLL FWD"

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define PEGBOX_SAVE     BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  BUTTON_A
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SAVE_TEXT "SELECT"
#define QUIT_TEXT "POWER"
#define RESTART_TEXT "A"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define PEGBOX_SAVE     BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_BACK
#define PEGBOX_RESTART  BUTTON_MENU
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SAVE_TEXT "SELECT"
#define QUIT_TEXT "BACK"
#define RESTART_TEXT "MENU"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"

#elif CONFIG_KEYPAD == MROBE100_PAD
#define PEGBOX_SAVE     BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  BUTTON_PLAY
#define PEGBOX_LVL_UP   BUTTON_MENU
#define PEGBOX_LVL_DOWN BUTTON_DISPLAY
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SAVE_TEXT "SELECT"
#define QUIT_TEXT "POWER"
#define RESTART_TEXT "PLAY"
#define LVL_UP_TEXT "MENU"
#define LVL_DOWN_TEXT "DISPLAY"

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define PEGBOX_SAVE     BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  BUTTON_REC
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SAVE_TEXT "SELECT"
#define QUIT_TEXT "POWER"
#define RESTART_TEXT "REC"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define PEGBOX_SAVE     BUTTON_RC_PLAY
#define PEGBOX_QUIT     BUTTON_RC_REC
#define PEGBOX_RESTART  BUTTON_RC_MODE
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_RC_VOL_UP
#define PEGBOX_DOWN     BUTTON_RC_VOL_DOWN
#define PEGBOX_RIGHT    BUTTON_RC_FF
#define PEGBOX_LEFT     BUTTON_RC_REW

#define SAVE_TEXT "REM. PLAY"
#define QUIT_TEXT "REM. REC"
#define RESTART_TEXT "REM. MODE"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"

#elif CONFIG_KEYPAD == COWOND2_PAD
#define PEGBOX_QUIT     BUTTON_POWER

#define QUIT_TEXT "POWER"
#else
#error Unsupported keymap!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef PEGBOX_QUIT
#define PEGBOX_QUIT     BUTTON_TOPLEFT
#endif
#ifndef PEGBOX_SAVE
#define PEGBOX_SAVE     BUTTON_CENTER
#endif
#ifndef PEGBOX_RESTART
#define PEGBOX_RESTART  BUTTON_TOPRIGHT
#endif
#ifndef PEGBOX_LVL_UP
#define PEGBOX_LVL_UP   BUTTON_BOTTOMLEFT
#endif
#ifndef PEGBOX_LVL_DOWN
#define PEGBOX_LVL_DOWN BUTTON_BOTTOMRIGHT
#endif
#ifndef PEGBOX_UP
#define PEGBOX_UP       BUTTON_TOPMIDDLE
#endif
#ifndef PEGBOX_DOWN
#define PEGBOX_DOWN     BUTTON_BOTTOMMIDDLE
#endif
#ifndef PEGBOX_RIGHT
#define PEGBOX_RIGHT    BUTTON_MIDRIGHT
#endif
#ifndef PEGBOX_LEFT
#define PEGBOX_LEFT     BUTTON_MIDLEFT
#endif
#ifndef SAVE_TEXT
#define SAVE_TEXT "CENTER"
#endif
#ifndef QUIT_TEXT
#define QUIT_TEXT "TOPLEFT"
#endif
#ifndef RESTART_TEXT
#define RESTART_TEXT "TOPRIGHT"
#endif
#ifndef LVL_UP_TEXT
#define LVL_UP_TEXT "BOTTOMLEFT"
#endif
#ifndef LVL_DOWN_TEXT
#define LVL_DOWN_TEXT "BOTTOMRIGHT"
#endif
#endif


/* get several sizes from the bitmaps */
#ifdef BMPWIDTH_pegbox_pieces
#define PIECE_WIDTH    BMPWIDTH_pegbox_pieces
#define PIECE_HEIGHT   (BMPHEIGHT_pegbox_pieces/7)
#else
/* dummy numbers to avoid #error in dependency generation */
#define PIECE_WIDTH  50
#define PIECE_HEIGHT 10
#endif
#define BOARD_WIDTH    (12*PIECE_WIDTH)
#define BOARD_HEIGHT   (8*PIECE_HEIGHT)


/* define a wide layout where the statistics are alongside the board, not above
*  base calculation on the piece bitmaps for the 8x12 board */
#if (LCD_WIDTH - BOARD_WIDTH) > (LCD_HEIGHT - BOARD_HEIGHT)
#define WIDE_LAYOUT
#endif


#define HEADER_WIDTH    BMPWIDTH_pegbox_header
#define HEADER_HEIGHT   BMPHEIGHT_pegbox_header


#if defined WIDE_LAYOUT

#if ((BOARD_WIDTH + HEADER_WIDTH + 4) <= LCD_WIDTH)
#define BOARD_X   2
#else
#define BOARD_X   1
#endif
#define BOARD_Y   (LCD_HEIGHT-BOARD_HEIGHT)/2

#if (LCD_WIDTH >= 132) && (LCD_HEIGHT >= 80)
#define TEXT_X         116
#define LEVEL_TEXT_Y   14
#define PEGS_TEXT_Y    58
#else
#error "Unsupported screen size"
#endif

#else /* "normal" layout */

#define BOARD_X   (LCD_WIDTH-BOARD_WIDTH)/2
#if ((BOARD_HEIGHT + HEADER_HEIGHT + 4) <= LCD_HEIGHT)
#define BOARD_Y   HEADER_HEIGHT+2
#else
#define BOARD_Y   HEADER_HEIGHT
#endif

#if LCD_WIDTH >= 320
#define LEVEL_TEXT_X   59
#define PEGS_TEXT_X    276
#define TEXT_Y         28
#elif LCD_WIDTH >= 240
#define LEVEL_TEXT_X   59
#define PEGS_TEXT_X    196
#define TEXT_Y         28
#elif LCD_WIDTH >= 220
#define LEVEL_TEXT_X   49
#define PEGS_TEXT_X    186
#define TEXT_Y         28
#elif LCD_WIDTH >= 176
#define LEVEL_TEXT_X   38
#define PEGS_TEXT_X    155
#define TEXT_Y         17
#elif LCD_WIDTH >= 160
#define LEVEL_TEXT_X   37
#define PEGS_TEXT_X    140
#define TEXT_Y         13
#elif LCD_WIDTH >= 138
#define LEVEL_TEXT_X   28
#define PEGS_TEXT_X    119
#define TEXT_Y         15
#elif LCD_WIDTH >= 128
#if HEADER_HEIGHT > 16
#define LEVEL_TEXT_X   26
#define PEGS_TEXT_X    107
#define TEXT_Y         31
#else
#define LEVEL_TEXT_X   15
#define PEGS_TEXT_X    100
#define TEXT_Y         5
#endif /* HEADER_HEIGHT */
#elif LCD_WIDTH >= 112
#define LEVEL_TEXT_X   25
#define PEGS_TEXT_X    90
#define TEXT_Y         0
#endif /* LCD_WIDTH */

#endif /* WIDE_LAYOUT */


#ifdef HAVE_LCD_COLOR
#define BG_COLOR           LCD_BLACK
#define TEXT_BG            LCD_RGBPACK(189,189,189)
#endif


#ifdef HAVE_TOUCHSCREEN
#include "lib/touchscreen.h"

static struct ts_mapping main_menu_items[5] =
{
{MENU_X, MENU_Y, ITEM_WIDTH,       ITEM_HEIGHT},
{MENU_X, MENU_Y+ITEM_HEIGHT,   ITEM_WIDTH, ITEM_HEIGHT},
{MENU_X, MENU_Y+ITEM_HEIGHT*2, ITEM_WIDTH, ITEM_HEIGHT},
{MENU_X, MENU_Y+ITEM_HEIGHT*3, ITEM_WIDTH, ITEM_HEIGHT},
{
#if (LCD_WIDTH >= 138) && (LCD_HEIGHT > 110)
0, MENU_Y+4*ITEM_HEIGHT+8, SYSFONT_WIDTH*28, SYSFONT_HEIGHT
#elif LCD_WIDTH > 112
0, LCD_HEIGHT - 8, SYSFONT_WIDTH*28, SYSFONT_HEIGHT
#else
#error "Touchscreen isn't supported on non-bitmap screens!"
#endif
}

};
static struct ts_mappings main_menu = {main_menu_items, 5};

static struct ts_raster pegbox_raster =
    { BOARD_X, BOARD_Y, COLS*PIECE_WIDTH, ROWS*PIECE_HEIGHT,
      PIECE_WIDTH, PIECE_HEIGHT };
static struct ts_raster_button_mapping pegbox_raster_btn =
    { &pegbox_raster, false, false, true, false, true, {0, 0}, 0, 0, 0 };
#endif

struct game_context {
    unsigned int level;
    unsigned int highlevel;
    signed int player_row;
    signed int player_col;
    unsigned int num_left;
    bool save_exist;
    unsigned int playboard[ROWS][COLS];
};

char levels[NUM_LEVELS][ROWS][COLS] = {
    /* Level 1 */
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,},
     {1, 0, 0, 0, 7, 0, 0, 5, 0, 0, 0, 1,},
     {1, 0, 0, 0, 0, 3, 3, 2, 0, 0, 0, 1,},
     {1, 0, 0, 0, 4, 6, 0, 5, 0, 0, 0, 1,},
     {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,},
     {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,}},

    /* Level 2 */
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {1, 1, 1, 0, 0, 0, 2, 2, 0, 0, 0, 1,},
     {1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1,},
     {1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1,},
     {7, 0, 0, 0, 2, 2, 5, 5, 0, 0, 0, 1,},
     {1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1,},
     {1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1,},
     {1, 1, 1, 0, 0, 0, 2, 2, 0, 0, 0, 1,}},

    /* Level 3 */
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {1, 0, 0, 0, 0, 0, 2, 0, 7, 0, 0, 0,},
     {1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 2, 1,},
     {1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,}},

    /* Level 4 */
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {6, 0, 4, 0, 2, 0, 2, 0, 0, 0, 0, 1,},
     {6, 0, 3, 0, 5, 0, 2, 0, 0, 0, 0, 1,},
     {6, 0, 5, 0, 4, 7, 2, 0, 0, 0, 0, 1,},
     {6, 0, 2, 0, 4, 0, 2, 0, 3, 0, 0, 1,},
     {6, 0, 4, 0, 5, 0, 2, 0, 0, 0, 0, 1,},
     {6, 0, 5, 0, 4, 0, 2, 0, 0, 0, 0, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,}},

    /* Level 5 */
    {{0, 6, 6, 0, 4, 6, 0, 0, 6, 0, 0, 0,},
     {0, 6, 6, 0, 4, 4, 0, 0, 6, 0, 0, 2,},
     {2, 6, 6, 0, 6, 6, 6, 0, 1, 2, 2, 2,},
     {0, 6, 6, 0, 6, 4, 6, 0, 1, 2, 0, 2,},
     {0, 6, 6, 0, 6, 7, 6, 5, 6, 0, 0, 0,},
     {2, 6, 6, 0, 6, 6, 6, 0, 6, 0, 0, 0,},
     {0, 6, 6, 0, 4, 0, 0, 0, 6, 0, 0, 0,},
     {0, 6, 6, 0, 0, 5, 0, 0, 6, 5, 5, 0,}},

    /* Level 6 */
    {{7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
     {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0,},
     {2, 0, 0, 2, 0, 0, 0, 2, 2, 0, 0, 0,},
     {0, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 0,},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
     {0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1,},
     {0, 3, 0, 0, 0, 0, 0, 0, 5, 4, 6, 0,},
     {0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1,}},

    /* Level 7 */
    {{1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1,},
     {1, 1, 1, 6, 0, 0, 4, 6, 0, 1, 1, 1,},
     {1, 1, 1, 1, 0, 1, 5, 1, 0, 1, 1, 1,},
     {1, 1, 1, 2, 3, 3, 7, 4, 2, 6, 1, 1,},
     {1, 1, 1, 1, 0, 1, 2, 1, 0, 0, 0, 1,},
     {1, 1, 1, 1, 0, 0, 5, 0, 0, 1, 0, 1,},
     {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1,},
     {1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,}},

    /* Level 8 */
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {0, 0, 3, 3, 3, 3, 3, 4, 3, 3, 0, 0,},
     {0, 0, 3, 3, 3, 2, 3, 3, 5, 3, 0, 0,},
     {7, 0, 3, 3, 3, 2, 3, 3, 4, 3, 0, 0,},
     {0, 0, 3, 3, 4, 5, 3, 3, 3, 3, 0, 0,},
     {0, 0, 3, 3, 5, 2, 3, 3, 3, 3, 0, 0,},
     {0, 0, 3, 3, 2, 4, 3, 3, 3, 3, 0, 0,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,}},

    /* Level 9 */
    {{0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,},
     {0, 3, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,},
     {0, 0, 0, 0, 2, 4, 4, 3, 0, 1, 1, 0,},
     {0, 1, 0, 0, 2, 1, 0, 0, 0, 1, 1, 1,},
     {0, 0, 0, 2, 2, 7, 1, 0, 0, 0, 0, 2,},
     {0, 0, 0, 0, 2, 1, 0, 0, 1, 1, 1, 1,},
     {0, 3, 1, 0, 2, 5, 5, 0, 0, 0, 3, 0,},
     {0, 0, 0, 0, 4, 1, 0, 0, 0, 0, 0, 0,}},

    /* Level 10 */
    {{1, 1, 1, 1, 2, 1, 1, 1, 0, 0, 0, 0,},
     {1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 7,},
     {0, 0, 4, 0, 6, 6, 6, 0, 0, 0, 3, 0,},
     {0, 3, 3, 0, 6, 6, 6, 0, 4, 3, 4, 0,},
     {0, 3, 3, 0, 6, 6, 6, 0, 4, 3, 4, 0,},
     {0, 0, 0, 0, 6, 6, 6, 0, 3, 0, 0, 0,},
     {1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0,},
     {1, 1, 1, 1, 1, 2, 1, 1, 0, 0, 0, 0,}},

    /* Level 11 */
    {{1, 7, 1, 0, 1, 1, 6, 0, 0, 1, 1, 0,},
     {1, 0, 0, 0, 5, 4, 6, 6, 0, 2, 2, 0,},
     {1, 2, 1, 2, 0, 1, 6, 0, 0, 2, 2, 0,},
     {1, 0, 0, 2, 0, 1, 0, 0, 0, 3, 3, 0,},
     {1, 2, 1, 0, 0, 1, 0, 1, 0, 3, 3, 0,},
     {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0,},
     {0, 3, 4, 3, 0, 1, 0, 1, 0, 0, 0, 0,},
     {0, 0, 0, 0, 2, 2, 2, 1, 1, 1, 1, 1,}},

    /* Level 12 */
    {{7, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,},
     {1, 2, 1, 2, 1, 2, 1, 1, 0, 0, 0, 1,},
     {0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1,},
     {1, 2, 1, 2, 1, 2, 1, 0, 0, 0, 0, 1,},
     {0, 0, 0, 0, 0, 0, 1, 1, 5, 5, 6, 1,},
     {1, 2, 1, 2, 1, 2, 1, 1, 0, 2, 2, 1,},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1,},
     {1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1,}},

    /* Level 13 */
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
     {0, 0, 4, 0, 2, 0, 5, 0, 4, 0, 0, 6,},
     {0, 0, 2, 0, 5, 0, 2, 0, 4, 0, 0, 6,},
     {7, 0, 3, 0, 4, 0, 5, 0, 4, 0, 0, 6,},
     {0, 0, 5, 0, 4, 0, 2, 0, 4, 0, 0, 6,},
     {0, 0, 4, 0, 2, 0, 5, 0, 4, 0, 0, 6,},
     {0, 0, 3, 0, 3, 0, 2, 0, 4, 0, 0, 6,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,}},

    /* Level 14 */
    {{1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1,},
     {1, 1, 0, 2, 0, 0, 4, 0, 0, 2, 0, 1,},
     {1, 6, 0, 0, 5, 1, 0, 1, 1, 0, 0, 1,},
     {1, 1, 1, 0, 0, 3, 5, 3, 0, 0, 1, 1,},
     {1, 1, 0, 0, 1, 1, 0, 1, 5, 0, 0, 6,},
     {1, 1, 0, 2, 0, 0, 4, 0, 0, 0, 7, 1,},
     {1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1,},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,}},

    /* Level 15 */
    {{0, 0, 0, 6, 0, 6, 0, 6, 0, 6, 0, 1,},
     {0, 0, 0, 6, 0, 6, 0, 6, 0, 6, 0, 1,},
     {0, 3, 4, 6, 0, 6, 0, 6, 0, 6, 0, 2,},
     {0, 4, 0, 6, 0, 6, 4, 6, 0, 6, 0, 1,},
     {0, 3, 0, 6, 0, 6, 0, 6, 0, 6, 0, 1,},
     {7, 0, 0, 6, 4, 6, 0, 6, 0, 6, 0, 1,},
     {0, 0, 4, 6, 0, 6, 0, 6, 4, 6, 0, 1,},
     {0, 0, 4, 6, 0, 6, 0, 6, 0, 6, 0, 1,}}
};


/*****************************************************************************
* display_text() formats and outputs text.
******************************************************************************/
static void display_text(char *str, bool waitkey)
{
    int chars_by_line;
    int lines_by_screen;
    int chars_for_line;
    int current_line = 0;
    int char_width, char_height;
    int first_char_index = 0;
    char *ptr_char;
    char *ptr_line;
    int i;
    char line[255];
    int key;
    bool go_on;

    rb->lcd_clear_display();
    
    rb->lcd_getstringsize("a", &char_width, &char_height);

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
#ifdef HAVE_TOUCHSCREEN
                    case BUTTON_TOUCHSCREEN:
#endif
                    case PEGBOX_QUIT:
                    case PEGBOX_LEFT:
                    case PEGBOX_DOWN:
                        go_on = true;
                        break;

                    default:
                       /*if (rb->default_event_handler(key) == SYS_USB_CONNECTED)
                        {
                            usb_detected = true;
                            go_on = true;
                            break;
                        }*/
                        break;
                }
            }
            rb->lcd_clear_display();
        }
    } while (*ptr_char != '\0');
}

/*****************************************************************************
* draw_board() draws the game's current level.
******************************************************************************/
static void draw_board(struct game_context* pb) {
    unsigned int r, c, type;
    pb->num_left = 0;
    char str[5];

    rb->lcd_clear_display();
#ifdef WIDE_LAYOUT
    rb->lcd_bitmap(pegbox_header,LCD_WIDTH-HEADER_WIDTH,0,
                   HEADER_WIDTH,LCD_HEIGHT);
#else
    rb->lcd_bitmap(pegbox_header,(LCD_WIDTH-HEADER_WIDTH)/2,0,
                   HEADER_WIDTH, HEADER_HEIGHT);
#endif /* WIDE_LAYOUT */

#if ((BOARD_HEIGHT + HEADER_HEIGHT + 4) <= LCD_HEIGHT)
    rb->lcd_drawrect(BOARD_X-2,BOARD_Y-2,BOARD_WIDTH+4,BOARD_HEIGHT+4);
#endif /* enough space for a frame? */

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_fillrect(BOARD_X-1,BOARD_Y-1,BOARD_WIDTH+2,BOARD_HEIGHT+2);
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(TEXT_BG);
#endif

    for (r=0 ; r < ROWS ; r++) {
        for (c = 0 ; c < COLS ; c++) {

            type = pb->playboard[r][c];

            switch(type) {
                case SPACE:
                    break;

                default:
                    rb->lcd_bitmap_part(pegbox_pieces, 0, (type-1)*PIECE_HEIGHT,
                                        PIECE_WIDTH, c * PIECE_WIDTH + BOARD_X,
                                        r * PIECE_HEIGHT + BOARD_Y, PIECE_WIDTH,
                                        PIECE_HEIGHT);
                    break;
            }

            if(pb->playboard[r][c] == PLAYER) {
                pb->player_row=r;
                pb->player_col=c;
            }
            else if (type != WALL && type != SPACE && type != HOLE)
                pb->num_left++;
        }
    }

#ifdef WIDE_LAYOUT
    rb->snprintf(str, 3, "%d", pb->level);
    rb->lcd_putsxy(TEXT_X, LEVEL_TEXT_Y, str);
    rb->snprintf(str, 3, "%d", pb->num_left);
    rb->lcd_putsxy(TEXT_X, PEGS_TEXT_Y, str);
#else
    rb->snprintf(str, 3, "%d", pb->level);
    rb->lcd_putsxy(LEVEL_TEXT_X, TEXT_Y, str);
    rb->snprintf(str, 3, "%d", pb->num_left);
    rb->lcd_putsxy(PEGS_TEXT_X, TEXT_Y, str);
#endif /*WIDE_LAYOUT*/

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(BG_COLOR);
    rb->lcd_set_foreground(LCD_WHITE);
#endif
    /* print out the screen */
    rb->lcd_update();
}

/*****************************************************************************
* load_level() loads the player's current level from the array and sets the
* player's position.
******************************************************************************/
static void load_level(struct game_context* pb) {
    int r, c;

    for(r = 0; r < ROWS; r++)
        for(c = 0; c < COLS; c++)
            pb->playboard[r][c] = levels[pb->level-1][r][c];
}

/*****************************************************************************
* new_piece() creates a new piece at a specified location. The player
* navigates through the pieces and selects one.
******************************************************************************/
static void new_piece(struct game_context* pb, unsigned int x_loc, 
                          unsigned int y_loc) {
    int button;
    bool exit = false;

    pb->playboard[x_loc][y_loc] = TRIANGLE;

    while (!exit) {
        draw_board(pb);
        button = rb->button_get(true);
#ifdef HAVE_TOUCHSCREEN
        if(button & BUTTON_TOUCHSCREEN)
        {
            pegbox_raster_btn.two_d_from.y = x_loc;
            pegbox_raster_btn.two_d_from.x = y_loc;

            struct ts_raster_button_result ret =
                  touchscreen_raster_map_button(&pegbox_raster_btn,
                                                rb->button_get_data() >> 16,
                                                rb->button_get_data() & 0xffff,
                                                button);
            if(ret.action == TS_ACTION_TWO_D_MOVEMENT)
            {
                if(ret.to.x > ret.from.x)
                    button = PEGBOX_UP;
                else if(ret.to.x < ret.from.x)
                    button = PEGBOX_DOWN;
                else if(ret.to.y > ret.from.y)
                    button = PEGBOX_LEFT;
                else if(ret.to.y < ret.from.y)
                    button = PEGBOX_RIGHT;
            }
            else if(ret.action == TS_ACTION_CLICK
                    && (unsigned)ret.to.x == y_loc
                    && (unsigned)ret.to.y == x_loc)
                button = PEGBOX_SAVE;
        }
#endif
        switch(button){
            case PEGBOX_LEFT:
            case (PEGBOX_LEFT|BUTTON_REPEAT):
            case PEGBOX_DOWN:
            case (PEGBOX_DOWN|BUTTON_REPEAT):
                if (pb->playboard[x_loc][y_loc] < 5)
                    pb->playboard[x_loc][y_loc]++;
                else
                    pb->playboard[x_loc][y_loc] = TRIANGLE;
                break;
            case PEGBOX_RIGHT:
            case (PEGBOX_RIGHT|BUTTON_REPEAT):
            case PEGBOX_UP:
            case (PEGBOX_UP|BUTTON_REPEAT):
                if (pb->playboard[x_loc][y_loc] > 2)
                    pb->playboard[x_loc][y_loc]--;
                else
                    pb->playboard[x_loc][y_loc] = CIRCLE;
                break;
            case PEGBOX_SAVE:
                exit = true;
                break;
        }
    }
}

/*****************************************************************************
* move_player() moves the player and pieces and updates the board accordingly.
******************************************************************************/
static void move_player(struct game_context* pb, signed int x_dir, 
                            signed int y_dir) {
    unsigned int type1, type2;
    signed int r,c;
    
    r = pb->player_row+y_dir;
    c = pb->player_col+x_dir;
    
    type1 = pb->playboard[r][c];
    type2 = pb->playboard[r+y_dir][c+x_dir];

    if (r == ROWS || c == COLS || r < 0 || c < 0 || type1 == WALL)
        return;
    else if(type1 != SPACE) {
        if (type2 == WALL || r+y_dir == ROWS || c+x_dir == COLS || 
            r+y_dir < 0 || c+x_dir < 0)
            return;
    }


    pb->playboard[pb->player_row][pb->player_col] = SPACE;
    pb->player_row += y_dir;
    pb->player_col += x_dir;

    if (type1 == HOLE) {
        draw_board(pb);
        rb->splash(HZ*2, "You fell down a hole!");
        load_level(pb);
    }
    else if (type1 == SPACE)
        pb->playboard[r][c] = PLAYER;
    else {
        pb->playboard[r][c] = PLAYER;
        if(type1 == type2) {
            if (type1 == TRIANGLE)
                pb->playboard[r+y_dir][c+x_dir] = WALL;
            else if (type1 == CROSS) {
                pb->playboard[r][c] = SPACE;
                new_piece(pb, r+y_dir, c+x_dir);
                pb->playboard[r][c] = PLAYER;
            }
            else
                pb->playboard[r+y_dir][c+x_dir] = SPACE;
        }
        else if (type2 == SPACE)
            pb->playboard[r+y_dir][c+x_dir] = type1;
        else if (type2 == HOLE) {
            if (type1 == SQUARE)
                pb->playboard[r+y_dir][c+x_dir] = SPACE;
        }
        else {
            rb->splash(HZ*2, "Illegal Move!");
            load_level(pb);
        }
    }

    draw_board(pb);
}

/*****************************************************************************
* pegbox_loadgame() loads the saved game and returns load success.
******************************************************************************/
static bool pegbox_loadgame(struct game_context* pb) {
    signed int fd;
    bool loaded = false;

    /* open game file */
    fd = rb->open(SAVE_FILE, O_RDONLY);
    if(fd < 0) return loaded;

    /* read in saved game */
    while(true) {
        if(rb->read(fd, &pb->level, sizeof(pb->level)) <= 0) break;
        if(rb->read(fd, &pb->playboard, sizeof(pb->playboard)) <= 0)
        {
            loaded = true;
            break;
        }
        break;
    }

    rb->close(fd);
    return loaded;
}

/*****************************************************************************
* pegbox_savegame() saves the current game state.
******************************************************************************/
static void pegbox_savegame(struct game_context* pb) {
    unsigned int fd;

    /* write out the game state to the save file */
    fd = rb->open(SAVE_FILE, O_WRONLY|O_CREAT);
    rb->write(fd, &pb->level, sizeof(pb->level));
    rb->write(fd, &pb->playboard, sizeof(pb->playboard));
    rb->close(fd);
}

/*****************************************************************************
* pegbox_loaddata() loads the level and highlevel and returns load success.
******************************************************************************/
static void pegbox_loaddata(struct game_context* pb) {
    signed int fd;

    /* open game file */
    fd = rb->open(DATA_FILE, O_RDONLY);
    if(fd < 0) {
        pb->level = 1;
        pb->highlevel = 1;
        return;
    }

    /* read in saved game */
    while(true) {
        if(rb->read(fd, &pb->level, sizeof(pb->level)) <= 0) break;
        if(rb->read(fd, &pb->highlevel, sizeof(pb->highlevel)) <= 0) break;
        break;
    }

    rb->close(fd);
    return;
}

/*****************************************************************************
* pegbox_savedata() saves the level and highlevel.
******************************************************************************/
static void pegbox_savedata(struct game_context* pb) {
    unsigned int fd;

    /* write out the game state to the save file */
    fd = rb->open(DATA_FILE, O_WRONLY|O_CREAT);
    rb->write(fd, &pb->level, sizeof(pb->level));
    rb->write(fd, &pb->highlevel, sizeof(pb->highlevel));
    rb->close(fd);
}

/*****************************************************************************
* pegbox_callback() is the default event handler callback which is called
*     on usb connect and shutdown.
******************************************************************************/
static void pegbox_callback(void* param) {
    struct game_context* pb = (struct game_context*) param;
    rb->splash(HZ, "Saving data...");
    pegbox_savedata(pb);
}

/*****************************************************************************
* pegbox_menu() is the initial menu at the start of the game.
******************************************************************************/
static unsigned int pegbox_menu(struct game_context* pb) {
    int button;
    char str[30];
    unsigned int startlevel = 1, loc = 0;
    bool breakout = false, can_resume = false;
    
    if (pb->num_left > 0 || pb->save_exist)
        can_resume = true;
        
    while(!breakout){
#if LCD_HEIGHT >= 80
        rb->lcd_clear_display();
        rb->lcd_bitmap(pegbox_menu_top,0,0,LCD_WIDTH, BMPHEIGHT_pegbox_menu_top);

         /* menu bitmaps */
        if (loc == 0) {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, ITEM_HEIGHT, ITEM_WIDTH,
                                MENU_X, MENU_Y, ITEM_WIDTH, ITEM_HEIGHT);
        }
        else {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, 0, ITEM_WIDTH,
                                MENU_X, MENU_Y, ITEM_WIDTH, ITEM_HEIGHT);
        }
        if (can_resume) {
            if (loc == 1) {
                rb->lcd_bitmap_part(pegbox_menu_items, 0, ITEM_HEIGHT*3, ITEM_WIDTH,
                                    MENU_X, MENU_Y+ITEM_HEIGHT, ITEM_WIDTH, ITEM_HEIGHT);
            }
            else {
                rb->lcd_bitmap_part(pegbox_menu_items, 0, ITEM_HEIGHT*2, ITEM_WIDTH,
                                    MENU_X, MENU_Y+ITEM_HEIGHT, ITEM_WIDTH, ITEM_HEIGHT);
            }
        }
        else {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, ITEM_HEIGHT*4, ITEM_WIDTH,
                                MENU_X, MENU_Y+ITEM_HEIGHT, ITEM_WIDTH, ITEM_HEIGHT);
        }

        if (loc==2) {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, ITEM_HEIGHT*6, ITEM_WIDTH,
                                MENU_X, MENU_Y+ITEM_HEIGHT*2, ITEM_WIDTH, ITEM_HEIGHT);
        }
        else {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, ITEM_HEIGHT*5, ITEM_WIDTH,
                                MENU_X, MENU_Y+ITEM_HEIGHT*2, ITEM_WIDTH, ITEM_HEIGHT);
        }

        if (loc==3) {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, ITEM_HEIGHT*8, ITEM_WIDTH,
                                MENU_X, MENU_Y+ITEM_HEIGHT*3, ITEM_WIDTH, ITEM_HEIGHT);
        }
        else {
            rb->lcd_bitmap_part(pegbox_menu_items, 0, ITEM_HEIGHT*7, ITEM_WIDTH,
                                MENU_X, MENU_Y+ITEM_HEIGHT*3, ITEM_WIDTH, ITEM_HEIGHT);
        }
#else
        unsigned int w,h;
        rb->lcd_clear_display();
        rb->lcd_getstringsize("PegBox", &w, &h);
        rb->lcd_putsxy((LCD_WIDTH-w)/2, 0, "PegBox");
        rb->lcd_putsxy((LCD_WIDTH)/4, 16, "New Game");
        rb->lcd_putsxy((LCD_WIDTH)/4, 24, "Resume");
        rb->lcd_putsxy((LCD_WIDTH)/4, 32, "Help");
        rb->lcd_putsxy((LCD_WIDTH)/4, 40, "Quit");

        if(!can_resume)
            rb->lcd_hline((LCD_WIDTH)/4, (LCD_WIDTH)/4+30, 28);

        rb->lcd_putsxy((LCD_WIDTH)/4-8, loc*8+16, "*");


#endif
        rb->snprintf(str, 28, "Start on level %d of %d", startlevel,
                     pb->highlevel);
#if LCD_HEIGHT > 110
        rb->lcd_putsxy(0, MENU_Y+4*ITEM_HEIGHT+8, str);
#elif LCD_HEIGHT > 64
        rb->lcd_putsxy(0, LCD_HEIGHT - 8, str);
#else
        rb->lcd_puts_scroll(0, 7, str);
#endif
        rb->lcd_update();

        /* handle menu button presses */
        button = rb->button_get(true);

#ifdef HAVE_TOUCHSCREEN
        if(button & BUTTON_TOUCHSCREEN)
        {
            unsigned int result = touchscreen_map(&main_menu,
                                               rb->button_get_data() >> 16,
                                               rb->button_get_data() & 0xffff);
            if(result != (unsigned)-1 && button & BUTTON_REL)
            {
                if(result == 4)
                    button = PEGBOX_LVL_UP;
                else
                {
                    if(loc == result)
                        button = PEGBOX_RIGHT;
                    loc = result;
                }
            }
        }
#endif

        switch(button) {
            case PEGBOX_SAVE: /* start playing */
            case PEGBOX_RIGHT:
                if (loc == 0) {
                    breakout = true;
                    pb->level = startlevel;
                    load_level(pb);
                }
                else if (loc == 1 && can_resume) {
                    if(pb->save_exist)
                    {
                        rb->remove(SAVE_FILE);
                        pb->save_exist = false;
                    }
                    breakout = true;
                } 
                else if (loc == 2)
                    display_text("How to Play\nTo beat each level, you must "
                                 "destroy all of the pegs. If two like pegs are "
                                 "pushed into each other they disappear except "
                                 "for triangles which form a solid block and "
                                 "crosses which allow you to choose a "
                                 "replacement block.\n\n"
                                 "Controls\n"
#if LCD_HEIGHT > 64
                                 RESTART_TEXT " to restart level\n"
                                 LVL_UP_TEXT " to go up a level\n"
                                 LVL_DOWN_TEXT " to go down a level\n"
                                 SAVE_TEXT " to select/save\n"
                                 QUIT_TEXT " to quit\n",true);
#else
                                 RESTART_TEXT ": restart\n"
                                 LVL_UP_TEXT ": level up\n"
                                 LVL_DOWN_TEXT " level down\n"
                                 SAVE_TEXT " select/save\n"
                                 QUIT_TEXT " quit\n",true);
#endif
                else if (loc == 3)
                    return PB_QUIT;
                break;

            case PEGBOX_QUIT:  /* quit program */
                return PB_QUIT;
                
            case (PEGBOX_UP|BUTTON_REPEAT):
            case PEGBOX_UP:
                if (loc <= 0)
                    loc = 3;
                else
                    loc--;
                if (!can_resume && loc == 1) {
                    loc = 0;
                }
                break;


            case (PEGBOX_DOWN|BUTTON_REPEAT):
            case PEGBOX_DOWN:
                if (loc >= 3)
                    loc = 0;
                else
                    loc++;
                if (!can_resume && loc == 1) {
                    loc = 2;
                }
                break;

            case (PEGBOX_LVL_UP|BUTTON_REPEAT):
            case PEGBOX_LVL_UP:     /* increase starting level */
                if(startlevel >= pb->highlevel) {
                    startlevel = 1;
                } else {
                    startlevel++;
                }
                break;

/* only for targets with enough buttons */
#ifdef PEGBOX_LVL_DOWN
            case (PEGBOX_LVL_DOWN|BUTTON_REPEAT):
            case PEGBOX_LVL_DOWN:   /* decrease starting level */
                if(startlevel <= 1) {
                    startlevel = pb->highlevel;
                } else {
                    startlevel--;
                }
                break;
#endif
            default:
                if(rb->default_event_handler_ex(button, pegbox_callback,
                   (void*) pb) == SYS_USB_CONNECTED)
                    return PB_USB;
                break;
        }

    }
    draw_board(pb);

    return 0;
}

/*****************************************************************************
* pegbox() is the main game subroutine, it returns the final game status.
******************************************************************************/
static int pegbox(struct game_context* pb) {
    int temp_var;
  
    /********************
    *       menu        *
    ********************/
    temp_var = pegbox_menu(pb);
    if (temp_var == PB_QUIT || temp_var == PB_USB)
        return temp_var;

    while (true) {
        temp_var = rb->button_get(true);
#ifdef HAVE_TOUCHSCREEN
        if(temp_var & BUTTON_TOUCHSCREEN)
        {
            pegbox_raster_btn.two_d_from.y = pb->player_row;
            pegbox_raster_btn.two_d_from.x = pb->player_col;
            
            struct ts_raster_button_result ret =
                  touchscreen_raster_map_button(&pegbox_raster_btn,
                                                rb->button_get_data() >> 16,
                                                rb->button_get_data() & 0xffff,
                                                temp_var);
            if(ret.action == TS_ACTION_TWO_D_MOVEMENT)
                move_player(pb, ret.to.x - ret.from.x, ret.to.y - ret.from.y);
        }
#endif
        switch(temp_var){
            case PEGBOX_LEFT:             /* move cursor left */
            case (PEGBOX_LEFT|BUTTON_REPEAT):
                move_player(pb, -1, 0);
                break;

            case PEGBOX_RIGHT:            /* move cursor right */
            case (PEGBOX_RIGHT|BUTTON_REPEAT):
                move_player(pb, 1, 0);
                break;

            case PEGBOX_DOWN:             /* move cursor down */
            case (PEGBOX_DOWN|BUTTON_REPEAT):
                move_player(pb, 0, 1);
                break;

            case PEGBOX_UP:               /* move cursor up */
            case (PEGBOX_UP|BUTTON_REPEAT):
                move_player(pb, 0, -1);
                break;

            case PEGBOX_SAVE:           /* save and end game */
                rb->splash(HZ, "Saving game...");
                pegbox_savegame(pb);
                /* fall through to PEGBOX_QUIT */

            case PEGBOX_QUIT:
                return PB_END;

            case PEGBOX_RESTART:
                load_level(pb);
                draw_board(pb);
                break;

            case (PEGBOX_LVL_UP|BUTTON_REPEAT):
            case PEGBOX_LVL_UP:
                if(pb->level >= pb->highlevel) {
                    pb->level = 1;
                } else {
                    pb->level++;
                }
                load_level(pb);
                draw_board(pb);
                break;

/* only for targets with enough buttons */
#ifdef PEGBOX_LVL_DOWN
            case (PEGBOX_LVL_DOWN|BUTTON_REPEAT):
            case PEGBOX_LVL_DOWN:
                if(pb->level <= 1) {
                    pb->level = pb->highlevel;
                } else {
                    pb->level--;
                }
                load_level(pb);
                draw_board(pb);
                break;
#endif

        }

        if(pb->num_left == 0) {
            rb->splash(HZ*2, "Nice Pegging!");
            if(pb->level == NUM_LEVELS) {
                draw_board(pb);
                rb->splash(HZ*2, "You Won!");
                break;
            }
            else {
                pb->level++;
                load_level(pb);
                draw_board(pb);
            }

            if(pb->level > pb->highlevel)
                pb->highlevel = pb->level;

        }
    }

    return PLUGIN_OK;
}


/*****************************************************************************
* plugin entry point.
******************************************************************************/
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter) {
    bool exit = false;
    struct game_context pb;

    (void)parameter;
    rb = api;

    rb->lcd_setfont(FONT_SYSFIXED);
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif 
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(BG_COLOR);
#endif

    rb->splash(0, "Loading...");
    pegbox_loaddata(&pb);
    pb.save_exist = pegbox_loadgame(&pb);
    pb.num_left = 0;
   
    rb->lcd_clear_display();


    while(!exit) {
        switch(pegbox(&pb)){
            case PB_END:
                break;

            case PB_USB:
                rb->lcd_setfont(FONT_UI);
                return PLUGIN_USB_CONNECTED;

            case PB_QUIT:
                rb->splash(HZ, "Saving data...");
                pegbox_savedata(&pb);
                exit = true;
                break;

            default:
                break;
        }
    }

    rb->lcd_setfont(FONT_UI);
    return PLUGIN_OK;
}
