/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$id $
 *
 * Copyright (C) 2009 by Christophe Gouiran <bechris13250 -at- gmail -dot- com>
 *
 * Based on lodepng, a lightweight png decoder/encoder
 * (c) 2005-2008 Lode Vandevenne
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

/*
LodePNG version 20080927

Copyright (c) 2005-2008 Lode Vandevenne

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

/*
The manual and changelog can be found in the header file "lodepng.h"
You are free to name this file lodepng.cpp or lodepng.c depending on your usage.
*/

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define PNG_ZOOM_IN BUTTON_PLAY
#define PNG_ZOOM_OUT BUTTON_ON
#define PNG_UP BUTTON_UP
#define PNG_DOWN BUTTON_DOWN
#define PNG_LEFT BUTTON_LEFT
#define PNG_RIGHT BUTTON_RIGHT
#define PNG_NEXT BUTTON_F3
#define PNG_PREVIOUS BUTTON_F2
#define PNG_MENU BUTTON_OFF

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define PNG_ZOOM_IN BUTTON_SELECT
#define PNG_ZOOM_OUT BUTTON_ON
#define PNG_UP BUTTON_UP
#define PNG_DOWN BUTTON_DOWN
#define PNG_LEFT BUTTON_LEFT
#define PNG_RIGHT BUTTON_RIGHT
#define PNG_NEXT BUTTON_F3
#define PNG_PREVIOUS BUTTON_F2
#define PNG_MENU BUTTON_OFF

