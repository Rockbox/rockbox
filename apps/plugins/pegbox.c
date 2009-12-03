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
#include "lib/configfile.h"
#include "lib/display_text.h"
#include "lib/playback_control.h"

#include "pluginbitmaps/pegbox_header.h"
#include "pluginbitmaps/pegbox_pieces.h"

PLUGIN_HEADER

#define CONFIG_FILE_NAME "pegbox.cfg"

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
#define PEGBOX_SELECT   BUTTON_ON
#define PEGBOX_QUIT     BUTTON_OFF
#define PEGBOX_RESTART  BUTTON_F2
#define PEGBOX_LVL_UP   BUTTON_F1
#define PEGBOX_LVL_DOWN BUTTON_F3
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "ON"
#define QUIT_TEXT "OFF"
#define RESTART_TEXT "F2"
#define LVL_UP_TEXT "F1"
#define LVL_DOWN_TEXT "F3"

#elif CONFIG_KEYPAD == ONDIO_PAD
#define PEGBOX_SELECT   BUTTON_OFF
#define PEGBOX_QUIT     (BUTTON_MENU | BUTTON_LEFT)
#define PEGBOX_RESTART  (BUTTON_MENU | BUTTON_RIGHT)
#define PEGBOX_LVL_UP   (BUTTON_MENU | BUTTON_UP)
#define PEGBOX_LVL_DOWN (BUTTON_MENU | BUTTON_DOWN)
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "OFF"
#define QUIT_TEXT "M+LEFT"
#define RESTART_TEXT "M+RIGHT"
#define LVL_UP_TEXT "M+UP"
#define LVL_DOWN_TEXT "M+DOWN"

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_OFF
#define PEGBOX_RESTART  BUTTON_ON
#define PEGBOX_LVL_UP   BUTTON_MODE
#define PEGBOX_LVL_DOWN BUTTON_REC
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "NAVI"
#define QUIT_TEXT "OFF"
#define RESTART_TEXT "ON"
#define LVL_UP_TEXT "AB"
#define LVL_DOWN_TEXT "REC"

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define PEGBOX_SELECT   (BUTTON_SELECT|BUTTON_RIGHT)
#define PEGBOX_QUIT     (BUTTON_SELECT|BUTTON_PLAY)
#define PEGBOX_RESTART  (BUTTON_SELECT|BUTTON_LEFT)
#define PEGBOX_LVL_UP   (BUTTON_SELECT|BUTTON_MENU)
#define PEGBOX_UP       BUTTON_MENU
#define PEGBOX_DOWN     BUTTON_PLAY
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "SELECT+RIGHT"
#define QUIT_TEXT "SELECT+PLAY"
#define RESTART_TEXT "SELECT+LEFT"
#define LVL_UP_TEXT  "SELECT+MENU"
#define LVL_DOWN_TEXT "-"

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  BUTTON_REC
#define PEGBOX_LVL_UP   BUTTON_PLAY
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "SELECT"
#define QUIT_TEXT "OFF"
#define RESTART_TEXT "REC"
#define LVL_UP_TEXT "PLAY"
#define LVL_DOWN_TEXT "-"

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define PEGBOX_SELECT   BUTTON_MODE
#define PEGBOX_QUIT     BUTTON_PLAY
#define PEGBOX_RESTART  (BUTTON_EQ|BUTTON_MODE)
#define PEGBOX_LVL_UP   (BUTTON_EQ|BUTTON_UP)
#define PEGBOX_LVL_DOWN (BUTTON_EQ|BUTTON_DOWN)
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "MODE"
#define QUIT_TEXT "PLAY"
#define RESTART_TEXT "EQ+MODE"
#define LVL_UP_TEXT "EQ+UP"
#define LVL_DOWN_TEXT "EQ+DOWN"

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define PEGBOX_SELECT   BUTTON_PLAY
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  (BUTTON_FF|BUTTON_REPEAT)
#define PEGBOX_LVL_UP   (BUTTON_FF|BUTTON_SCROLL_UP)
#define PEGBOX_LVL_DOWN (BUTTON_FF|BUTTON_SCROLL_DOWN)
#define PEGBOX_UP       BUTTON_SCROLL_UP
#define PEGBOX_DOWN     BUTTON_SCROLL_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "PLAY"
#define QUIT_TEXT "OFF"
#define RESTART_TEXT "LONG FF"
#define LVL_UP_TEXT "FF+SCROLL_UP"
#define LVL_DOWN_TEXT "FF+SCROLL_DOWN"

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  BUTTON_REC
#define PEGBOX_LVL_UP   BUTTON_SCROLL_BACK
#define PEGBOX_LVL_DOWN BUTTON_SCROLL_FWD
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "SELECT"
#define QUIT_TEXT "POWER"
#define RESTART_TEXT "REC"
#define LVL_UP_TEXT "SCROLL BACK"
#define LVL_DOWN_TEXT "SCROLL FWD"

