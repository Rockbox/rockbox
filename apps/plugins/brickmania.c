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

#elif CONFIG_KEYPAD == MROBE500_PAD
#define QUIT    BUTTON_POWER

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
#include "pluginbitmaps/brickmania_bricks.h"
#include "pluginbitmaps/brickmania_powerups.h"
#include "pluginbitmaps/brickmania_ball.h"
#include "pluginbitmaps/brickmania_gameover.h"

#define PAD_WIDTH        BMPWIDTH_brickmania_pads
#define PAD_HEIGHT       (BMPHEIGHT_brickmania_pads/3)
#define BRICK_HEIGHT     (BMPHEIGHT_brickmania_bricks/7)
#define BRICK_WIDTH      BMPWIDTH_brickmania_bricks
#define LEFTMARGIN       ((LCD_WIDTH-10*BRICK_WIDTH)/2)
#define POWERUP_HEIGHT   (BMPHEIGHT_brickmania_powerups/7)
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

#define HIGHSCORE_XPOS      (LCD_WIDTH - 60)
#define HIGHSCORE_YPOS      0

#define STRINGPOS_FINISH    (LCD_HEIGHT - (LCD_HEIGHT / 6))
#define STRINGPOS_CONGRATS  (STRINGPOS_FINISH - 20)
#define STRINGPOS_NAVI      (STRINGPOS_FINISH - 10)
#define STRINGPOS_FLIP      (STRINGPOS_FINISH - 10)

#if LCD_WIDTH<=LCD_HEIGHT
/* Maintain a 4/3 ratio (Width/Height) */
#define GAMESCREEN_HEIGHT   (LCD_WIDTH * 3 / 4)
#define BMPYOFS_start       (LCD_HEIGHT / 2)
#else
#define GAMESCREEN_HEIGHT   LCD_HEIGHT
#define BMPYOFS_start       (LCD_HEIGHT / 3)
#endif

/* calculate menu item offsets from the first defined and the height*/
#define BMPYOFS_resume      (BMPYOFS_start + MENU_ITEMHEIGHT)
#define BMPYOFS_help        (BMPYOFS_start + 2*MENU_ITEMHEIGHT)
#define BMPYOFS_quit        (BMPYOFS_start + 3*MENU_ITEMHEIGHT)

/*calculate paddle y-position */
#define PAD_POS_Y           (GAMESCREEN_HEIGHT - PAD_HEIGHT - 1)


#define MARGIN 5

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
    EASY, HARD
};

int pad_pos_x;
int x[MAX_BALLS],y[MAX_BALLS];
int life;
int start_game,con_game;
int pad_type;
int score=0,vscore=0;
bool flip_sides=false;
int level=0;
int brick_on_board=0;
int used_balls=1;
bool saved_game=false;
int l_score=0;
int difficulty = EASY;

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
    int pos_x;
    int pos_y;
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

static struct configdata config[] = {
   {TYPE_INT, 0, 1, { .int_p = &difficulty }, "difficulty", NULL},
   {TYPE_BOOL, 0, 1, { .bool_p = &saved_game }, "saved_game", NULL},
   {TYPE_INT, 0, 40000, { .int_p = &l_score }, "l_score", NULL},
   {TYPE_INT, 0, 29, { .int_p = &level }, "level", NULL},
   {TYPE_INT, 0, 30, { .int_p = &life }, "life", NULL},
};

#define HIGH_SCORE PLUGIN_GAMES_DIR "/brickmania.score"
#define NUM_SCORES 5

struct highscore highest[NUM_SCORES];

static void brickmania_int_game(int new_game)
{
    int i,j;

    pad_pos_x=LCD_WIDTH/2-PAD_WIDTH/2;

    for(i=0;i<MAX_BALLS;i++) {
        ball[i].x=0;
        ball[i].y=0;
        ball[i].tempy=0;
        ball[i].tempx=0;
        ball[i].pos_y=PAD_POS_Y-BALL;
        ball[i].pos_x=pad_pos_x+(PAD_WIDTH/2)-2;
        ball[i].glue=false;
    }

    used_balls=1;
    start_game =1;
    con_game =0;
    pad_type=0;

    flip_sides=false;

    if (new_game==1) {
        brick_on_board=0;
        /* add one life per achieved level */
        if (difficulty==EASY && life<2) {
            score-=100;
            life++;
        }
    }
    for(i=0;i<=7;i++) {
        for(j=0;j<=9;j++) {
            brick[i*10+j].poweruse=(levels[level][i][j]==0?0:1);
            if (i*10+j<=30)
                fire[i*10+j].top=-8;
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

int sw,i,w;

/* brickmania_sleep timer counting the score */
static void brickmania_sleep(int secs)
{
    bool done=false;
    char s[20];
    int count=0;

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
#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
            rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 0, s);
#else
            rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 2, s);
#endif
            rb->lcd_update_rect(0,0,LCD_WIDTH,w+2);
        }
        rb->yield();
    }
}

