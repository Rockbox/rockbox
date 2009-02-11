/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007-2009 Joshua Simmons <mud at majidejima dot com>
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

#ifndef GOBAN_MAIN_H
#define GOBAN_MAIN_H

/* Enable this to run test mode.  (see the end of util.c) */
#if 0
#define GBN_TEST
#endif

#include "types.h"
#include "util.h"


/* Colors of various things.  The colors on mono bitmap targets is fixed
   based on the background/foreground color. */
#ifdef HAVE_LCD_COLOR
#define BOARD_COLOR LCD_RGBPACK(184,136,72)
#define WHITE_COLOR LCD_RGBPACK(255,255,255)
#define BLACK_COLOR LCD_RGBPACK(0,0,0)
#define LINE_COLOR LCD_RGBPACK(0,0,0)
#define BACKGROUND_COLOR LCD_RGBPACK(41,104,74)
#define CURSOR_COLOR LCD_RGBPACK(222,0,0)
#define MARK_COLOR LCD_RGBPACK(0,0,255)
#elif LCD_DEPTH > 1             /* grayscale */
#define BOARD_COLOR		LCD_LIGHTGRAY
#define WHITE_COLOR		LCD_WHITE
#define BLACK_COLOR		LCD_BLACK
#define LINE_COLOR		LCD_BLACK
#define BACKGROUND_COLOR	LCD_DARKGRAY
#define CURSOR_COLOR	LCD_DARKGRAY
#define MARK_COLOR	LCD_DARKGRAY
#endif

/* Key setups */
#ifdef HAVE_TOUCHSCREEN
#define  GBN_BUTTON_UP                 BUTTON_TOPMIDDLE
#define  GBN_BUTTON_DOWN               BUTTON_BOTTOMMIDDLE
#define  GBN_BUTTON_LEFT               BUTTON_MIDLEFT
#define  GBN_BUTTON_RIGHT              BUTTON_MIDRIGHT
#define  GBN_BUTTON_RETREAT            BUTTON_BOTTOMLEFT
#define  GBN_BUTTON_ADVANCE            BUTTON_BOTTOMRIGHT
#define  GBN_BUTTON_MENU               BUTTON_TOPLEFT
#define  GBN_BUTTON_PLAY               BUTTON_CENTER | BUTTON_REL
#define  GBN_BUTTON_CONTEXT            BUTTON_CENTER | BUTTON_REPEAT
#define  GBN_BUTTON_NEXT_VAR           BUTTON_TOPRIGHT

#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) \
   || (CONFIG_KEYPAD == IPOD_3G_PAD) \
   || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define  GBN_BUTTON_UP                 BUTTON_MENU
