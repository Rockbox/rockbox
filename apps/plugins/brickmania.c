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
 * Copyright (C) 2009 Karl Kurbjun
 *  check_lines is based off an explanation and expanded math presented by Paul
 *   Bourke: http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/
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

#include "pluginbitmaps/brickmania_pads.h"
#include "pluginbitmaps/brickmania_short_pads.h"
#include "pluginbitmaps/brickmania_long_pads.h"
#include "pluginbitmaps/brickmania_bricks.h"
#include "pluginbitmaps/brickmania_powerups.h"
#include "pluginbitmaps/brickmania_ball.h"
#include "pluginbitmaps/brickmania_gameover.h"

#ifdef HAVE_LCD_COLOR /* currently no transparency for non-colour */
#include "pluginbitmaps/brickmania_break.h"
#endif

PLUGIN_HEADER


/*
 *
 * Keymaps
 *
 */

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define CONTINUE_TEXT "Press NAVI To Continue"
#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN
#define RC_QUIT BUTTON_RC_STOP

#elif CONFIG_KEYPAD == ONDIO_PAD
#define CONTINUE_TEXT "MENU To Continue"
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
#define CONTINUE_TEXT "PLAY To Continue"
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

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define QUIT BUTTON_POWER
#define LEFT BUTTON_PREV
#define RIGHT BUTTON_NEXT
#define SELECT BUTTON_PLAY
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == COWON_D2_PAD
#define CONTINUE_TEXT "MENU To Continue"
#define QUIT    BUTTON_POWER
#define LEFT    BUTTON_MINUS
#define RIGHT   BUTTON_PLUS
#define SELECT  BUTTON_MENU

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

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define QUIT     BUTTON_REC
#define LEFT     BUTTON_PREV
#define RIGHT    BUTTON_NEXT
#define ALTLEFT  BUTTON_MENU
#define ALTRIGHT BUTTON_PLAY
#define SELECT   BUTTON_OK
#define UP       BUTTON_UP
#define DOWN     BUTTON_DOWN

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifdef LEFT
#define ALTLEFT   BUTTON_BOTTOMLEFT
#else
#define LEFT      BUTTON_BOTTOMLEFT
#endif
#ifdef RIGHT
#define ALTRIGHT  BUTTON_BOTTOMRIGHT
#else
#define RIGHT     BUTTON_BOTTOMRIGHT
#endif
#ifdef SELECT
#define ALTSELECT BUTTON_CENTER
#else
#define SELECT    BUTTON_CENTER
#endif
#ifndef UP
#define UP        BUTTON_TOPMIDDLE
#endif
#ifndef DOWN
#define DOWN      BUTTON_BOTTOMMIDDLE
#endif
#endif

/* Continue text is used as a string later when the game is paused.  This allows
 *  targets to specify their own text if needed.
 */
#if !defined(CONTINUE_TEXT)
#define CONTINUE_TEXT "Press SELECT To Continue"
#endif

#ifndef SCROLL_FWD /* targets without scroll wheel*/
#define SCROLL_FWD(x) (0)
#define SCROLL_BACK(x) (0)
#endif


/*
 *
 * Geometric dimensions
 *
 */

/* If there are three fractional bits, the smallest screen size that will scale
 * properly is 28x22.  If you have a smaller screen increase the fractional
 * precision.  If you have a precision of 4 the smallest screen size would be
 * 14x11.  Note though that this will decrease the maximum resolution due to
 * the line intersection tests.  These defines are used for all of the fixed
 * point calculations/conversions.
 */
#define FIXED3(x)           ((x)<<3)
#define FIXED3_MUL(x, y)    ((long long)((x)*(y))>>3)
#define FIXED3_DIV(x, y)    (((x)<<3)/(y))
#define INT3(x)             ((x)>>3)

#define GAMESCREEN_WIDTH    FIXED3(LCD_WIDTH)
#define GAMESCREEN_HEIGHT   FIXED3(LCD_HEIGHT)

#define PAD_WIDTH        FIXED3(BMPWIDTH_brickmania_pads)
#define PAD_HEIGHT       FIXED3(BMPHEIGHT_brickmania_pads/3)
#define SHORT_PAD_WIDTH  FIXED3(BMPWIDTH_brickmania_short_pads)
#define LONG_PAD_WIDTH   FIXED3(BMPWIDTH_brickmania_long_pads)
#define BRICK_HEIGHT     FIXED3(BMPHEIGHT_brickmania_bricks/7)
#define BRICK_WIDTH      FIXED3(BMPWIDTH_brickmania_bricks)
#define LEFTMARGIN       ((GAMESCREEN_WIDTH-10*BRICK_WIDTH)/2)
#define POWERUP_WIDTH    FIXED3(BMPWIDTH_brickmania_powerups)
#define BALL             FIXED3(BMPHEIGHT_brickmania_ball)
#define HALFBALL         (BALL / 2)
#define PAD_POS_Y        (GAMESCREEN_HEIGHT - PAD_HEIGHT - 1)
#define ON_PAD_POS_Y     (PAD_POS_Y - HALFBALL)

#define GAMEOVER_WIDTH   FIXED3(BMPWIDTH_brickmania_gameover)
#define GAMEOVER_HEIGHT  FIXED3(BMPHEIGHT_brickmania_gameover)

#define TOPMARGIN           MAX(BRICK_HEIGHT, FIXED3(8))

#define STRINGPOS_FINISH    (GAMESCREEN_HEIGHT - (GAMESCREEN_HEIGHT / 6))
#define STRINGPOS_CONGRATS  (STRINGPOS_FINISH - 20)
#define STRINGPOS_NAVI      (STRINGPOS_FINISH - 10)
#define STRINGPOS_FLIP      (STRINGPOS_FINISH - 10)


/*
 *
 * Speeds
 *
 */

/* Brickmania was originally designed for the H300, other targets should scale
 *  the speed up/down as needed based on the screen height.
 */
#define SPEED_SCALE_H(y)    FIXED3_DIV(GAMESCREEN_HEIGHT, FIXED3(176)/(y) )
#define SPEED_SCALE_W(x)    FIXED3_DIV(GAMESCREEN_WIDTH, FIXED3(220)/(x) )

/* These are all used as ball speeds depending on where the ball hit the
 *  paddle.
 *
 * Note that all of these speeds (including pad, power, and fire)
 *  could be made variable and could be raised to be much higher to add
 *  additional difficulty to the game.  The line intersection tests allow this
 *  to be drastically increased without the collision detection failing
 *  (ideally).
 */
#define SPEED_1Q_X  SPEED_SCALE_W( 6)
#define SPEED_1Q_Y  SPEED_SCALE_H(-2)

#define SPEED_2Q_X  SPEED_SCALE_W( 4)
#define SPEED_2Q_Y  SPEED_SCALE_H(-3)

#define SPEED_3Q_X  SPEED_SCALE_W( 3)
#define SPEED_3Q_Y  SPEED_SCALE_H(-4)

#define SPEED_4Q_X  SPEED_SCALE_W( 2)
#define SPEED_4Q_Y  SPEED_SCALE_H(-4)

/* This is used to determine the speed of the paddle */
#define SPEED_PAD   SPEED_SCALE_W( 8)

/* This defines the speed that the powerups drop */
#define SPEED_POWER SPEED_SCALE_H( 2)

/* This defines the speed that the shot moves */
#define SPEED_FIRE  SPEED_SCALE_H( 4)
#define FIRE_LENGTH SPEED_SCALE_H( 7)


/*
 *
 * Timings
 *
 */

/* The time ms for one iteration through the game loop - decrease this to speed
 * up the game - note that current_tick is (currently) only accurate to 10ms.
 */
#define CYCLETIME           30 /* ms */

#define FLIP_SIDES_DELAY    10 /* seconds */


/*
 *
 * Scores
 *
 */

#define SCORE_BALL_HIT_BRICK          2
#define SCORE_BALL_DEMOLISHED_BRICK   8
#define SCORE_FIRE_HIT_BRICK         13
#define SCORE_LEVEL_COMPLETED       100
#define SCORE_POWER_LIFE_GAIN        50
#define SCORE_POWER_PADDLE_STICKY    34
#define SCORE_POWER_PADDLE_SHOOTER   47
#define SCORE_POWER_PADDLE_NORMAL    23
#define SCORE_POWER_FLIP             23
#define SCORE_POWER_EXTRA_BALL       23
#define SCORE_POWER_LONG_PADDLE      23


/*
 *
 * Limits
 *
 */

#define MAX_BALLS 10
#define MAX_FIRES 30
#define MAX_POWERS 10


/*
 *
 * Files
 *
 */

#define CONFIG_FILE_NAME "brickmania.cfg"
#define SAVE_FILE  PLUGIN_GAMES_DIR "/brickmania.save"
#define SCORE_FILE PLUGIN_GAMES_DIR "/brickmania.score"
#define NUM_SCORES 5


/*
 *
 * Game levels
 *
 */

#define NUM_BRICKS_ROWS 8
#define NUM_BRICKS_COLS 10

/* change to however many levels there are, i.e. how many arrays there are total */
#define NUM_LEVELS 40

