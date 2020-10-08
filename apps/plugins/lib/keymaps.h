/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2017 Franklin Wei
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

/* The purpose of this file is to provide a supplement to the
 * functionality provided by PLA (pluginlib_actions), not to replace
 * it. Using PLA benefits both the plugin author, who no longer needs
 * to manually create keymaps, as well as the author of a new port,
 * who can easily adjust a centralized keymap. However, it lacks
 * flexibility for plugin authors in multiple aspects, namely the lack
 * of release detection and simultaneous button presses, leading some
 * plugins to rely upon individual keymaps, which complicates a new port.
 *
 * This file aims to remedy this by providing a centralized keymap of
 * the most basic keys: 4 directions, "fire" or select, and a
 * menu/pause/meta key.
 *
 * If you are writing a new plugin, *please* consider using these
 * keymaps instead of rolling your own!
 */

#ifndef __PLUGINLIB_KEYMAPS_H__
#define __PLUGINLIB_KEYMAPS_H__

#if (CONFIG_KEYPAD == PHILIPS_HDD1630_PAD)  || \
    (CONFIG_KEYPAD == PHILIPS_HDD6330_PAD)  || \
    (CONFIG_KEYPAD == PHILIPS_SA9200_PAD)   || \
    (CONFIG_KEYPAD == CREATIVE_ZENXFI3_PAD) || \
    (CONFIG_KEYPAD == SANSA_CONNECT_PAD)    || \
    (CONFIG_KEYPAD == SANSA_C200_PAD)       || \
    (CONFIG_KEYPAD == SANSA_CLIP_PAD)       || \
    (CONFIG_KEYPAD == SANSA_E200_PAD)       || \
    (CONFIG_KEYPAD == SANSA_FUZE_PAD)       || \
    (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)   || \
    (CONFIG_KEYPAD == GIGABEAT_PAD)         || \
    (CONFIG_KEYPAD == GIGABEAT_S_PAD)       || \
    (CONFIG_KEYPAD == SAMSUNG_YH92X_PAD)    || \
    (CONFIG_KEYPAD == SAMSUNG_YH820_PAD)    || \
    (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)      || \
    (CONFIG_KEYPAD == CREATIVE_ZEN_PAD)     || \
    (CONFIG_KEYPAD == SONY_NWZ_PAD)         || \
    (CONFIG_KEYPAD == CREATIVEZVM_PAD)      || \
    (CONFIG_KEYPAD == SAMSUNG_YPR0_PAD)     || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)      || \
    (CONFIG_KEYPAD == HM801_PAD)            || \
    (CONFIG_KEYPAD == HM60X_PAD)            || \
    (CONFIG_KEYPAD == AGPTEK_ROCKER_PAD)
#define BTN_UP         BUTTON_UP
#define BTN_DOWN       BUTTON_DOWN
#define BTN_LEFT       BUTTON_LEFT
#define BTN_RIGHT      BUTTON_RIGHT

#if (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
#define BTN_HAVE_DIAGONAL
#define BTN_UP_LEFT    BUTTON_BACK
#define BTN_UP_RIGHT   BUTTON_PLAYPAUSE
#define BTN_DOWN_LEFT  BUTTON_BOTTOMLEFT
#define BTN_DOWN_RIGHT BUTTON_BOTTOMRIGHT
#endif

/* X-Fi doesn't have a separate *_PAD macro */
#ifdef CREATIVE_ZENXFI
#define BTN_HAVE_DIAGONAL
#define BTN_DOWN_LEFT  BUTTON_BOTTOMLEFT
#define BTN_DOWN_RIGHT BUTTON_BOTTOMRIGHT
#define BTN_UP_LEFT    BUTTON_TOPLEFT
#define BTN_UP_RIGHT   BUTTON_TOPRIGHT
#endif

#if (CONFIG_KEYPAD == AGPTEK_ROCKER_PAD)
#define BTN_FIRE       BUTTON_SELECT
#define BTN_PAUSE      BUTTON_POWER
#endif

#if (CONFIG_KEYPAD == HM60X_PAD)
#define BTN_FIRE       BUTTON_POWER
#define BTN_PAUSE      BUTTON_SELECT
#endif