#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
#define PEGBOX_SELECT   BUTTON_SELECT|BUTTON_REL
#define PEGBOX_QUIT     (BUTTON_HOME|BUTTON_REPEAT)
#define PEGBOX_RESTART  (BUTTON_SELECT|BUTTON_LEFT)
#define PEGBOX_LVL_UP   (BUTTON_SELECT|BUTTON_UP)
#define PEGBOX_LVL_DOWN (BUTTON_SELECT|BUTTON_DOWN)
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "SELECT"
#define QUIT_TEXT "HOME"
#define RESTART_TEXT "SELECT & LEFT"
#define LVL_UP_TEXT "SELECT & UP"
#define LVL_DOWN_TEXT "SELECT & DOWN"

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  BUTTON_A
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "SELECT"
#define QUIT_TEXT "POWER"
#define RESTART_TEXT "A"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_BACK
#define PEGBOX_RESTART  BUTTON_MENU
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "SELECT"
#define QUIT_TEXT "BACK"
#define RESTART_TEXT "MENU"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"

#elif CONFIG_KEYPAD == MROBE100_PAD
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  BUTTON_PLAY
#define PEGBOX_LVL_UP   BUTTON_MENU
#define PEGBOX_LVL_DOWN BUTTON_DISPLAY
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "SELECT"
#define QUIT_TEXT "POWER"
#define RESTART_TEXT "PLAY"
#define LVL_UP_TEXT "MENU"
#define LVL_DOWN_TEXT "DISPLAY"

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  BUTTON_REC
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "SELECT"
#define QUIT_TEXT "POWER"
#define RESTART_TEXT "REC"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"

#elif CONFIG_KEYPAD == SANSA_CLIP_PAD
#define PEGBOX_SELECT   BUTTON_SELECT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  BUTTON_HOME
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "SELECT"
#define QUIT_TEXT "POWER"
#define RESTART_TEXT "HOME"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"

#elif CONFIG_KEYPAD == SANSA_M200_PAD
#define PEGBOX_SELECT   (BUTTON_SELECT | BUTTON_REL)
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  (BUTTON_SELECT | BUTTON_UP)
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "SELECT"
#define QUIT_TEXT "POWER"
#define RESTART_TEXT "SELECT+UP"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"


#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define PEGBOX_SELECT   BUTTON_RC_PLAY
#define PEGBOX_QUIT     BUTTON_RC_REC
#define PEGBOX_RESTART  BUTTON_RC_MODE
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_RC_VOL_UP
#define PEGBOX_DOWN     BUTTON_RC_VOL_DOWN
#define PEGBOX_RIGHT    BUTTON_RC_FF
#define PEGBOX_LEFT     BUTTON_RC_REW

#define SELECT_TEXT "REM. PLAY"
#define QUIT_TEXT "REM. REC"
#define RESTART_TEXT "REM. MODE"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"

#elif CONFIG_KEYPAD == COWOND2_PAD
#define PEGBOX_QUIT     BUTTON_POWER

#define QUIT_TEXT "POWER"

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define PEGBOX_SELECT   BUTTON_CUSTOM
#define PEGBOX_QUIT     BUTTON_BACK
#define PEGBOX_RESTART  BUTTON_SELECT
#define PEGBOX_LVL_UP   BUTTON_PLAY
#define PEGBOX_LVL_DOWN BUTTON_MENU
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "CUSTOM"
#define QUIT_TEXT "BACK"
#define RESTART_TEXT "MIDDLE"
#define LVL_UP_TEXT "SELECT"
#define LVL_DOWN_TEXT "MENU"

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define PEGBOX_SELECT   BUTTON_VIEW
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  BUTTON_MENU
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT

#define SELECT_TEXT "VIEW"
#define QUIT_TEXT "POWER"
#define RESTART_TEXT "MENU"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define PEGBOX_SELECT   BUTTON_RIGHT
#define PEGBOX_QUIT     BUTTON_POWER
#define PEGBOX_RESTART  BUTTON_MENU
#define PEGBOX_LVL_UP   BUTTON_VOL_UP
#define PEGBOX_LVL_DOWN BUTTON_VOL_DOWN
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_NEXT
#define PEGBOX_LEFT     BUTTON_PREV

