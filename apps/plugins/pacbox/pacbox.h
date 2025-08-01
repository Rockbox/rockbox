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

#elif CONFIG_KEYPAD == SANSA_FUZE_PAD

#define PACMAN_UP       BUTTON_RIGHT
#define PACMAN_DOWN     BUTTON_LEFT
#define PACMAN_LEFT     BUTTON_UP
#define PACMAN_RIGHT    BUTTON_DOWN
#define PACMAN_1UP      BUTTON_SELECT
#define PACMAN_COIN_PRE BUTTON_SELECT
#define PACMAN_COIN     (BUTTON_SELECT | BUTTON_DOWN)
#define PACMAN_MENU     BUTTON_HOME

#elif CONFIG_KEYPAD == SANSA_CLIP_PAD

#define PACMAN_UP       BUTTON_UP
#define PACMAN_DOWN     BUTTON_DOWN
#define PACMAN_LEFT     BUTTON_LEFT
#define PACMAN_RIGHT    BUTTON_RIGHT
#define PACMAN_1UP      BUTTON_SELECT
#define PACMAN_COIN_PRE BUTTON_SELECT
#define PACMAN_COIN     (BUTTON_SELECT | BUTTON_DOWN)
#define PACMAN_MENU     BUTTON_HOME

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

#elif CONFIG_KEYPAD == COWON_D2_PAD

#define PACMAN_MENU     (BUTTON_MENU|BUTTON_REL)

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD

#define PACMAN_UP      BUTTON_UP
#define PACMAN_DOWN    BUTTON_DOWN
#define PACMAN_LEFT    BUTTON_LEFT
#define PACMAN_RIGHT   BUTTON_RIGHT
#define PACMAN_1UP     BUTTON_CUSTOM
#define PACMAN_2UP     BUTTON_PLAY
#define PACMAN_COIN    BUTTON_SELECT
#define PACMAN_MENU    BUTTON_MENU

#elif CONFIG_KEYPAD == CREATIVE_ZENXFI3_PAD

#define PACMAN_UP       BUTTON_UP
#define PACMAN_DOWN     BUTTON_DOWN
#define PACMAN_LEFT     BUTTON_BACK
#define PACMAN_RIGHT    BUTTON_MENU
#define PACMAN_1UP      BUTTON_PLAY
#define PACMAN_COIN_PRE BUTTON_PLAY
#define PACMAN_COIN     (BUTTON_PLAY | BUTTON_DOWN)
#define PACMAN_MENU     BUTTON_POWER

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD

#define PACMAN_UP      BUTTON_UP
#define PACMAN_DOWN    BUTTON_DOWN
#define PACMAN_LEFT    BUTTON_LEFT
#define PACMAN_RIGHT   BUTTON_RIGHT
#define PACMAN_1UP     BUTTON_VOL_UP
#define PACMAN_2UP     BUTTON_VOL_DOWN
#define PACMAN_COIN    BUTTON_VIEW
#define PACMAN_MENU    BUTTON_MENU

#elif CONFIG_KEYPAD == PHILIPS_HDD6330_PAD

#define PACMAN_UP      BUTTON_NEXT
#define PACMAN_DOWN    BUTTON_PREV
#define PACMAN_LEFT    BUTTON_UP
#define PACMAN_RIGHT   BUTTON_DOWN
#define PACMAN_1UP     BUTTON_VOL_UP
#define PACMAN_2UP     BUTTON_DOWN
#define PACMAN_COIN    BUTTON_RIGHT
#define PACMAN_MENU    BUTTON_MENU

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD

#define PACMAN_UP      BUTTON_UP
#define PACMAN_DOWN    BUTTON_DOWN
#define PACMAN_LEFT    BUTTON_PREV
#define PACMAN_RIGHT   BUTTON_NEXT
#define PACMAN_1UP     BUTTON_VOL_UP
#define PACMAN_2UP     BUTTON_VOL_DOWN
#define PACMAN_COIN    BUTTON_MENU
#define PACMAN_MENU    BUTTON_POWER

#elif CONFIG_KEYPAD == ONDAVX747_PAD

#define PACMAN_MENU     (BUTTON_MENU|BUTTON_REL)

#elif CONFIG_KEYPAD == ONDAVX777_PAD
#define PACMAN_MENU     BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YH92X_PAD

#define PACMAN_UP       BUTTON_RIGHT
#define PACMAN_DOWN     BUTTON_LEFT
#define PACMAN_LEFT     BUTTON_UP
#define PACMAN_RIGHT    BUTTON_DOWN
#define PACMAN_1UP      BUTTON_FFWD
#define PACMAN_COIN     BUTTON_PLAY
#define PACMAN_MENU     BUTTON_REW