static int brickmania_help(void)
{
    rb->lcd_setfont(FONT_UI);
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
    
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
#endif
    int button;
    if (display_text(WORDS, help_text, formation, NULL)==PLUGIN_USB_CONNECTED)
        return PLUGIN_USB_CONNECTED;
    do {
        button = rb->button_get(true);
        if (button == SYS_USB_CONNECTED) {
            return PLUGIN_USB_CONNECTED;
        }
    } while( ( button == BUTTON_NONE )
    || ( button & (BUTTON_REL|BUTTON_REPEAT) ) );
    rb->lcd_setfont(FONT_SYSFIXED);
    return 0;
}

static bool _ingame;
static int brickmania_menu_cb(int action, const struct menu_item_ex *this_item)
{
    if(action == ACTION_REQUEST_MENUITEM
       && !_ingame && ((intptr_t)this_item)==0)
        return ACTION_EXIT_MENUITEM;
    return action;
}

static int brickmania_menu(bool ingame)
{
    rb->button_clear_queue();
    int choice = 0;

    _ingame = ingame;
    
    static struct opt_items options[] = {
        { "Easy", -1 },
        { "Hard", -1 },
    };

    MENUITEM_STRINGLIST (main_menu, "Brickmania Menu", brickmania_menu_cb,
                             "Resume Game",
                             "Start New Game",
                             "Difficulty",
                             "Help",
                             "High Score",
                             "Playback Control",
                             "Quit");
                             
    while (true) {
        switch (rb->do_menu(&main_menu, &choice, NULL, false)) {
            case 0:
                if (saved_game) {
                    saved_game = false;
                    vscore=l_score-1;
                    score=l_score;
                    brickmania_int_game(1);
                } else {
                    int i;
                    for(i=0;i<used_balls;i++)
                        if (ball[i].x!=0 && ball[i].y !=0)
                            con_game=1;
                }
                return 0;
            case 1:
                score=0;
                vscore=0;
                life=2;
                level=0;
                brickmania_int_game(1);
                return 0;
            case 2:
                rb->set_option("Difficulty", &difficulty, INT, options, 2, NULL);
                break;
            case 3:
                if (brickmania_help())
                    return 1;
                break;
            case 4:
                highscore_show(NUM_SCORES, highest, NUM_SCORES);
                break;
            case 5:
                playback_control(NULL);
                break;
            case 6:
                if (level>0 && ingame) {
                    saved_game=true;
                    rb->splash(HZ*1, "Saving last achieved level ...");
                    configfile_save(CONFIG_FILE_NAME,config,5,0);
                } else {
                    saved_game=false;
                    configfile_save(CONFIG_FILE_NAME,config,1,0);
                }
                return 1;
            case MENU_ATTACHED_USB:
                return 1;
            default:
                break;
        }
    }
}

static int brickmania_pad_check(int ballxc, int mode, int pon ,int ballnum)
{
    /* pon: positive(1) or negative(0) */

    if (mode==0) {
        if (pon == 0)
            return -ballxc;
        else
            return ballxc;
    } else {
        if (ball[ballnum].x > 0)
            return ballxc;
        else
           return ballxc*-1;
    }
}

static int brickmania_fire_space(void)
{
    int t;
    for(t=0;t<=30;t++)
        if (fire[t].top+7 < 0)
            return t;

    return 0;
}