#define SELECT_TEXT "RIGHT"
#define QUIT_TEXT "POWER"
#define RESTART_TEXT "MENU"
#define LVL_UP_TEXT "VOL+"
#define LVL_DOWN_TEXT "VOL-"

#elif CONFIG_KEYPAD == ONDAVX747_PAD || \
CONFIG_KEYPAD == ONDAVX777_PAD || \
CONFIG_KEYPAD == MROBE500_PAD
#define PEGBOX_QUIT     BUTTON_POWER

#define QUIT_TEXT "POWER"

#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define PEGBOX_SAVE     (BUTTON_REC|BUTTON_FFWD)
#define PEGBOX_QUIT     (BUTTON_REC|BUTTON_PLAY)
#define PEGBOX_RESTART  (BUTTON_REC|BUTTON_REW)
#define PEGBOX_LVL_UP   BUTTON_FFWD
#define PEGBOX_LVL_DOWN BUTTON_REW
#define PEGBOX_UP       BUTTON_UP
#define PEGBOX_DOWN     BUTTON_DOWN
#define PEGBOX_RIGHT    BUTTON_RIGHT
#define PEGBOX_LEFT     BUTTON_LEFT
#define PEGBOX_SELECT     BUTTON_PLAY

#define SAVE_TEXT "REC + FFWD"
#define QUIT_TEXT "REC + PLAY"
#define RESTART_TEXT "REC + REW"
#define LVL_UP_TEXT "FFWD"
#define LVL_DOWN_TEXT "REW"
#define SELECT_TEXT "PLAY"

#else
#error Unsupported keymap!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef PEGBOX_QUIT
#define PEGBOX_QUIT     BUTTON_TOPLEFT
#endif
#ifndef PEGBOX_SELECT
#define PEGBOX_SELECT   BUTTON_CENTER
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
#ifndef SELECT_TEXT
#define SELECT_TEXT "CENTER"
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
#elif (LCD_WIDTH >= 128) && (LCD_HEIGHT >= 64)
#define TEXT_X         112
#define LEVEL_TEXT_Y   27
#define PEGS_TEXT_Y    50
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

#if LCD_WIDTH >= 640
#define LEVEL_TEXT_X   118
#define PEGS_TEXT_X    552
#define TEXT_Y         56
#elif LCD_WIDTH >= 320
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
#include "lib/pluginlib_touchscreen.h"
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

