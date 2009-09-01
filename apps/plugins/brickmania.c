/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005, 2006 Ben Basha (Paprica)
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
#include "lib/helper.h"
#include "lib/highscore.h"
#include "lib/playback_control.h"

PLUGIN_HEADER

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN
#define RC_QUIT BUTTON_RC_STOP

#elif CONFIG_KEYPAD == ONDIO_PAD
#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_MENU
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == RECORDER_PAD
#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_PLAY
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define QUIT BUTTON_MENU
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_SCROLL_BACK
#define DOWN BUTTON_SCROLL_FWD
#define SCROLL_FWD(x) ((x) & BUTTON_SCROLL_FWD)
#define SCROLL_BACK(x) ((x) & BUTTON_SCROLL_BACK)

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_PLAY
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define QUIT    BUTTON_POWER
#define LEFT    BUTTON_LEFT
#define RIGHT   BUTTON_RIGHT
#define SELECT  BUTTON_SELECT
#define UP      BUTTON_UP
#define DOWN    BUTTON_DOWN
#define SCROLL_FWD(x) ((x) & BUTTON_SCROLL_FWD)
#define SCROLL_BACK(x) ((x) & BUTTON_SCROLL_BACK)


#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define QUIT   (BUTTON_HOME|BUTTON_REPEAT)
#define LEFT   BUTTON_LEFT
#define RIGHT  BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP     BUTTON_UP
#define DOWN   BUTTON_DOWN

#define SCROLL_FWD(x) ((x) & BUTTON_SCROLL_FWD)
#define SCROLL_BACK(x) ((x) & BUTTON_SCROLL_BACK)


#elif CONFIG_KEYPAD == SANSA_C200_PAD || \
CONFIG_KEYPAD == SANSA_CLIP_PAD || \
CONFIG_KEYPAD == SANSA_M200_PAD
#define QUIT     BUTTON_POWER
#define LEFT     BUTTON_LEFT
#define RIGHT    BUTTON_RIGHT
#define ALTLEFT  BUTTON_VOL_DOWN
#define ALTRIGHT BUTTON_VOL_UP
#define SELECT   BUTTON_SELECT
#define UP       BUTTON_UP
#define DOWN     BUTTON_DOWN

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_PLAY
#define UP BUTTON_SCROLL_UP
#define DOWN BUTTON_SCROLL_DOWN

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define QUIT BUTTON_BACK
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define QUIT BUTTON_RC_REC
#define LEFT BUTTON_RC_REW
#define RIGHT BUTTON_RC_FF
#define SELECT BUTTON_RC_PLAY
#define UP BUTTON_RC_VOL_UP
#define DOWN BUTTON_RC_VOL_DOWN
#define RC_QUIT BUTTON_REC

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define QUIT BUTTON_BACK
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == COWOND2_PAD
#define QUIT    BUTTON_POWER

#elif CONFIG_KEYPAD == ONDAVX747_PAD
#define QUIT    BUTTON_POWER
#define LEFT    BUTTON_VOL_DOWN
#define RIGHT   BUTTON_VOL_UP
#define SELECT  BUTTON_MENU
#elif CONFIG_KEYPAD == ONDAVX777_PAD
#define QUIT    BUTTON_POWER

#elif CONFIG_KEYPAD == MROBE500_PAD
#define QUIT    BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define QUIT   BUTTON_FFWD
#define SELECT BUTTON_PLAY
#define LEFT   BUTTON_LEFT
#define RIGHT  BUTTON_RIGHT
#define UP     BUTTON_UP
#define DOWN   BUTTON_DOWN


#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef LEFT
#define LEFT    BUTTON_MIDLEFT
#endif
#ifndef RIGHT
#define RIGHT   BUTTON_MIDRIGHT
#endif
#ifndef SELECT
#define SELECT  BUTTON_CENTER
#endif
#ifndef UP
#define UP      BUTTON_TOPMIDDLE
#endif
#ifndef DOWN
#define DOWN    BUTTON_BOTTOMMIDDLE
#endif
#endif

#ifndef SCROLL_FWD /* targets without scroll wheel*/
#define SCROLL_FWD(x) (0)
#define SCROLL_BACK(x) (0)
#endif

#include "pluginbitmaps/brickmania_pads.h"
#include "pluginbitmaps/brickmania_short_pads.h"
#include "pluginbitmaps/brickmania_long_pads.h"
#include "pluginbitmaps/brickmania_bricks.h"
#include "pluginbitmaps/brickmania_powerups.h"
#include "pluginbitmaps/brickmania_ball.h"
#include "pluginbitmaps/brickmania_gameover.h"

#define PAD_WIDTH        BMPWIDTH_brickmania_pads
#define PAD_HEIGHT       (BMPHEIGHT_brickmania_pads/3)
#define SHORT_PAD_WIDTH  BMPWIDTH_brickmania_short_pads
#define LONG_PAD_WIDTH   BMPWIDTH_brickmania_long_pads
#define BRICK_HEIGHT     (BMPHEIGHT_brickmania_bricks/7)
#define BRICK_WIDTH      BMPWIDTH_brickmania_bricks
#define LEFTMARGIN       ((LCD_WIDTH-10*BRICK_WIDTH)/2)
#define POWERUP_HEIGHT   (BMPHEIGHT_brickmania_powerups/9)
#define POWERUP_WIDTH    BMPWIDTH_brickmania_powerups
#define BALL             BMPHEIGHT_brickmania_ball
#define HALFBALL         ((BALL+1)/2)
#define GAMEOVER_WIDTH   BMPWIDTH_brickmania_gameover
#define GAMEOVER_HEIGHT  BMPHEIGHT_brickmania_gameover

#ifdef HAVE_LCD_COLOR /* currently no transparency for non-colour */
#include "pluginbitmaps/brickmania_break.h"
#endif

/* The time (in ms) for one iteration through the game loop - decrease this
   to speed up the game - note that current_tick is (currently) only accurate
   to 10ms.
*/
#define CYCLETIME           50

#define TOPMARGIN           (BRICK_HEIGHT * 2)

#if LCD_WIDTH<=LCD_HEIGHT
/* Maintain a 4/3 ratio (Width/Height) */
#define GAMESCREEN_HEIGHT   (LCD_WIDTH * 3 / 4)
#else
#define GAMESCREEN_HEIGHT   LCD_HEIGHT
#endif

#define STRINGPOS_FINISH    (GAMESCREEN_HEIGHT - (GAMESCREEN_HEIGHT / 6))
#define STRINGPOS_CONGRATS  (STRINGPOS_FINISH - 20)
#define STRINGPOS_NAVI      (STRINGPOS_FINISH - 10)
#define STRINGPOS_FLIP      (STRINGPOS_FINISH - 10)

/* Brickmania was originally designed for the H300, other targets should scale
 *  up/down as needed based on the screen height.
 */
#define SPEED_SCALE GAMESCREEN_HEIGHT/176

/* These are all used as ball speeds depending on where the ball hit the
 *  paddle.
 */
#define SPEED_1Q_X  ( 6 * SPEED_SCALE)
#define SPEED_1Q_Y  (-2 * SPEED_SCALE)
#define SPEED_2Q_X  ( 4 * SPEED_SCALE)
#define SPEED_2Q_Y  (-3 * SPEED_SCALE)
#define SPEED_3Q_X  ( 3 * SPEED_SCALE)
#define SPEED_3Q_Y  (-4 * SPEED_SCALE)
#define SPEED_4Q_X  ( 2 * SPEED_SCALE)
#define SPEED_4Q_Y  (-4 * SPEED_SCALE)

/* This is used to determine the speed of the paddle */
#define SPEED_PAD   ( 8 * SPEED_SCALE)

/* This defines the speed that the powerups drop */
#define SPEED_POWER ( 2 * SPEED_SCALE)

/* This defines the speed that the shot moves */
#define SPEED_FIRE  ( 4 * SPEED_SCALE)

/*calculate paddle y-position */
#define PAD_POS_Y           (GAMESCREEN_HEIGHT - PAD_HEIGHT - 1)

int levels_num = 29;

