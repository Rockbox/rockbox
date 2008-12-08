/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* JPEG image viewer
* (This is a real mess if it has to be coded in one single C file)
*
* File scrolling addition (C) 2005 Alexander Spyridakis
* Copyright (C) 2004 Jörg Hohensohn aka [IDC]Dragon
* Heavily borrowed from the IJG implementation (C) Thomas G. Lane
* Small & fast downscaling IDCT (C) 2002 by Guido Vollbeding  JPEGclub.org
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

#ifndef _JPEG_JPEG_H
#define _JPEG_JPEG_H

#include "plugin.h"

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define JPEG_ZOOM_IN BUTTON_PLAY
#define JPEG_ZOOM_OUT BUTTON_ON
#define JPEG_UP BUTTON_UP
#define JPEG_DOWN BUTTON_DOWN
#define JPEG_LEFT BUTTON_LEFT
#define JPEG_RIGHT BUTTON_RIGHT
#define JPEG_NEXT BUTTON_F3
#define JPEG_PREVIOUS BUTTON_F2
#define JPEG_MENU BUTTON_OFF

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define JPEG_ZOOM_IN BUTTON_SELECT
#define JPEG_ZOOM_OUT BUTTON_ON
#define JPEG_UP BUTTON_UP
#define JPEG_DOWN BUTTON_DOWN
#define JPEG_LEFT BUTTON_LEFT
#define JPEG_RIGHT BUTTON_RIGHT
#define JPEG_NEXT BUTTON_F3
#define JPEG_PREVIOUS BUTTON_F2
#define JPEG_MENU BUTTON_OFF