#define  GBN_BUTTON_DOWN               BUTTON_PLAY
#define  GBN_BUTTON_LEFT               BUTTON_LEFT
#define  GBN_BUTTON_RIGHT              BUTTON_RIGHT
#define  GBN_BUTTON_RETREAT            BUTTON_SCROLL_BACK
#define  GBN_BUTTON_ADVANCE            BUTTON_SCROLL_FWD
#define  GBN_BUTTON_PLAY               BUTTON_SELECT | BUTTON_REL
#define  GBN_BUTTON_MENU               BUTTON_SELECT | BUTTON_REPEAT
/* no context */
/* no next var */

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) \
   || (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define  GBN_BUTTON_UP                 BUTTON_UP
#define  GBN_BUTTON_DOWN               BUTTON_DOWN
#define  GBN_BUTTON_LEFT               BUTTON_LEFT
#define  GBN_BUTTON_RIGHT              BUTTON_RIGHT
#define  GBN_BUTTON_RETREAT            BUTTON_SCROLL_BACK
#define  GBN_BUTTON_ADVANCE            BUTTON_SCROLL_FWD
#define  GBN_BUTTON_MENU               BUTTON_POWER
#define  GBN_BUTTON_PLAY               BUTTON_SELECT | BUTTON_REL
#define  GBN_BUTTON_CONTEXT            BUTTON_SELECT | BUTTON_REPEAT
#define  GBN_BUTTON_NEXT_VAR           BUTTON_REC

#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#define  GBN_BUTTON_UP                 BUTTON_UP
#define  GBN_BUTTON_DOWN               BUTTON_DOWN
#define  GBN_BUTTON_LEFT               BUTTON_LEFT
#define  GBN_BUTTON_RIGHT              BUTTON_RIGHT
#define  GBN_BUTTON_RETREAT            BUTTON_VOL_DOWN
#define  GBN_BUTTON_ADVANCE            BUTTON_VOL_UP
#define  GBN_BUTTON_MENU               BUTTON_POWER
#define  GBN_BUTTON_PLAY               BUTTON_SELECT | BUTTON_REL
#define  GBN_BUTTON_CONTEXT            BUTTON_SELECT | BUTTON_REPEAT
#define  GBN_BUTTON_NEXT_VAR           BUTTON_REC

#elif (CONFIG_KEYPAD == GIGABEAT_PAD) \
#define  GBN_BUTTON_UP                 BUTTON_UP
#define  GBN_BUTTON_DOWN               BUTTON_DOWN
#define  GBN_BUTTON_LEFT               BUTTON_LEFT
#define  GBN_BUTTON_RIGHT              BUTTON_RIGHT
#define  GBN_BUTTON_RETREAT            BUTTON_VOL_DOWN
#define  GBN_BUTTON_ADVANCE            BUTTON_VOL_UP
#define  GBN_BUTTON_MENU               BUTTON_MENU
#define  GBN_BUTTON_PLAY               BUTTON_SELECT | BUTTON_REL
#define  GBN_BUTTON_CONTEXT            BUTTON_SELECT | BUTTON_REPEAT
#define  GBN_BUTTON_NEXT_VAR           BUTTON_A

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define  GBN_BUTTON_UP                 BUTTON_UP
#define  GBN_BUTTON_DOWN               BUTTON_DOWN
#define  GBN_BUTTON_LEFT               BUTTON_LEFT
#define  GBN_BUTTON_RIGHT              BUTTON_RIGHT
#define  GBN_BUTTON_RETREAT            BUTTON_VOL_DOWN
#define  GBN_BUTTON_ADVANCE            BUTTON_VOL_UP
#define  GBN_BUTTON_MENU               BUTTON_MENU
#define  GBN_BUTTON_PLAY               BUTTON_SELECT | BUTTON_REL
#define  GBN_BUTTON_CONTEXT            BUTTON_SELECT | BUTTON_REPEAT
#define  GBN_BUTTON_NEXT_VAR           BUTTON_PLAY

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define  GBN_BUTTON_UP	               BUTTON_SCROLL_UP
#define  GBN_BUTTON_DOWN               BUTTON_SCROLL_DOWN
#define  GBN_BUTTON_LEFT               BUTTON_LEFT
#define  GBN_BUTTON_RIGHT              BUTTON_RIGHT
#define  GBN_BUTTON_RETREAT            BUTTON_FF
#define  GBN_BUTTON_ADVANCE            BUTTON_REW
#define  GBN_BUTTON_MENU               BUTTON_POWER
#define  GBN_BUTTON_PLAY               BUTTON_PLAY | BUTTON_REL
#define  GBN_BUTTON_CONTEXT            BUTTON_PLAY | BUTTON_REPEAT
/* No next var */

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define  GBN_BUTTON_UP                 BUTTON_UP
#define  GBN_BUTTON_DOWN               BUTTON_DOWN
#define  GBN_BUTTON_LEFT               BUTTON_LEFT
#define  GBN_BUTTON_RIGHT              BUTTON_RIGHT
#define  GBN_BUTTON_RETREAT            BUTTON_OFF
#define  GBN_BUTTON_ADVANCE            BUTTON_ON
#define  GBN_BUTTON_MENU               BUTTON_MODE
#define  GBN_BUTTON_PLAY               BUTTON_SELECT | BUTTON_REL
#define  GBN_BUTTON_CONTEXT            BUTTON_SELECT | BUTTON_REPEAT
#define  GBN_BUTTON_NEXT_VAR           BUTTON_REC

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define  GBN_BUTTON_UP                 BUTTON_UP
#define  GBN_BUTTON_DOWN               BUTTON_DOWN
#define  GBN_BUTTON_LEFT               BUTTON_LEFT
#define  GBN_BUTTON_RIGHT              BUTTON_RIGHT
#define  GBN_BUTTON_RETREAT            BUTTON_MENU
#define  GBN_BUTTON_ADVANCE            BUTTON_PLAY
#define  GBN_BUTTON_MENU               BUTTON_DISPLAY
#define  GBN_BUTTON_PLAY               BUTTON_SELECT | BUTTON_REL
#define  GBN_BUTTON_CONTEXT            BUTTON_SELECT | BUTTON_REPEAT
#define  GBN_BUTTON_NEXT_VAR           BUTTON_POWER

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define  GBN_BUTTON_UP                 BUTTON_UP
#define  GBN_BUTTON_DOWN               BUTTON_DOWN
#define  GBN_BUTTON_LEFT               BUTTON_LEFT
#define  GBN_BUTTON_RIGHT              BUTTON_RIGHT
#define  GBN_BUTTON_RETREAT            BUTTON_PLAY
#define  GBN_BUTTON_ADVANCE            BUTTON_REC
#define  GBN_BUTTON_MENU               BUTTON_POWER
#define  GBN_BUTTON_PLAY               BUTTON_SELECT | BUTTON_REL
#define  GBN_BUTTON_CONTEXT            BUTTON_SELECT | BUTTON_REPEAT
/* no next var */

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
/* TODO: these are basically complete guesses, I have no manual to go by */
#define  GBN_BUTTON_UP                 BUTTON_RC_VOL_UP
#define  GBN_BUTTON_DOWN               BUTTON_RC_VOL_DOWN
#define  GBN_BUTTON_LEFT               BUTTON_RC_REW
#define  GBN_BUTTON_RIGHT              BUTTON_RC_FF
#define  GBN_BUTTON_RETREAT            BUTTON_VOL_DOWN
#define  GBN_BUTTON_ADVANCE            BUTTON_VOL_UP
#define  GBN_BUTTON_MENU               BUTTON_MODE
#define  GBN_BUTTON_PLAY               BUTTON_PLAY | BUTTON_REL
#define  GBN_BUTTON_CONTEXT            BUTTON_PLAY | BUTTON_REPEAT
/* no next var */

#elif (CONFIG_KEYPAD == RECORDER_PAD)
#define  GBN_BUTTON_UP                 BUTTON_UP
#define  GBN_BUTTON_DOWN               BUTTON_DOWN
#define  GBN_BUTTON_LEFT               BUTTON_LEFT
#define  GBN_BUTTON_RIGHT              BUTTON_RIGHT
#define  GBN_BUTTON_RETREAT            BUTTON_F1
#define  GBN_BUTTON_ADVANCE            BUTTON_F3
#define  GBN_BUTTON_MENU               BUTTON_F2
#define  GBN_BUTTON_PLAY               BUTTON_PLAY | BUTTON_REL
#define  GBN_BUTTON_CONTEXT            BUTTON_PLAY | BUTTON_REPEAT
#define  GBN_BUTTON_NEXT_VAR           BUTTON_ON

#elif (CONFIG_KEYPAD == ONDIO_PAD)
#define  GBN_BUTTON_UP                 BUTTON_UP
#define  GBN_BUTTON_DOWN               BUTTON_DOWN
#define  GBN_BUTTON_LEFT               BUTTON_LEFT
#define  GBN_BUTTON_RIGHT              BUTTON_RIGHT
#define  GBN_BUTTON_MENU               BUTTON_MENU | BUTTON_REPEAT
#define  GBN_BUTTON_PLAY               BUTTON_MENU | BUTTON_REL
#define  GBN_BUTTON_NAV_MODE           BUTTON_OFF
/* No context */
/* No advance/retreat */
/* no next var */

#else
#error Unsupported keypad
#endif


/* The smallest dimension of the LCD */
#define LCD_MIN_DIMENSION (LCD_HEIGHT > LCD_WIDTH ? LCD_WIDTH : LCD_HEIGHT)


/* Determine if we have a wide screen or a tall screen.  This is used to
   place the board and footer in acceptable locations also, set the
   LCD_BOARD_SIZE, making sure that we have at least 16 pixels for the
   "footer" on either the bottom or the right. */

#define FOOTER_RESERVE (16)

#if (LCD_WIDTH > LCD_HEIGHT)

#define GBN_WIDE_SCREEN

#define LCD_BOARD_WIDTH (LCD_WIDTH - FOOTER_RESERVE)
#define LCD_BOARD_HEIGHT LCD_HEIGHT

#else

#define GBN_TALL_SCREEN

#define LCD_BOARD_WIDTH LCD_WIDTH
#define LCD_BOARD_HEIGHT (LCD_HEIGHT - FOOTER_RESERVE)

#endif // LCD_WIDTH > LCD_HEIGHT


/* The directory we default to for saving crap */
#define DEFAULT_SAVE_DIR "/sgf"

/* The default file we save to */
#define DEFAULT_SAVE (DEFAULT_SAVE_DIR "/gbn_def.sgf")

/* The size of the buffer we store filenames in (1 reserved for '\0') */
#define SAVE_FILE_LENGTH 256

/* The maximum setting for idle autosave time, in minutes */
#define MAX_AUTOSAVE (30)

/* On mono targets, draw while stones with a black outline so they are
   actually visibile instead of being white on white */
#if (LCD_DEPTH == 1)
#define OUTLINE_STONES
#endif

/* The current play mode */
extern enum play_mode_t play_mode;

/* Show comments when redoing onto a move? */
extern bool auto_show_comments;

/* A stack used for parsing/outputting as well as some board functions
   such as counting liberties and filling in/ removing stones */
extern struct stack_t parse_stack;

#endif