static unsigned char levels[29][8][10] = {
    { /* level1 */
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x2,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x2},
        {0x0,0x2,0x1,0x0,0x0,0x0,0x0,0x1,0x2,0x0},
        {0x0,0x0,0x2,0x1,0x0,0x0,0x1,0x2,0x0,0x0},
        {0x0,0x0,0x0,0x2,0x1,0x1,0x2,0x0,0x0,0x0},
        {0x7,0x0,0x0,0x7,0x2,0x2,0x7,0x0,0x0,0x7},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    },
    { /* level2 */
        {0x0,0x0,0x7,0x7,0x1,0x1,0x7,0x7,0x0,0x0},
        {0x0,0x1,0x0,0x0,0x1,0x1,0x0,0x0,0x1,0x0},
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x1,0x1,0x1,0x1,0x0,0x0,0x1,0x1,0x1,0x1},
        {0x1,0x1,0x2,0x1,0x0,0x0,0x1,0x2,0x1,0x1},
        {0x1,0x2,0x0,0x2,0x2,0x2,0x2,0x0,0x2,0x1},
        {0x0,0x1,0x2,0x0,0x0,0x0,0x0,0x2,0x1,0x0},
        {0x0,0x0,0x1,0x2,0x2,0x2,0x2,0x1,0x0,0x0}
    },
    { /* level3 */
        {0x3,0x3,0x3,0x3,0x0,0x0,0x2,0x2,0x2,0x2},
        {0x3,0x23,0x23,0x3,0x0,0x0,0x2,0x22,0x22,0x2},
        {0x3,0x3,0x3,0x3,0x0,0x0,0x2,0x2,0x2,0x2},
        {0x0,0x0,0x0,0x0,0x37,0x37,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x37,0x37,0x0,0x0,0x0,0x0},
        {0x5,0x5,0x5,0x5,0x0,0x0,0x6,0x6,0x6,0x6},
        {0x5,0x25,0x25,0x5,0x0,0x0,0x6,0x26,0x26,0x6},
        {0x5,0x5,0x5,0x5,0x0,0x0,0x6,0x6,0x6,0x6}
    },
    { /* level4 */
        {0x0,0x0,0x0,0x27,0x27,0x27,0x27,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x27,0x7,0x7,0x27,0x0,0x0,0x0},
        {0x22,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x22},
        {0x22,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x22},
        {0x26,0x6,0x0,0x2,0x2,0x2,0x2,0x0,0x6,0x26},
        {0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0},
        {0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0},
        {0x1,0x1,0x1,0x1,0x0,0x0,0x1,0x1,0x1,0x1}
    },
    { /* level5 */
        {0x1,0x0,0x2,0x2,0x0,0x3,0x3,0x0,0x4,0x4},
        {0x0,0x2,0x2,0x0,0x3,0x3,0x0,0x4,0x4,0x0},
        {0x2,0x2,0x0,0x3,0x3,0x0,0x4,0x4,0x0,0x5},
        {0x2,0x0,0x3,0x3,0x0,0x4,0x4,0x0,0x5,0x5},
        {0x0,0x33,0x3,0x0,0x4,0x4,0x0,0x5,0x5,0x0},
        {0x3,0x33,0x0,0x4,0x4,0x0,0x5,0x5,0x0,0x36},
        {0x3,0x0,0x4,0x4,0x0,0x5,0x5,0x0,0x6,0x36},
        {0x0,0x24,0x24,0x0,0x25,0x25,0x0,0x26,0x26,0x0}
    },
    { /* level6 */
        {0x0,0x1,0x3,0x7,0x7,0x7,0x7,0x3,0x1,0x0},
        {0x3,0x1,0x3,0x7,0x0,0x0,0x7,0x3,0x1,0x3},
        {0x3,0x1,0x3,0x7,0x7,0x7,0x7,0x3,0x1,0x3},
        {0x0,0x0,0x0,0x2,0x2,0x2,0x2,0x0,0x0,0x0},
        {0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5},
        {0x5,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x5},
        {0x0,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    },
    { /* level7 */
        {0x0,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x6,0x0,0x0,0x2,0x2,0x2,0x2,0x0,0x0,0x6},
        {0x6,0x0,0x0,0x2,0x2,0x2,0x2,0x0,0x0,0x6},
        {0x6,0x6,0x6,0x0,0x0,0x0,0x0,0x6,0x6,0x6},
        {0x0,0x0,0x0,0x1,0x1,0x1,0x1,0x0,0x0,0x0},
        {0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    },
    { /* level8 */
        {0x0,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x0},
        {0x0,0x0,0x0,0x4,0x0,0x0,0x4,0x0,0x0,0x0},
        {0x6,0x6,0x0,0x2,0x32,0x32,0x2,0x0,0x6,0x6},
        {0x0,0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x0,0x0},
        {0x0,0x6,0x6,0x0,0x0,0x0,0x0,0x6,0x6,0x0},
        {0x0,0x0,0x0,0x5,0x25,0x25,0x5,0x0,0x0,0x0},
        {0x0,0x5,0x5,0x25,0x5,0x5,0x25,0x5,0x5,0x0},
        {0x5,0x5,0x25,0x5,0x5,0x5,0x5,0x25,0x5,0x5}
    },
    { /* level9 */
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2},
        {0x2,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x3,0x2},
        {0x2,0x0,0x3,0x0,0x1,0x1,0x0,0x3,0x0,0x2},
        {0x2,0x0,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x2},
        {0x2,0x0,0x1,0x0,0x3,0x3,0x0,0x1,0x0,0x2},
        {0x2,0x0,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x2},
        {0x2,0x2,0x0,0x0,0x1,0x1,0x0,0x0,0x2,0x2},
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2}
    },
    { /* level10 */
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x5,0x0,0x5,0x0,0x5,0x0,0x5,0x0,0x5},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x1,0x1,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x4,0x1,0x1,0x4,0x0,0x0,0x0},
        {0x0,0x0,0x3,0x4,0x1,0x1,0x4,0x3,0x0,0x0},
        {0x0,0x2,0x3,0x4,0x1,0x1,0x4,0x3,0x2,0x0},
        {0x1,0x2,0x3,0x4,0x1,0x1,0x4,0x3,0x2,0x1}
    },
    { /* level11 */
        {0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3},
        {0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x2},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2},
        {0x2,0x0,0x0,0x0,0x7,0x7,0x0,0x0,0x0,0x2},
        {0x2,0x0,0x0,0x7,0x7,0x7,0x7,0x0,0x0,0x2},
        {0x0,0x0,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x0},
        {0x0,0x2,0x0,0x1,0x0,0x0,0x1,0x0,0x2,0x0},
        {0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5}
    },
    { /* level 12 */
        {0x2,0x1,0x3,0x1,0x0,0x0,0x1,0x3,0x1,0x2},
        {0x1,0x1,0x1,0x1,0x0,0x0,0x1,0x1,0x1,0x1},
        {0x1,0x1,0x1,0x0,0x1,0x1,0x0,0x1,0x1,0x1},
        {0x0,0x1,0x0,0x1,0x6,0x6,0x1,0x0,0x1,0x0},
        {0x0,0x0,0x1,0x1,0x6,0x6,0x1,0x1,0x0,0x0},
        {0x1,0x1,0x1,0x7,0x0,0x0,0x7,0x1,0x1,0x1},
        {0x1,0x1,0x7,0x1,0x0,0x0,0x1,0x7,0x1,0x1},
        {0x2,0x2,0x0,0x2,0x2,0x2,0x2,0x0,0x2,0x2}
    },
    {/* levell13 */
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2},
        {0x2,0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x0,0x2},
        {0x2,0x0,0x2,0x3,0x3,0x3,0x3,0x3,0x0,0x2},
        {0x2,0x0,0x2,0x4,0x4,0x4,0x4,0x4,0x0,0x2},
        {0x2,0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2}
    },
    {/* level14 */
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x4,0x4,0x4,0x4,0x2,0x2,0x4,0x4,0x4,0x4},
        {0x4,0x0,0x0,0x0,0x2,0x2,0x0,0x0,0x0,0x4},
        {0x4,0x0,0x0,0x2,0x3,0x3,0x2,0x0,0x0,0x4},
        {0x4,0x0,0x2,0x23,0x3,0x3,0x23,0x2,0x0,0x4},
        {0x4,0x0,0x2,0x22,0x2,0x2,0x22,0x2,0x0,0x4},
        {0x4,0x0,0x6,0x21,0x5,0x5,0x21,0x6,0x0,0x4},
        {0x4,0x6,0x1,0x1,0x5,0x5,0x1,0x1,0x6,0x4}
    },
    {/* level 15 */
        {0x4,0x4,0x4,0x4,0x4,0x3,0x3,0x3,0x3,0x3},
        {0x2,0x2,0x1,0x1,0x1,0x1,0x1,0x5,0x0,0x0},
        {0x2,0x2,0x1,0x1,0x1,0x0,0x1,0x6,0x0,0x0},
        {0x2,0x1,0x1,0x2,0x1,0x1,0x1,0x5,0x0,0x0},
        {0x2,0x1,0x2,0x2,0x2,0x1,0x1,0x6,0x0,0x0},
        {0x2,0x1,0x2,0x2,0x2,0x1,0x3,0x5,0x3,0x0},
        {0x2,0x1,0x1,0x2,0x1,0x1,0x1,0x6,0x0,0x0},
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2}
    },
    {/* level 16 (Rockbox) by ts-x */
        {0x2,0x2,0x3,0x3,0x3,0x4,0x4,0x5,0x0,0x5},
        {0x2,0x0,0x3,0x0,0x3,0x4,0x0,0x5,0x5,0x0},
        {0x2,0x0,0x3,0x3,0x3,0x4,0x4,0x5,0x0,0x5},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x6,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x7,0x7,0x7,0x1,0x1,0x1,0x2,0x0,0x2,0x0},
        {0x7,0x0,0x7,0x1,0x0,0x1,0x0,0x2,0x0,0x0},
        {0x7,0x7,0x7,0x1,0x1,0x1,0x2,0x0,0x2,0x0}
    },
    {/* level 17 (Alien) by ts-x */
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2},
        {0x1,0x0,0x0,0x0,0x1,0x1,0x0,0x0,0x0,0x1},
        {0x2,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x2},
        {0x1,0x0,0x1,0x2,0x2,0x2,0x2,0x1,0x0,0x1},
        {0x2,0x0,0x0,0x1,0x2,0x2,0x1,0x0,0x0,0x2},
        {0x2,0x1,0x0,0x0,0x1,0x1,0x0,0x0,0x1,0x2},
        {0x2,0x2,0x1,0x0,0x1,0x1,0x0,0x1,0x2,0x2}
    },
    {/* level 18 (Tetris) by ts-x */
        {0x0,0x2,0x0,0x0,0x3,0x4,0x0,0x2,0x2,0x0},
        {0x0,0x2,0x7,0x0,0x3,0x4,0x0,0x2,0x2,0x0},
        {0x2,0x2,0x7,0x0,0x3,0x4,0x0,0x6,0x2,0x2},
        {0x2,0x2,0x7,0x7,0x3,0x4,0x0,0x6,0x2,0x2},
        {0x2,0x1,0x7,0x7,0x3,0x4,0x4,0x6,0x5,0x5},
        {0x2,0x1,0x0,0x7,0x3,0x4,0x4,0x6,0x5,0x5},
        {0x1,0x1,0x1,0x7,0x3,0x0,0x6,0x6,0x5,0x5},
        {0x1,0x1,0x1,0x0,0x3,0x0,0x6,0x6,0x5,0x5}
    },
    { /* level 19 (Stalactites) by ts-x */
        {0x5,0x2,0x6,0x3,0x4,0x7,0x5,0x3,0x1,0x2},
        {0x5,0x2,0x6,0x3,0x4,0x7,0x5,0x3,0x1,0x2},
        {0x5,0x0,0x6,0x3,0x4,0x7,0x5,0x0,0x1,0x2},
        {0x5,0x2,0x6,0x3,0x4,0x0,0x5,0x3,0x1,0x2},
        {0x5,0x0,0x6,0x0,0x4,0x7,0x5,0x0,0x1,0x0},
        {0x5,0x0,0x0,0x3,0x4,0x0,0x0,0x0,0x1,0x2},
        {0x0,0x0,0x6,0x0,0x0,0x0,0x5,0x0,0x0,0x0},
        {0x5,0x0,0x0,0x0,0x4,0x0,0x0,0x0,0x1,0x0}
    },
    { /* level 20 (Maze) by ts-x */
        {0x1,0x1,0x21,0x1,0x1,0x1,0x1,0x1,0x1,0x21},
        {0x1,0x0,0x0,0x3,0x0,0x0,0x3,0x1,0x31,0x1},
        {0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x31,0x0,0x1},
        {0x21,0x0,0x21,0x3,0x0,0x3,0x0,0x3,0x0,0x2},
        {0x1,0x0,0x1,0x21,0x0,0x12,0x0,0x0,0x0,0x0},
        {0x31,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x3,0x0},
        {0x1,0x0,0x1,0x0,0x1,0x1,0x31,0x1,0x1,0x2},
        {0x22,0x0,0x2,0x1,0x1,0x1,0x1,0x1,0x1,0x21}
    },
    { /* level 21 (Dentist) by ts-x */
        {0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x0},
        {0x2,0x2,0x0,0x6,0x0,0x6,0x0,0x6,0x2,0x2},
        {0x2,0x6,0x0,0x6,0x0,0x6,0x0,0x6,0x0,0x2},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x6,0x0,0x2},
        {0x2,0x0,0x6,0x0,0x6,0x0,0x0,0x0,0x0,0x2},
        {0x2,0x0,0x6,0x0,0x6,0x0,0x6,0x0,0x6,0x2},
        {0x2,0x2,0x6,0x0,0x6,0x0,0x6,0x0,0x2,0x2},
        {0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x0}
    },
    { /* level 22 (Spider) by ts-x */
        {0x31,0x3,0x1,0x1,0x0,0x0,0x1,0x1,0x3,0x31},
        {0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0},
        {0x33,0x1,0x1,0x36,0x1,0x1,0x36,0x1,0x1,0x33},
        {0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0},
        {0x0,0x0,0x1,0x1,0x0,0x0,0x1,0x1,0x0,0x0},
        {0x21,0x3,0x1,0x21,0x2,0x2,0x21,0x1,0x3,0x21},
        {0x0,0x0,0x0,0x1,0x21,0x1,0x1,0x0,0x0,0x0},
        {0x3,0x1,0x3,0x1,0x0,0x0,0x1,0x3,0x1,0x3}
    },
    { /* level 23 (Pool) by ts-x */
        {0x0,0x7,0x7,0x7,0x0,0x7,0x7,0x7,0x7,0x0},
        {0x0,0x0,0x5,0x0,0x2,0x0,0x0,0x0,0x2,0x0},
        {0x7,0x3,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0x7},
        {0x7,0x0,0x0,0x0,0x0,0x0,0x0,0x5,0x0,0x7},
        {0x7,0x0,0x4,0x0,0x0,0x3,0x0,0x0,0x0,0x7},
        {0x7,0x0,0x0,0x6,0x0,0x0,0x0,0x0,0x4,0x7},
        {0x0,0x0,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x7,0x7,0x7,0x7,0x0,0x7,0x7,0x7,0x0}
    },
    { /* level 24 (Vorbis Fish) by ts-x */
        {0x0,0x0,0x4,0x4,0x5,0x5,0x5,0x0,0x0,0x5},
        {0x0,0x4,0x6,0x4,0x4,0x5,0x5,0x5,0x0,0x5},
        {0x5,0x6,0x0,0x6,0x4,0x4,0x4,0x5,0x5,0x5},
        {0x5,0x6,0x0,0x6,0x4,0x4,0x4,0x4,0x5,0x5},
        {0x0,0x5,0x6,0x4,0x4,0x5,0x5,0x4,0x5,0x0},
        {0x5,0x5,0x4,0x4,0x5,0x5,0x5,0x4,0x5,0x5},
        {0x5,0x4,0x4,0x4,0x5,0x5,0x4,0x4,0x5,0x5},
        {0x0,0x0,0x4,0x4,0x4,0x4,0x4,0x5,0x0,0x5}
    },
    {/* level 25 (Rainbow) by ts-x */
        {0x0,0x4,0x1,0x0,0x0,0x0,0x0,0x1,0x4,0x0},
        {0x24,0x1,0x3,0x1,0x0,0x0,0x21,0x3,0x1,0x24},
        {0x1,0x23,0x5,0x3,0x1,0x21,0x3,0x5,0x3,0x21},
        {0x3,0x5,0x6,0x5,0x3,0x3,0x5,0x6,0x5,0x3},
        {0x5,0x6,0x7,0x6,0x5,0x5,0x6,0x7,0x6,0x5},
        {0x6,0x7,0x2,0x27,0x6,0x6,0x27,0x2,0x7,0x6},
        {0x7,0x2,0x0,0x2,0x27,0x27,0x2,0x0,0x2,0x7},
        {0x32,0x0,0x0,0x0,0x2,0x2,0x0,0x0,0x0,0x32}
    },
    { /* level 26 (Bowtie) by ts-x */
        {0x5,0x1,0x5,0x1,0x0,0x0,0x1,0x5,0x1,0x5},
        {0x1,0x0,0x0,0x1,0x5,0x5,0x1,0x0,0x0,0x1},
        {0x5,0x0,0x6,0x0,0x1,0x1,0x0,0x6,0x0,0x5},
        {0x1,0x0,0x6,0x6,0x0,0x0,0x6,0x6,0x0,0x1},
        {0x1,0x0,0x6,0x6,0x0,0x0,0x6,0x6,0x0,0x1},
        {0x5,0x0,0x6,0x0,0x1,0x1,0x0,0x6,0x0,0x5},
        {0x1,0x0,0x0,0x1,0x5,0x5,0x1,0x0,0x0,0x1},
        {0x5,0x1,0x5,0x1,0x0,0x0,0x1,0x5,0x1,0x5}
    },
    { /* level 27 (Frog) by ts-x */
        {0x0,0x5,0x25,0x0,0x0,0x0,0x0,0x25,0x5,0x0},
        {0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5},
        {0x25,0x0,0x0,0x5,0x6,0x6,0x5,0x0,0x0,0x25},
        {0x5,0x0,0x3,0x0,0x6,0x6,0x0,0x3,0x0,0x5},
        {0x5,0x0,0x31,0x0,0x6,0x6,0x0,0x31,0x0,0x5},
        {0x5,0x0,0x0,0x5,0x6,0x6,0x5,0x0,0x0,0x5},
        {0x5,0x5,0x5,0x35,0x0,0x0,0x35,0x5,0x5,0x5},
        {0x0,0x25,0x5,0x0,0x4,0x4,0x0,0x5,0x25,0x0}
    },
    { /* level 28 (DigDug) by ts-x */
        {0x35,0x5,0x5,0x25,0x0,0x25,0x25,0x5,0x5,0x35},
        {0x6,0x0,0x0,0x6,0x0,0x6,0x6,0x0,0x0,0x6},
        {0x7,0x0,0x37,0x37,0x0,0x37,0x37,0x7,0x0,0x7},
        {0x7,0x0,0x7,0x0,0x0,0x0,0x7,0x7,0x7,0x7},
        {0x4,0x4,0x4,0x24,0x0,0x24,0x4,0x0,0x0,0x4},
        {0x4,0x4,0x0,0x0,0x0,0x4,0x4,0x0,0x4,0x4},
        {0x24,0x24,0x4,0x4,0x4,0x4,0x0,0x0,0x24,0x4},
        {0x1,0x1,0x1,0x1,0x1,0x1,0x21,0x21,0x1,0x1}
    },
    { /* TheEnd */
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x22,0x22,0x26,0x0,0x0,0x26,0x24,0x24,0x0,0x0},
        {0x22,0x0,0x26,0x26,0x0,0x26,0x24,0x0,0x24,0x0},
        {0x22,0x22,0x26,0x26,0x0,0x26,0x24,0x0,0x24,0x0},
        {0x22,0x22,0x26,0x0,0x26,0x26,0x24,0x0,0x24,0x0},
        {0x22,0x0,0x26,0x0,0x26,0x26,0x24,0x0,0x24,0x0},
        {0x22,0x22,0x26,0x0,0x0,0x26,0x24,0x24,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    }
};