#if (CONFIG_KEYPAD == PHILIPS_HDD1630_PAD)  || \
    (CONFIG_KEYPAD == SANSA_CONNECT_PAD)    || \
    (CONFIG_KEYPAD == SANSA_C200_PAD)       || \
    (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)   || \
    (CONFIG_KEYPAD == ONDAVX747_PAD)
#define BTN_FIRE       BUTTON_SELECT
#define BTN_PAUSE      BUTTON_POWER

#elif (CONFIG_KEYPAD == PHILIPS_SA9200_PAD)   || \
      (CONFIG_KEYPAD == PHILIPS_HDD6330_PAD)  || \
      (CONFIG_KEYPAD == CREATIVE_ZENXFI3_PAD)
#define BTN_FIRE       BUTTON_VOL_UP
#define BTN_PAUSE      BUTTON_VOL_DOWN

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define BTN_FIRE       BUTTON_SELECT
#define BTN_PAUSE      BUTTON_HOME

#elif (CONFIG_KEYPAD == SAMSUNG_YH92X_PAD)
#define BTN_FIRE       BUTTON_FFWD
#define BTN_PAUSE      BUTTON_REW

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define BTN_FIRE       BUTTON_REC
#define BTN_PAUSE      BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_CLIP_PAD)
#define BTN_FIRE       BUTTON_SELECT
#define BTN_PAUSE      BUTTON_POWER

#elif (CONFIG_KEYPAD == CREATIVE_ZEN_PAD)
#define BTN_FIRE       BUTTON_SELECT
#define BTN_PAUSE      BUTTON_BACK

#elif (CONFIG_KEYPAD == CREATIVEZVM_PAD)
#define BTN_FIRE       BUTTON_PLAY
#define BTN_PAUSE      BUTTON_MENU

#elif (CONFIG_KEYPAD == SAMSUNG_YPR0_PAD)
#define BTN_FIRE       BUTTON_USER
#define BTN_PAUSE      BUTTON_MENU

#elif (CONFIG_KEYPAD == SONY_NWZ_PAD)
#define BTN_FIRE       BUTTON_PLAY
#define BTN_PAUSE      BUTTON_BACK

#elif (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define BTN_FIRE       BUTTON_REC
#define BTN_PAUSE      BUTTON_MODE

#elif (CONFIG_KEYPAD == HM801_PAD)
#define BTN_FIRE       BUTTON_PREV
#define BTN_PAUSE      BUTTON_NEXT