#elif CONFIG_KEYPAD == SAMSUNG_YH820_PAD

#define PACMAN_UP       BUTTON_RIGHT
#define PACMAN_DOWN     BUTTON_LEFT
#define PACMAN_LEFT     BUTTON_UP
#define PACMAN_RIGHT    BUTTON_DOWN
#define PACMAN_1UP      BUTTON_FFWD
#define PACMAN_2UP      BUTTON_REW
#define PACMAN_COIN     BUTTON_PLAY
#define PACMAN_MENU     BUTTON_REC

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD

#define PACMAN_UP       BUTTON_UP
#define PACMAN_DOWN     BUTTON_DOWN
#define PACMAN_LEFT     BUTTON_PREV
#define PACMAN_RIGHT    BUTTON_NEXT
#define PACMAN_1UP      BUTTON_PLAY
#define PACMAN_2UP      BUTTON_REC
#define PACMAN_COIN     BUTTON_OK
#define PACMAN_MENU     BUTTON_MENU

#elif CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD

#define PACMAN_UP       BUTTON_UP
#define PACMAN_DOWN     BUTTON_DOWN
#define PACMAN_LEFT     BUTTON_LEFT
#define PACMAN_RIGHT    BUTTON_RIGHT
#define PACMAN_1UP      BUTTON_SELECT
#define PACMAN_2UP      BUTTON_BOTTOMRIGHT
#define PACMAN_COIN     BUTTON_PLAYPAUSE
#define PACMAN_MENU     BUTTON_POWER
#define SKIP_LEVEL      BUTTON_VOL_DOWN

#elif CONFIG_KEYPAD == SANSA_CONNECT_PAD

#define PACMAN_UP       BUTTON_UP
#define PACMAN_DOWN     BUTTON_DOWN
#define PACMAN_LEFT     BUTTON_LEFT
#define PACMAN_RIGHT    BUTTON_RIGHT
#define PACMAN_1UP      BUTTON_SELECT
#define PACMAN_2UP      BUTTON_NEXT
#define PACMAN_COIN     BUTTON_VOL_DOWN
#define PACMAN_MENU     BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YPR0_PAD

#define PACMAN_UP       BUTTON_UP
#define PACMAN_DOWN     BUTTON_DOWN
#define PACMAN_LEFT     BUTTON_LEFT
#define PACMAN_RIGHT    BUTTON_RIGHT
#define PACMAN_1UP      BUTTON_SELECT
#define PACMAN_2UP      BUTTON_POWER
#define PACMAN_COIN     BUTTON_USER
#define PACMAN_MENU     BUTTON_MENU

#elif CONFIG_KEYPAD == HM60X_PAD

#define PACMAN_UP       BUTTON_UP
#define PACMAN_DOWN     BUTTON_DOWN
#define PACMAN_LEFT     BUTTON_LEFT
#define PACMAN_RIGHT    BUTTON_RIGHT
#define PACMAN_1UP      BUTTON_SELECT
#define PACMAN_2UP      (BUTTON_POWER | BUTTON_UP)
#define PACMAN_COIN     (BUTTON_POWER | BUTTON_DOWN)
#define PACMAN_MENU     BUTTON_POWER

#elif CONFIG_KEYPAD == HM801_PAD

#define PACMAN_UP       BUTTON_UP
#define PACMAN_DOWN     BUTTON_DOWN
#define PACMAN_LEFT     BUTTON_LEFT
#define PACMAN_RIGHT    BUTTON_RIGHT
#define PACMAN_1UP      BUTTON_SELECT
#define PACMAN_2UP      BUTTON_PLAY
#define PACMAN_COIN     BUTTON_PREV
#define PACMAN_MENU     BUTTON_POWER

#elif CONFIG_KEYPAD == SONY_NWZ_PAD
#define PACMAN_UP       BUTTON_UP
#define PACMAN_DOWN     BUTTON_DOWN
#define PACMAN_LEFT     BUTTON_LEFT
#define PACMAN_RIGHT    BUTTON_RIGHT
#define PACMAN_MENU     BUTTON_BACK
#define PACMAN_1UP      BUTTON_POWER
#define PACMAN_COIN     BUTTON_PLAY