#define MAX_BALLS 10

enum difficulty_options {
    EASY, NORMAL
};

int pad_pos_x;
int life;
enum { ST_READY, ST_START, ST_PAUSE } game_state = ST_READY;

enum {
    PLAIN   = 0,
    STICKY  = 1,
    SHOOTER = 2
} pad_type;

int score=0,vscore=0;
bool flip_sides=false;
int level=0;
int brick_on_board=0;
int used_balls=1;
int difficulty = NORMAL;
int pad_width;
int num_count;
bool resume = false;

typedef struct cube {
    int powertop;
    int power;
    char poweruse;
    char used;
    int color;
    int hits;
    int hiteffect;
} cube;
cube brick[80];

typedef struct balls {
    /* pos_x and y store the current position of the ball */
    int pos_x;
    int pos_y;
    /* x and y store the current speed of the ball */
    int y;
    int tempy;
    int x;
    int tempx;
    bool glue;
} balls;

balls ball[MAX_BALLS];

typedef struct sfire {
    int top;
    int left;
} sfire;
sfire fire[30];

#define CONFIG_FILE_NAME "brickmania.cfg"
#define SAVE_FILE  PLUGIN_GAMES_DIR "/brickmania.save"
#define HIGH_SCORE PLUGIN_GAMES_DIR "/brickmania.score"
#define NUM_SCORES 5

