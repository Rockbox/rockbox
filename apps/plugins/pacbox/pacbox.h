/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Pacbox - a Pacman Emulator for Rockbox
 *
 * Based on PIE - Pacman Instructional Emulator
 *
 * Copyright (c) 1997-2003,2004 Alessandro Scotti
 * http://www.ascotti.org/
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

/* Platform-specific defines - used in both C and ASM files */

#ifndef _PACBOX_H
#define _PACBOX_H

#include "config.h"

#if CONFIG_KEYPAD == IPOD_4G_PAD

#define PACMAN_UP      BUTTON_RIGHT
#define PACMAN_DOWN    BUTTON_LEFT
#define PACMAN_LEFT    BUTTON_MENU
#define PACMAN_RIGHT   BUTTON_PLAY
#define PACMAN_1UP     BUTTON_SELECT
#define PACMAN_COIN    BUTTON_SELECT
#define PACMAN_MENU    (BUTTON_MENU | BUTTON_SELECT)

#elif CONFIG_KEYPAD == IRIVER_H100_PAD || CONFIG_KEYPAD == IRIVER_H300_PAD

#define PACMAN_UP      BUTTON_RIGHT
#define PACMAN_DOWN    BUTTON_LEFT
#define PACMAN_LEFT    BUTTON_UP
#define PACMAN_RIGHT   BUTTON_DOWN
#define PACMAN_1UP     BUTTON_SELECT
#define PACMAN_2UP     BUTTON_ON
#define PACMAN_COIN    BUTTON_REC
#define PACMAN_MENU    BUTTON_MODE

#ifdef HAVE_REMOTE_LCD

#define PACMAN_HAS_REMOTE

#define PACMAN_RC_UP      BUTTON_RC_VOL_UP
#define PACMAN_RC_DOWN    BUTTON_RC_VOL_DOWN
#define PACMAN_RC_LEFT    BUTTON_RC_REW
#define PACMAN_RC_RIGHT   BUTTON_RC_FF
#define PACMAN_RC_1UP     BUTTON_RC_SOURCE
#define PACMAN_RC_2UP     BUTTON_RC_BITRATE
#define PACMAN_RC_COIN    BUTTON_RC_REC
#define PACMAN_RC_MENU    BUTTON_RC_MODE

#endif

#elif CONFIG_KEYPAD == GIGABEAT_PAD

#define PACMAN_UP      BUTTON_UP
#define PACMAN_DOWN    BUTTON_DOWN
#define PACMAN_LEFT    BUTTON_LEFT
#define PACMAN_RIGHT   BUTTON_RIGHT
#define PACMAN_1UP     BUTTON_SELECT
#define PACMAN_2UP     BUTTON_POWER
#define PACMAN_COIN    BUTTON_A
#define PACMAN_MENU    BUTTON_MENU

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD

#define PACMAN_UP      BUTTON_UP
#define PACMAN_DOWN    BUTTON_DOWN
#define PACMAN_LEFT    BUTTON_LEFT
#define PACMAN_RIGHT   BUTTON_RIGHT
#define PACMAN_1UP     BUTTON_SELECT
#define PACMAN_2UP     BUTTON_POWER
#define PACMAN_COIN    BUTTON_PLAY
#define PACMAN_MENU    BUTTON_MENU

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD

#define PACMAN_UP      BUTTON_RIGHT
#define PACMAN_DOWN    BUTTON_LEFT
#define PACMAN_LEFT    BUTTON_UP
#define PACMAN_RIGHT   BUTTON_DOWN
#define PACMAN_1UP     BUTTON_SELECT
#define PACMAN_2UP     BUTTON_POWER
#define PACMAN_COIN    BUTTON_REC
#define PACMAN_MENU    BUTTON_PLAY

#elif CONFIG_KEYPAD == SANSA_E200_PAD

#define PACMAN_UP       BUTTON_UP
#define PACMAN_DOWN     BUTTON_DOWN
#define PACMAN_LEFT     BUTTON_LEFT
#define PACMAN_RIGHT    BUTTON_RIGHT
#define PACMAN_1UP      BUTTON_SELECT
#define PACMAN_2UP      BUTTON_REC
#define PACMAN_COIN_PRE BUTTON_SELECT
#define PACMAN_COIN     (BUTTON_SELECT | BUTTON_DOWN)
#define PACMAN_MENU     BUTTON_POWER