#elif CONFIG_KEYPAD == CREATIVE_ZEN_PAD
#define PACMAN_UP       BUTTON_LEFT
#define PACMAN_DOWN     BUTTON_RIGHT
#define PACMAN_LEFT     BUTTON_DOWN
#define PACMAN_RIGHT    BUTTON_UP
#define PACMAN_MENU     BUTTON_MENU
#define PACMAN_1UP      BUTTON_SELECT
#define PACMAN_COIN     BUTTON_PLAYPAUSE

#elif CONFIG_KEYPAD == DX50_PAD
#define PACMAN_MENU     BUTTON_POWER

#elif CONFIG_KEYPAD == CREATIVE_ZENXFI2_PAD
#define PACMAN_MENU     BUTTON_MENU

#elif CONFIG_KEYPAD == AGPTEK_ROCKER_PAD
#define PACMAN_UP       BUTTON_UP
#define PACMAN_DOWN     BUTTON_DOWN
#define PACMAN_LEFT     BUTTON_LEFT
#define PACMAN_RIGHT    BUTTON_RIGHT
#define PACMAN_MENU     BUTTON_POWER
#define PACMAN_1UP      BUTTON_VOLUP
#define PACMAN_COIN     BUTTON_VOLDOWN

#elif CONFIG_KEYPAD == XDUOO_X3II_PAD || CONFIG_KEYPAD == XDUOO_X20_PAD
#define PACMAN_UP       BUTTON_PREV
#define PACMAN_DOWN     BUTTON_NEXT
#define PACMAN_LEFT     BUTTON_HOME
#define PACMAN_RIGHT    BUTTON_VOL_DOWN
#define PACMAN_MENU     BUTTON_POWER
#define PACMAN_1UP      BUTTON_VOL_UP
#define PACMAN_COIN     BUTTON_PLAY

#elif CONFIG_KEYPAD == FIIO_M3K_LINUX_PAD
#define PACMAN_UP       BUTTON_PREV
#define PACMAN_DOWN     BUTTON_NEXT
#define PACMAN_LEFT     BUTTON_HOME
#define PACMAN_RIGHT    BUTTON_VOL_DOWN
#define PACMAN_MENU     BUTTON_POWER
#define PACMAN_1UP      BUTTON_VOL_UP
#define PACMAN_COIN     BUTTON_PLAY

#elif CONFIG_KEYPAD == IHIFI_770_PAD || CONFIG_KEYPAD == IHIFI_800_PAD

#define PACMAN_UP       BUTTON_PREV
#define PACMAN_DOWN     BUTTON_NEXT
#define PACMAN_LEFT     BUTTON_HOME
#define PACMAN_RIGHT    BUTTON_VOL_DOWN
#define PACMAN_MENU     BUTTON_POWER
#define PACMAN_1UP      BUTTON_VOL_UP
#define PACMAN_COIN     BUTTON_PLAY

#elif CONFIG_KEYPAD == EROSQ_PAD
#define PACMAN_UP       BUTTON_PREV
#define PACMAN_DOWN     BUTTON_NEXT
#define PACMAN_LEFT     BUTTON_SCROLL_BACK
#define PACMAN_RIGHT    BUTTON_SCROLL_FWD
#define PACMAN_MENU     BUTTON_MENU
#define PACMAN_1UP      BUTTON_VOL_UP
#define PACMAN_COIN     BUTTON_PLAY

#elif CONFIG_KEYPAD == FIIO_M3K_PAD
#define PACMAN_UP       BUTTON_UP
#define PACMAN_DOWN     BUTTON_DOWN
#define PACMAN_LEFT     BUTTON_LEFT
#define PACMAN_RIGHT    BUTTON_RIGHT
#define PACMAN_MENU     BUTTON_MENU
#define PACMAN_1UP      BUTTON_VOL_UP
#define PACMAN_COIN     BUTTON_PLAY

#elif CONFIG_KEYPAD == SHANLING_Q1_PAD
#define PACMAN_UP       BUTTON_TOPMIDDLE
#define PACMAN_DOWN     BUTTON_BOTTOMMIDDLE
#define PACMAN_LEFT     BUTTON_MIDLEFT
#define PACMAN_RIGHT    BUTTON_MIDRIGHT
#define PACMAN_MENU     BUTTON_TOPLEFT
#define PACMAN_1UP      BUTTON_BOTTOMLEFT
#define PACMAN_2UP      BUTTON_BOTTOMRIGHT
#define PACMAN_COIN     BUTTON_CENTER

#elif CONFIG_KEYPAD == MA_PAD