static struct configdata config[] = {
    {TYPE_INT, 0, 1, { .int_p = &difficulty }, "difficulty", NULL},
};

struct highscore highest[NUM_SCORES];

static void brickmania_init_game(int new_game)
{
    int i,j;

    pad_pos_x=LCD_WIDTH/2-PAD_WIDTH/2;

    for(i=0;i<MAX_BALLS;i++) {
        ball[i].x=0;
        ball[i].y=0;
        ball[i].tempy=0;
        ball[i].tempx=0;
        ball[i].pos_y=PAD_POS_Y-BALL;
        ball[i].pos_x=LCD_WIDTH/2-2;
        ball[i].glue=false;
    }

    used_balls=1;
    game_state=ST_READY;
    pad_type = PLAIN;
    pad_width=PAD_WIDTH;
    flip_sides=false;
    num_count=10;

    if (new_game==1) {
        brick_on_board=0;
        /* add one life per achieved level */
        if (difficulty==EASY && life<2) {
            score-=100;
            life++;
        }
    }
    for(i=0;i<30;i++) {
        fire[i].top=-8;
    }
    for(i=0;i<=7;i++) {
        for(j=0;j<=9;j++) {
            brick[i*10+j].poweruse=(levels[level][i][j]==0?0:1);
            if (new_game==1) {
                brick[i*10+j].power=rb->rand()%25;
                /* +8 make the game with less powerups */

                brick[i*10+j].hits=levels[level][i][j]>=10?
                    levels[level][i][j]/16-1:0;
                brick[i*10+j].hiteffect=0;
                brick[i*10+j].powertop=TOPMARGIN+i*BRICK_HEIGHT+BRICK_HEIGHT;
                brick[i*10+j].used=(levels[level][i][j]==0?0:1);
                brick[i*10+j].color=(levels[level][i][j]>=10?
                                     levels[level][i][j]%16:
                                     levels[level][i][j])-1;
                if (levels[level][i][j]!=0)
                    brick_on_board++;
            }
        }
    }
}

static void brickmania_loadgame(void)
{
    int fd;

    resume = false;

    /* open game file */
    fd = rb->open(SAVE_FILE, O_RDONLY);
    if(fd < 0) return;

    /* read in saved game */
    while(true) {
        if(rb->read(fd, &pad_pos_x, sizeof(pad_pos_x)) <= 0) break;
        if(rb->read(fd, &life, sizeof(life)) <= 0) break;
        if(rb->read(fd, &game_state, sizeof(game_state)) <= 0) break;
        if(rb->read(fd, &pad_type, sizeof(pad_type)) <= 0) break;
        if(rb->read(fd, &score, sizeof(score)) <= 0) break;
        if(rb->read(fd, &flip_sides, sizeof(flip_sides)) <= 0) break;
        if(rb->read(fd, &level, sizeof(level)) <= 0) break;
        if(rb->read(fd, &brick_on_board, sizeof(brick_on_board)) <= 0) break;
        if(rb->read(fd, &used_balls, sizeof(used_balls)) <= 0) break;
        if(rb->read(fd, &pad_width, sizeof(pad_width)) <= 0) break;
        if(rb->read(fd, &num_count, sizeof(num_count)) <= 0) break;
        if(rb->read(fd, &brick, sizeof(brick)) <= 0) break;
        if(rb->read(fd, &ball, sizeof(ball)) <= 0) break;
        if(rb->read(fd, &fire, sizeof(fire)) <= 0) break;
        vscore = score;
        resume = true;
        break;
    }

    rb->close(fd);

    /* delete saved file */
    rb->remove(SAVE_FILE);
    return;
}

static void brickmania_savegame(void)
{
    int fd;

    /* write out the game state to the save file */
    fd = rb->open(SAVE_FILE, O_WRONLY|O_CREAT);
    rb->write(fd, &pad_pos_x, sizeof(pad_pos_x));
    rb->write(fd, &life, sizeof(life));
    rb->write(fd, &game_state, sizeof(game_state));
    rb->write(fd, &pad_type, sizeof(pad_type));
    rb->write(fd, &score, sizeof(score));
    rb->write(fd, &flip_sides, sizeof(flip_sides));
    rb->write(fd, &level, sizeof(level));
    rb->write(fd, &brick_on_board, sizeof(brick_on_board));
    rb->write(fd, &used_balls, sizeof(used_balls));
    rb->write(fd, &pad_width, sizeof(pad_width));
    rb->write(fd, &num_count, sizeof(num_count));
    rb->write(fd, &brick, sizeof(brick));
    rb->write(fd, &ball, sizeof(ball));
    rb->write(fd, &fire, sizeof(fire));
    rb->close(fd);
}

/* brickmania_sleep timer counting the score */
static void brickmania_sleep(int secs)
{
    bool done=false;
    char s[20];
    int count=0;
    int sw, w;

    while (!done) {
        if (vscore == score) {
            if (count==0)
                count=*rb->current_tick+HZ*secs;
            if (*rb->current_tick>=count)
                done=true;
        } else {
            if (vscore<score)
                vscore++;
            if (vscore>score)
                vscore--;
            rb->snprintf(s, sizeof(s), "%d", vscore);
            rb->lcd_getstringsize(s, &sw, &w);
            rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 0, s);
            rb->lcd_update_rect(0,0,LCD_WIDTH,w+2);
        }
        rb->yield();
    }
}