#elif CONFIG_KEYPAD == ONDIO_PAD
#define JPEG_ZOOM_PRE BUTTON_MENU
#define JPEG_ZOOM_IN (BUTTON_MENU | BUTTON_REL)
#define JPEG_ZOOM_OUT (BUTTON_MENU | BUTTON_DOWN)
#define JPEG_UP BUTTON_UP
#define JPEG_DOWN BUTTON_DOWN
#define JPEG_LEFT BUTTON_LEFT
#define JPEG_RIGHT BUTTON_RIGHT
#define JPEG_NEXT (BUTTON_MENU | BUTTON_RIGHT)
#define JPEG_PREVIOUS (BUTTON_MENU | BUTTON_LEFT)
#define JPEG_MENU BUTTON_OFF

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define JPEG_ZOOM_IN BUTTON_SELECT
#define JPEG_ZOOM_OUT BUTTON_MODE
#define JPEG_UP BUTTON_UP
#define JPEG_DOWN BUTTON_DOWN
#define JPEG_LEFT BUTTON_LEFT
#define JPEG_RIGHT BUTTON_RIGHT
#if (CONFIG_KEYPAD == IRIVER_H100_PAD)
#define JPEG_NEXT BUTTON_ON
#define JPEG_PREVIOUS BUTTON_REC
#else
#define JPEG_NEXT BUTTON_REC
#define JPEG_PREVIOUS BUTTON_ON
#endif
#define JPEG_MENU BUTTON_OFF
#define JPEG_RC_MENU BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define JPEG_ZOOM_IN BUTTON_SCROLL_FWD
#define JPEG_ZOOM_OUT BUTTON_SCROLL_BACK
#define JPEG_UP BUTTON_MENU
#define JPEG_DOWN BUTTON_PLAY
#define JPEG_LEFT BUTTON_LEFT
#define JPEG_RIGHT BUTTON_RIGHT
#define JPEG_MENU (BUTTON_SELECT | BUTTON_MENU)
#define JPEG_NEXT (BUTTON_SELECT | BUTTON_RIGHT)
#define JPEG_PREVIOUS (BUTTON_SELECT | BUTTON_LEFT)

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define JPEG_ZOOM_PRE BUTTON_SELECT
#define JPEG_ZOOM_IN (BUTTON_SELECT | BUTTON_REL)
#define JPEG_ZOOM_OUT (BUTTON_SELECT | BUTTON_REPEAT)
#define JPEG_UP BUTTON_UP
#define JPEG_DOWN BUTTON_DOWN
#define JPEG_LEFT BUTTON_LEFT
#define JPEG_RIGHT BUTTON_RIGHT
#define JPEG_MENU BUTTON_POWER
#define JPEG_NEXT BUTTON_PLAY
#define JPEG_PREVIOUS BUTTON_REC

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define JPEG_ZOOM_IN BUTTON_VOL_UP
#define JPEG_ZOOM_OUT BUTTON_VOL_DOWN
#define JPEG_UP BUTTON_UP
#define JPEG_DOWN BUTTON_DOWN
#define JPEG_LEFT BUTTON_LEFT
#define JPEG_RIGHT BUTTON_RIGHT
#define JPEG_MENU BUTTON_MENU
#define JPEG_NEXT (BUTTON_A | BUTTON_RIGHT)
#define JPEG_PREVIOUS (BUTTON_A | BUTTON_LEFT)

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define JPEG_ZOOM_PRE           BUTTON_SELECT
#define JPEG_ZOOM_IN            (BUTTON_SELECT | BUTTON_REL)
#define JPEG_ZOOM_OUT           (BUTTON_SELECT | BUTTON_REPEAT)
#define JPEG_UP                 BUTTON_UP
#define JPEG_DOWN               BUTTON_DOWN
#define JPEG_LEFT               BUTTON_LEFT
#define JPEG_RIGHT              BUTTON_RIGHT
#define JPEG_MENU               BUTTON_POWER
#define JPEG_SLIDE_SHOW         BUTTON_REC
#define JPEG_NEXT               BUTTON_SCROLL_FWD
#define JPEG_NEXT_REPEAT        (BUTTON_SCROLL_FWD|BUTTON_REPEAT)
#define JPEG_PREVIOUS           BUTTON_SCROLL_BACK
#define JPEG_PREVIOUS_REPEAT    (BUTTON_SCROLL_BACK|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define JPEG_ZOOM_PRE           BUTTON_SELECT
#define JPEG_ZOOM_IN            (BUTTON_SELECT | BUTTON_REL)
#define JPEG_ZOOM_OUT           (BUTTON_SELECT | BUTTON_REPEAT)
#define JPEG_UP                 BUTTON_UP
#define JPEG_DOWN               BUTTON_DOWN
#define JPEG_LEFT               BUTTON_LEFT
#define JPEG_RIGHT              BUTTON_RIGHT
#define JPEG_MENU               BUTTON_POWER
#define JPEG_SLIDE_SHOW         BUTTON_REC
#define JPEG_NEXT               BUTTON_VOL_UP
#define JPEG_NEXT_REPEAT        (BUTTON_VOL_UP|BUTTON_REPEAT)
#define JPEG_PREVIOUS           BUTTON_VOL_DOWN
#define JPEG_PREVIOUS_REPEAT    (BUTTON_VOL_DOWN|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == SANSA_CLIP_PAD
#define JPEG_ZOOM_PRE           BUTTON_SELECT
#define JPEG_ZOOM_IN            (BUTTON_SELECT | BUTTON_REL)
#define JPEG_ZOOM_OUT           (BUTTON_SELECT | BUTTON_REPEAT)
#define JPEG_UP                 BUTTON_UP
#define JPEG_DOWN               BUTTON_DOWN
#define JPEG_LEFT               BUTTON_LEFT
#define JPEG_RIGHT              BUTTON_RIGHT
#define JPEG_MENU               BUTTON_POWER
#define JPEG_SLIDE_SHOW         BUTTON_HOME
#define JPEG_NEXT               BUTTON_VOL_UP
#define JPEG_NEXT_REPEAT        (BUTTON_VOL_UP|BUTTON_REPEAT)
#define JPEG_PREVIOUS           BUTTON_VOL_DOWN
#define JPEG_PREVIOUS_REPEAT    (BUTTON_VOL_DOWN|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define JPEG_ZOOM_PRE BUTTON_PLAY
#define JPEG_ZOOM_IN (BUTTON_PLAY | BUTTON_REL)
#define JPEG_ZOOM_OUT (BUTTON_PLAY | BUTTON_REPEAT)
#define JPEG_UP BUTTON_SCROLL_UP
#define JPEG_DOWN BUTTON_SCROLL_DOWN
#define JPEG_LEFT BUTTON_LEFT
#define JPEG_RIGHT BUTTON_RIGHT
#define JPEG_MENU BUTTON_POWER
#define JPEG_NEXT BUTTON_FF
#define JPEG_PREVIOUS BUTTON_REW