#elif CONFIG_KEYPAD == ONDIO_PAD
#define PNG_ZOOM_PRE BUTTON_MENU
#define PNG_ZOOM_IN (BUTTON_MENU | BUTTON_REL)
#define PNG_ZOOM_OUT (BUTTON_MENU | BUTTON_DOWN)
#define PNG_UP BUTTON_UP
#define PNG_DOWN BUTTON_DOWN
#define PNG_LEFT BUTTON_LEFT
#define PNG_RIGHT BUTTON_RIGHT
#define PNG_NEXT (BUTTON_MENU | BUTTON_RIGHT)
#define PNG_PREVIOUS (BUTTON_MENU | BUTTON_LEFT)
#define PNG_MENU BUTTON_OFF

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define PNG_ZOOM_IN BUTTON_SELECT
#define PNG_ZOOM_OUT BUTTON_MODE
#define PNG_UP BUTTON_UP
#define PNG_DOWN BUTTON_DOWN
#define PNG_LEFT BUTTON_LEFT
#define PNG_RIGHT BUTTON_RIGHT
#if (CONFIG_KEYPAD == IRIVER_H100_PAD)
#define PNG_NEXT BUTTON_ON
#define PNG_PREVIOUS BUTTON_REC
#else
#define PNG_NEXT BUTTON_REC
#define PNG_PREVIOUS BUTTON_ON
#endif
#define PNG_MENU BUTTON_OFF
#define PNG_RC_MENU BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define PNG_ZOOM_IN BUTTON_SCROLL_FWD
#define PNG_ZOOM_OUT BUTTON_SCROLL_BACK
#define PNG_UP BUTTON_MENU
#define PNG_DOWN BUTTON_PLAY
#define PNG_LEFT BUTTON_LEFT
#define PNG_RIGHT BUTTON_RIGHT
#define PNG_MENU (BUTTON_SELECT | BUTTON_MENU)
#define PNG_NEXT (BUTTON_SELECT | BUTTON_RIGHT)
#define PNG_PREVIOUS (BUTTON_SELECT | BUTTON_LEFT)

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define PNG_ZOOM_PRE BUTTON_SELECT
#define PNG_ZOOM_IN (BUTTON_SELECT | BUTTON_REL)
#define PNG_ZOOM_OUT (BUTTON_SELECT | BUTTON_REPEAT)
#define PNG_UP BUTTON_UP
#define PNG_DOWN BUTTON_DOWN
#define PNG_LEFT BUTTON_LEFT
#define PNG_RIGHT BUTTON_RIGHT
#define PNG_MENU BUTTON_POWER
#define PNG_NEXT BUTTON_PLAY
#define PNG_PREVIOUS BUTTON_REC

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define PNG_ZOOM_IN BUTTON_VOL_UP
#define PNG_ZOOM_OUT BUTTON_VOL_DOWN
#define PNG_UP BUTTON_UP
#define PNG_DOWN BUTTON_DOWN
#define PNG_LEFT BUTTON_LEFT
#define PNG_RIGHT BUTTON_RIGHT
#define PNG_MENU BUTTON_MENU
#define PNG_NEXT (BUTTON_A | BUTTON_RIGHT)
#define PNG_PREVIOUS (BUTTON_A | BUTTON_LEFT)

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define PNG_ZOOM_PRE           BUTTON_SELECT
#define PNG_ZOOM_IN            (BUTTON_SELECT | BUTTON_REL)
#define PNG_ZOOM_OUT           (BUTTON_SELECT | BUTTON_REPEAT)
#define PNG_UP                 BUTTON_UP
#define PNG_DOWN               BUTTON_DOWN
#define PNG_LEFT               BUTTON_LEFT
#define PNG_RIGHT              BUTTON_RIGHT
#define PNG_MENU               BUTTON_POWER
#define PNG_SLIDE_SHOW         BUTTON_REC
#define PNG_NEXT               BUTTON_SCROLL_FWD
#define PNG_NEXT_REPEAT        (BUTTON_SCROLL_FWD|BUTTON_REPEAT)
#define PNG_PREVIOUS           BUTTON_SCROLL_BACK
#define PNG_PREVIOUS_REPEAT    (BUTTON_SCROLL_BACK|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
#define PNG_ZOOM_PRE           BUTTON_SELECT
#define PNG_ZOOM_IN            (BUTTON_SELECT | BUTTON_REL)
#define PNG_ZOOM_OUT           (BUTTON_SELECT | BUTTON_REPEAT)
#define PNG_UP                 BUTTON_UP
#define PNG_DOWN               BUTTON_DOWN
#define PNG_LEFT               BUTTON_LEFT
#define PNG_RIGHT              BUTTON_RIGHT
#define PNG_MENU               (BUTTON_HOME|BUTTON_REPEAT)
#define PNG_NEXT               BUTTON_SCROLL_FWD
#define PNG_NEXT_REPEAT        (BUTTON_SCROLL_FWD|BUTTON_REPEAT)
#define PNG_PREVIOUS           BUTTON_SCROLL_BACK
#define PNG_PREVIOUS_REPEAT    (BUTTON_SCROLL_BACK|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define PNG_ZOOM_PRE           BUTTON_SELECT
#define PNG_ZOOM_IN            (BUTTON_SELECT | BUTTON_REL)
#define PNG_ZOOM_OUT           (BUTTON_SELECT | BUTTON_REPEAT)
#define PNG_UP                 BUTTON_UP
#define PNG_DOWN               BUTTON_DOWN
#define PNG_LEFT               BUTTON_LEFT
#define PNG_RIGHT              BUTTON_RIGHT
#define PNG_MENU               BUTTON_POWER
#define PNG_SLIDE_SHOW         BUTTON_REC
#define PNG_NEXT               BUTTON_VOL_UP
#define PNG_NEXT_REPEAT        (BUTTON_VOL_UP|BUTTON_REPEAT)
#define PNG_PREVIOUS           BUTTON_VOL_DOWN
#define PNG_PREVIOUS_REPEAT    (BUTTON_VOL_DOWN|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == SANSA_CLIP_PAD
#define PNG_ZOOM_PRE           BUTTON_SELECT
#define PNG_ZOOM_IN            (BUTTON_SELECT | BUTTON_REL)
#define PNG_ZOOM_OUT           (BUTTON_SELECT | BUTTON_REPEAT)
#define PNG_UP                 BUTTON_UP
#define PNG_DOWN               BUTTON_DOWN
#define PNG_LEFT               BUTTON_LEFT
#define PNG_RIGHT              BUTTON_RIGHT
#define PNG_MENU               BUTTON_POWER
#define PNG_SLIDE_SHOW         BUTTON_HOME
#define PNG_NEXT               BUTTON_VOL_UP
#define PNG_NEXT_REPEAT        (BUTTON_VOL_UP|BUTTON_REPEAT)
#define PNG_PREVIOUS           BUTTON_VOL_DOWN
#define PNG_PREVIOUS_REPEAT    (BUTTON_VOL_DOWN|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == SANSA_M200_PAD
#define PNG_ZOOM_PRE           BUTTON_SELECT
#define PNG_ZOOM_IN            (BUTTON_SELECT | BUTTON_REL)
#define PNG_ZOOM_OUT           (BUTTON_SELECT | BUTTON_REPEAT)
#define PNG_UP                 BUTTON_UP
#define PNG_DOWN               BUTTON_DOWN
#define PNG_LEFT               BUTTON_LEFT
#define PNG_RIGHT              BUTTON_RIGHT
#define PNG_MENU               BUTTON_POWER
#define PNG_SLIDE_SHOW         (BUTTON_SELECT | BUTTON_UP)
#define PNG_NEXT               BUTTON_VOL_UP
#define PNG_NEXT_REPEAT        (BUTTON_VOL_UP|BUTTON_REPEAT)
#define PNG_PREVIOUS           BUTTON_VOL_DOWN
#define PNG_PREVIOUS_REPEAT    (BUTTON_VOL_DOWN|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define PNG_ZOOM_PRE BUTTON_PLAY
#define PNG_ZOOM_IN (BUTTON_PLAY | BUTTON_REL)
#define PNG_ZOOM_OUT (BUTTON_PLAY | BUTTON_REPEAT)
#define PNG_UP BUTTON_SCROLL_UP
#define PNG_DOWN BUTTON_SCROLL_DOWN
#define PNG_LEFT BUTTON_LEFT
#define PNG_RIGHT BUTTON_RIGHT
#define PNG_MENU BUTTON_POWER
#define PNG_NEXT BUTTON_FF
#define PNG_PREVIOUS BUTTON_REW