static int brickmania_help(void)
{
#define WORDS (sizeof help_text / sizeof (char*))
    static char *help_text[] = {
        "Brickmania", "", "Aim", "",
        "Destroy", "all", "the", "bricks", "by", "bouncing",
        "the", "ball", "of", "them", "using", "the", "paddle.", "", "",
        "Controls", "",
        "< & >", "Moves", "the", "paddle", "",
#if CONFIG_KEYPAD == ONDIO_PAD
        "MENU:",
#elif (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == IAUDIO_M3_PAD)
        "PLAY:",
#elif CONFIG_KEYPAD == IRIVER_H300_PAD
        "NAVI:",
#else
        "SELECT:",
#endif
        "Releases", "the", "ball/Fire!", "",
#if CONFIG_KEYPAD == IAUDIO_M3_PAD
        "REC:",
#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD) || \
      (CONFIG_KEYPAD == CREATIVEZVM_PAD)
        "BACK:",
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD) || \
      (CONFIG_KEYPAD == SANSA_FUZE_PAD)
        "MENU:",
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD) || \
      (CONFIG_KEYPAD == ONDIO_PAD) || \
      (CONFIG_KEYPAD == RECORDER_PAD) || \
      (CONFIG_KEYPAD == ARCHOS_AV300_PAD)
        "STOP:",
#else
        "POWER:",
#endif
        "Returns", "to", "menu", "", "",
        "Specials", "",
        "N", "Normal:", "returns", "paddle", "to", "normal", "",
        "D", "DIE!:", "loses", "a", "life", "",
        "L", "Life:", "gains", "a", "life/power", "up", "",
        "F", "Fire:", "allows", "you", "to", "shoot", "bricks", "",
        "G", "Glue:", "ball", "sticks", "to", "paddle", "",
        "B", "Ball:", "generates", "another", "ball", "",
        "FL", "Flip:", "flips", "left / right", "movement", "",
        "<->", "or", "<E>:", "enlarges", "the", "paddle", "",
        ">-<", "or", ">S<:", "shrinks", "the", "paddle", "",
    };
    static struct style_text formation[]={
        { 0, TEXT_CENTER|TEXT_UNDERLINE },
        { 2, C_RED },
        { 19, C_RED },
        { 37, C_RED },
        { 39, C_BLUE },
        { 46, C_RED },
        { 52, C_GREEN },
        { 59, C_ORANGE },
        { 67, C_GREEN },
        { 74, C_YELLOW },
        { 80, C_RED },
        { -1, 0 }
    };
    int button;

    rb->lcd_setfont(FONT_UI);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
#endif

    if (display_text(WORDS, help_text, formation, NULL))
        return 1;
    do {
        button = rb->button_get(true);
        if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
            return 1;
        }
    } while( ( button == BUTTON_NONE )
          || ( button & (BUTTON_REL|BUTTON_REPEAT) ) );
    rb->lcd_setfont(FONT_SYSFIXED);
    return 0;
}

static int brickmania_menu_cb(int action, const struct menu_item_ex *this_item)
{
    int i = ((intptr_t)this_item);
    if(action == ACTION_REQUEST_MENUITEM
       && !resume && (i==0 || i==6))
        return ACTION_EXIT_MENUITEM;
    return action;
}

static int brickmania_menu(void)
{
    int selected = 0;

    static struct opt_items options[] = {
        { "Easy", -1 },
        { "Normal", -1 },
    };

    MENUITEM_STRINGLIST(main_menu, "Brickmania Menu", brickmania_menu_cb,
                        "Resume Game", "Start New Game",
                        "Difficulty", "Help", "High Scores",
                        "Playback Control",
                        "Quit without Saving", "Quit");

    rb->button_clear_queue();
    while (true) {
        switch (rb->do_menu(&main_menu, &selected, NULL, false)) {
            case 0:
                if(game_state!=ST_READY)
                    game_state = ST_PAUSE;
                return 0;
            case 1:
                score=0;
                vscore=0;
                life=2;
                level=0;
                brickmania_init_game(1);
                return 0;
            case 2:
                rb->set_option("Difficulty", &difficulty, INT, options, 2, NULL);
                break;
            case 3:
                if (brickmania_help())
                    return 1;
                break;
            case 4:
                highscore_show(NUM_SCORES, highest, NUM_SCORES, true);
                break;
            case 5:
                if (playback_control(NULL))
                    return 1;
                break;
            case 6:
                return 1;
            case 7:
                if (resume) {
                    rb->splash(HZ*1, "Saving game ...");
                    brickmania_savegame();
                }
                return 1;
            case MENU_ATTACHED_USB:
                return 1;
            default:
                break;
        }
    }
}

static int brickmania_fire_space(void)
{
    int t;
    for(t=0;t<30;t++)
        if (fire[t].top+7 < 0)
            return t;

    return 0;
}