/* change the first number in [ ] to however many levels there are */
static unsigned char levels[NUM_LEVELS][NUM_BRICKS_ROWS][NUM_BRICKS_COLS] =
/* You can set up new levels with the level editor
   ( http://plugbox.rockbox-lounge.com/brickmania.htm ).
   With 0x1, it refers to the first brick in the bitmap, 0x2 would refer to the
   second, ect., 0x0 leaves a empty space. If you add a 2 before the 2nd number,
   it will take two hits to break, and 3 hits if you add a 3. That is 0x24, will
   result with the fourth brick being displayed and having take 2 hits to break.
   You could do the same with the 3, just replace the 2 with a 3 for it to take
   three hits to break it apart. */
{
    { /* level 1 */
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x2,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x2},
        {0x0,0x2,0x1,0x0,0x0,0x0,0x0,0x1,0x2,0x0},
        {0x0,0x0,0x2,0x1,0x0,0x0,0x1,0x2,0x0,0x0},
        {0x0,0x0,0x0,0x2,0x1,0x1,0x2,0x0,0x0,0x0},
        {0x7,0x0,0x0,0x7,0x2,0x2,0x7,0x0,0x0,0x7},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    },
    { /* level 2 */
        {0x0,0x0,0x7,0x7,0x1,0x1,0x7,0x7,0x0,0x0},
        {0x0,0x1,0x0,0x0,0x1,0x1,0x0,0x0,0x1,0x0},
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x1,0x1,0x1,0x1,0x0,0x0,0x1,0x1,0x1,0x1},
        {0x1,0x1,0x2,0x1,0x0,0x0,0x1,0x2,0x1,0x1},
        {0x1,0x2,0x0,0x2,0x2,0x2,0x2,0x0,0x2,0x1},
        {0x0,0x1,0x2,0x0,0x0,0x0,0x0,0x2,0x1,0x0},
        {0x0,0x0,0x1,0x2,0x2,0x2,0x2,0x1,0x0,0x0}
    },
    { /* level 3 */
        {0x3,0x3,0x3,0x3,0x0,0x0,0x2,0x2,0x2,0x2},
        {0x3,0x23,0x23,0x3,0x0,0x0,0x2,0x22,0x22,0x2},
        {0x3,0x3,0x3,0x3,0x0,0x0,0x2,0x2,0x2,0x2},
        {0x0,0x0,0x0,0x0,0x37,0x37,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x37,0x37,0x0,0x0,0x0,0x0},
        {0x5,0x5,0x5,0x5,0x0,0x0,0x6,0x6,0x6,0x6},
        {0x5,0x25,0x25,0x5,0x0,0x0,0x6,0x26,0x26,0x6},
        {0x5,0x5,0x5,0x5,0x0,0x0,0x6,0x6,0x6,0x6}
    },
    { /* level 4 */
        {0x0,0x0,0x0,0x27,0x27,0x27,0x27,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x27,0x7,0x7,0x27,0x0,0x0,0x0},
        {0x22,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x22},
        {0x22,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x22},
        {0x26,0x6,0x0,0x2,0x2,0x2,0x2,0x0,0x6,0x26},
        {0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0},
        {0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0},
        {0x1,0x1,0x1,0x1,0x0,0x0,0x1,0x1,0x1,0x1}
    },
    { /* level 5 */
        {0x1,0x0,0x2,0x2,0x0,0x3,0x3,0x0,0x4,0x4},
        {0x0,0x2,0x2,0x0,0x3,0x3,0x0,0x4,0x4,0x0},
        {0x2,0x2,0x0,0x3,0x3,0x0,0x4,0x4,0x0,0x5},
        {0x2,0x0,0x3,0x3,0x0,0x4,0x4,0x0,0x5,0x5},
        {0x0,0x33,0x3,0x0,0x4,0x4,0x0,0x5,0x5,0x0},
        {0x3,0x33,0x0,0x4,0x4,0x0,0x5,0x5,0x0,0x36},
        {0x3,0x0,0x4,0x4,0x0,0x5,0x5,0x0,0x6,0x36},
        {0x0,0x24,0x24,0x0,0x25,0x25,0x0,0x26,0x26,0x0}
    },
    { /* level 6 */
        {0x0,0x1,0x3,0x7,0x7,0x7,0x7,0x3,0x1,0x0},
        {0x3,0x1,0x3,0x7,0x0,0x0,0x7,0x3,0x1,0x3},
        {0x3,0x1,0x3,0x7,0x7,0x7,0x7,0x3,0x1,0x3},
        {0x0,0x0,0x0,0x2,0x2,0x2,0x2,0x0,0x0,0x0},
        {0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5},
        {0x5,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x5},
        {0x0,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    },
    { /* level 7 */
        {0x0,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x6,0x0,0x0,0x2,0x2,0x2,0x2,0x0,0x0,0x6},
        {0x6,0x0,0x0,0x2,0x2,0x2,0x2,0x0,0x0,0x6},
        {0x6,0x6,0x6,0x0,0x0,0x0,0x0,0x6,0x6,0x6},
        {0x0,0x0,0x0,0x1,0x1,0x1,0x1,0x0,0x0,0x0},
        {0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    },
    { /* level 8 */
        {0x0,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x0},
        {0x0,0x0,0x0,0x4,0x0,0x0,0x4,0x0,0x0,0x0},
        {0x6,0x6,0x0,0x2,0x32,0x32,0x2,0x0,0x6,0x6},
        {0x0,0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x0,0x0},
        {0x0,0x6,0x6,0x0,0x0,0x0,0x0,0x6,0x6,0x0},
        {0x0,0x0,0x0,0x5,0x25,0x25,0x5,0x0,0x0,0x0},
        {0x0,0x5,0x5,0x25,0x5,0x5,0x25,0x5,0x5,0x0},
        {0x5,0x5,0x25,0x5,0x5,0x5,0x5,0x25,0x5,0x5}
    },
    { /* level 9 */
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2},
        {0x2,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x3,0x2},
        {0x2,0x0,0x3,0x0,0x1,0x1,0x0,0x3,0x0,0x2},
        {0x2,0x0,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x2},
        {0x2,0x0,0x1,0x0,0x3,0x3,0x0,0x1,0x0,0x2},
        {0x2,0x0,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x2},
        {0x2,0x2,0x0,0x0,0x1,0x1,0x0,0x0,0x2,0x2},
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2}
    },
    { /* level 10 */
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x5,0x0,0x5,0x0,0x5,0x0,0x5,0x0,0x5},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x1,0x1,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x4,0x1,0x1,0x4,0x0,0x0,0x0},
        {0x0,0x0,0x3,0x4,0x1,0x1,0x4,0x3,0x0,0x0},
        {0x0,0x2,0x3,0x4,0x1,0x1,0x4,0x3,0x2,0x0},
        {0x1,0x2,0x3,0x4,0x1,0x1,0x4,0x3,0x2,0x1}
    },
    { /* level 11 */
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
    {/* level 14 */
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
    { /* level 29 UK Flag by Seth Opgenorth */
        {0x32,0x0,0x3,0x3,0x2,0x2,0x3,0x3,0x0,0x32},
        {0x0,0x2,0x0,0x3,0x32,0x22,0x33,0x0,0x32,0x0},
        {0x33,0x0,0x22,0x0,0x2,0x2,0x0,0x2,0x0,0x33},
        {0x22,0x32,0x2,0x2,0x2,0x2,0x2,0x2,0x22,0x2},
        {0x3,0x0,0x0,0x32,0x22,0x2,0x2,0x0,0x0,0x3},
        {0x23,0x0,0x32,0x0,0x32,0x2,0x0,0x2,0x0,0x3},
        {0x0,0x2,0x0,0x3,0x2,0x2,0x3,0x0,0x22,0x0},
        {0x32,0x0,0x3,0x23,0x2,0x2,0x23,0x33,0x0,0x32}
    },
    { /* level 30 Win-Logo by Seth Opgenorth */
        {0x0,0x0,0x0,0x22,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x32,0x2,0x2,0x25,0x0,0x5,0x0,0x0},
        {0x0,0x0,0x2,0x22,0x2,0x5,0x5,0x35,0x0,0x0},
        {0x0,0x0,0x2,0x1,0x2,0x5,0x5,0x25,0x0,0x0},
        {0x0,0x0,0x21,0x1,0x1,0x36,0x7,0x26,0x0,0x0},
        {0x0,0x0,0x1,0x1,0x1,0x6,0x6,0x6,0x0,0x0},
        {0x0,0x0,0x21,0x0,0x21,0x6,0x6,0x26,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x6,0x0,0x0,0x0}
    },
    { /* level 31 Color wave/V by Seth Opgenorth */
        {0x25,0x34,0x2,0x31,0x33,0x23,0x1,0x2,0x34,0x5},
        {0x3,0x5,0x24,0x2,0x1,0x1,0x2,0x4,0x5,0x3},
        {0x1,0x3,0x5,0x4,0x2,0x2,0x4,0x5,0x3,0x1},
        {0x2,0x1,0x33,0x35,0x4,0x4,0x5,0x33,0x1,0x22},
        {0x31,0x22,0x1,0x3,0x5,0x25,0x3,0x1,0x2,0x31},
        {0x3,0x1,0x2,0x1,0x3,0x3,0x1,0x2,0x21,0x3},
        {0x5,0x23,0x1,0x32,0x1,0x1,0x2,0x1,0x3,0x5},
        {0x4,0x35,0x3,0x1,0x2,0x22,0x31,0x3,0x5,0x4}
    },
    { /* level 32 Sweedish Flag by Seth Opgenorth */
        {0x3,0x3,0x3,0x36,0x3,0x3,0x3,0x3,0x3,0x3},
        {0x3,0x3,0x3,0x36,0x3,0x3,0x3,0x3,0x3,0x3},
        {0x3,0x3,0x3,0x36,0x3,0x3,0x3,0x3,0x3,0x3},
        {0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36},
        {0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36},
        {0x3,0x3,0x3,0x36,0x3,0x3,0x3,0x3,0x3,0x3},
        {0x3,0x3,0x3,0x36,0x3,0x3,0x3,0x3,0x3,0x3},
        {0x3,0x3,0x3,0x36,0x3,0x3,0x3,0x3,0x3,0x3}
    },
    { /* level 33 Color Pyramid by Seth Opgenorth */
        {0x0,0x0,0x0,0x0,0x2,0x2,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x4,0x24,0x4,0x24,0x0,0x0,0x0},
        {0x0,0x0,0x23,0x3,0x3,0x3,0x23,0x3,0x0,0x0},
        {0x0,0x0,0x25,0x5,0x25,0x35,0x5,0x35,0x0,0x0},
        {0x0,0x36,0x6,0x6,0x6,0x6,0x26,0x6,0x6,0x0},
        {0x0,0x7,0x7,0x7,0x7,0x25,0x27,0x7,0x27,0x0},
        {0x2,0x2,0x22,0x2,0x2,0x2,0x22,0x2,0x2,0x2},
        {0x21,0x1,0x1,0x31,0x1,0x21,0x1,0x1,0x31,0x1}
    },
    { /* level 34 Rhombus by Seth Opgenorth */
        {0x0,0x0,0x0,0x0,0x3,0x33,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x3,0x32,0x22,0x23,0x0,0x0,0x0},
        {0x0,0x0,0x3,0x2,0x24,0x4,0x2,0x23,0x0,0x0},
        {0x26,0x3,0x2,0x4,0x5,0x5,0x4,0x22,0x3,0x6},
        {0x36,0x3,0x2,0x34,0x5,0x5,0x4,0x2,0x3,0x36},
        {0x0,0x0,0x3,0x2,0x4,0x34,0x2,0x23,0x0,0x0},
        {0x0,0x0,0x0,0x23,0x2,0x2,0x3,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x3,0x33,0x0,0x0,0x0,0x0}
    },
    { /* level 35 PacMan Ghost by Seth Opgenorth */
        {0x0,0x0,0x0,0x0,0x2,0x32,0x2,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x2,0x2,0x2,0x2,0x2,0x0,0x0},
        {0x0,0x0,0x2,0x24,0x4,0x2,0x4,0x4,0x32,0x0},
        {0x0,0x0,0x2,0x24,0x0,0x22,0x24,0x0,0x22,0x0},
        {0x0,0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x0},
        {0x0,0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x0},
        {0x0,0x0,0x2,0x32,0x2,0x2,0x22,0x2,0x32,0x0},
        {0x0,0x0,0x0,0x22,0x0,0x32,0x0,0x22,0x0,0x0}
    },
    { /* level 36 Star by Seth Opgenorth */
        {0x3,0x4,0x3,0x4,0x6,0x24,0x3,0x24,0x3,0x0},
        {0x24,0x0,0x0,0x6,0x6,0x6,0x0,0x0,0x4,0x0},
        {0x3,0x26,0x6,0x2,0x6,0x2,0x6,0x26,0x23,0x0},
        {0x4,0x0,0x6,0x6,0x36,0x6,0x6,0x0,0x4,0x0},
        {0x3,0x0,0x0,0x26,0x6,0x26,0x0,0x0,0x33,0x0},
        {0x34,0x0,0x6,0x6,0x0,0x6,0x6,0x0,0x4,0x0},
        {0x3,0x26,0x6,0x0,0x0,0x0,0x6,0x6,0x3,0x0},
        {0x4,0x3,0x4,0x23,0x24,0x3,0x4,0x33,0x4,0x0}
    },
    { /* level 37 It's 8-Bit by Seth Opgenorth */
        {0x26,0x26,0x6,0x6,0x5,0x6,0x26,0x6,0x26,0x6},
        {0x2,0x2,0x22,0x3,0x3,0x0,0x0,0x0,0x4,0x0},
        {0x2,0x0,0x2,0x33,0x3,0x3,0x5,0x0,0x24,0x0},
        {0x32,0x2,0x2,0x33,0x0,0x23,0x0,0x4,0x4,0x4},
        {0x2,0x22,0x2,0x3,0x3,0x0,0x5,0x4,0x4,0x24},
        {0x2,0x0,0x2,0x23,0x0,0x3,0x25,0x0,0x4,0x0},
        {0x22,0x2,0x2,0x3,0x23,0x0,0x5,0x0,0x4,0x0},
        {0x6,0x26,0x6,0x36,0x6,0x36,0x6,0x6,0x6,0x6}
    },
    { /* level 38 Linux by Seth Opgenorth */
        {0x27,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x7,0x0,0x0,0x0,0x33,0x0,0x23,0x0,0x0,0x0},
        {0x7,0x32,0x0,0x0,0x3,0x0,0x23,0x6,0x0,0x6},
        {0x37,0x0,0x0,0x0,0x23,0x0,0x3,0x6,0x0,0x26},
        {0x7,0x22,0x24,0x0,0x3,0x33,0x3,0x0,0x26,0x0},
        {0x37,0x22,0x24,0x24,0x4,0x0,0x0,0x0,0x26,0x0},
        {0x7,0x2,0x4,0x0,0x4,0x0,0x0,0x6,0x0,0x26},
        {0x7,0x27,0x4,0x0,0x34,0x0,0x0,0x6,0x0,0x26}
    },
    { /* level 39 Colorful Squares by Seth Opgenorth*/
        {0x0,0x4,0x4,0x4,0x4,0x4,0x4,0x4,0x4,0x0},
        {0x0,0x4,0x5,0x5,0x5,0x5,0x5,0x5,0x4,0x0},
        {0x0,0x4,0x5,0x3,0x3,0x3,0x3,0x5,0x4,0x0},
        {0x0,0x4,0x5,0x3,0x4,0x4,0x3,0x5,0x4,0x0},
        {0x0,0x4,0x5,0x3,0x4,0x4,0x3,0x5,0x4,0x0},
        {0x0,0x4,0x5,0x3,0x3,0x3,0x3,0x5,0x4,0x0},
        {0x0,0x4,0x5,0x5,0x5,0x5,0x5,0x5,0x4,0x0},
        {0x0,0x4,0x4,0x4,0x4,0x4,0x4,0x4,0x4,0x0}
    },
    { /* TheEnd */
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x32,0x32,0x36,0x0,0x0,0x36,0x34,0x34,0x0,0x0},
        {0x32,0x0,0x36,0x36,0x0,0x36,0x34,0x0,0x34,0x0},
        {0x32,0x32,0x36,0x36,0x0,0x36,0x34,0x0,0x34,0x0},
        {0x32,0x32,0x36,0x0,0x36,0x36,0x34,0x0,0x34,0x0},
        {0x32,0x0,0x36,0x0,0x36,0x36,0x34,0x0,0x34,0x0},
        {0x32,0x32,0x36,0x0,0x0,0x36,0x34,0x34,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    }
};


/*
 *
 * Enums and structs
 *
 */

enum power_types
{
    POWER_TYPE_LIFE_GAIN = 0,
    POWER_TYPE_LIFE_LOSS,
    POWER_TYPE_PADDLE_STICKY,
    POWER_TYPE_PADDLE_SHOOTER,
    POWER_TYPE_PADDLE_NORMAL,
    POWER_TYPE_PADDLE_FLIP,
    POWER_TYPE_EXTRA_BALL,
    POWER_TYPE_PADDLE_LONG,
    POWER_TYPE_PADDLE_SHORT,
    NUMBER_OF_POWERUPS,
};

#define POWERUP_HEIGHT   FIXED3(BMPHEIGHT_brickmania_powerups/NUMBER_OF_POWERUPS)
/* Increasing this value makes the game with less powerups */
#define POWER_RAND       (NUMBER_OF_POWERUPS + 15)

enum difficulty_options
{
    EASY,
    NORMAL
};

enum game_state
{
    ST_READY,
    ST_START,
    ST_PAUSE
};

enum paddle_type
{
    PADDLE_TYPE_NORMAL = 0,
    PADDLE_TYPE_STICKY,
    PADDLE_TYPE_SHOOTER,
};

enum intersection
{
    INTERSECTION_TOP,
    INTERSECTION_BOTTOM,
    INTERSECTION_LEFT,
    INTERSECTION_RIGHT,
    INTERSECTION_ALL,
};

struct brick
{
    bool used;      /* Is the brick still in play? */
    int color;
    int hits;       /* How many hits can this brick take? */
    int hiteffect;
};

struct ball
{
    /* pos_x and y store the current center position of the ball */
    int pos_x;
    int pos_y;
    /* tempx and tempy store an absolute position the ball should be in.  If
     *  they are equal to 0, they are not used when positioning the ball.
     */
    int tempx;
    int tempy;
    /* speedx and speedy store the current speed of the ball */
    int speedx;
    int speedy;
    bool glue;  /* Is the ball stuck to the paddle? */
};

struct fire
{
    int top;    /* This stores the fire y position, it is a fixed point num */
    int x_pos;  /* This stores the fire x position, it is a whole number */
};

struct power
{
    int top;    /* This stores the powerup y position, it is a fixed point num */
    int x_pos;  /* This stores the (middle of) powerup x position, it is a whole number */
    enum power_types type;   /* This stores the powerup type */
};

struct point
{
    int x;
    int y;
};

struct line
{
    struct point p1;
    struct point p2;
};

struct rect
{
    struct point top_left;
    struct point bottom_right;
};

/*
 *
 * Globals
 *
 */

static enum game_state game_state = ST_READY;
static int pad_pos_x;
static int life;
static int score=0,vscore=0;
static bool flip_sides=false;
static int level=0;
static int brick_on_board=0;
static int used_balls=1;
static int used_fires=0;
static int used_powers=0;
static int difficulty = NORMAL;
static int pad_width;
static int flip_sides_delay;
static bool resume = false;
static bool resume_file = false;
static struct brick brick[NUM_BRICKS_ROWS][NUM_BRICKS_COLS];
static struct ball ball[MAX_BALLS];
static struct fire fire[MAX_FIRES];
static struct power power[MAX_POWERS];
static enum paddle_type paddle_type;

static struct configdata config[] =
{
    {TYPE_INT, 0, 1, { .int_p = &difficulty }, "difficulty", NULL},
};

static struct highscore highscores[NUM_SCORES];


/*
 *
 * Functions
 *
 */

/*
 * check_lines:
 * This is based off an explanation and expanded math presented by Paul Bourke:
 * http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/
 *
 * It takes two lines as inputs and returns 1 if they intersect, 0 if they do
 * not.  hitp returns the point where the two lines intersected.
 *
 * This function expects fixed point inputs with a precision of 3.  When a
 * collision occurs hitp is updated with a fixed point location (precision 3)
 * where the collision happened.  The internal calculations are fixed
 * point with a 7 bit fractional precision.
 *
 * If you choose 10 bits of precision a screen size of about 640x480 is the
 * largest this can go.  7 bits allows for an accurate intersection calculation
 * with a line length of about 64 and a rougher line lenght of 128 which is
 * larger than any target currently needs (the pad is the longest line and it
 * only needs an accuracy of 2^4 at most to figure out which section of the pad
 * the ball hit).  A precision of 7 gives breathing room for larger screens.
 * Longer line sizes that need accurate intersection points will need more
 * precision, but will decrease the maximum screen resolution.
 */

#define LINE_PREC 7
static int check_lines(struct line *line1, struct line *line2,
    struct point *hitp)
{
    /* Introduction:
     * This code is based on the solution of these two input equations:
     *  Pa = P1 + ua (P2-P1)
     *  Pb = P3 + ub (P4-P3)
     *
     * Where line one is composed of points P1 and P2 and line two is composed
     *  of points P3 and P4.
     *
     * ua/b is the fractional value you can multiply the x and y legs of the
     *  triangle formed by each line to find a point on the line.
     *
     * The two equations can be expanded to their x/y components:
     *  Pa.x = p1.x + ua(p2.x - p1.x)
     *  Pa.y = p1.y + ua(p2.y - p1.y)
     *
     *  Pb.x = p3.x + ub(p4.x - p3.x)
     *  Pb.y = p3.y + ub(p4.y - p3.y)
     *
     * When Pa.x == Pb.x and Pa.y == Pb.y the lines intersect so you can come
     *  up with two equations (one for x and one for y):
     *
     * p1.x + ua(p2.x - p1.x) = p3.x + ub(p4.x - p3.x)
     * p1.y + ua(p2.y - p1.y) = p3.y + ub(p4.y - p3.y)
     *
     * ua and ub can then be individually solved for.  This results in the
     *  equations used in the following code.
     */

    /* Denominator for ua and ub are the same so store this calculation */
    int d   = FIXED3_MUL((line2->p2.y - line2->p1.y),(line1->p2.x-line1->p1.x))
             -FIXED3_MUL((line2->p2.x - line2->p1.x),(line1->p2.y-line1->p1.y));

    /* n_a and n_b are calculated as seperate values for readability */
    int n_a = FIXED3_MUL((line2->p2.x - line2->p1.x),(line1->p1.y-line2->p1.y))
             -FIXED3_MUL((line2->p2.y - line2->p1.y),(line1->p1.x-line2->p1.x));

    int n_b = FIXED3_MUL((line1->p2.x - line1->p1.x),(line1->p1.y-line2->p1.y))
             -FIXED3_MUL((line1->p2.y - line1->p1.y),(line1->p1.x-line2->p1.x));

    /* Make sure there is not a division by zero - this also indicates that
     * the lines are parallel.
     *
     * If n_a and n_b were both equal to zero the lines would be on top of each
     * other (coincidental).  This check is not done because it is not
     * necessary for this implementation (the parallel check accounts for this).
     */
    if(d == 0)
        return 0;

    /* Calculate the intermediate fractional point that the lines potentially
     *  intersect.
     */
    int ua = (n_a << LINE_PREC)/d;
    int ub = (n_b << LINE_PREC)/d;

    /* The fractional point will be between 0 and 1 inclusive if the lines
     * intersect.  If the fractional calculation is larger than 1 or smaller
     * than 0 the lines would need to be longer to intersect.
     */
    if(ua >=0 && ua <= (1<<LINE_PREC) && ub >= 0 && ub <= (1<<LINE_PREC))
    {
        hitp->x = line1->p1.x + ((ua * (line1->p2.x - line1->p1.x))>>LINE_PREC);
        hitp->y = line1->p1.y + ((ua * (line1->p2.y - line1->p1.y))>>LINE_PREC);
        return 1;
    }
    return 0;
}


static int check_rect(struct line *line, struct rect *rect,
    enum intersection intersection, struct point *hitp)
{
    struct line edge;

    switch (intersection)
    {
        case INTERSECTION_TOP:
        {
            edge.p1.x = rect->top_left.x;
            edge.p1.y = rect->top_left.y;

            edge.p2.x = rect->bottom_right.x;
            edge.p2.y = rect->top_left.y;

            break;
        }
        case INTERSECTION_BOTTOM:
        {
            edge.p1.x = rect->top_left.x;
            edge.p1.y = rect->bottom_right.y;

            edge.p2.x = rect->bottom_right.x;
            edge.p2.y = rect->bottom_right.y;

            break;
        }
        case INTERSECTION_LEFT:
        {
            edge.p1.x = rect->top_left.x;
            edge.p1.y = rect->top_left.y;

            edge.p2.x = rect->top_left.x;
            edge.p2.y = rect->bottom_right.y;

            break;
        }
        case INTERSECTION_RIGHT:
        {
            edge.p1.x = rect->bottom_right.x;
            edge.p1.y = rect->top_left.y;

            edge.p2.x = rect->bottom_right.x;
            edge.p2.y = rect->bottom_right.y;

            break;
        }
        case INTERSECTION_ALL: /* Test hit on all edges */
        {
            return (check_rect(line, rect, INTERSECTION_TOP, hitp) ||
                    check_rect(line, rect, INTERSECTION_BOTTOM, hitp) ||
                    check_rect(line, rect, INTERSECTION_LEFT, hitp) ||
                    check_rect(line, rect, INTERSECTION_RIGHT, hitp));
        }
    }

    return check_lines(line, &edge, hitp);
}

static void brickmania_init_game(bool new_game)
{
    int i,j;

    pad_pos_x   =   GAMESCREEN_WIDTH/2 - PAD_WIDTH/2;

    for(i=0;i<MAX_BALLS;i++)
    {
        ball[i].speedx  =   0;
        ball[i].speedy  =   0;
        ball[i].tempy   =   0;
        ball[i].tempx   =   0;
        ball[i].pos_y   =   ON_PAD_POS_Y;
        ball[i].pos_x   =   GAMESCREEN_WIDTH/2;
        ball[i].glue    =   false;
    }

    used_balls  =   1;
    used_fires  =   0;
    used_powers =   0;
    game_state  =   ST_READY;
    paddle_type =   PADDLE_TYPE_NORMAL;
    pad_width   =   PAD_WIDTH;
    flip_sides  =   false;
    flip_sides_delay   =   FLIP_SIDES_DELAY;

    if (new_game) {
        brick_on_board=0;
        /* add one life per achieved level */
        if (difficulty==EASY && life<2) {
            score+=SCORE_LEVEL_COMPLETED;
            life++;
        }
    }

    for(i=0;i<NUM_BRICKS_ROWS;i++) {
        for(j=0;j<NUM_BRICKS_COLS;j++) {
            if (new_game) {
                brick[i][j].hits=levels[level][i][j]>=10?
                    levels[level][i][j]/16-1:0;
                brick[i][j].hiteffect=0;
                brick[i][j].used=!(levels[level][i][j]==0);
                brick[i][j].color=(levels[level][i][j]>=10?
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
    if((rb->read(fd, &pad_pos_x, sizeof(pad_pos_x)) <= 0) ||
        (rb->read(fd, &life, sizeof(life)) <= 0) ||
        (rb->read(fd, &game_state, sizeof(game_state)) <= 0) ||
        (rb->read(fd, &paddle_type, sizeof(paddle_type)) <= 0) ||
        (rb->read(fd, &score, sizeof(score)) <= 0) ||
        (rb->read(fd, &flip_sides, sizeof(flip_sides)) <= 0) ||
        (rb->read(fd, &level, sizeof(level)) <= 0) ||
        (rb->read(fd, &brick_on_board, sizeof(brick_on_board)) <= 0) ||
        (rb->read(fd, &used_balls, sizeof(used_balls)) <= 0) ||
        (rb->read(fd, &used_fires, sizeof(used_fires)) <= 0) ||
        (rb->read(fd, &used_powers, sizeof(used_powers)) <= 0) ||
        (rb->read(fd, &pad_width, sizeof(pad_width)) <= 0) ||
        (rb->read(fd, &flip_sides_delay, sizeof(flip_sides_delay)) <= 0) ||
        (rb->read(fd, &brick, sizeof(brick)) <= 0) ||
        (rb->read(fd, &ball, sizeof(ball)) <= 0) ||
        (rb->read(fd, &fire, sizeof(fire)) <= 0) ||
        (rb->read(fd, &power, sizeof(power)) <= 0))
    {
        rb->splash(HZ/2, "Failed to load game");
    }
    else
    {
        vscore = score;
        resume = true;
    }

    rb->close(fd);

    return;
}

static void brickmania_savegame(void)
{
    int fd;

    /* write out the game state to the save file */
    fd = rb->open(SAVE_FILE, O_WRONLY|O_CREAT);
    if(fd < 0) return;

    if ((rb->write(fd, &pad_pos_x, sizeof(pad_pos_x)) <= 0) ||
            (rb->write(fd, &life, sizeof(life)) <= 0) ||
            (rb->write(fd, &game_state, sizeof(game_state)) <= 0) ||
            (rb->write(fd, &paddle_type, sizeof(paddle_type)) <= 0) ||
            (rb->write(fd, &score, sizeof(score)) <= 0) ||
            (rb->write(fd, &flip_sides, sizeof(flip_sides)) <= 0) ||
            (rb->write(fd, &level, sizeof(level)) <= 0) ||
            (rb->write(fd, &brick_on_board, sizeof(brick_on_board)) <= 0) ||
            (rb->write(fd, &used_balls, sizeof(used_balls)) <= 0) ||
            (rb->write(fd, &used_fires, sizeof(used_fires)) <= 0) ||
            (rb->write(fd, &used_powers, sizeof(used_powers)) <= 0) ||
            (rb->write(fd, &pad_width, sizeof(pad_width)) <= 0) ||
            (rb->write(fd, &flip_sides_delay, sizeof(flip_sides_delay)) <= 0) ||
            (rb->write(fd, &brick, sizeof(brick)) <= 0) ||
            (rb->write(fd, &ball, sizeof(ball)) <= 0) ||
            (rb->write(fd, &fire, sizeof(fire)) <= 0) ||
            (rb->write(fd, &power, sizeof(power)) <= 0))
    {
        rb->close(fd);
        rb->remove(SAVE_FILE);
        rb->splash(HZ/2, "Failed to save game");
        return;
    }

    rb->close(fd);
}

/* brickmania_sleep timer counting the score */
static void brickmania_sleep(int secs)
{
    bool done=false;
    char s[20];
    int count=0;
    int sw, w;

    while (!done)
    {
        if (count == 0)
            count = *rb->current_tick + HZ*secs;
        if ( (TIME_AFTER(*rb->current_tick, count)) && (vscore == score) )
            done = true;

        if(vscore != score)
        {
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
    static char *help_text[] = {
        "Brickmania", "", "Aim", "",
        "Destroy", "all", "the", "bricks", "by", "bouncing",
        "the", "ball", "of", "them", "using", "the", "paddle.", "", "",
        "Controls", "",
#if CONFIG_KEYPAD == COWON_D2_PAD
        "- & +:",
#else
        "< & >:",
#endif
        "Moves", "the", "paddle", "",
#if CONFIG_KEYPAD == ONDIO_PAD
        "MENU:",
#elif (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == IAUDIO_M3_PAD)
        "PLAY:",
#elif CONFIG_KEYPAD == IRIVER_H300_PAD
        "NAVI:",
#elif CONFIG_KEYPAD == COWON_D2_PAD
        "MENU:",
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
        LAST_STYLE_ITEM
    };

    rb->lcd_setfont(FONT_UI);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
#endif
    if (display_text(ARRAYLEN(help_text), help_text, formation, NULL, true))
        return 1;
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

#ifdef HAVE_TOUCHSCREEN
    /* Entering Menu, set the touchscreen to the global setting */
    rb->touchscreen_set_mode(rb->global_settings->touch_mode);
#endif

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
                if(resume_file)
                    rb->remove(SAVE_FILE);
                return 0;
            case 1:
                score=0;
                vscore=0;
                life=2;
                level=0;
                brickmania_init_game(true);
                return 0;
            case 2:
                rb->set_option("Difficulty", &difficulty, INT,
                                    options, 2, NULL);
                break;
            case 3:
                if (brickmania_help())
                    return 1;
                break;
            case 4:
                highscore_show(-1, highscores, NUM_SCORES, true);
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
#ifdef HAVE_TOUCHSCREEN
    rb->touchscreen_set_mode(TOUCHSCREEN_POINT);
#endif
}

static void brick_hit(int i, int j)
{
    if(!brick[i][j].used)
        return;

    /* if this is a crackable brick hits starts as
     * greater than 0.
     */
    if (brick[i][j].hits > 0) {
        brick[i][j].hits--;
        brick[i][j].hiteffect++;
        score+=SCORE_BALL_HIT_BRICK;
    }
    else {
        brick[i][j].used=false;
        if (used_powers<MAX_POWERS)
        {
            int ran = rb->rand()%POWER_RAND;

            if (ran<NUMBER_OF_POWERUPS)
            {
                power[used_powers].top = TOPMARGIN + i*BRICK_HEIGHT;
                power[used_powers].x_pos = LEFTMARGIN + j*BRICK_WIDTH +
                    (BRICK_WIDTH >> 1);
                power[used_powers].type = ran;
                used_powers++;
            }
        }
        brick_on_board--;
        score+=SCORE_BALL_DEMOLISHED_BRICK;
    }
}

static int brickmania_game_loop(void)
{
    int j,i,k;
    int sw;
    char s[30];
    int sec_count=0;
    int end;

    /* pad_rect used for powerup/ball checks */
    struct rect pad_rect;
    /* This is used for various lines that are checked (ball and powerup) */
    struct line misc_line;

    /* This stores the point that the two lines intersected in a test */
    struct point pt_hit;

    if (brickmania_menu()) {
        return 1;
    }
    resume = false;
    resume_file = false;

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_clear_display();
#endif

    while(true) {
        /* Convert CYCLETIME (in ms) to HZ */
        end = *rb->current_tick + (CYCLETIME * HZ) / 1000;

        if (life >= 0) {
            rb->lcd_clear_display();

            if (flip_sides)
            {
                if (TIME_AFTER(*rb->current_tick, sec_count))
                {
                    sec_count=*rb->current_tick+HZ;
                    if (flip_sides_delay!=0)
                        flip_sides_delay--;
                    else
                        flip_sides=false;
                }
                rb->snprintf(s, sizeof(s), "%d", flip_sides_delay);
                rb->lcd_getstringsize(s, &sw, NULL);
                rb->lcd_putsxy(LCD_WIDTH/2-2, INT3(STRINGPOS_FLIP), s);
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
            if (game_state == ST_PAUSE)
            {
                rb->snprintf(s, sizeof(s), CONTINUE_TEXT);
                rb->lcd_getstringsize(s, &sw, NULL);
                rb->lcd_putsxy(LCD_WIDTH/2-sw/2, INT3(STRINGPOS_NAVI), s);

                sec_count=*rb->current_tick+HZ;
            }

            /* draw the ball */
            for(i=0;i<used_balls;i++)
                rb->lcd_bitmap(brickmania_ball,
                    INT3(ball[i].pos_x - HALFBALL),
                    INT3(ball[i].pos_y - HALFBALL),
                    INT3(BALL), INT3(BALL));

            if (brick_on_board==0)
                brick_on_board--;

            /* Setup the pad line-later used in intersection test */
            pad_rect.top_left.x = pad_pos_x;
            pad_rect.top_left.y = PAD_POS_Y;

            pad_rect.bottom_right.x = pad_pos_x + pad_width;
            pad_rect.bottom_right.y = PAD_POS_Y + PAD_HEIGHT;

            if (game_state!=ST_PAUSE)
            {
                /* move the fires */
                for(k=0;k<used_fires;k++)
                {
                    fire[k].top -= SPEED_FIRE;
                    if (fire[k].top < 0)
                    {
                        used_fires--;
                        fire[k].top = fire[used_fires].top;
                        fire[k].x_pos = fire[used_fires].x_pos;
                        k--;
                    }
                    else if (fire[k].x_pos >= LEFTMARGIN &&
                        fire[k].x_pos < LEFTMARGIN + NUM_BRICKS_COLS * BRICK_WIDTH)
                    {
                        j = (fire[k].x_pos - LEFTMARGIN) / BRICK_WIDTH;
                        for (i=NUM_BRICKS_ROWS-1;i>=0;i--)
                        {

                            if (brick[i][j].used)
                            {
                                score += SCORE_FIRE_HIT_BRICK;
                                brick_hit(i, j);
                                used_fires--;
                                fire[k].top = fire[used_fires].top;
                                fire[k].x_pos = fire[used_fires].x_pos;
                                k--;
                                break;
                            }
                            if (TOPMARGIN + i*BRICK_HEIGHT<=fire[k].top)
                                break;
                        }
                    }
                }

                /* move and handle the powerups */
                for (k=0;k<used_powers;k++)
                {
                    int remove_power = 0;

                    power[k].top += SPEED_POWER;

                    if (power[k].top > PAD_POS_Y)
                    {
                        /* power hit bottom */
                        remove_power = 1;
                    }
                    else
                    {
                        /* Use misc_line to check if the center of the powerup
                         *  hit the paddle.
                         */
                        misc_line.p1.x = power[k].x_pos;
                        misc_line.p1.y = power[k].top;

                        misc_line.p2 = misc_line.p1;
                        misc_line.p2.y += SPEED_POWER;

                        /* Check if the powerup will hit the paddle */
                        if (check_rect(&misc_line, &pad_rect, INTERSECTION_ALL,
                            &pt_hit))
                        {

                            /* power hit paddle */
                            remove_power = 1;
                            switch(power[k].type)
                            {
                                case POWER_TYPE_LIFE_GAIN:
                                    life++;
                                    score += SCORE_POWER_LIFE_GAIN;
                                    break;
                                case POWER_TYPE_LIFE_LOSS:
                                    life--;
                                    if (life>=0)
                                    {
                                        brickmania_init_game(false);
                                        brickmania_sleep(2);
                                    }
                                    break;
                                case POWER_TYPE_PADDLE_STICKY:
                                    score += SCORE_POWER_PADDLE_STICKY;
                                    paddle_type = PADDLE_TYPE_STICKY;
                                    break;
                                case POWER_TYPE_PADDLE_SHOOTER:
                                    score += SCORE_POWER_PADDLE_SHOOTER;
                                    paddle_type = PADDLE_TYPE_SHOOTER;
                                    for(i=0;i<used_balls;i++)
                                        ball[i].glue=false;
                                    break;
                                case POWER_TYPE_PADDLE_NORMAL:
                                    score += SCORE_POWER_PADDLE_NORMAL;
                                    paddle_type = PADDLE_TYPE_NORMAL;
                                    for(i=0;i<used_balls;i++)
                                        ball[i].glue=false;
                                    flip_sides=false;
                                    pad_pos_x += (pad_width-PAD_WIDTH)/2;
                                    pad_width = PAD_WIDTH;
                                    break;
                                case POWER_TYPE_PADDLE_FLIP:
                                    score += SCORE_POWER_FLIP;
                                    sec_count = *rb->current_tick+HZ;
                                    flip_sides_delay = FLIP_SIDES_DELAY;
                                    flip_sides = true;
                                    break;
                                case POWER_TYPE_EXTRA_BALL:
                                    score += SCORE_POWER_EXTRA_BALL;
                                    if(used_balls<MAX_BALLS)
                                    {
                                        /* Set the speed */
                                        if(rb->rand()%2 == 0)
                                            ball[used_balls].speedx=-SPEED_4Q_X;
                                        else
                                            ball[used_balls].speedx= SPEED_4Q_X;
                                        ball[used_balls].speedy= SPEED_4Q_Y;
                                        /* Ball is not glued */
                                        ball[used_balls].glue= false;
                                        used_balls++;
                                    }
                                    break;
                                case POWER_TYPE_PADDLE_LONG:
                                    score+=SCORE_POWER_LONG_PADDLE;
                                    if (pad_width==PAD_WIDTH)
                                    {
                                        pad_width = LONG_PAD_WIDTH;
                                        pad_pos_x -= (LONG_PAD_WIDTH -
                                                PAD_WIDTH)/2;
                                    }
                                    else if (pad_width==SHORT_PAD_WIDTH)
                                    {
                                        pad_width = PAD_WIDTH;
                                        pad_pos_x-=(PAD_WIDTH-
                                                SHORT_PAD_WIDTH)/2;
                                    }
                                    if (pad_pos_x < 0)
                                        pad_pos_x = 0;
                                    else if(pad_pos_x + pad_width >
                                            GAMESCREEN_WIDTH)
                                        pad_pos_x = GAMESCREEN_WIDTH-pad_width;
                                    break;
                                case POWER_TYPE_PADDLE_SHORT:
                                    if (pad_width==PAD_WIDTH)
                                    {
                                        pad_width=SHORT_PAD_WIDTH;
                                        pad_pos_x+=(PAD_WIDTH-
                                                SHORT_PAD_WIDTH)/2;
                                    }
                                    else if (pad_width==LONG_PAD_WIDTH)
                                    {
                                        pad_width=PAD_WIDTH;
                                        pad_pos_x+=(LONG_PAD_WIDTH-PAD_WIDTH)/2;
                                    }
                                    break;
                                default:
                                    break;
                            }
                        }

                        if (remove_power)
                        {
                            used_powers--;
                            if (k != used_powers)
                            {
                                power[k].top = power[used_powers].top;
                                power[k].x_pos = power[used_powers].x_pos;
                                power[k].type = power[used_powers].type;
                            }
                            k--;
                        }
                    }
                }
            }

            /* draw the fires */
            for(k=0;k<used_fires;k++)
            {
                rb->lcd_vline(INT3(fire[k].x_pos), INT3(fire[k].top), 
                        INT3(fire[k].top + FIRE_LENGTH));
            }

            /* draw the powerups */
            for(k=0;k<used_powers;k++)
            {
                rb->lcd_bitmap_part(brickmania_powerups,0,
                    INT3(POWERUP_HEIGHT)*power[k].type,
                    STRIDE(SCREEN_MAIN, BMPWIDTH_brickmania_powerups,
                        BMPHEIGHT_brickmania_powerups),
                    INT3(power[k].x_pos - (POWERUP_WIDTH >> 1)),
                    INT3(power[k].top), INT3(POWERUP_WIDTH),
                    INT3(POWERUP_HEIGHT));
            }

            /* handle all of the bricks */
            for (i=0; i<NUM_BRICKS_ROWS; i++)
            {
                for (j=0; j<NUM_BRICKS_COLS ;j++)
                {
                    int brickx,bricky;

                    /* The brick is a brick, but it may or may not be in use */
                    if(brick[i][j].used)
                    {
                        struct rect brick_rect;

                        brickx = LEFTMARGIN + j*BRICK_WIDTH;
                        bricky = TOPMARGIN + i*BRICK_HEIGHT;

                        brick_rect.top_left.x = brickx;
                        brick_rect.top_left.y = bricky;

                        brick_rect.bottom_right.x = brickx + BRICK_WIDTH;
                        brick_rect.bottom_right.y = bricky + BRICK_HEIGHT;

                        /* Draw the brick */
                        rb->lcd_bitmap_part(brickmania_bricks,0,
                            INT3(BRICK_HEIGHT)*brick[i][j].color,
                            STRIDE( SCREEN_MAIN,
                                    BMPWIDTH_brickmania_bricks,
                                    BMPHEIGHT_brickmania_bricks),
                            INT3(brickx),
                            INT3(bricky),
                            INT3(BRICK_WIDTH), INT3(BRICK_HEIGHT) );

#ifdef HAVE_LCD_COLOR  /* No transparent effect for greyscale lcds for now */
                        if (brick[i][j].hiteffect > 0)
                            rb->lcd_bitmap_transparent_part(brickmania_break,0,
                                INT3(BRICK_HEIGHT)*brick[i][j].hiteffect,
                                STRIDE( SCREEN_MAIN,
                                        BMPWIDTH_brickmania_break,
                                        BMPHEIGHT_brickmania_break),
                                INT3(brickx),
                                INT3(bricky),
                                INT3(BRICK_WIDTH), INT3(BRICK_HEIGHT) );
#endif

                        /* Check if any balls collided with the brick */
                        for(k=0; k<used_balls; k++)
                        {
                            int hit = 0;

                            /* Setup the ball path to describe the current ball
                             * position and the line it makes to its next
                             * position.
                             */
                            misc_line.p1.x = ball[k].pos_x;
                            misc_line.p1.y = ball[k].pos_y;

                            misc_line.p2.x = ball[k].pos_x + ball[k].speedx;
                            misc_line.p2.y = ball[k].pos_y + ball[k].speedy;

                            /* Check to see if the ball and the bottom hit. If
                             *  the ball is moving down we don't want to
                             *  include the bottom line intersection.
                             *
                             * The order that the sides are checked matters.
                             *
                             * Note that tempx/tempy store the next position
                             *  that the ball should be drawn.
                             */
                            if (ball[k].speedy <= 0 && check_rect(&misc_line,
                                &brick_rect, INTERSECTION_BOTTOM, &pt_hit))
                            {
                                ball[k].speedy = -ball[k].speedy;
                                hit = 1;
                            }
                            /* Check the top, if the ball is moving up dont
                             *  count it as a hit.
                             */
                            else if (ball[k].speedy > 0 && check_rect(&misc_line,
                                &brick_rect, INTERSECTION_TOP, &pt_hit))
                            {
                                ball[k].speedy = -ball[k].speedy;
                                hit = 1;
                            }
                            /* Check the left side of the brick */
                            else if (check_rect(&misc_line, &brick_rect,
                                INTERSECTION_LEFT, &pt_hit))
                            {
                                ball[k].speedx = -ball[k].speedx;
                                hit = 1;
                            }
                            /* Check the right side of the brick */
                            else if (check_rect(&misc_line, &brick_rect,
                                INTERSECTION_RIGHT, &pt_hit))
                            {
                                ball[k].speedx = -ball[k].speedx;
                                hit = 1;
                            }

                            if (hit)
                            {
                                ball[k].tempy = pt_hit.y;
                                ball[k].tempx = pt_hit.x;
                                brick_hit(i, j);
                                break;
                            }
                        } /* for k */
                    } /* if(used) */

                } /* for j */
            } /* for i */

            /* draw the paddle according to the PAD_WIDTH */
            if( pad_width == PAD_WIDTH ) /* Normal width */
            {
                rb->lcd_bitmap_part(
                    brickmania_pads,
                    0, paddle_type*INT3(PAD_HEIGHT),
                    STRIDE( SCREEN_MAIN, BMPWIDTH_brickmania_pads,
                            BMPHEIGHT_brickmania_pads),
                    INT3(pad_pos_x), INT3(PAD_POS_Y),
                    INT3(pad_width), INT3(PAD_HEIGHT) );
            }
            else if( pad_width == LONG_PAD_WIDTH ) /* Long Pad */
            {
                rb->lcd_bitmap_part(
                    brickmania_long_pads,
                    0,paddle_type*INT3(PAD_HEIGHT),
                    STRIDE( SCREEN_MAIN, BMPWIDTH_brickmania_long_pads,
                            BMPHEIGHT_brickmania_long_pads),
                    INT3(pad_pos_x), INT3(PAD_POS_Y),
                    INT3(pad_width), INT3(PAD_HEIGHT) );
            }
            else /* Short pad */
            {
                rb->lcd_bitmap_part(
                    brickmania_short_pads,
                    0,paddle_type*INT3(PAD_HEIGHT),
                    STRIDE( SCREEN_MAIN, BMPWIDTH_brickmania_short_pads,
                            BMPHEIGHT_brickmania_short_pads),
                    INT3(pad_pos_x), INT3(PAD_POS_Y),
                    INT3(pad_width), INT3(PAD_HEIGHT) );
            }

            /* If the game is not paused continue */
            if (game_state!=ST_PAUSE)
            {
                /* Loop through all of the balls in play */
                for(k=0;k<used_balls;k++)
                {
                    struct line screen_edge;

                    /* Describe the ball movement for the edge collision detection */
                    misc_line.p1.x = ball[k].pos_x;
                    misc_line.p1.y = ball[k].pos_y;

                    misc_line.p2.x = ball[k].pos_x + ball[k].speedx;
                    misc_line.p2.y = ball[k].pos_y + ball[k].speedy;

                    /* Did the Ball hit the top of the screen? */
                    screen_edge.p1.x = 0;
                    screen_edge.p1.y = 0;

                    screen_edge.p2.x = GAMESCREEN_WIDTH;
                    screen_edge.p2.y = 0;
                    /* the test for pos_y prevents the ball from bouncing back
                     * from _over_ the top to infinity on some rare cases */
                    if (ball[k].pos_y > 0 &&
                        check_lines(&misc_line, &screen_edge, &pt_hit))
                    {
                        ball[k].tempy = pt_hit.y + 1;
                        ball[k].tempx = pt_hit.x;
                        /* Reverse the direction */
                        ball[k].speedy = -ball[k].speedy;
                    }

                    /* Player missed the ball and hit bottom of screen */
                    if (ball[k].pos_y >= GAMESCREEN_HEIGHT)
                    {
                        /* Player had balls to spare, so handle the removal */
                        if (used_balls>1)
                        {
                            /* decrease number of balls in play */
                            used_balls--;
                            /* Replace removed ball with the last ball */
                            ball[k].pos_x = ball[used_balls].pos_x;
                            ball[k].pos_y = ball[used_balls].pos_y;
                            ball[k].speedy = ball[used_balls].speedy;
                            ball[k].tempy = ball[used_balls].tempy;
                            ball[k].speedx = ball[used_balls].speedx;
                            ball[k].tempx = ball[used_balls].tempx;
                            ball[k].glue = ball[used_balls].glue;

                            /* Reset the last ball that was removed */
                            ball[used_balls].speedx=0;
                            ball[used_balls].speedy=0;
                            ball[used_balls].tempy=0;
                            ball[used_balls].tempx=0;
                            ball[used_balls].pos_y=ON_PAD_POS_Y;
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
                                brickmania_init_game(false);
                                brickmania_sleep(2);
                                rb->button_clear_queue();
                            }
                        }
                    }

                    if (game_state != ST_READY && !ball[k].glue)
                    {
                        /* Check if the ball hit the left side */
                        screen_edge.p1.x = 0;
                        screen_edge.p1.y = 0;

                        screen_edge.p2.x = 0;
                        screen_edge.p2.y = GAMESCREEN_HEIGHT;
                        if (check_lines(&misc_line, &screen_edge, &pt_hit))
                        {
                            /* Reverse direction */
                            ball[k].speedx = -ball[k].speedx;

                            /* Re-position ball in gameboard */
                            ball[k].tempy = pt_hit.y;
                            ball[k].tempx = 0;
                        }

                        /* Check if the ball hit the right side */
                        screen_edge.p1.x = GAMESCREEN_WIDTH;
                        screen_edge.p1.y = 0;

                        screen_edge.p2.x = GAMESCREEN_WIDTH;
                        screen_edge.p2.y = GAMESCREEN_HEIGHT;
                        if (check_lines(&misc_line, &screen_edge, &pt_hit))
                        {
                            /* Reverse direction */
                            ball[k].speedx = -ball[k].speedx;

                            /* Re-position ball in gameboard */
                            ball[k].tempy = pt_hit.y;
                            ball[k].tempx = GAMESCREEN_WIDTH - FIXED3(1);
                        }

                        /* Did the ball hit the paddle? Depending on where the ball
                         *  Hit set the x/y speed appropriately.
                         */
                        if(check_rect(&misc_line, &pad_rect, INTERSECTION_TOP,
                            &pt_hit) )
                        {
                            /* Re-position ball based on collision */
                            ball[k].tempy = ON_PAD_POS_Y;
                            ball[k].tempx = pt_hit.x;

                            /* Calculate the ball position relative to the paddle width */
                            int ball_repos = pt_hit.x - pad_pos_x;
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
                                    ball[k].speedy = SPEED_1Q_Y;
                                    ball[k].speedx = SPEED_1Q_X * x_direction;
                                    break;
                                    /* Ball hit the next fourth of the paddle */
                                case 1:
                                    ball[k].speedy = SPEED_2Q_Y;
                                    ball[k].speedx = SPEED_2Q_X * x_direction;
                                    break;
                                    /* Ball hit the third fourth of the paddle */
                                case 2:
                                    ball[k].speedy = SPEED_3Q_Y;
                                    ball[k].speedx = SPEED_3Q_X * x_direction;
                                    break;
                                    /* Ball hit the fourth fourth of the paddle or dead
                                     *  center.
                                     */
                                case 3:
                                case 4:
                                    ball[k].speedy = SPEED_4Q_Y;
                                    /* Since this is the middle we don't want to
                                     *  force the ball in a different direction.
                                     *  Just keep it going in the same direction
                                     *  with a specific speed.
                                     */
                                    if(ball[k].speedx > 0)
                                    {
                                        ball[k].speedx = SPEED_4Q_X;
                                    }
                                    else
                                    {
                                        ball[k].speedx = -SPEED_4Q_X;
                                    }
                                    break;

                                default:
                                    ball[k].speedy = SPEED_4Q_Y;
                                    break;
                            }

                            if(paddle_type == PADDLE_TYPE_STICKY)
                            {
                                ball[k].speedy = -ball[k].speedy;
                                ball[k].glue=true;

                                /* X location should not be forced since that is moved with the paddle.  The Y
                                 *  position should be forced to keep the ball at the paddle.
                                 */
                                ball[k].tempx = 0;
                                ball[k].tempy = ON_PAD_POS_Y;
                            }
                        }
                    }

                    /* Update the ball position */
                    if (!ball[k].glue)
                    {
                        if(ball[k].tempx)
                            ball[k].pos_x = ball[k].tempx;
                        else
                            ball[k].pos_x += ball[k].speedx;

                        if(ball[k].tempy)
                            ball[k].pos_y = ball[k].tempy;
                        else
                            ball[k].pos_y += ball[k].speedy;

                        ball[k].tempy=0;
                        ball[k].tempx=0;
                    }
                } /* for k */
            }

            rb->lcd_update();

            if (brick_on_board < 0)
            {
                if (level+1<NUM_LEVELS)
                {
                    level++;
                    if (difficulty==NORMAL)
                        score+=SCORE_LEVEL_COMPLETED;
                    brickmania_init_game(true);
                    brickmania_sleep(2);
                    rb->button_clear_queue();
                }
                else
                {
                    rb->lcd_getstringsize("Congratulations!", &sw, NULL);
                    rb->lcd_putsxy(LCD_WIDTH/2-sw/2, INT3(STRINGPOS_CONGRATS),
                                   "Congratulations!");
#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
                    rb->lcd_getstringsize("No more levels", &sw, NULL);
                    rb->lcd_putsxy(LCD_WIDTH/2-sw/2, INT3(STRINGPOS_FINISH),
                                   "No more levels");
#else
                    rb->lcd_getstringsize("You have finished the game!",
                                          &sw, NULL);
                    rb->lcd_putsxy(LCD_WIDTH/2-sw/2, INT3(STRINGPOS_FINISH),
                                   "You have finished the game!");
#endif
                    vscore=score;
                    rb->lcd_update();
                    brickmania_sleep(2);
                    return 0;
                }
            }

            int button=rb->button_get(false);
            int move_button = rb->button_status();

#if defined(HAS_BUTTON_HOLD) && !defined(HAVE_REMOTE_LCD_AS_MAIN)
            /* FIXME: Should probably check remote hold here */
            if (rb->button_hold())
                button = QUIT;
#endif

#ifdef HAVE_TOUCHSCREEN
            if( move_button & BUTTON_TOUCHSCREEN)
            {
                int data;
                short touch_x, touch_y;
                rb->button_status_wdata(&data);
                touch_x = FIXED3(data >> 16);
                touch_y = FIXED3(data & 0xffff);

                if(flip_sides)
                {
                    pad_pos_x = GAMESCREEN_WIDTH - (touch_x + pad_width/2);
                }
                else
                {
                    pad_pos_x = (touch_x - pad_width/2);
                }

                if(pad_pos_x < 0)
                    pad_pos_x = 0;
                else if(pad_pos_x + pad_width > GAMESCREEN_WIDTH)
                    pad_pos_x = GAMESCREEN_WIDTH-pad_width;
                for(k=0; k<used_balls; k++)
                    if (game_state==ST_READY || ball[k].glue)
                        ball[k].pos_x = pad_pos_x + pad_width/2;
            }
            else
#endif
            {
                int button_right, button_left;
#ifdef ALTRIGHT
                button_right =  move_button & (RIGHT | ALTRIGHT);
                button_left  =  move_button & (LEFT | ALTLEFT);
#else
                button_right =((move_button & RIGHT)|| SCROLL_FWD(button));
                button_left  =((move_button & LEFT) ||SCROLL_BACK(button));
#endif
                if ((game_state==ST_PAUSE) && (button_right || button_left))
                    continue;

                if (button_left || button_right)
                {
                    int dx = 0;

                    if ((button_right && !flip_sides) ||
                        (button_left && flip_sides))
                    {
                        if (pad_pos_x+SPEED_PAD+pad_width > GAMESCREEN_WIDTH)
                            dx = GAMESCREEN_WIDTH - pad_pos_x - pad_width;
                        else
                            dx = SPEED_PAD;
                    }
                    else if ((button_left && !flip_sides) ||
                            (button_right && flip_sides))
                    {
                        if (pad_pos_x-SPEED_PAD < 0)
                            dx = -pad_pos_x;
                        else
                            dx = -SPEED_PAD;
                    }

                    pad_pos_x+=dx;
                    for(k=0;k<used_balls;k++)
                    {
                        if (game_state==ST_READY || ball[k].glue)
                        {
                            ball[k].pos_x+=dx;

                            if (ball[k].pos_x < HALFBALL)
                                ball[k].pos_x = HALFBALL;
                            else if (ball[k].pos_x > GAMESCREEN_WIDTH - HALFBALL)
                                ball[k].pos_x = GAMESCREEN_WIDTH - HALFBALL;
                        }
                    }
                }
            }

            switch(button)
            {
#if defined(HAVE_TOUCHSCREEN)
                case (BUTTON_REL | BUTTON_TOUCHSCREEN):
#endif
                case UP:
                case SELECT:
#ifdef ALTSELECT
                case ALTSELECT:
#endif
                    if (game_state==ST_READY)
                    {
                        /* Initialize used balls starting speed */
                        for(k=0 ; k < used_balls ; k++)
                        {
                            ball[k].speedy = SPEED_4Q_Y;
                            if(pad_pos_x + (pad_width/2) >= GAMESCREEN_WIDTH/2)
                            {
                                ball[k].speedx = SPEED_4Q_X;
                            }
                            else
                            {
                                ball[k].speedx = -SPEED_4Q_X;
                            }
                        }
                        game_state=ST_START;
                    }
                    else if (game_state==ST_PAUSE)
                    {
                        game_state=ST_START;
                    }
                    else if (paddle_type == PADDLE_TYPE_STICKY)
                    {
                        for(k=0;k<used_balls;k++)
                        {
                            if (ball[k].glue)
                            {
                                ball[k].glue=false;
                                ball[k].speedy = -ball[k].speedy;
                            }
                        }
                    }
                    else if (paddle_type == PADDLE_TYPE_SHOOTER)
                    {
                        if (used_fires < MAX_FIRES)
                        {
                            fire[used_fires].top = PAD_POS_Y - FIRE_LENGTH;
                            fire[used_fires].x_pos = pad_pos_x + 1; /* Add 1 for edge */
                            used_fires++;
                        }
                        if (used_fires < MAX_FIRES)
                        {
                            fire[used_fires].top = PAD_POS_Y - FIRE_LENGTH;
                            fire[used_fires].x_pos = pad_pos_x + pad_width - 1; /* Sub1 edge*/
                            used_fires++;
                        }
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
        else
        {
#ifdef HAVE_LCD_COLOR
            rb->lcd_bitmap_transparent(brickmania_gameover,
                           (LCD_WIDTH - INT3(GAMEOVER_WIDTH))/2,
                           INT3(GAMESCREEN_HEIGHT - GAMEOVER_HEIGHT)/2,
                           INT3(GAMEOVER_WIDTH),INT3(GAMEOVER_HEIGHT));
#else /* greyscale and mono */
            rb->lcd_bitmap(brickmania_gameover,(LCD_WIDTH -
                            INT3(GAMEOVER_WIDTH))/2,
                           INT3(GAMESCREEN_HEIGHT - GAMEOVER_HEIGHT)/2,
                           INT3(GAMEOVER_WIDTH),INT3(GAMEOVER_HEIGHT) );
#endif
            rb->lcd_update();
            brickmania_sleep(2);
            return 0;
        }

        /* Game always needs to yield for other threads */
        rb->yield();

        /* Sleep for a bit if there is time to spare */
        if (TIME_BEFORE(*rb->current_tick, end))
            rb->sleep(end-*rb->current_tick);
    }
    return 0;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    int last_difficulty;

    highscore_load(SCORE_FILE, highscores, NUM_SCORES);
    configfile_load(CONFIG_FILE_NAME,config,1,0);
    last_difficulty = difficulty;

#ifdef HAVE_TOUCHSCREEN
    rb->touchscreen_set_mode(TOUCHSCREEN_POINT);
#endif

    rb->lcd_setfont(FONT_SYSFIXED);
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    /* Turn off backlight timeout */
    backlight_force_on(); /* backlight control in lib/helper.c */

    /* now go ahead and have fun! */
    rb->srand( *rb->current_tick );
    brickmania_loadgame();
    resume_file = resume;
    while(!brickmania_game_loop())
    {
        if(!resume)
        {
            int position = highscore_update(score, level+1, "",
                                            highscores, NUM_SCORES);
            if (position != -1)
            {
                if (position == 0)
                    rb->splash(HZ*2, "New High Score");
                highscore_show(position, highscores, NUM_SCORES, true);
            }
            else
            {
                brickmania_sleep(3);
            }
        }
    }

    highscore_save(SCORE_FILE, highscores, NUM_SCORES);
    if(last_difficulty != difficulty)
        configfile_save(CONFIG_FILE_NAME,config,1,0);
    /* Restore user's original backlight setting */
    rb->lcd_setfont(FONT_UI);
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(); /* backlight control in lib/helper.c */

    return PLUGIN_OK;
}