#elif CONFIG_KEYPAD == MROBE500_PAD

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define PNG_ZOOM_IN    BUTTON_VOL_UP
#define PNG_ZOOM_OUT   BUTTON_VOL_DOWN
#define PNG_UP         BUTTON_UP
#define PNG_DOWN       BUTTON_DOWN
#define PNG_LEFT       BUTTON_LEFT
#define PNG_RIGHT      BUTTON_RIGHT
#define PNG_MENU       BUTTON_MENU
#define PNG_NEXT       BUTTON_NEXT
#define PNG_PREVIOUS   BUTTON_PREV

#elif CONFIG_KEYPAD == MROBE100_PAD
#define PNG_ZOOM_IN BUTTON_SELECT
#define PNG_ZOOM_OUT BUTTON_PLAY
#define PNG_UP BUTTON_UP
#define PNG_DOWN BUTTON_DOWN
#define PNG_LEFT BUTTON_LEFT
#define PNG_RIGHT BUTTON_RIGHT
#define PNG_MENU BUTTON_MENU
#define PNG_NEXT (BUTTON_DISPLAY | BUTTON_RIGHT)
#define PNG_PREVIOUS (BUTTON_DISPLAY | BUTTON_LEFT)

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define PNG_ZOOM_PRE    BUTTON_RC_PLAY
#define PNG_ZOOM_IN     (BUTTON_RC_PLAY|BUTTON_REL)
#define PNG_ZOOM_OUT    (BUTTON_RC_PLAY|BUTTON_REPEAT)
#define PNG_UP          BUTTON_RC_VOL_UP
#define PNG_DOWN        BUTTON_RC_VOL_DOWN
#define PNG_LEFT        BUTTON_RC_REW
#define PNG_RIGHT       BUTTON_RC_FF
#define PNG_MENU        BUTTON_RC_REC
#define PNG_NEXT        BUTTON_RC_MODE
#define PNG_PREVIOUS    BUTTON_RC_MENU