static int brickmania_game_loop(void)
{
    int j,i,k,bricky,brickx;
    char s[30];
    int sec_count=0,num_count=10;
    int end;
    int position;
    
    configfile_load(CONFIG_FILE_NAME,config,5,0);

    rb->srand( *rb->current_tick );
    if (saved_game) {
        if (brickmania_menu(true)!=0) {
            return 1;
        }
    } else {
        if (brickmania_menu(false)!=0) {
            return 1;
        }
    }

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
            rb->lcd_putsxy(0, 0, s);
#else
            rb->snprintf(s, sizeof(s), "Life: %d", life);
            rb->lcd_putsxy(2, 2, s);
#endif

#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
            rb->snprintf(s, sizeof(s), "L%d", level+1);
            rb->lcd_getstringsize(s, &sw, NULL);
            rb->lcd_putsxy(LCD_WIDTH-sw, 0, s);
#else
            rb->snprintf(s, sizeof(s), "Level %d", level+1);
            rb->lcd_getstringsize(s, &sw, NULL);
            rb->lcd_putsxy(LCD_WIDTH-sw-2, 2, s);
#endif

            if (vscore<score) vscore++;
            rb->snprintf(s, sizeof(s), "%d", vscore);
            rb->lcd_getstringsize(s, &sw, NULL);
#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
            rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 0, s);
#else
            rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 2, s);