#elif CONFIG_KEYPAD == MROBE500_PAD
#define JPEG_ZOOM_IN    BUTTON_RC_VOL_UP
#define JPEG_ZOOM_OUT   BUTTON_RC_VOL_DOWN
#define JPEG_UP         BUTTON_RC_PLAY
#define JPEG_DOWN       BUTTON_RC_DOWN
#define JPEG_LEFT       BUTTON_LEFT
#define JPEG_RIGHT      BUTTON_RIGHT
#define JPEG_MENU       BUTTON_POWER
#define JPEG_NEXT       BUTTON_RC_HEART
#define JPEG_PREVIOUS   BUTTON_RC_MODE

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define JPEG_ZOOM_IN    BUTTON_VOL_UP
#define JPEG_ZOOM_OUT   BUTTON_VOL_DOWN
#define JPEG_UP         BUTTON_UP
#define JPEG_DOWN       BUTTON_DOWN
#define JPEG_LEFT       BUTTON_LEFT
#define JPEG_RIGHT      BUTTON_RIGHT
#define JPEG_MENU       BUTTON_MENU
#define JPEG_NEXT       BUTTON_NEXT
#define JPEG_PREVIOUS   BUTTON_PREV

#elif CONFIG_KEYPAD == MROBE100_PAD
#define JPEG_ZOOM_IN BUTTON_SELECT
#define JPEG_ZOOM_OUT BUTTON_PLAY
#define JPEG_UP BUTTON_UP
#define JPEG_DOWN BUTTON_DOWN
#define JPEG_LEFT BUTTON_LEFT
#define JPEG_RIGHT BUTTON_RIGHT
#define JPEG_MENU BUTTON_MENU
#define JPEG_NEXT (BUTTON_DISPLAY | BUTTON_RIGHT)
#define JPEG_PREVIOUS (BUTTON_DISPLAY | BUTTON_LEFT)

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define JPEG_ZOOM_PRE    BUTTON_RC_PLAY
#define JPEG_ZOOM_IN     (BUTTON_RC_PLAY|BUTTON_REL)
#define JPEG_ZOOM_OUT    (BUTTON_RC_PLAY|BUTTON_REPEAT)
#define JPEG_UP          BUTTON_RC_VOL_UP
#define JPEG_DOWN        BUTTON_RC_VOL_DOWN
#define JPEG_LEFT        BUTTON_RC_REW
#define JPEG_RIGHT       BUTTON_RC_FF
#define JPEG_MENU        BUTTON_RC_REC
#define JPEG_NEXT        BUTTON_RC_MODE
#define JPEG_PREVIOUS    BUTTON_RC_MENU

#elif CONFIG_KEYPAD == COWOND2_PAD

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define JPEG_ZOOM_IN     BUTTON_VOLUP
#define JPEG_ZOOM_OUT    BUTTON_VOLDOWN
#define JPEG_UP          BUTTON_STOP
#define JPEG_DOWN        BUTTON_PLAY
#define JPEG_LEFT        BUTTON_LEFT
#define JPEG_RIGHT       BUTTON_RIGHT
#define JPEG_MENU        BUTTON_MENU
#define JPEG_NEXT        (BUTTON_PLAY|BUTTON_VOLUP)
#define JPEG_PREVIOUS    (BUTTON_PLAY|BUTTON_VOLDOWN)

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD

#define JPEG_ZOOM_IN    BUTTON_PLAY
#define JPEG_ZOOM_OUT   BUTTON_CUSTOM
#define JPEG_UP         BUTTON_UP
#define JPEG_DOWN       BUTTON_DOWN
#define JPEG_LEFT       BUTTON_LEFT
#define JPEG_RIGHT      BUTTON_RIGHT
#define JPEG_MENU       BUTTON_MENU
#define JPEG_NEXT       BUTTON_SELECT
#define JPEG_PREVIOUS   BUTTON_BACK

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef JPEG_UP
#define JPEG_UP         BUTTON_TOPMIDDLE
#endif
#ifndef JPEG_DOWN
#define JPEG_DOWN       BUTTON_BOTTOMMIDDLE
#endif
#ifndef JPEG_LEFT
#define JPEG_LEFT       BUTTON_MIDLEFT
#endif
#ifndef JPEG_RIGHT
#define JPEG_RIGHT      BUTTON_MIDRIGHT
#endif
#ifndef JPEG_ZOOM_IN
#define JPEG_ZOOM_IN    BUTTON_TOPRIGHT
#endif
#ifndef JPEG_ZOOM_OUT
#define JPEG_ZOOM_OUT   BUTTON_TOPLEFT
#endif
#ifndef JPEG_MENU
#define JPEG_MENU       (BUTTON_CENTER|BUTTON_REL)
#endif
#ifndef JPEG_NEXT
#define JPEG_NEXT       BUTTON_BOTTOMRIGHT
#endif
#ifndef JPEG_PREVIOUS
#define JPEG_PREVIOUS   BUTTON_BOTTOMLEFT
#endif
#endif


#endif /* _JPEG_JPEG_H */