#elif CONFIG_KEYPAD == IRIVER_H10_PAD

#if defined(IRIVER_H10_5GB)
#define PACMAN_UP      BUTTON_SCROLL_UP
#define PACMAN_DOWN    BUTTON_SCROLL_DOWN
#define PACMAN_LEFT    BUTTON_LEFT
#define PACMAN_RIGHT   BUTTON_RIGHT
#else
#define PACMAN_UP      BUTTON_RIGHT
#define PACMAN_DOWN    BUTTON_LEFT
#define PACMAN_LEFT    BUTTON_SCROLL_UP
#define PACMAN_RIGHT   BUTTON_SCROLL_DOWN
#endif

#define PACMAN_1UP     BUTTON_REW
#define PACMAN_2UP     BUTTON_POWER
#define PACMAN_COIN    BUTTON_FF
#define PACMAN_MENU    BUTTON_PLAY

#elif CONFIG_KEYPAD == MROBE500_PAD

#define PACMAN_UP       BUTTON_RC_PLAY
#define PACMAN_DOWN     BUTTON_RC_DOWN
#define PACMAN_LEFT     BUTTON_RC_REW
#define PACMAN_RIGHT    BUTTON_RC_FF
#define PACMAN_1UP      BUTTON_RC_VOL_DOWN
#define PACMAN_2UP      BUTTON_RC_VOL_UP
#define PACMAN_COIN_PRE BUTTON_RC_MODE
#define PACMAN_COIN     (BUTTON_RC_MODE | BUTTON_RC_DOWN)
#define PACMAN_MENU     BUTTON_POWER

#elif CONFIG_KEYPAD == COWOND2_PAD

#define PACMAN_MENU     (BUTTON_MENU|BUTTON_REL)

#else

#error Keymap not defined!

#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef PACMAN_UP
#define PACMAN_UP       BUTTON_MIDRIGHT
#endif
#ifndef PACMAN_DOWN
#define PACMAN_DOWN     BUTTON_MIDLEFT
#endif
#ifndef PACMAN_LEFT
#define PACMAN_LEFT     BUTTON_TOPMIDDLE
#endif
#ifndef PACMAN_RIGHT
#define PACMAN_RIGHT    BUTTON_BOTTOMMIDDLE
#endif
#ifndef PACMAN_1UP
#define PACMAN_1UP      BUTTON_BOTTOMLEFT
#endif
#ifndef PACMAN_2UP
#define PACMAN_2UP      BUTTON_BOTTOMRIGHT
#endif
#ifndef PACMAN_COIN
#define PACMAN_COIN     BUTTON_CENTER
#endif
#ifndef PACMAN_MENU
#define PACMAN_MENU    (BUTTON_TOPLEFT|BUTTON_REL)
#endif
#endif

#if (LCD_HEIGHT >= 288)
#define XOFS ((LCD_WIDTH-224)/2)
#define YOFS ((LCD_HEIGHT-288)/2)
#elif (LCD_WIDTH >= 288)
#define XOFS ((LCD_WIDTH-288)/2)
#define YOFS ((LCD_HEIGHT-224)/2)
#elif (LCD_WIDTH >= 220)
#define XOFS ((LCD_WIDTH-(288*3/4))/2)
#define YOFS ((LCD_HEIGHT-(224*3/4))/2)
#elif (LCD_WIDTH >= 168) && (LCD_HEIGHT >= 216)
#define XOFS ((LCD_WIDTH-(224*3/4))/2)
#define YOFS ((LCD_HEIGHT-(288*3/4))/2)
#elif (LCD_WIDTH >= 144)
#define XOFS ((LCD_WIDTH-288/2)/2)
#define YOFS ((LCD_HEIGHT-224/2)/2)
#elif (LCD_WIDTH >= 128)
#define XOFS ((LCD_WIDTH-224/2)/2)
#define YCLIP ((288-2*LCD_HEIGHT)/2)
#endif

/* How many video frames (out of a possible 60) we display each second.
   NOTE: pacbox.c assumes this is an integer divisor of 60
 */
#if defined(IPOD_NANO) || defined (TOSHIBA_GIGABEAT_F)
/* The Nano and Gigabeat can manage full-speed at 30fps (1 in 2 frames) */
#define FPS 30
#else
/* We aim for 20fps on the other targets (1 in 3 frames) */
#define FPS 20
#endif

#endif