#elif (CONFIG_KEYPAD == SAMSUNG_YH820_PAD) || \
      (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define BTN_FIRE       BUTTON_REC
#define BTN_PAUSE      BUTTON_PLAY

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)      || \
      (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define BTN_FIRE       BUTTON_VOL_UP
#define BTN_PAUSE      BUTTON_MENU
/* #if CONFIG_KEYPAD == PHILIPS_HDD1630_PAD */
#endif

/* ... and now for the bad ones that don't have
 * standard names for the directional buttons */
#elif (CONFIG_KEYPAD == PBELL_VIBE500_PAD)
#define BTN_UP         BUTTON_OK
#define BTN_DOWN       BUTTON_CANCEL
#define BTN_LEFT       BUTTON_MENU
#define BTN_RIGHT      BUTTON_PLAY
#define BTN_FIRE       BUTTON_POWER
#define BTN_PAUSE      BUTTON_REC

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define BTN_UP         BUTTON_SCROLL_UP
#define BTN_DOWN       BUTTON_SCROLL_DOWN
#define BTN_LEFT       BUTTON_LEFT
#define BTN_RIGHT      BUTTON_RIGHT
#define BTN_FIRE       BUTTON_REW
#define BTN_PAUSE      BUTTON_PLAY

#elif (CONFIG_KEYPAD == MROBE500_PAD)
#define BTN_FIRE      BUTTON_POWER

#elif (CONFIG_KEYPAD == MROBE_REMOTE)
#define BTN_UP         BUTTON_RC_PLAY
#define BTN_DOWN       BUTTON_RC_DOWN
#define BTN_LEFT       BUTTON_RC_REW
#define BTN_RIGHT      BUTTON_RC_FF

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define BTN_UP         BUTTON_MENU
#define BTN_DOWN       BUTTON_PLAY
#define BTN_LEFT       BUTTON_LEFT
#define BTN_RIGHT      BUTTON_RIGHT
#define BTN_FIRE       BUTTON_SELECT
#define BTN_PAUSE      (BUTTON_MENU | BUTTON_SELECT)

#elif (CONFIG_KEYPAD == ONDAVX777_PAD)
#define BTN_FIRE       BUTTON_POWER

#elif (CONFIG_KEYPAD == COWON_D2_PAD)
#define BTN_FIRE       BUTTON_PLUS
#define BTN_PAUSE      BUTTON_MINUS

#elif (CONFIG_KEYPAD == ONDAVX747_PAD) || \
      (CONFIG_KEYPAD == DX50_PAD)
#define BTN_LEFT       BUTTON_LEFT
#define BTN_RIGHT      BUTTON_RIGHT
#define BTN_FIRE       BUTTON_BOTTOMLEFT
#define BTN_PAUSE      BUTTON_TOPLEFT

#elif CONFIG_KEYPAD == CREATIVE_ZENXFI2_PAD
#define BTN_FIRE       BUTTON_MENU
#define BTN_PAUSE      BUTTON_POWER

#elif (CONFIG_KEYPAD == XDUOO_X3_PAD) || (CONFIG_KEYPAD == XDUOO_X3II_PAD) || (CONFIG_KEYPAD == XDUOO_X20_PAD)
#define BTN_UP           BUTTON_VOL_UP
#define BTN_DOWN         BUTTON_VOL_DOWN
#define BTN_LEFT         BUTTON_PREV
#define BTN_RIGHT        BUTTON_NEXT
#define BTN_FIRE         BUTTON_PLAY
#define BTN_PAUSE        BUTTON_POWER

#elif (CONFIG_KEYPAD == FIIO_M3K_PAD)
#define BTN_UP           BUTTON_VOL_UP
#define BTN_DOWN         BUTTON_VOL_DOWN
#define BTN_LEFT         BUTTON_PREV
#define BTN_RIGHT        BUTTON_NEXT
#define BTN_FIRE         BUTTON_PLAY
#define BTN_PAUSE        BUTTON_POWER

#elif (CONFIG_KEYPAD == EROSQ_PAD)
#define BTN_UP           BUTTON_PREV
#define BTN_DOWN         BUTTON_NEXT
#define BTN_LEFT         BUTTON_SCROLL_BACK
#define BTN_RIGHT        BUTTON_SCROLL_FWD
#define BTN_FIRE         BUTTON_PLAY
#define BTN_PAUSE        BUTTON_POWER

#elif (CONFIG_KEYPAD == IHIFI_770_PAD) || (CONFIG_KEYPAD == IHIFI_800_PAD)
#define BTN_UP           BUTTON_PREV
#define BTN_DOWN         BUTTON_NEXT
#define BTN_LEFT         BUTTON_HOME
#define BTN_RIGHT        BUTTON_VOL_DOWN
#define BTN_FIRE         BUTTON_PLAY
#define BTN_PAUSE        BUTTON_POWER

#else
#error Unsupported keypad
#endif

#ifdef HAVE_TOUCHSCREEN
#define BTN_UP         BUTTON_TOPMIDDLE
#define BTN_DOWN       BUTTON_BOTTOMMIDDLE
#define BTN_LEFT       BUTTON_LEFT
#define BTN_RIGHT      BUTTON_RIGHT

#if (CONFIG_KEYPAD == MROBE500_PAD) || \
    (CONFIG_KEYPAD == ONDAVX777_PAD)
#define BTN_PAUSE      BUTTON_BOTTOMLEFT

#elif (CONFIG_KEYPAD != COWON_D2_PAD) && \
      (CONFIG_KEYPAD != DX50_PAD)     && \
      (CONFIG_KEYPAD != ONDAVX777_PAD) && \
      (CONFIG_KEYPAD != CREATIVE_ZENXFI2_PAD)
#define BTN_FIRE       BUTTON_BOTTOMLEFT
#define BTN_PAUSE      BUTTON_TOPLEFT
#endif

/* HAVE_TOUCHSCREEN */
#endif

/* __PLUGINLIB_KEYMAPS_H__ */
#endif