#define PACMAN_UP      BUTTON_UP
#define PACMAN_DOWN    BUTTON_DOWN
#define PACMAN_LEFT    BUTTON_LEFT
#define PACMAN_RIGHT   BUTTON_RIGHT
#define PACMAN_1UP     BUTTON_BACK
#define PACMAN_COIN    BUTTON_PLAY
#define PACMAN_MENU    BUTTON_MENU

#elif CONFIG_KEYPAD == RG_NANO_PAD

#define PACMAN_UP      BUTTON_UP
#define PACMAN_DOWN    BUTTON_DOWN
#define PACMAN_LEFT    BUTTON_LEFT
#define PACMAN_RIGHT   BUTTON_RIGHT
#define PACMAN_1UP     BUTTON_L
#define PACMAN_2UP     BUTTON_R
#define PACMAN_COIN    BUTTON_A
#define PACMAN_MENU    BUTTON_START

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

/* Calculate scaling and screen offset/clipping.
   Put native portrait mode before landscape, if a screen resulution allows both.
 */
#if (LCD_WIDTH >= 224) && (LCD_HEIGHT >= 288)
#define LCD_SCALE  100
#define LCD_ROTATE 0
#define XOFS ((LCD_WIDTH-224)/2)
#define YOFS ((LCD_HEIGHT-288)/2)

#elif (LCD_WIDTH >= 288) && (LCD_HEIGHT >= 224)
#if defined(CREATIVE_ZEN)
#define LCD_SCALE  100
#define LCD_ROTATE 2
#define XOFS ((LCD_WIDTH-288)/2)
#define YOFS ((LCD_HEIGHT-224)/2)
#else
#define LCD_SCALE  100
#define LCD_ROTATE 1
#define XOFS ((LCD_WIDTH-288)/2)
#define YOFS ((LCD_HEIGHT-224)/2)
#endif

#elif (LCD_WIDTH >= 168) && (LCD_HEIGHT >= 216)
#define LCD_SCALE  75
#define LCD_ROTATE 0
#define XOFS ((LCD_WIDTH-(224*3/4))/2)
#define YOFS ((LCD_HEIGHT-(288*3/4))/2)

#elif (LCD_WIDTH >= 216) && (LCD_HEIGHT >= 168)
#define LCD_SCALE  75
#define LCD_ROTATE 1
#define XOFS ((LCD_WIDTH-(288*3/4))/2)
#define YOFS ((LCD_HEIGHT-(224*3/4))/2)

#elif (LCD_WIDTH >= 144) && (LCD_HEIGHT >= 112)
#define LCD_SCALE  50
#define LCD_ROTATE 1
#define XOFS ((LCD_WIDTH-288/2)/2)
#define YOFS ((LCD_HEIGHT-224/2)/2)

#elif (LCD_WIDTH >= 112) && (LCD_HEIGHT >= 128)
#define LCD_SCALE  50
#define LCD_ROTATE 0
#define XOFS ((LCD_WIDTH-224/2)/2)
#if LCD_HEIGHT < 144
#define YCLIP ((288-2*LCD_HEIGHT)/2)
#define YOFS 0
#else
#define YCLIP 0
#define YOFS ((LCD_HEIGHT-288/2)/2)
#endif

#elif (LCD_WIDTH >= 116) && (LCD_HEIGHT >= 90)
#define LCD_SCALE  40
#define LCD_ROTATE 1
#define XOFS  ((LCD_HEIGHT-224*2/5)/2)
#define YOFS  ((LCD_WIDTH-288*2/5)/2)

#elif (LCD_WIDTH >= 75) && (LCD_HEIGHT >= 96)
#define LCD_SCALE  33
#define LCD_ROTATE 0
#define XOFS  ((LCD_HEIGHT-224/3)/2)
#define YOFS  ((LCD_WIDTH-288/3)/2)

#else
#error "unsupported screen resolution"
#endif

/* How many video frames (out of a possible 60) we display each second.
   NOTE: pacbox.c assumes this is an integer divisor of 60
 */
#if defined(TOSHIBA_GIGABEAT_S) || defined (TOSHIBA_GIGABEAT_F) || \
    defined(SANSA_FUZEPLUS)
/* Gigabeat S,F and Sansa Fuze+ can manage the full framerate
   (1 in 1 frames) */
#define FPS 60
#elif defined(IPOD_NANO)
/* The Nano can manage full-speed at 30fps (1 in 2 frames) */
#define FPS 30
#else
/* We aim for 20fps on the other targets (1 in 3 frames) */
#define FPS 20
#endif

/* 16kHz sounds pretty good - use it if available */
#define PREFERRED_SAMPLING_RATE SAMPR_16

#endif