static int brickmania_game_loop(void)
{
    int j,i,k,bricky,brickx;
    int sw;
    char s[30];
    int sec_count=0;
    int end;

    if (brickmania_menu()!=0) {
        return 1;
    }
    resume = false;

    while(true) {
        /* Convert CYCLETIME (in ms) to HZ */
        end = *rb->current_tick + (CYCLETIME * HZ) / 1000;

        if (life >= 0) {
#ifdef HAVE_LCD_COLOR
            rb->lcd_set_background(LCD_BLACK);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_clear_display();
            rb->lcd_set_background(LCD_BLACK);
#if LCD_HEIGHT > GAMESCREEN_HEIGHT
            rb->lcd_set_foreground(rb->global_settings->bg_color);
            rb->lcd_fillrect(0, GAMESCREEN_HEIGHT, LCD_WIDTH,
                             LCD_HEIGHT - GAMESCREEN_HEIGHT);
#endif
            rb->lcd_set_foreground(LCD_WHITE);
#else
            rb->lcd_clear_display();
#endif

            if (flip_sides) {
                if (*rb->current_tick>=sec_count) {
                    sec_count=*rb->current_tick+HZ;
                    if (num_count!=0)
                        num_count--;
                    else
                        flip_sides=false;
                }
                rb->snprintf(s, sizeof(s), "%d", num_count);
                rb->lcd_getstringsize(s, &sw, NULL);
                rb->lcd_putsxy(LCD_WIDTH/2-2, STRINGPOS_FLIP, s);
            }

            /* write life num */
#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
            rb->snprintf(s, sizeof(s), "L:%d", life);
#else
            rb->snprintf(s, sizeof(s), "Life: %d", life);
#endif
            rb->lcd_putsxy(0, 0, s);

#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
            rb->snprintf(s, sizeof(s), "L%d", level+1);
#else
            rb->snprintf(s, sizeof(s), "Level %d", level+1);
#endif

            rb->lcd_getstringsize(s, &sw, NULL);
            rb->lcd_putsxy(LCD_WIDTH-sw, 0, s);

            if (vscore<score) vscore++;
            rb->snprintf(s, sizeof(s), "%d", vscore);
            rb->lcd_getstringsize(s, &sw, NULL);
            rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 0, s);

            /* continue game */
            if (game_state==ST_PAUSE) {
#if CONFIG_KEYPAD == ONDIO_PAD
                rb->snprintf(s, sizeof(s), "MENU To Continue");
#elif CONFIG_KEYPAD == IRIVER_H300_PAD
                rb->snprintf(s, sizeof(s), "Press NAVI To Continue");
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
                rb->snprintf(s, sizeof(s), "PLAY To Continue");
#else
                rb->snprintf(s, sizeof(s), "Press SELECT To Continue");
#endif
                rb->lcd_getstringsize(s, &sw, NULL);
                rb->lcd_putsxy(LCD_WIDTH/2-sw/2, STRINGPOS_NAVI, s);

                sec_count=*rb->current_tick+HZ;
            }

            /* draw the ball */
            for(i=0;i<used_balls;i++)
                rb->lcd_bitmap(brickmania_ball,ball[i].pos_x, ball[i].pos_y,
                               BALL, BALL);

            if (brick_on_board==0)
                brick_on_board--;

            /* if the pad is fire */
            for(i=0;i<30;i++) {
                if (fire[i].top+7>0) {
                    if (game_state!=ST_PAUSE)
                        fire[i].top -= SPEED_FIRE;
                    rb->lcd_vline(fire[i].left, fire[i].top, fire[i].top+7);
                }
            }

            /* the bricks */
            for (i=0;i<=7;i++) {
                for (j=0;j<=9;j++) {
                    if (brick[i*10+j].power<9) {
                        if (brick[i*10+j].poweruse==2) {
                            if (game_state!=ST_PAUSE)
                                brick[i*10+j].powertop+=SPEED_POWER;
                            rb->lcd_bitmap_part(brickmania_powerups,0,
                                POWERUP_HEIGHT*brick[i*10+j].power,
                                STRIDE( BMPWIDTH_brickmania_powerups,
                                        BMPHEIGHT_brickmania_powerups),
                                LEFTMARGIN+j*BRICK_WIDTH+
                                    (BRICK_WIDTH/2-POWERUP_WIDTH/2),
                                brick[i*10+j].powertop,
                                POWERUP_WIDTH,
                                POWERUP_HEIGHT);
                        }
                    }

                    /* Did the brick hit the pad */
                    if ((pad_pos_x<LEFTMARGIN+j*BRICK_WIDTH+5 &&
                         pad_pos_x+pad_width>LEFTMARGIN+j*BRICK_WIDTH+5) &&
                        brick[i*10+j].powertop+6>=PAD_POS_Y &&
                        brick[i*10+j].poweruse==2) {
                        switch(brick[i*10+j].power) {
                            case 0:
                                life++;
                                score+=50;
                                break;
                            case 1:
                                life--;
                                if (life>=0) {
                                    brickmania_init_game(0);
                                    brickmania_sleep(2);
                                }
                                break;
                            case 2:
                                score+=34;
                                pad_type = STICKY;
                                break;
                            case 3:
                                score+=47;
                                pad_type = SHOOTER;
                                for(k=0;k<used_balls;k++)
                                    ball[k].glue=false;
                                break;
                            case 4:
                                score+=23;
                                pad_type = PLAIN;
                                for(k=0;k<used_balls;k++)
                                    ball[k].glue=false;
                                flip_sides=false;
                                pad_pos_x+=(pad_width-PAD_WIDTH)/2;
                                pad_width=PAD_WIDTH;
                                break;
                            case 5:
                                score+=23;
                                sec_count=*rb->current_tick+HZ;
                                num_count=10;
                                flip_sides=!flip_sides;
                                break;
                            case 6:
                                score+=23;
                                if(used_balls<MAX_BALLS) {
                                    ball[used_balls].x= rb->rand()%2 == 0 ?
                                        -1 : 1;
                                    ball[used_balls].y= -4;
                                    ball[used_balls].glue= false;
                                    used_balls++;
                                }
                                break;
                            case 7:
                                score+=23;
                                if (pad_width==PAD_WIDTH) {
                                    pad_width=LONG_PAD_WIDTH;
                                    pad_pos_x-=(LONG_PAD_WIDTH-PAD_WIDTH)/2;
                                }
                                else if (pad_width==SHORT_PAD_WIDTH) {
                                    pad_width=PAD_WIDTH;
                                    pad_pos_x-=(PAD_WIDTH-SHORT_PAD_WIDTH)/2;
                                }
                                if (pad_pos_x < 0)
                                    pad_pos_x = 0;
                                else if(pad_pos_x+pad_width > LCD_WIDTH)
                                    pad_pos_x = LCD_WIDTH-pad_width;
                                break;
                            case 8:
                                if (pad_width==PAD_WIDTH) {
                                    pad_width=SHORT_PAD_WIDTH;
                                    pad_pos_x+=(PAD_WIDTH-SHORT_PAD_WIDTH)/2;
                                }
                                else if (pad_width==LONG_PAD_WIDTH) {
                                    pad_width=PAD_WIDTH;
                                    pad_pos_x+=(LONG_PAD_WIDTH-PAD_WIDTH)/2;
                                }
                                break;
                        }
                        brick[i*10+j].poweruse=1;
                    }

                    if (brick[i*10+j].powertop>PAD_POS_Y)
                        brick[i*10+j].poweruse=1;

                    brickx=LEFTMARGIN+j*BRICK_WIDTH;
                    bricky=TOPMARGIN+i*BRICK_HEIGHT;
                    if (pad_type == SHOOTER) {
                        for (k=0;k<30;k++) {
                            if (fire[k].top+7>0) {
                                if (brick[i*10+j].used==1 &&
                                    (fire[k].left+1 >= brickx &&
                                     fire[k].left+1 <= brickx+BRICK_WIDTH) &&
                                    (bricky+BRICK_HEIGHT>fire[k].top)) {
                                    score+=13;
                                    fire[k].top=-16;
                                    if (brick[i*10+j].hits > 0) {
                                        brick[i*10+j].hits--;
                                        brick[i*10+j].hiteffect++;
                                        score+=3;
                                    }
                                    else {
                                        brick[i*10+j].used=0;
                                        if (brick[i*10+j].power!=10)
                                            brick[i*10+j].poweruse=2;
                                        brick_on_board--;
                                    }
                                }
                            }
                        }
                    }

                    if (brick[i*10+j].used==1) {
                        rb->lcd_bitmap_part(brickmania_bricks,0,
                            BRICK_HEIGHT*brick[i*10+j].color,
                            STRIDE( BMPWIDTH_brickmania_bricks,
                                    BMPHEIGHT_brickmania_bricks),
                            LEFTMARGIN+j*BRICK_WIDTH,
                            TOPMARGIN+i*BRICK_HEIGHT,
                            BRICK_WIDTH, BRICK_HEIGHT);
#ifdef HAVE_LCD_COLOR  /* No transparent effect for greyscale lcds for now */
                        if (brick[i*10+j].hiteffect>0)
                            rb->lcd_bitmap_transparent_part(brickmania_break,0,
                                BRICK_HEIGHT*brick[i*10+j].hiteffect,
                                STRIDE( BMPWIDTH_brickmania_break, 
                                        BMPHEIGHT_brickmania_break),
                                LEFTMARGIN+j*BRICK_WIDTH,
                                TOPMARGIN+i*BRICK_HEIGHT,
                                BRICK_WIDTH, BRICK_HEIGHT);
#endif
                    }
                    /* Somewhere in here collision checking is done b/w ball and
                     *  brick.
                     */
                    for(k=0;k<used_balls;k++) {
                        if (ball[k].pos_y < PAD_POS_Y) {
                            if (brick[i*10+j].used==1) {
                                if ((ball[k].pos_x+ball[k].x+HALFBALL >=
                                     brickx &&
                                     ball[k].pos_x+ball[k].x+HALFBALL <=
                                     brickx+BRICK_WIDTH) &&
                                    ((bricky-4<ball[k].pos_y+BALL &&
                                      bricky>ball[k].pos_y+BALL) ||
                                     (bricky+4>ball[k].pos_y+BALL+BALL &&
                                      bricky<ball[k].pos_y+BALL+BALL)) &&
                                    (ball[k].y >0)) {
                                    ball[k].tempy=bricky-ball[k].pos_y-BALL;
                                }
                                else if ((ball[k].pos_x+ball[k].x+HALFBALL >=
                                          brickx &&
                                          ball[k].pos_x+ball[k].x+HALFBALL <=
                                          brickx+BRICK_WIDTH) &&
                                         ((bricky+BRICK_HEIGHT+4>ball[k].pos_y &&
                                           bricky+BRICK_HEIGHT<ball[k].pos_y) ||
                                          (bricky+BRICK_HEIGHT-4<ball[k].pos_y-BALL &&
                                           bricky+BRICK_HEIGHT>ball[k].pos_y-BALL)) &&
                                         (ball[k].y <0)) {
                                    ball[k].tempy=
                                        -(ball[k].pos_y-(bricky+BRICK_HEIGHT));
                                }

                                if ((ball[k].pos_y+HALFBALL >=
                                     bricky &&
                                     ball[k].pos_y+HALFBALL <=
                                     bricky+BRICK_HEIGHT) &&
                                    ((brickx-4<ball[k].pos_x+BALL &&
                                      brickx>ball[k].pos_x+BALL) ||
                                     (brickx+4>ball[k].pos_x+BALL+BALL &&
                                      brickx<ball[k].pos_x+BALL+BALL)) &&
                                    (ball[k].x >0)) {
                                    ball[k].tempx=brickx-ball[k].pos_x-BALL;
                                }
                                else if ((ball[k].pos_y+ball[k].y+HALFBALL >=
                                          bricky &&
                                          ball[k].pos_y+ball[k].y+HALFBALL <=
                                          bricky+BRICK_HEIGHT) &&
                                         ((brickx+BRICK_WIDTH+4>ball[k].pos_x &&
                                           brickx+BRICK_WIDTH<ball[k].pos_x) ||
                                          (brickx+BRICK_WIDTH-4<ball[k].pos_x-
                                           BALL &&
                                           brickx+BRICK_WIDTH>ball[k].pos_x-
                                           BALL)) && (ball[k].x <0)) {
                                    ball[k].tempx=
                                        -(ball[k].pos_x-(brickx+BRICK_WIDTH));
                                }

                                if ((ball[k].pos_x+HALFBALL >= brickx &&
                                     ball[k].pos_x+HALFBALL <=
                                     brickx+BRICK_WIDTH) &&
                                    ((bricky+BRICK_HEIGHT==ball[k].pos_y) ||
                                     (bricky+BRICK_HEIGHT-6<=ball[k].pos_y &&
                                      bricky+BRICK_HEIGHT>ball[k].pos_y)) &&
                                    (ball[k].y <0)) { /* bottom line */
                                    if (brick[i*10+j].hits > 0) {
                                        brick[i*10+j].hits--;
                                        brick[i*10+j].hiteffect++;
                                        score+=2;
                                    }
                                    else {
                                        brick[i*10+j].used=0;
                                        if (brick[i*10+j].power!=10)
                                            brick[i*10+j].poweruse=2;
                                    }

                                    ball[k].y = ball[k].y*-1;
                                }
                                else if ((ball[k].pos_x+HALFBALL >= brickx &&
                                          ball[k].pos_x+HALFBALL <=
                                          brickx+BRICK_WIDTH) &&
                                         ((bricky==ball[k].pos_y+BALL) ||
                                          (bricky+6>=ball[k].pos_y+BALL &&
                                           bricky<ball[k].pos_y+BALL)) &&
                                         (ball[k].y >0)) { /* top line */
                                    if (brick[i*10+j].hits > 0) {
                                        brick[i*10+j].hits--;
                                        brick[i*10+j].hiteffect++;
                                        score+=2;
                                    }
                                    else {
                                        brick[i*10+j].used=0;
                                        if (brick[i*10+j].power!=10)
                                            brick[i*10+j].poweruse=2;
                                    }

                                    ball[k].y = ball[k].y*-1;
                                }

                                if ((ball[k].pos_y+HALFBALL >= bricky &&
                                     ball[k].pos_y+HALFBALL <=
                                     bricky+BRICK_HEIGHT) &&
                                    ((brickx==ball[k].pos_x+BALL) ||
                                     (brickx+6>=ball[k].pos_x+BALL &&
                                      brickx<ball[k].pos_x+BALL)) &&
                                    (ball[k].x > 0)) { /* left line */
                                    if (brick[i*10+j].hits > 0) {
                                        brick[i*10+j].hits--;
                                        brick[i*10+j].hiteffect++;
                                        score+=2;
                                    }
                                    else {
                                        brick[i*10+j].used=0;
                                        if (brick[i*10+j].power!=10)
                                            brick[i*10+j].poweruse=2;
                                    }
                                    ball[k].x = ball[k].x*-1;

                                }
                                else if ((ball[k].pos_y+HALFBALL >= bricky &&
                                          ball[k].pos_y+HALFBALL <=
                                          bricky+BRICK_HEIGHT) &&
                                         ((brickx+BRICK_WIDTH==
                                           ball[k].pos_x) ||
                                          (brickx+BRICK_WIDTH-6<=
                                           ball[k].pos_x &&
                                           brickx+BRICK_WIDTH>
                                           ball[k].pos_x)) &&
                                         (ball[k].x < 0)) { /* Right line */
                                    if (brick[i*10+j].hits > 0) {
                                        brick[i*10+j].hits--;
                                        brick[i*10+j].hiteffect++;
                                        score+=2;
                                    }
                                    else {
                                        brick[i*10+j].used=0;
                                        if (brick[i*10+j].power!=10)
                                            brick[i*10+j].poweruse=2;
                                    }

                                    ball[k].x = ball[k].x*-1;
                                }

                                if (brick[i*10+j].used==0) {
                                    brick_on_board--;
                                    score+=8;
                                }
                            }
                        }
                    } /* for k */
                } /* for j */
            } /* for i */

            /* draw the pad */
            if( pad_width == PAD_WIDTH ) /* Normal width */
            {
                rb->lcd_bitmap_part(
                    brickmania_pads,
                    0, pad_type*PAD_HEIGHT,
                    STRIDE(BMPWIDTH_brickmania_pads, BMPHEIGHT_brickmania_pads),
                    pad_pos_x, PAD_POS_Y, pad_width, PAD_HEIGHT);
            }
            else if( pad_width == LONG_PAD_WIDTH ) /* Long Pad */
            {
                rb->lcd_bitmap_part(
                    brickmania_long_pads,
                    0,pad_type*PAD_HEIGHT,
                    STRIDE( BMPWIDTH_brickmania_long_pads, 
                            BMPHEIGHT_brickmania_long_pads),
                    pad_pos_x, PAD_POS_Y, pad_width, PAD_HEIGHT);
            }
            else /* Short pad */
            {
                rb->lcd_bitmap_part(
                    brickmania_short_pads,
                    0,pad_type*PAD_HEIGHT,
                    STRIDE( BMPWIDTH_brickmania_short_pads, 
                            BMPHEIGHT_brickmania_short_pads),
                    pad_pos_x, PAD_POS_Y, pad_width, PAD_HEIGHT);
            }
            

            /* If the game is not paused continue */
            if (game_state!=ST_PAUSE)
            {
                /* Loop through all of the balls in play */
                for(k=0;k<used_balls;k++) 
                {
                    if (    (ball[k].pos_x >= pad_pos_x &&
                             ball[k].pos_x <= pad_pos_x+pad_width) &&
                            (PAD_POS_Y-4<ball[k].pos_y+BALL &&
                             PAD_POS_Y>ball[k].pos_y+BALL) && (ball[k].y >0))
                    {
                        ball[k].tempy=PAD_POS_Y-ball[k].pos_y-BALL;
                    }
                    else if ((4>ball[k].pos_y && 0<ball[k].pos_y) &&
                             (ball[k].y <0))
                    {
                        ball[k].tempy=-ball[k].pos_y;
                    }
                    if ((LCD_WIDTH-4<ball[k].pos_x+BALL &&
                         LCD_WIDTH>ball[k].pos_x+BALL) && (ball[k].x >0))
                    {
                        ball[k].tempx=LCD_WIDTH-ball[k].pos_x-BALL;
                    }
                    else if ((4>ball[k].pos_x && 0<ball[k].pos_x) &&
                             (ball[k].x <0))
                    {
                        ball[k].tempx=-ball[k].pos_x;
                    }

                    /* Did the Ball hit the top of the screen? */
                    if (ball[k].pos_y<= 0)
                    {
                        /* Reverse the direction */
                        ball[k].y = ball[k].y*-1;
                    }
                    /* Player missed the ball and hit bottom of screen */
                    else if (ball[k].pos_y+BALL >= GAMESCREEN_HEIGHT) 
                    {
                        /* Player had balls to spare, so handle the removal */
                        if (used_balls>1) 
                        {
                            /* decrease number of balls in play */
                            used_balls--;
                            /* Replace removed ball with the last ball */
                            ball[k].pos_x = ball[used_balls].pos_x;
                            ball[k].pos_y = ball[used_balls].pos_y;
                            ball[k].y = ball[used_balls].y;
                            ball[k].tempy = ball[used_balls].tempy;
                            ball[k].x = ball[used_balls].x;
                            ball[k].tempx = ball[used_balls].tempx;
                            ball[k].glue = ball[used_balls].glue;

                            /* Reset the last ball that was removed */
                            ball[used_balls].x=0;
                            ball[used_balls].y=0;
                            ball[used_balls].tempy=0;
                            ball[used_balls].tempx=0;
                            ball[used_balls].pos_y=PAD_POS_Y-BALL;
                            ball[used_balls].pos_x=pad_pos_x+(pad_width/2)-2;

                            k--;
                            continue;
                        }
                        else 
                        {
                            /* Player lost a life */
                            life--;
                            if (life>=0) 
                            {
                                /* No lives left reset game */
                                brickmania_init_game(0);
                                brickmania_sleep(2);
                            }
                        }
                    }

                    /* Check if the ball hit the left or right side */
                    if (    (ball[k].pos_x <= 0) || 
                            (ball[k].pos_x+BALL >= LCD_WIDTH))
                    {
                        /* Reverse direction */
                        ball[k].x = ball[k].x*-1;
                        /* Re-position ball in gameboard */
                        ball[k].pos_x = ball[k].pos_x <= 0 ? 0 : LCD_WIDTH-BALL;
                    }

                    /* Did the ball hit the paddle? Depending on where the ball
                     *  Hit set the x/y speed appropriately.
                     */
                    if( (ball[k].pos_y + BALL       >= PAD_POS_Y &&
                        (ball[k].pos_x + HALFBALL   >= pad_pos_x  &&
                         ball[k].pos_x + HALFBALL   <= pad_pos_x+pad_width)) &&
                         game_state!=ST_READY && !ball[k].glue) 
                    {
                        /* Position the ball relative to the paddle width */
                        int ball_repos=ball[k].pos_x + HALFBALL - pad_pos_x;
                        /* If the ball hits the right half of paddle, x speed
                         *  should be positive, if it hits the left half it
                         *  should be negative.
                         */
                        int x_direction = -1;
                        
                        /* Comparisons are done with respect to 1/2 pad_width */
                        if(ball_repos > pad_width/2)
                        {
                            /* flip the relative position */
                            ball_repos -= ((ball_repos - pad_width/2) << 1);
                            /* Ball hit the right half so X speed calculations
                             *  should be positive.
                             */
                             x_direction = 1;
                        }
                        
                        /* Figure out where the ball hit relative to 1/2 pad 
                         *  and in divisions of 4.
                         */
                        ball_repos = ball_repos / (pad_width/2/4);
                        
                        switch(ball_repos)
                        {
                        /* Ball hit the outer edge of the paddle */
                        case 0:
                            ball[k].y = SPEED_1Q_Y;
                            ball[k].x = SPEED_1Q_X * x_direction;
                            break;
                        /* Ball hit the next fourth of the paddle */
                        case 1:
                            ball[k].y = SPEED_2Q_Y;
                            ball[k].x = SPEED_2Q_X * x_direction;
                            break;
                        /* Ball hit the third fourth of the paddle */
                        case 2:
                            ball[k].y = SPEED_3Q_Y;
                            ball[k].x = SPEED_3Q_X * x_direction;
                            break;
                        /* Ball hit the fourth fourth of the paddle or dead
                         *  center.
                         */
                        case 3:
                        case 4:
                            ball[k].y = SPEED_4Q_Y;
                            /* Since this is the middle we don't want to
                             *  force the ball in a different direction.
                             *  Just keep it going in the same direction
                             *  with a specific speed.
                             */
                            ball[k].x = (ball[k].x > 0) ? 
                                    SPEED_4Q_X: -SPEED_4Q_X;
                            break;

                        default:
                            ball[k].y = SPEED_4Q_Y;
                            break;
                        }
                    }

                    /* Update the ball position */
                    if (!ball[k].glue) {
                        ball[k].pos_x+=ball[k].tempx!=0?ball[k].tempx:ball[k].x;
                        ball[k].pos_y+=ball[k].tempy!=0?ball[k].tempy:ball[k].y;

                        ball[k].tempy=0;
                        ball[k].tempx=0;
                    }
                    
                    /* Handle the sticky situation */
                    if (ball[k].pos_y + BALL >= PAD_POS_Y &&
                        (pad_type == STICKY && !ball[k].glue) &&
                        (ball[k].pos_x >= pad_pos_x &&
                         ball[k].pos_x <= pad_pos_x+pad_width)) {
                        ball[k].y=0;
                        ball[k].pos_y=PAD_POS_Y-BALL;
                        ball[k].glue=true;
                    }
                } /* for k */
            }

            rb->lcd_update();

            if (brick_on_board < 0) {
                if (level+1<levels_num) {
                    level++;
                    if (difficulty==NORMAL)
                        score+=100;
                    brickmania_init_game(1);
                    brickmania_sleep(2);
                }
                else {
                    rb->lcd_getstringsize("Congratulations!", &sw, NULL);
                    rb->lcd_putsxy(LCD_WIDTH/2-sw/2, STRINGPOS_CONGRATS,
                                   "Congratulations!");
#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
                    rb->lcd_getstringsize("No more levels", &sw, NULL);
                    rb->lcd_putsxy(LCD_WIDTH/2-sw/2, STRINGPOS_FINISH,
                                   "No more levels");
#else
                    rb->lcd_getstringsize("You have finished the game!",
                                          &sw, NULL);
                    rb->lcd_putsxy(LCD_WIDTH/2-sw/2, STRINGPOS_FINISH,
                                   "You have finished the game!");
#endif
                    vscore=score;
                    rb->lcd_update();
                    brickmania_sleep(2);
                    return 0;
                }
            }

            int move_button,button;
            int button_right,button_left;
            button=rb->button_get(false);

#if defined(HAS_BUTTON_HOLD) && !defined(HAVE_REMOTE_LCD_AS_MAIN)
            /* FIXME: Should probably check remote hold here */
            if (rb->button_hold())
                button = QUIT;
#endif

#ifdef HAVE_TOUCHSCREEN
            if(button & BUTTON_TOUCHSCREEN)
            {
                short touch_x, touch_y;
                touch_x = rb->button_get_data() >> 16;
                touch_y = rb->button_get_data() & 0xffff;
                if(touch_y >= PAD_POS_Y && touch_y <= PAD_POS_Y+PAD_HEIGHT)
                {
                    pad_pos_x += (flip_sides ? -1 : 1) * ( (touch_x-pad_pos_x-pad_width/2) / 4 );

                    if(pad_pos_x < 0)
                        pad_pos_x = 0;
                    else if(pad_pos_x+pad_width > LCD_WIDTH)
                        pad_pos_x = LCD_WIDTH-pad_width;
                    for(k=0;k<used_balls;k++)
                        if (game_state==ST_READY || ball[k].glue)
                            ball[k].pos_x = pad_pos_x+pad_width/2;
                }

                if(button & BUTTON_REL)
                    button = SELECT;
            }
            else
            {
#endif
                move_button=rb->button_status();
                #ifdef ALTRIGHT
                button_right=((move_button & RIGHT) || (move_button & ALTRIGHT));
                button_left=((move_button & LEFT) || (move_button & ALTLEFT));
                #else
                button_right=((move_button & RIGHT) || (SCROLL_FWD(button)));
                button_left=((move_button & LEFT) || (SCROLL_BACK(button)));
                #endif
                if ((game_state==ST_PAUSE) && (button_right || button_left))
                    continue;
                if ((button_right && flip_sides==false) ||
                    (button_left && flip_sides==true)) {
                    if (pad_pos_x+SPEED_PAD+pad_width > LCD_WIDTH) {
                        for(k=0;k<used_balls;k++)
                            if (game_state==ST_READY || ball[k].glue)
                                ball[k].pos_x+=LCD_WIDTH-pad_pos_x-pad_width;
                        pad_pos_x+=LCD_WIDTH-pad_pos_x-pad_width;
                    }
                    else {
                        for(k=0;k<used_balls;k++)
                            if ((game_state==ST_READY || ball[k].glue))
                                ball[k].pos_x+=SPEED_PAD;
                        pad_pos_x+=SPEED_PAD;
                    }
                }
                else if ((button_left && flip_sides==false) ||
                         (button_right && flip_sides==true)) {
                    if (pad_pos_x-SPEED_PAD < 0) {
                        for(k=0;k<used_balls;k++)
                            if (game_state==ST_READY || ball[k].glue)
                                ball[k].pos_x-=pad_pos_x;
                        pad_pos_x-=pad_pos_x;
                    }
                    else {
                        for(k=0;k<used_balls;k++)
                            if (game_state==ST_READY || ball[k].glue)
                                ball[k].pos_x-=SPEED_PAD;
                        pad_pos_x-=SPEED_PAD;
                    }
                }
#ifdef HAVE_TOUCHSCREEN
            }
#endif


            switch(button) {
                case UP:
                case SELECT:
                    if (game_state==ST_READY) {
                        for(k=0;k<used_balls;k++) {
                            ball[k].y=-4;
                            ball[k].x=pad_pos_x+(pad_width/2)-2>=
                                LCD_WIDTH/2?2:-2;
                        }
                        game_state=ST_START;
                    }
                    else if (game_state==ST_PAUSE) {
                        game_state=ST_START;
                    }
                    else if (pad_type == STICKY) {
                        for(k=0;k<used_balls;k++) {
                            if (ball[k].glue)
                                ball[k].glue=false;
                        }
                    }
                    else if (pad_type == SHOOTER) {
                        k=brickmania_fire_space();
                        fire[k].top=PAD_POS_Y-7;
                        fire[k].left=pad_pos_x+1;
                        k=brickmania_fire_space();
                        fire[k].top=PAD_POS_Y-7;
                        fire[k].left=pad_pos_x+pad_width-1;
                    }
                    break;
#ifdef RC_QUIT
                case RC_QUIT:
#endif
                case QUIT:
                    resume = true;
                    return 0;
                    break;

                default:
                    if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                        return 1;
                    break;
            }
        }
        else {
#ifdef HAVE_LCD_COLOR
            rb->lcd_bitmap_transparent(brickmania_gameover,
                           (LCD_WIDTH - GAMEOVER_WIDTH)/2,
                           (GAMESCREEN_HEIGHT - GAMEOVER_HEIGHT)/2,
                           GAMEOVER_WIDTH,GAMEOVER_HEIGHT);
#else /* greyscale and mono */
            rb->lcd_bitmap(brickmania_gameover,(LCD_WIDTH - GAMEOVER_WIDTH)/2,
                           (GAMESCREEN_HEIGHT - GAMEOVER_HEIGHT)/2,
                           GAMEOVER_WIDTH,GAMEOVER_HEIGHT);
#endif
            rb->lcd_update();
            brickmania_sleep(2);
            return 0;
        }
        if (end > *rb->current_tick)
            rb->sleep(end-*rb->current_tick);
        else
            rb->yield();
    }
    return 0;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    int last_difficulty;

    highscore_load(HIGH_SCORE,highest,NUM_SCORES);
    configfile_load(CONFIG_FILE_NAME,config,1,0);
    last_difficulty = difficulty;

    rb->lcd_setfont(FONT_SYSFIXED);
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    /* Turn off backlight timeout */
    backlight_force_on(); /* backlight control in lib/helper.c */

    /* now go ahead and have fun! */
    rb->srand( *rb->current_tick );
    brickmania_loadgame();
    while(brickmania_game_loop() == 0) {
        if(!resume) {
            int position = highscore_update(score, level+1, "", highest, NUM_SCORES);
            if (position == 0) {
                rb->splash(HZ*2, "New High Score");
            }
            if (position != -1) {
                highscore_show(position, highest, NUM_SCORES, true);
            } else {
                brickmania_sleep(3);
            }
        }
    }

    highscore_save(HIGH_SCORE,highest,NUM_SCORES);
    if(last_difficulty != difficulty)
        configfile_save(CONFIG_FILE_NAME,config,1,0);
    /* Restore user's original backlight setting */
    rb->lcd_setfont(FONT_UI);
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(); /* backlight control in lib/helper.c */

    return PLUGIN_OK;
}