#endif

            /* continue game */
            if (con_game== 1 && start_game!=1) {
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
            for(i=0;i<=30;i++) {
                if (fire[i].top+7>0) {
                    if (con_game!=1)
                        fire[i].top-=4;
                    rb->lcd_vline(fire[i].left, fire[i].top, fire[i].top+7);
                }
            }

            /* the bricks */
            for (i=0;i<=7;i++) {
                for (j=0;j<=9;j++) {
                    if (brick[i*10+j].power<7) {
                        if (brick[i*10+j].poweruse==2) {
                            if (con_game!=1)
                                brick[i*10+j].powertop+=2;
                            rb->lcd_bitmap_part(brickmania_powerups,0,
                                                POWERUP_HEIGHT*brick[i*10+j
                                                    ].power,
                                                POWERUP_WIDTH,
                                                LEFTMARGIN+j*BRICK_WIDTH+
                                                (BRICK_WIDTH/2-
                                                 POWERUP_WIDTH/2),
                                                brick[i*10+j].powertop,
                                                POWERUP_WIDTH,
                                                POWERUP_HEIGHT);
                        }
                    }

                    if ((pad_pos_x<LEFTMARGIN+j*BRICK_WIDTH+5 &&
                         pad_pos_x+PAD_WIDTH>LEFTMARGIN+j*BRICK_WIDTH+5) &&
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
                                    brickmania_int_game(0);
                                    brickmania_sleep(2);
                                }
                                break;
                            case 2:
                                score+=34;
                                pad_type=1;
                                break;
                            case 3:
                                score+=47;
                                pad_type=2;
                                for(k=0;k<used_balls;k++)
                                    ball[k].glue=false;
                                break;
                            case 4:
                                score+=23;
                                pad_type=0;
                                for(k=0;k<used_balls;k++)
                                    ball[k].glue=false;
                                flip_sides=false;
                                break;
                            case 5:
                                score+=23;
                                sec_count=*rb->current_tick+HZ;
                                num_count=10;
                                flip_sides=!flip_sides;
                                break;
                            case 6:
                                score+=23;
                                used_balls++;
                                ball[used_balls-1].x= rb->rand()%1 == 0 ?
                                    -1 : 1;
                                ball[used_balls-1].y= -4;
                                break;
                        }
                        brick[i*10+j].poweruse=1;
                    }

                    if (brick[i*10+j].powertop>PAD_POS_Y)
                        brick[i*10+j].poweruse=1;

                    brickx=LEFTMARGIN+j*BRICK_WIDTH;
                    bricky=TOPMARGIN+i*BRICK_HEIGHT;
                    if (pad_type==2) {
                        for (k=0;k<=30;k++) {
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
                                            BRICK_WIDTH,
                                            LEFTMARGIN+j*BRICK_WIDTH,
                                            TOPMARGIN+i*BRICK_HEIGHT,
                                            BRICK_WIDTH, BRICK_HEIGHT);
#ifdef HAVE_LCD_COLOR  /* No transparent effect for greyscale lcds for now */
                        if (brick[i*10+j].hiteffect>0)
                            rb->lcd_bitmap_transparent_part(brickmania_break,
                                                            0,
                                                            BRICK_HEIGHT*brick[i*10+j].hiteffect,
                                                            BRICK_WIDTH,
                                                            LEFTMARGIN+j*BRICK_WIDTH,
                                                            TOPMARGIN+i*BRICK_HEIGHT,
                                                            BRICK_WIDTH,
                                                            BRICK_HEIGHT);
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
            rb->lcd_bitmap_part(brickmania_pads,0,pad_type*PAD_HEIGHT,
                                PAD_WIDTH,pad_pos_x, PAD_POS_Y, PAD_WIDTH,
                                PAD_HEIGHT);

            for(k=0;k<used_balls;k++) {

                if ((ball[k].pos_x >= pad_pos_x &&
                     ball[k].pos_x <= pad_pos_x+PAD_WIDTH) &&
                    (PAD_POS_Y-4<ball[k].pos_y+BALL &&
                     PAD_POS_Y>ball[k].pos_y+BALL) && (ball[k].y >0))
                    ball[k].tempy=PAD_POS_Y-ball[k].pos_y-BALL;
                else if ((4>ball[k].pos_y && 0<ball[k].pos_y) &&
                         (ball[k].y <0))
                    ball[k].tempy=-ball[k].pos_y;
                if ((LCD_WIDTH-4<ball[k].pos_x+BALL &&
                     LCD_WIDTH>ball[k].pos_x+BALL) && (ball[k].x >0))
                    ball[k].tempx=LCD_WIDTH-ball[k].pos_x-BALL;
                else if ((4>ball[k].pos_x && 0<ball[k].pos_x) &&
                         (ball[k].x <0))
                    ball[k].tempx=-ball[k].pos_x;

                /* top line */
                if (ball[k].pos_y<= 0)
                    ball[k].y = ball[k].y*-1;
                /* bottom line */
                else if (ball[k].pos_y+BALL >= GAMESCREEN_HEIGHT) {
                    if (used_balls>1) {
                        used_balls--;
                        ball[k].pos_x = ball[used_balls].pos_x;
                        ball[k].pos_y = ball[used_balls].pos_y;
                        ball[k].y = ball[used_balls].y;
                        ball[k].tempy = ball[used_balls].tempy;
                        ball[k].x = ball[used_balls].x;
                        ball[k].tempx = ball[used_balls].tempx;
                        ball[k].glue = ball[used_balls].glue;

                        ball[used_balls].x=0;
                        ball[used_balls].y=0;
                        ball[used_balls].tempy=0;
                        ball[used_balls].tempx=0;
                        ball[used_balls].pos_y=PAD_POS_Y-BALL;
                        ball[used_balls].pos_x=pad_pos_x+(PAD_WIDTH/2)-2;

                        k--;
                        continue;
                    } else {
                        life--;
                        if (life>=0) {
                            brickmania_int_game(0);
                            brickmania_sleep(2);
                        }
                    }
                }

                /* left line ,right line */
                if ((ball[k].pos_x <= 0) ||
                    (ball[k].pos_x+BALL >= LCD_WIDTH)) {
                    ball[k].x = ball[k].x*-1;
                    ball[k].pos_x = ball[k].pos_x <= 0 ? 0 : LCD_WIDTH-BALL;
                }

                if ((ball[k].pos_y+BALL >= PAD_POS_Y &&
                     (ball[k].pos_x >= pad_pos_x &&
                      ball[k].pos_x <= pad_pos_x+PAD_WIDTH)) &&
                    start_game != 1 && !ball[k].glue) {

                    if ((ball[k].pos_x+HALFBALL >= pad_pos_x &&
                         ball[k].pos_x+HALFBALL <=
                         pad_pos_x+(PAD_WIDTH/2/4)) ||
                        (ball[k].pos_x +HALFBALL>=
                         pad_pos_x+(PAD_WIDTH-(PAD_WIDTH/2/4)) &&
                         ball[k].pos_x+HALFBALL <= pad_pos_x+PAD_WIDTH)) {

                        ball[k].y = -2;
                        if (ball[k].pos_x != 0 &&
                            ball[k].pos_x+BALL!=LCD_WIDTH)
                            ball[k].x = brickmania_pad_check(6,0,ball[k].pos_x+2<=
                                                  pad_pos_x+(PAD_WIDTH/2)?
                                                  0:1,k);

                    }
                    else if ((ball[k].pos_x+HALFBALL >=
                              pad_pos_x+(PAD_WIDTH/2/4) &&
                              ball[k].pos_x+HALFBALL <=
                              pad_pos_x+2*(PAD_WIDTH/2/4)) ||
                             (ball[k].pos_x+HALFBALL >=
                              pad_pos_x+(PAD_WIDTH-2*(PAD_WIDTH/2/4)) &&
                              ball[k].pos_x+HALFBALL <=
                              pad_pos_x+(PAD_WIDTH-(PAD_WIDTH/2/4)) )) {

                        ball[k].y = -3;
                        if (ball[k].pos_x != 0 &&
                            ball[k].pos_x+BALL!=LCD_WIDTH)
                            ball[k].x = brickmania_pad_check(4,0,ball[k].pos_x+2<=
                                                  pad_pos_x+(PAD_WIDTH/2)?
                                                  0:1,k);

                    }
                    else if ((ball[k].pos_x+HALFBALL >=
                              pad_pos_x+2*(PAD_WIDTH/2/4) &&
                              ball[k].pos_x+HALFBALL <=
                              pad_pos_x+3*(PAD_WIDTH/2/4)) ||
                             (ball[k].pos_x+2 >=
                              pad_pos_x+(PAD_WIDTH-3*(PAD_WIDTH/2/4)) &&
                              ball[k].pos_x+2 <=
                              pad_pos_x+ ((PAD_WIDTH/2)-2*(PAD_WIDTH/2/4)) )) {

                        ball[k].y = -4;
                        if (ball[k].pos_x != 0 &&
                            ball[k].pos_x+BALL!=LCD_WIDTH)
                            ball[k].x = brickmania_pad_check(3,0,ball[k].pos_x+2<=
                                                  pad_pos_x+(PAD_WIDTH/2)?
                                                  0:1,k);

                    }
                    else if ((ball[k].pos_x+HALFBALL >=
                              pad_pos_x+3*(PAD_WIDTH/2/4) &&
                              ball[k].pos_x+HALFBALL <=
                              pad_pos_x+4*(PAD_WIDTH/2/4)-2) ||
                             (ball[k].pos_x+2 >= pad_pos_x+(PAD_WIDTH/2+2) &&
                              ball[k].pos_x+2 <=
                              pad_pos_x+(PAD_WIDTH-3*(PAD_WIDTH/2/4)) )) {

                        ball[k].y = -4;
                        if (ball[k].pos_x != 0 &&
                            ball[k].pos_x+BALL!=LCD_WIDTH)
                            ball[k].x = brickmania_pad_check(2,1,0,k);

                    }
                    else {
                        ball[k].y = -4;
                    }
                }

                if (!ball[k].glue) {
                    ball[k].pos_x+=ball[k].tempx!=0?ball[k].tempx:ball[k].x;
                    ball[k].pos_y+=ball[k].tempy!=0?ball[k].tempy:ball[k].y;

                    ball[k].tempy=0;
                    ball[k].tempx=0;
                }

                if (ball[k].pos_y+5 >= PAD_POS_Y &&
                    (pad_type==1 && !ball[k].glue) &&
                    (ball[k].pos_x >= pad_pos_x &&
                     ball[k].pos_x <= pad_pos_x+PAD_WIDTH)) {
                    ball[k].y=0;
                    ball[k].pos_y=PAD_POS_Y-BALL;
                    ball[k].glue=true;
                }
            } /* for k */

            rb->lcd_update();

            if (brick_on_board < 0) {
                if (level+1<levels_num) {
                    level++;
                    if (difficulty==HARD)
                        score+=100;
                    l_score=score;
                    brickmania_int_game(1);
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
                    rb->lcd_clear_display();
                    rb->lcd_update();
                    rb->sleep(2);
                    position=highscore_update(score, level+1, "",
                                            highest,NUM_SCORES);            
                    if (position == 0) {
                        rb->splash(HZ*2, "New High Score");
                    }
                    if (position != -1) {
                        highscore_show(position, highest, NUM_SCORES);
                    } else {
                        brickmania_sleep(3);
                    }
                    if (brickmania_menu(false)!=0) {
                        return 0;
                    }
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
                    pad_pos_x += (flip_sides ? -1 : 1) * ( (touch_x-pad_pos_x-PAD_WIDTH/2) / 4 );
                    
                    if(pad_pos_x < 0)
                        pad_pos_x = 0;
                    else if(pad_pos_x+PAD_WIDTH > LCD_WIDTH)
                        pad_pos_x = LCD_WIDTH-PAD_WIDTH;
                    for(k=0;k<used_balls;k++)
                        if ((start_game==1 || ball[k].glue))
                            ball[k].pos_x = pad_pos_x+PAD_WIDTH/2;
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
                if ((con_game== 1 && start_game!=1) && (button_right || button_left))
                    continue;
                if ((button_right && flip_sides==false) ||
                    (button_left && flip_sides==true)) {
                    if (pad_pos_x+8+PAD_WIDTH > LCD_WIDTH) {
                        for(k=0;k<used_balls;k++)
                            if (start_game==1 || ball[k].glue)
                                ball[k].pos_x+=LCD_WIDTH-pad_pos_x-PAD_WIDTH;
                        pad_pos_x+=LCD_WIDTH-pad_pos_x-PAD_WIDTH;
                    }
                    else {
                        for(k=0;k<used_balls;k++)
                            if ((start_game==1 || ball[k].glue))
                                ball[k].pos_x+=8;
                        pad_pos_x+=8;
                    }
                }
                else if ((button_left && flip_sides==false) ||
                         (button_right && flip_sides==true)) {
                    if (pad_pos_x-8 < 0) {
                        for(k=0;k<used_balls;k++)
                            if (start_game==1 || ball[k].glue)
                                ball[k].pos_x-=pad_pos_x;
                        pad_pos_x-=pad_pos_x;
                    }
                    else {
                        for(k=0;k<used_balls;k++)
                            if (start_game==1 || ball[k].glue)
                                ball[k].pos_x-=8;
                        pad_pos_x-=8;
                    }
                }
#ifdef HAVE_TOUCHSCREEN
            }
#endif


            switch(button) {
                case UP:
                case SELECT:
                    if (start_game==1 && con_game!=1 && pad_type!=1) {
                        for(k=0;k<used_balls;k++) {
                            ball[k].y=-4;
                            ball[k].x=pad_pos_x+(PAD_WIDTH/2)-2>=
                                LCD_WIDTH/2?2:-2;
                        }
                        start_game =0;
                    }
                    else if (pad_type==1) {
                        for(k=0;k<used_balls;k++) {
                            if (ball[k].glue)
                                ball[k].glue=false;
                            else if (start_game==1) {
                                ball[k].x = x[k];
                                ball[k].y = y[k];
                            }
                        }

                        if (start_game!=1 && con_game==1) {
                            start_game =0;
                            con_game=0;
                        }
                    } else if (pad_type==2 && con_game!=1) {
                        int tfire;
                        tfire=brickmania_fire_space();
                        fire[tfire].top=PAD_POS_Y-7;
                        fire[tfire].left=pad_pos_x+1;
                        tfire=brickmania_fire_space();
                        fire[tfire].top=PAD_POS_Y-7;
                        fire[tfire].left=pad_pos_x+PAD_WIDTH-1;
                    } else if (con_game==1 && start_game!=1) {
                        for(k=0;k<used_balls;k++) {
                            ball[k].x=x[k];
                            ball[k].y=y[k];
                        }
                        con_game=0;
                    }
                    break;
#ifdef RC_QUIT
                case RC_QUIT:
#endif
                case QUIT:
                    if (brickmania_menu(true)!=0) {
                        return 1;
                    }

                    for(k=0;k<used_balls;k++) {
                        if (ball[k].x!=0)
                            x[k]=ball[k].x;
                        ball[k].x=0;
                        if (ball[k].y!=0)
                            y[k]=ball[k].y;
                        ball[k].y=0;
                    }

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
            position=highscore_update(score, level+1, "", highest, NUM_SCORES);            
            if (position == 0) {
                rb->splash(HZ*2, "New High Score");
            }
            if (position != -1) {
                highscore_show(position, highest, NUM_SCORES);
            } else {
                brickmania_sleep(3);
            }

            for(k=0;k<used_balls;k++) {
                ball[k].x=0;
                ball[k].y=0;
            }

            if (brickmania_menu(false)!=0) {
                return 0;
            }
        }
        if (end > *rb->current_tick)
            rb->sleep(end-*rb->current_tick);
        else
            rb->yield();
    }
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

    highscore_load(HIGH_SCORE,highest,NUM_SCORES);
    rb->lcd_setfont(FONT_SYSFIXED);
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    /* Turn off backlight timeout */
    backlight_force_on(); /* backlight control in lib/helper.c */

    /* now go ahead and have fun! */
    brickmania_game_loop();

    highscore_save(HIGH_SCORE,highest,NUM_SCORES);
    /* Restore user's original backlight setting */
    rb->lcd_setfont(FONT_UI);
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(); /* backlight control in lib/helper.c */

    return PLUGIN_OK;
}