/***********************************************************************
* pegbox_draw_board() draws the game's current level.
************************************************************************/
static void pegbox_draw_board(struct game_context* pb)
{
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

            if(type != SPACE) {
                rb->lcd_bitmap_part(pegbox_pieces, 0, (type-1)*PIECE_HEIGHT,
                        STRIDE( SCREEN_MAIN, 
                                BMPWIDTH_pegbox_pieces,BMPHEIGHT_pegbox_pieces),
                        c * PIECE_WIDTH + BOARD_X,
                        r * PIECE_HEIGHT + BOARD_Y, PIECE_WIDTH,
                        PIECE_HEIGHT);
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
* pegbox_load_level() loads the player's current level from the array and sets the
* player's position.
******************************************************************************/
static void pegbox_load_level(struct game_context* pb)
{
    int r, c;

    for(r = 0; r < ROWS; r++)
        for(c = 0; c < COLS; c++)
            pb->playboard[r][c] = levels[pb->level-1][r][c];
}

/*****************************************************************************
* pegbox_new_piece() creates a new piece at a specified location. The player
* navigates through the pieces and selects one.
******************************************************************************/
static void pegbox_new_piece(struct game_context* pb, unsigned int x_loc,
                               unsigned int y_loc)
{
    int button;
    bool exit = false;

    pb->playboard[x_loc][y_loc] = TRIANGLE;

    while (!exit) {
        pegbox_draw_board(pb);
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
                button = PEGBOX_SELECT;
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
            case PEGBOX_SELECT:
                exit=true;
                break;
        }
    }
}

/*****************************************************************************
* pegbox_move_player() moves the player and pieces and updates the board accordingly.
******************************************************************************/
static void pegbox_move_player(struct game_context* pb, signed int x_dir,
                                signed int y_dir)
{
    unsigned int type1, type2;
    signed int r1,c1,r2,c2;

    r1 = pb->player_row+y_dir;
    c1 = pb->player_col+x_dir;
    r2 = pb->player_row+y_dir*2;
    c2 = pb->player_col+x_dir*2;

    type1 = pb->playboard[r1][c1];
    type2 = pb->playboard[r2][c2];

    if (r1 == ROWS || c1 == COLS || r1 < 0 || c1 < 0 || type1 == WALL)
        return;
    else if(type1 != SPACE) {
        if (r2 == ROWS || c2 == COLS || r2 < 0 || c2 < 0 || type2 == WALL)
            return;
    }


    pb->playboard[pb->player_row][pb->player_col] = SPACE;
    pb->player_row += y_dir;
    pb->player_col += x_dir;

    if (type1 == HOLE) {
        pegbox_draw_board(pb);
        rb->splash(HZ*2, "You fell down a hole!");
        pegbox_load_level(pb);
    }
    else if (type1 == SPACE)
        pb->playboard[r1][c1] = PLAYER;
    else {
        pb->playboard[r1][c1] = PLAYER;
        if(type1 == type2) {
            if (type1 == TRIANGLE)
                pb->playboard[r2][c2] = WALL;
            else if (type1 == CROSS) {
                pb->playboard[r1][c1] = SPACE;
                pegbox_new_piece(pb, r2, c2);
                pb->playboard[r1][c1] = PLAYER;
            }
            else
                pb->playboard[r2][c2] = SPACE;
        }
        else if (type2 == SPACE)
            pb->playboard[r2][c2] = type1;
        else if (type2 == HOLE) {
            if (type1 == SQUARE)
                pb->playboard[r2][c2] = SPACE;
        }
        else {
            rb->splash(HZ*2, "Illegal Move!");
            pegbox_load_level(pb);
        }
    }

    pegbox_draw_board(pb);
}

/***********************************************************************
* pegbox_help() display help text
************************************************************************/
static bool pegbox_help(void)
{
    int button;
#define WORDS (sizeof help_text / sizeof (char*))
    static char* help_text[] = {
        "Pegbox", "", "Aim", "",
        "To", "beat", "each", "level,", "you", "must", "destroy", "all", "of",
        "the", "pegs.", "If", "two", "like", "pegs", "are", "pushed", "into",
        "each", "other,", "they", "disappear", "except", "for", "triangles",
        "which", "form", "a", "solid", "block", "and", "crosses", "which",
        "allow", "you", "to", "choose", "a", "replacement", "block.", "", "",
        "Controls", "",
        RESTART_TEXT, "to", "restart", "level", "",
        LVL_UP_TEXT, "to", "go", "up", "a", "level", "",
        LVL_DOWN_TEXT, "to", "go", "down", "a", "level", "",
        SELECT_TEXT, "to", "choose", "peg", "",
        QUIT_TEXT, "to", "quit"
    };
    static struct style_text formation[]={
        { 0, TEXT_CENTER|TEXT_UNDERLINE },
        { 2, C_RED },
        { 46, C_RED },
        { -1, 0 }
    };

    rb->lcd_setfont(FONT_UI);

    if (display_text(WORDS, help_text, formation, NULL))
        return true;
    do {
        button = rb->button_get(true);
        if ( rb->default_event_handler( button ) == SYS_USB_CONNECTED )
            return true;
    } while( ( button == BUTTON_NONE )
            || ( button & (BUTTON_REL|BUTTON_REPEAT) ) );
    rb->lcd_setfont(FONT_SYSFIXED);
    return false;
}

/***********************************************************************
* pegbox_menu() is the game menu
************************************************************************/
static bool _ingame;
static int pegbox_menu_cb(int action, const struct menu_item_ex *this_item)
{
    int i = (intptr_t)this_item;
    if( action == ACTION_REQUEST_MENUITEM )
    {
        if((!_ingame && (i==0 || i==1)) || ( _ingame && i==2 ))
            return ACTION_EXIT_MENUITEM;
    }
    return action;
}

static unsigned int pegbox_menu(struct game_context* pb, bool ingame)
{
    int selected = 0;
    int last_level = pb->level;

    MENUITEM_STRINGLIST (main_menu, "Pegbox Menu", pegbox_menu_cb,
                         "Resume Game", "Restart Level", "Start Game",
                         "Select Level", "Help",
                         "Playback Control", "Quit");
    _ingame = ingame;
    rb->button_clear_queue();

    while (true) {
        switch (rb->do_menu(&main_menu, &selected, NULL, false)) {
            case 0:
                pb->level = last_level;
                pegbox_draw_board(pb);
                return 0;
            case 1:
            case 2:
                pegbox_load_level(pb);
                pegbox_draw_board(pb);
                return 0;
            case 3:
                if(rb->set_int("Select Level", "", UNIT_INT, &pb->level,
                               NULL, 1, 1, pb->highlevel, NULL))
                    return 1;
                break;
            case 4:
                if (pegbox_help())
                    return 1;
                break;
            case 5:
                if (playback_control(NULL))
                    return 1;
                break;
            case 6:
                return 1;
            case MENU_ATTACHED_USB:
                return 1;
            default:
                break;
        }
    }
}

/***********************************************************************
* pegbox_main() is the main game subroutine
************************************************************************/
static int pegbox_main(struct game_context* pb)
{
    int button;

    if (pegbox_menu(pb, false)==1) {
        return 1;
    }

    while (true) {
        button = rb->button_get(true);
#ifdef HAVE_TOUCHSCREEN
        if(button & BUTTON_TOUCHSCREEN)
        {
            pegbox_raster_btn.two_d_from.y = pb->player_row;
            pegbox_raster_btn.two_d_from.x = pb->player_col;

            struct ts_raster_button_result ret =
                  touchscreen_raster_map_button(&pegbox_raster_btn,
                                                rb->button_get_data() >> 16,
                                                rb->button_get_data() & 0xffff,
                                                button);
            if(ret.action == TS_ACTION_TWO_D_MOVEMENT)
                pegbox_move_player(pb, ret.to.x - ret.from.x, ret.to.y - ret.from.y);
        }
#endif
        switch(button){
            case PEGBOX_LEFT:             /* move cursor left */
            case (PEGBOX_LEFT|BUTTON_REPEAT):
                pegbox_move_player(pb, -1, 0);
                break;

            case PEGBOX_RIGHT:            /* move cursor right */
            case (PEGBOX_RIGHT|BUTTON_REPEAT):
                pegbox_move_player(pb, 1, 0);
                break;

            case PEGBOX_DOWN:             /* move cursor down */
            case (PEGBOX_DOWN|BUTTON_REPEAT):
                pegbox_move_player(pb, 0, 1);
                break;

            case PEGBOX_UP:               /* move cursor up */
            case (PEGBOX_UP|BUTTON_REPEAT):
                pegbox_move_player(pb, 0, -1);
                break;

            case PEGBOX_QUIT:
                if (pegbox_menu(pb, true)==1) {
                    return 1;
                }
                break;

#ifdef PEGBOX_RESTART
            case PEGBOX_RESTART:
                pegbox_load_level(pb);
                pegbox_draw_board(pb);
                break;
#endif

#ifdef PEGBOX_LVL_UP
            case (PEGBOX_LVL_UP|BUTTON_REPEAT):
            case PEGBOX_LVL_UP:
                if (pb->level >= pb->highlevel) {
                    pb->level = 1;
                } else {
                    pb->level++;
                }
                pegbox_load_level(pb);
                pegbox_draw_board(pb);
                break;
#endif

#ifdef PEGBOX_LVL_DOWN
            case (PEGBOX_LVL_DOWN|BUTTON_REPEAT):
            case PEGBOX_LVL_DOWN:
                if(pb->level <= 1) {
                    pb->level = pb->highlevel;
                } else {
                    pb->level--;
                }
                pegbox_load_level(pb);
                pegbox_draw_board(pb);
                break;
#endif
        }

        if (pb->num_left == 0) {
            rb->splash(HZ*2, "Nice Pegging!");
            if (pb->level == NUM_LEVELS) {
                pegbox_draw_board(pb);
                rb->splash(HZ*2, "Congratulations!");
                rb->splash(HZ*2, "You have finished the game!");
                if (pegbox_menu(pb,false)==1) {
                    return 1;
                }
            }
            else {
                pb->level++;
                pegbox_load_level(pb);
                pegbox_draw_board(pb);
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
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
#ifdef HAVE_LCD_BITMAP
    rb->lcd_setfont(FONT_SYSFIXED);
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(BG_COLOR);
#endif
    rb->lcd_clear_display();

    struct game_context pb;
    pb.level=1;
    pb.highlevel=1;
    struct configdata config[] = {
        {TYPE_INT, 1, NUM_LEVELS, { .int_p = &(pb.level) }, "level", NULL},
        {TYPE_INT, 1, NUM_LEVELS, { .int_p = &(pb.highlevel) }, "highlevel", NULL},
    };
    configfile_load(CONFIG_FILE_NAME,config,2,0);
    pegbox_main(&pb);
    configfile_save(CONFIG_FILE_NAME,config,2,0);
    rb->lcd_setfont(FONT_UI);
#endif /* HAVE_LCD_BITMAP */

    return PLUGIN_OK;
}