#elif CONFIG_KEYPAD == COWON_D2_PAD

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define PNG_ZOOM_IN     BUTTON_VOLUP
#define PNG_ZOOM_OUT    BUTTON_VOLDOWN
#define PNG_UP          BUTTON_STOP
#define PNG_DOWN        BUTTON_PLAY
#define PNG_LEFT        BUTTON_LEFT
#define PNG_RIGHT       BUTTON_RIGHT
#define PNG_MENU        BUTTON_MENU
#define PNG_NEXT        (BUTTON_PLAY|BUTTON_VOLUP)
#define PNG_PREVIOUS    (BUTTON_PLAY|BUTTON_VOLDOWN)

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD

#define PNG_ZOOM_IN    BUTTON_PLAY
#define PNG_ZOOM_OUT   BUTTON_CUSTOM
#define PNG_UP         BUTTON_UP
#define PNG_DOWN       BUTTON_DOWN
#define PNG_LEFT       BUTTON_LEFT
#define PNG_RIGHT      BUTTON_RIGHT
#define PNG_MENU       BUTTON_MENU
#define PNG_NEXT       BUTTON_SELECT
#define PNG_PREVIOUS   BUTTON_BACK

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define PNG_ZOOM_IN    BUTTON_VOL_UP
#define PNG_ZOOM_OUT   BUTTON_VOL_DOWN
#define PNG_UP         BUTTON_UP
#define PNG_DOWN       BUTTON_DOWN
#define PNG_LEFT       BUTTON_LEFT
#define PNG_RIGHT      BUTTON_RIGHT
#define PNG_MENU       BUTTON_MENU
#define PNG_NEXT       BUTTON_VIEW
#define PNG_PREVIOUS   BUTTON_PLAYLIST

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define PNG_ZOOM_IN    BUTTON_VOL_UP
#define PNG_ZOOM_OUT   BUTTON_VOL_DOWN
#define PNG_UP         BUTTON_UP
#define PNG_DOWN       BUTTON_DOWN
#define PNG_LEFT       BUTTON_PREV
#define PNG_RIGHT      BUTTON_NEXT
#define PNG_MENU       BUTTON_MENU
#define PNG_NEXT       BUTTON_RIGHT
#define PNG_PREVIOUS   BUTTON_LEFT

#elif CONFIG_KEYPAD == ONDAVX747_PAD
#define PNG_MENU       BUTTON_POWER
#elif CONFIG_KEYPAD == ONDAVX777_PAD
#define PNG_MENU       BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define PNG_ZOOM_IN    (BUTTON_PLAY|BUTTON_UP)
#define PNG_ZOOM_OUT   (BUTTON_PLAY|BUTTON_DOWN)
#define PNG_UP         BUTTON_UP
#define PNG_DOWN       BUTTON_DOWN
#define PNG_LEFT       BUTTON_LEFT
#define PNG_RIGHT      BUTTON_RIGHT
#define PNG_MENU       BUTTON_PLAY
#define PNG_NEXT       BUTTON_FFWD
#define PNG_PREVIOUS   BUTTON_REW

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef PNG_UP
#define PNG_UP         BUTTON_TOPMIDDLE
#endif
#ifndef PNG_DOWN
#define PNG_DOWN       BUTTON_BOTTOMMIDDLE
#endif
#ifndef PNG_LEFT
#define PNG_LEFT       BUTTON_MIDLEFT
#endif
#ifndef PNG_RIGHT
#define PNG_RIGHT      BUTTON_MIDRIGHT
#endif
#ifndef PNG_ZOOM_IN
#define PNG_ZOOM_IN    BUTTON_TOPRIGHT
#endif
#ifndef PNG_ZOOM_OUT
#define PNG_ZOOM_OUT   BUTTON_TOPLEFT
#endif
#ifndef PNG_MENU
#define PNG_MENU       (BUTTON_CENTER|BUTTON_REL)
#endif
#ifndef PNG_NEXT
#define PNG_NEXT       BUTTON_BOTTOMRIGHT
#endif
#ifndef PNG_PREVIOUS
#define PNG_PREVIOUS   BUTTON_BOTTOMLEFT
#endif
#endif

#define PLUGIN_OTHER    10 /* State code for output with return. */
#define PLUGIN_ABORT    11
#define PLUGIN_OUTOFMEM 12
#define OUT_OF_MEMORY   9900
#define FILE_TOO_LARGE  9910
