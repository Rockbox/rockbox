/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Oscilloscope, with many different modes of operation.
*
* Copyright (C) 2004-2006 Jens Arnold
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
#include "lib/helper.h"
#include "lib/pluginlib_exit.h"

#include "lib/xlcd.h"
#include "lib/configfile.h"
#include "fixedpoint.h"
#include "lib/fixedpoint.h"

#if 0
#undef DEBUGF
#define DEBUGF(...)
#endif

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_OFF
#define OSCILLOSCOPE_DRAWMODE        BUTTON_F1
#define OSCILLOSCOPE_ADVMODE         BUTTON_F2
#define OSCILLOSCOPE_ORIENTATION     BUTTON_F3
#define OSCILLOSCOPE_PAUSE           BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_OFF
#define OSCILLOSCOPE_DRAWMODE        BUTTON_F1
#define OSCILLOSCOPE_ADVMODE         BUTTON_F2
#define OSCILLOSCOPE_ORIENTATION     BUTTON_F3
#define OSCILLOSCOPE_PAUSE           BUTTON_SELECT
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN

#elif CONFIG_KEYPAD == ONDIO_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_OFF
#define OSCILLOSCOPE_DRAWMODE_PRE    BUTTON_MENU
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_MENU | BUTTON_REL)
#define OSCILLOSCOPE_ADVMODE         (BUTTON_MENU | BUTTON_RIGHT)
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_MENU | BUTTON_LEFT)
#define OSCILLOSCOPE_PAUSE           (BUTTON_MENU | BUTTON_OFF)
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN
#define NEED_LASTBUTTON

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_OFF
#define OSCILLOSCOPE_DRAWMODE        BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE         BUTTON_MODE
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_REC
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_REC | BUTTON_REL)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_REC
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_REC | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE           BUTTON_ON
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN
#define OSCILLOSCOPE_RC_QUIT         BUTTON_RC_STOP
#define NEED_LASTBUTTON

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define OSCILLOSCOPE_QUIT            (BUTTON_SELECT | BUTTON_MENU)
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_SELECT | BUTTON_PLAY)
#define OSCILLOSCOPE_ADVMODE         (BUTTON_SELECT | BUTTON_RIGHT)
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_SELECT | BUTTON_LEFT)
#define OSCILLOSCOPE_GRAPHMODE       BUTTON_MENU
#define OSCILLOSCOPE_PAUSE           BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_SCROLL_FWD
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_SCROLL_BACK
/* Need GRAPHMODE */

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE        BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE         BUTTON_DOWN
#define OSCILLOSCOPE_ORIENTATION     BUTTON_UP
#define OSCILLOSCOPE_GRAPHMODE       BUTTON_MENU
#define OSCILLOSCOPE_PAUSE           BUTTON_A
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE_PRE    BUTTON_SELECT
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_SELECT | BUTTON_REL)
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_SELECT
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_SELECT | BUTTON_REPEAT)
#define OSCILLOSCOPE_ADVMODE         BUTTON_DOWN
#define OSCILLOSCOPE_PAUSE_PRE       BUTTON_UP
#define OSCILLOSCOPE_PAUSE           (BUTTON_UP | BUTTON_REL)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_UP
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_UP | BUTTON_REPEAT)
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_SCROLL_FWD
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_SCROLL_BACK
#define NEED_LASTBUTTON

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define OSCILLOSCOPE_QUIT            (BUTTON_HOME|BUTTON_REPEAT)
#define OSCILLOSCOPE_DRAWMODE_PRE    BUTTON_SELECT
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_SELECT | BUTTON_REL)
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_SELECT
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_SELECT | BUTTON_REPEAT)
#define OSCILLOSCOPE_ADVMODE         BUTTON_DOWN
#define OSCILLOSCOPE_PAUSE_PRE       BUTTON_UP
#define OSCILLOSCOPE_PAUSE           (BUTTON_UP | BUTTON_REL)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_UP
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_UP | BUTTON_REPEAT)
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_SCROLL_FWD
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_SCROLL_BACK
#define NEED_LASTBUTTON

#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE        BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE         BUTTON_DOWN
#define OSCILLOSCOPE_ORIENTATION     BUTTON_UP
#define OSCILLOSCOPE_PAUSE           BUTTON_REC
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
/* v1 - Need GRAPHMODE */
/* v2 - Not enough plugin RAM for waveform view */

#elif (CONFIG_KEYPAD == SANSA_CLIP_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE_PRE    BUTTON_SELECT
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_SELECT | BUTTON_REL)
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_SELECT
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_SELECT | BUTTON_REPEAT)
#define OSCILLOSCOPE_ADVMODE         BUTTON_DOWN
#define OSCILLOSCOPE_PAUSE           BUTTON_UP
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
#define NEED_LASTBUTTON
/* Not enough plugin RAM for waveform view */

#elif (CONFIG_KEYPAD == SANSA_M200_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_SELECT | BUTTON_REL)
#define OSCILLOSCOPE_ADVMODE         BUTTON_DOWN
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_UP
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_UP | BUTTON_REL)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_UP
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_UP | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE           (BUTTON_SELECT | BUTTON_UP)
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
#define NEED_LASTBUTTON

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE_PRE    BUTTON_SELECT
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_SELECT | BUTTON_REL)
#define OSCILLOSCOPE_ADVMODE         BUTTON_REC
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_SELECT
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_SELECT | BUTTON_REPEAT)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_PLAY
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_PLAY | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE_PRE       BUTTON_PLAY
#define OSCILLOSCOPE_PAUSE           (BUTTON_PLAY | BUTTON_REL)
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN
#define NEED_LASTBUTTON

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE_PRE    BUTTON_REW
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_REW | BUTTON_REL)
#define OSCILLOSCOPE_ADVMODE         BUTTON_FF
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_REW
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_REW | BUTTON_REPEAT)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_PLAY
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_PLAY | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE_PRE       BUTTON_PLAY
#define OSCILLOSCOPE_PAUSE           (BUTTON_PLAY | BUTTON_REL)
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_SCROLL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_SCROLL_DOWN
#define NEED_LASTBUTTON

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_BACK
#define OSCILLOSCOPE_DRAWMODE        BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE         BUTTON_DOWN
#define OSCILLOSCOPE_ORIENTATION     BUTTON_UP
#define OSCILLOSCOPE_GRAPHMODE       BUTTON_MENU
#define OSCILLOSCOPE_PAUSE           BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE        BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE         BUTTON_MENU
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_PLAY
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_PLAY | BUTTON_REL)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_PLAY
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_PLAY | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE           BUTTON_DISPLAY
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN
#define NEED_LASTBUTTON

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_RC_REC
#define OSCILLOSCOPE_DRAWMODE_PRE    BUTTON_RC_MODE
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_RC_MODE|BUTTON_REL)
#define OSCILLOSCOPE_ADVMODE         BUTTON_RC_MENU
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_RC_MODE
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_RC_MODE|BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE_PRE       BUTTON_RC_PLAY
#define OSCILLOSCOPE_PAUSE           (BUTTON_RC_PLAY|BUTTON_REL)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_RC_PLAY
#define SCILLLOSCOPE_GRAPHMODE       (BUTTON_RC_PLAY|BUTTON_REPEAT)
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RC_FF
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_RC_REW
#define OSCILLOSCOPE_VOL_UP          BUTTON_RC_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_RC_VOL_DOWN
#define NEED_LASTBUTTON

#elif CONFIG_KEYPAD == COWON_D2_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_VOL_UP          BUTTON_PLUS
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_MINUS

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_BACK
#define OSCILLOSCOPE_DRAWMODE        BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE         BUTTON_CUSTOM
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_MENU
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_MENU | BUTTON_REL)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_MENU
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_MENU | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE           BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN
#define NEED_LASTBUTTON

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE        BUTTON_MENU
#define OSCILLOSCOPE_ADVMODE         BUTTON_VIEW
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_UP
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_UP | BUTTON_REL)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_UP
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_UP | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE           BUTTON_SELECT
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
#define NEED_LASTBUTTON

#elif CONFIG_KEYPAD == PHILIPS_HDD6330_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE        BUTTON_MENU
#define OSCILLOSCOPE_ADVMODE         BUTTON_RIGHT
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_UP
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_UP | BUTTON_REL)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_UP
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_UP | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE           BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP        BUTTON_NEXT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_PREV
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
#define NEED_LASTBUTTON

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE        BUTTON_MENU
#define OSCILLOSCOPE_ADVMODE         BUTTON_RIGHT
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_UP
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_UP | BUTTON_REL)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_UP
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_UP | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE           BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP        BUTTON_NEXT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_PREV
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
#define NEED_LASTBUTTON

#elif CONFIG_KEYPAD == ONDAVX747_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN

#elif CONFIG_KEYPAD == ONDAVX777_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER

#elif CONFIG_KEYPAD == MROBE500_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_REC
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_PLAY|BUTTON_LEFT)
#define OSCILLOSCOPE_ADVMODE         (BUTTON_PLAY|BUTTON_RIGHT)
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_PLAY|BUTTON_UP)
#define OSCILLOSCOPE_PAUSE           (BUTTON_PLAY|BUTTON_DOWN)
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN
/* Need GRAPHMODE */

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_REC
#define OSCILLOSCOPE_DRAWMODE        BUTTON_MENU
#define OSCILLOSCOPE_ADVMODE         BUTTON_CANCEL
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_OK
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_OK | BUTTON_REL)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_OK
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_OK | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE           BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP        BUTTON_PREV
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_NEXT
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN
#define NEED_LASTBUTTON

#elif CONFIG_KEYPAD == MPIO_HD200_PAD
#define OSCILLOSCOPE_QUIT            (BUTTON_REC | BUTTON_PLAY)
#define OSCILLOSCOPE_DRAWMODE        BUTTON_FUNC
#define OSCILLOSCOPE_ADVMODE         BUTTON_REC
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_FUNC | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE           BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP        BUTTON_FF
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_REW
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
/* Need GRAPHMODE */

#elif CONFIG_KEYPAD == MPIO_HD300_PAD
#define OSCILLOSCOPE_QUIT            (BUTTON_MENU | BUTTON_REPEAT)
#define OSCILLOSCOPE_DRAWMODE        BUTTON_ENTER
#define OSCILLOSCOPE_ADVMODE         BUTTON_REC
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_MENU
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_MENU | BUTTON_REL)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_MENU
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_MENU | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE           BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP        BUTTON_FF
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_REW
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN
#define NEED_LASTBUTTON

#elif CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE        BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE         BUTTON_BACK
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_BACK|BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE           BUTTON_PLAYPAUSE
#define OSCILLOSCOPE_SPEED_UP        BUTTON_LEFT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_RIGHT
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
/* Need GRAPHMODE */

#elif (CONFIG_KEYPAD == SANSA_CONNECT_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE        BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE         BUTTON_DOWN
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_UP
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_UP | BUTTON_REL)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_UP
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_UP | BUTTON_REPEAT)
#define OSCILLOSCOPE_PAUSE           BUTTON_NEXT
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
#define NEED_LASTBUTTON

#elif (CONFIG_KEYPAD == SAMSUNG_YPR0_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_BACK
#define OSCILLOSCOPE_DRAWMODE        BUTTON_USER
#define OSCILLOSCOPE_ADVMODE         BUTTON_MENU
#define OSCILLOSCOPE_ORIENTATION     BUTTON_POWER
#define OSCILLOSCOPE_PAUSE           BUTTON_SELECT
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN
/* Need GRAPHMODE */

#elif (CONFIG_KEYPAD == HM60X_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_POWER | BUTTON_SELECT)
#define OSCILLOSCOPE_ADVMODE         (BUTTON_POWER | BUTTON_RIGHT)
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_POWER | BUTTON_LEFT)
#define OSCILLOSCOPE_PAUSE           BUTTON_SELECT
#define OSCILLOSCOPE_SPEED_UP        BUTTON_UP
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_DOWN
#define OSCILLOSCOPE_VOL_UP          BUTTON_RIGHT
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_LEFT
/* Need GRAPHMODE */

#elif (CONFIG_KEYPAD == HM801_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE        BUTTON_PREV
#define OSCILLOSCOPE_ADVMODE         BUTTON_NEXT
#define OSCILLOSCOPE_ORIENTATION     BUTTON_PLAY
#define OSCILLOSCOPE_PAUSE           BUTTON_SELECT
#define OSCILLOSCOPE_SPEED_UP        BUTTON_UP
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_DOWN
#define OSCILLOSCOPE_VOL_UP          BUTTON_RIGHT
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_LEFT
/* Need GRAPHMODE */

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef OSCILLOSCOPE_QUIT
#define OSCILLOSCOPE_QUIT            BUTTON_TOPLEFT
#endif
#ifndef OSCILLOSCOPE_DRAWMODE
#define OSCILLOSCOPE_DRAWMODE        BUTTON_TOPMIDDLE
#endif
#ifndef OSCILLOSCOPE_ADVMODE
#define OSCILLOSCOPE_ADVMODE         BUTTON_BOTTOMMIDDLE
#endif
#ifndef OSCILLOSCOPE_ORIENTATION
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_BOTTOMLEFT
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_BOTTOMLEFT|BUTTON_REL)
#endif
#ifndef OSCILLOSCOPE_GRAPHMODE
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_BOTTOMLEFT
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_BOTTOMLEFT|BUTTON_REPEAT)
#endif
#ifndef OSCILLOSCOPE_PAUSE
#define OSCILLOSCOPE_PAUSE           BUTTON_CENTER
#endif
#ifndef OSCILLOSCOPE_SPEED_UP
#define OSCILLOSCOPE_SPEED_UP        BUTTON_MIDRIGHT
#endif
#ifndef OSCILLOSCOPE_SPEED_DOWN
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_MIDLEFT
#endif
#ifndef OSCILLOSCOPE_VOL_UP
#define OSCILLOSCOPE_VOL_UP          BUTTON_TOPRIGHT
#endif
#ifndef OSCILLOSCOPE_VOL_DOWN
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_BOTTOMRIGHT
#endif
#define NEED_LASTBUTTON
#endif /* HAVE_TOUCHSCREEN */

/* colours */
#if LCD_DEPTH > 1
#ifdef HAVE_LCD_COLOR
#define BACKG_COLOR         LCD_BLACK
#define GRAPH_COLOR         LCD_RGBPACK(128, 255, 0)
#define CURSOR_COLOR        LCD_RGBPACK(255, 0, 0)
#define GRID_COLOR          LCD_RGBPACK(192, 255, 64)
#define OSD_TEXT_COLOR      LCD_WHITE
#define OSD_BACKG_COLOR     LCD_BLACK
#define OSD_BORDER_COLOR    LCD_RGBPACK(255, 0, 0)
#define OSD_MARGIN_SIZE     0 /* Border and text are different */
#else
#define BACKG_COLOR         LCD_WHITE
#define GRAPH_COLOR         LCD_BLACK
#define CURSOR_COLOR        LCD_DARKGRAY
#define GRID_COLOR          LCD_DARKGRAY
#define OSD_TEXT_COLOR      LCD_BLACK
#define OSD_BACKG_COLOR     LCD_WHITE
#define OSD_BORDER_COLOR    LCD_BLACK
#endif
#endif

#ifndef OSD_MARGIN_SIZE
#define OSD_MARGIN_SIZE     1
#endif

enum { DRAW_FILLED, DRAW_LINE, DRAW_PIXEL, MAX_DRAW };
enum { ADV_SCROLL, ADV_WRAP, MAX_ADV };
enum { OSC_HORIZ, OSC_VERT, MAX_OSC };
#ifdef OSCILLOSCOPE_GRAPHMODE
enum { GRAPH_PEAKS, GRAPH_WAVEFORM, MAX_GRAPH };
#endif

#define CFGFILE_VERSION    1  /* Current config file version */
#define CFGFILE_MINVERSION 0  /* Minimum config file version to accept */

/* global variables */

/* settings */
struct osc_config
{
    int speed;     /* 1-100 */
    int draw;
    int advance;
    int orientation;
#ifdef OSCILLOSCOPE_GRAPHMODE
    int wavespeed; /* 1-100 */
    int graphmode;
#endif
};

static struct osc_config osc_disk =
{
    .speed = 68,
    .draw = DRAW_FILLED,
    .advance = ADV_SCROLL,
    .orientation = OSC_HORIZ,
#ifdef OSCILLOSCOPE_GRAPHMODE
    .wavespeed = 50,
    .graphmode = GRAPH_PEAKS,
#endif
};

static struct osc_config osc;       /* running config */

static const char cfg_filename[] =  "oscilloscope.cfg";
static char *draw_str[3]        = { "filled", "line", "pixel" };
static char *advance_str[2]     = { "scroll", "wrap" };
static char *orientation_str[2] = { "horizontal", "vertical" };
#ifdef OSCILLOSCOPE_GRAPHMODE
static char *graphmode_str[2]   = { "peaks", "waveform" };
#endif

struct configdata disk_config[] =
{
   { TYPE_INT,  1, 100, { .int_p = &osc_disk.speed }, "speed", NULL },
   { TYPE_ENUM, 0, MAX_DRAW, { .int_p = &osc_disk.draw }, "draw", draw_str },
   { TYPE_ENUM, 0, MAX_ADV, { .int_p = &osc_disk.advance }, "advance",
     advance_str },
   { TYPE_ENUM, 0, MAX_OSC, { .int_p = &osc_disk.orientation }, "orientation",
     orientation_str },
#ifdef OSCILLOSCOPE_GRAPHMODE
   { TYPE_INT,  1, 100, { .int_p = &osc_disk.wavespeed }, "wavespeed", NULL },
   { TYPE_ENUM, 0, MAX_GRAPH, { .int_p = &osc_disk.graphmode }, "graphmode",
     graphmode_str },
#endif
};

enum osc_message_ids
{
    OSC_MSG_ADVMODE,
    OSC_MSG_DRAWMODE,
#ifdef OSCILLOSCOPE_GRAPHMODE
    OSC_MSG_GRAPHMODE,
#endif
    OSC_MSG_ORIENTATION,
    OSC_MSG_PAUSED,
    OSC_MSG_SPEED,
    OSC_MSG_VOLUME,
    OSC_MSG_COUNT,
};

static int message = -1, msgval = 0; /* popup message */
static long last_tick = 0;  /* time of last drawing */
static long last_delay = 0; /* last delay value used */
static int  last_pos = 0;   /* last x or y drawing position. Reset for aspect switch. */
static int  last_left;      /* last channel values */
static int  last_right;
static bool paused = false;
static bool one_frame_paused = false; /* Allow one frame to be drawn when paused if
                                         view is erased */
static long osc_delay;       /* delay in 100ths of a tick */
static long osc_delay_error; /* delay fraction error accumulator */

static inline void osc_popupmsg(int msg, int val)
{
    message = msg; msgval = val;
}

/* implementation */


/** On-screen Display **/

static enum osd_status
{
    OSD_HIDDEN,   /* OSD is hidden from view */
    OSD_VISIBLE,  /* OSD is visible on screen */
    OSD_ERASED,   /* OSD is erased in preparation for regular drawing */
    OSD_DISABLED, /* OSD is disabled entirely */
} osd_status = OSD_DISABLED;          /* OSD view status */
static struct viewport osd_vp;        /* OSD clipping viewport */
static struct bitmap osd_lcd_bitmap;  /* The main LCD fb bitmap */
static struct bitmap osd_back_bitmap; /* The OSD backbuffer fb bitmap */
static unsigned char osd_message[16]; /* message to display (formatted) */
static int osd_maxwidth = 0;          /* How wide may it be at most? */
static int osd_maxheight = 0;         /* How high may it be at most? */
static int osd_timeout = -1;          /* Current popup stay duration */
static long osd_hide_tick;            /* Tick when it should be hidden */

/* NOTE: Will make the OSD into a libplugin object with some tweaking. This
 * isn't the only plugin that could use this to improve its message displays.
 */

/* Framebuffer allocation macros */
#if LCD_DEPTH == 1
#  if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#    define LCD_WIDTH2BYTES(w)  (((w)+7)/8)
#    define LCD_BYTES2WIDTH(b)  ((b)*8)
#  elif LCD_PIXELFORMAT == VERTICAL_PACKING
#    define LCD_HEIGHT2BYTES(h) (((h)+7)/8)
#    define LCD_BYTES2HEIGHT(b) ((b)*8)
#  else
#    error Unknown 1-bit format; please define macros
#  endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 2
#  if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#    define LCD_WIDTH2BYTES(w)  (((w)+3)/4)
#    define LCD_BYTES2WIDTH(b)  ((b)*4)
#  elif LCD_PIXELFORMAT == VERTICAL_PACKING
#    define LCD_HEIGHT2BYTES(h) (((h)+3)/4)
#    define LCD_BYTES2HEIGHT(b) ((b)*4)
#  elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
#    define LCD_WIDTH2BYTES(w)  ((w)*2)
#    define LCD_BYTES2WIDTH(b)  ((b)/2)
#    define LCD_HEIGHT2BYTES(h) (((h)+7)/8*2)
#    define LCD_BYTES2HEIGHT(b) ((b)/2*8)
#  else
#    error Unknown 2-bit format; please define macros
#  endif /* LCD_PIXELFORMAT */
#elif LCD_DEPTH == 16
#  define LCD_WIDTH2BYTES(w)    ((w)*2)
#  define LCD_BYTES2WIDTH(b)    ((b)/2)
#else
#  error Unknown LCD depth; please define macros
#endif /* LCD_DEPTH */
/* Set defaults if not defined different yet. */
#ifndef LCD_WIDTH2BYTES
#  define LCD_WIDTH2BYTES(w)    (w)
#endif
#ifndef LCD_BYTES2WIDTH
#  define LCD_BYTES2WIDTH(b)    (b)
#endif
#ifndef LCD_HEIGHT2BYTES
#  define LCD_HEIGHT2BYTES(h)   (h)
#endif
#ifndef LCD_BYTES2HEIGHT
#  define LCD_BYTES2HEIGHT(b)   (b)
#endif

/* Create a bitmap framebuffer from a buffer */
static fb_data * buf_to_fb_bitmap(void *buf, size_t bufsize,
                                  int *width, int *height)
{
    /* Used as dest, the LCD functions cannot deal with alternate
       strides as of now - the stride guides the calulations */
    DEBUGF("buf: %p bufsize: %lu\n", buf, (unsigned long)bufsize);
#if defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
    int h = LCD_BYTES2HEIGHT(LCD_HEIGHT2BYTES(LCD_HEIGHT));
    int w = bufsize / LCD_HEIGHT2BYTES(h);

    if (w == 0)
        return NULL; /* not enough buffer */

    if (w > LCD_BYTES2WIDTH(LCD_WIDTH2BYTES(LCD_WIDTH)))
        w = LCD_BYTES2WIDTH(LCD_WIDTH2BYTES(LCD_WIDTH));
#else
    int w = LCD_BYTES2WIDTH(LCD_WIDTH2BYTES(LCD_WIDTH));
    int h = bufsize / LCD_WIDTH2BYTES(w);

    if (h == 0)
        return NULL; /* not enough buffer */

    if (h > LCD_BYTES2HEIGHT(LCD_HEIGHT2BYTES(LCD_HEIGHT)))
        h = LCD_BYTES2HEIGHT(LCD_HEIGHT2BYTES(LCD_HEIGHT));
#endif
    DEBUGF("fbw:%d fbh:%d\n", w, h);

    *width = w;
    *height = h;
   
    return (fb_data *)buf;
}

/* Center the viewport on screen */
static void osd_update_vp(int width, int height)
{
    osd_vp.x = (LCD_WIDTH - width) / 2;
    osd_vp.y = (LCD_HEIGHT - height) / 2;
    osd_vp.width = width;
    osd_vp.height = height;
}

/* Perform all necessary basic setup tasks for the OSD */
static void osd_init(void)
{
    fb_data *buf;
    size_t bufsize;

    /* Grab the plugin buffer for the OSD back buffer */
    buf = rb->plugin_get_buffer(&bufsize);
    if (buf == NULL)
        return;

    ALIGN_BUFFER(buf, bufsize, FB_DATA_SZ);

    if (bufsize == 0)
        return;

    /* Find the largest display that will fit using the desired height as
       the reference */
    bool retry = true;
    fb_data *backfb;

    rb->viewport_set_fullscreen(&osd_vp, SCREEN_MAIN);

    while (1)
    {
        backfb = buf_to_fb_bitmap(buf, bufsize, &osd_maxwidth,
                                  &osd_maxheight);

        if (backfb)
            break;

        if (!retry)
            return;

        /* Try system font - maybe fits */
        rb->lcd_setfont(FONT_SYSFIXED);
        retry = false;
    }

    /* LCD framebuffer bitmap */
    osd_lcd_bitmap.width = LCD_BYTES2WIDTH(LCD_WIDTH2BYTES(LCD_WIDTH));
    osd_lcd_bitmap.height = LCD_BYTES2HEIGHT(LCD_HEIGHT2BYTES(LCD_HEIGHT));
#if LCD_DEPTH > 1
    osd_lcd_bitmap.format = FORMAT_NATIVE;
    osd_lcd_bitmap.maskdata = NULL;
#endif
#ifdef HAVE_LCD_COLOR
    osd_lcd_bitmap.alpha_offset = 0;
#endif
    osd_lcd_bitmap.data = (void *)rb->lcd_framebuffer;

    /* Backbuffer bitmap */
    osd_back_bitmap.width = osd_maxwidth;
    osd_back_bitmap.height = osd_maxheight;
#if LCD_DEPTH > 1
    osd_back_bitmap.format = FORMAT_NATIVE;
    osd_back_bitmap.maskdata = NULL;
#endif
#ifdef HAVE_LCD_COLOR
    osd_back_bitmap.alpha_offset = 0;
#endif
    osd_back_bitmap.data = (void *)backfb;

    DEBUGF("FB:%p BB:%p\n", osd_lcd_bitmap.data, osd_back_bitmap.data);

    osd_update_vp(osd_maxwidth, osd_maxheight);
    osd_status = OSD_HIDDEN;
}

/* Actually draw the OSD within the OSD viewport */
static void osd_lcd_draw(void)
{
    rb->lcd_set_viewport(&osd_vp);
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(OSD_TEXT_COLOR);
    rb->lcd_set_background(OSD_BACKG_COLOR);
#endif
#if OSD_MARGIN_SIZE != 0
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(1, 1, osd_vp.width - 2, osd_vp.height - 2);
    rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
    rb->lcd_putsxy(1+OSD_MARGIN_SIZE, 1+OSD_MARGIN_SIZE, osd_message);
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(OSD_BORDER_COLOR);
#endif
    rb->lcd_drawrect(0, 0, osd_vp.width, osd_vp.height);
    rb->lcd_set_viewport(NULL);
}

/* Same for now but other methods of restoration are possible besides
   redrawing it from scratch */
#define osd_lcd_restore osd_lcd_draw

/* Sync the backbuffer to the on-screen image */
static void osd_lcd_update_back_buffer(void)
{
    rb->lcd_set_framebuffer((fb_data *)osd_back_bitmap.data);
    rb->lcd_bmp_part(&osd_lcd_bitmap, osd_vp.x, osd_vp.y,
                     0, 0, osd_vp.width, osd_vp.height);
    rb->lcd_set_framebuffer(NULL);
}

/* Erase the OSD to restore the framebuffer */
static void osd_lcd_erase(void)
{
    rb->lcd_bmp_part(&osd_back_bitmap, 0, 0, osd_vp.x, osd_vp.y,
                     osd_vp.width, osd_vp.height);
}

/* Hide the OSD from view */
static void osd_hide(void)
{
    if (osd_status == OSD_HIDDEN)
        return;

    if (osd_status == OSD_VISIBLE)
    {
        osd_lcd_erase();
        rb->lcd_update_rect(osd_vp.x, osd_vp.y, osd_vp.width, osd_vp.height);
    }

    osd_status = OSD_HIDDEN;
}

/* Format a message to display */
static void osd_format_message(enum osc_message_ids id, int val)
{
    switch (id)
    {
    case OSC_MSG_ADVMODE:
        rb->strlcpy(osd_message, val == ADV_WRAP ? "Wrap" : "Scroll",
                    sizeof (osd_message));
        break;

    case OSC_MSG_DRAWMODE:
        rb->strlcpy(osd_message,
                    val == DRAW_PIXEL ?
                        "Pixel" : (val == DRAW_LINE ? "Line" : "Filled"),
                    sizeof (osd_message));
        break;

#ifdef OSCILLOSCOPE_GRAPHMODE
    case OSC_MSG_GRAPHMODE:
        rb->strlcpy(osd_message, val == GRAPH_PEAKS ? "Peaks" : "Waveform",
                    sizeof (osd_message));
        break;
#endif

    case OSC_MSG_ORIENTATION:
        rb->strlcpy(osd_message, val == OSC_VERT ? "Vertical" : "Horizontal",
                     sizeof (osd_message));
        break;

    case OSC_MSG_PAUSED:
        rb->strlcpy(osd_message, val ? "Paused" : "Running", sizeof (osd_message));
        break;

    case OSC_MSG_SPEED:
        rb->snprintf(osd_message, sizeof (osd_message), "Speed: %d", val);
        break;

    case OSC_MSG_VOLUME:
        rb->snprintf(osd_message, sizeof (osd_message), "Volume: %d%s",
                     rb->sound_val2phys(SOUND_VOLUME, val),
                     rb->sound_unit(SOUND_VOLUME));
        break;

    default:
        *osd_message = '\0';
        break;
    }
}

/* Format a message by ID and show the OSD */
static void osd_show_message(int id, int val)
{
    osd_format_message(id, val);

    if (osd_status == OSD_DISABLED)
        return;

    int width, height;
    rb->lcd_set_viewport(&osd_vp);
    rb->lcd_getstringsize(osd_message, &width, &height);
    rb->lcd_set_viewport(NULL);

    width += 2 + 2*OSD_MARGIN_SIZE;
    if (width > osd_maxwidth)
        width = osd_maxwidth;

    height += 2 + 2*OSD_MARGIN_SIZE;
    if (height > osd_maxheight)
        height = osd_maxheight;

    /* Save old rectangle for union operation below */
    int l = osd_vp.x,
        t = osd_vp.y,
        r = l + osd_vp.width,
        b = t + osd_vp.height;

    bool vp_changed = width != osd_vp.width || height != osd_vp.height;

    if (vp_changed)
    {
        if (osd_status == OSD_VISIBLE)
            osd_lcd_erase(); /* Visible area has changed */

        osd_update_vp(width, height);
    }

    if (osd_status == OSD_HIDDEN || vp_changed)
        osd_lcd_update_back_buffer();

    if (osd_status != OSD_ERASED)
    {
        osd_status = OSD_VISIBLE;
        osd_lcd_draw();

        /* Update the smallest rectangle that encloses both the old and new
           regions to make the change free of flicker (they may overlap) */
        l = MIN(l, osd_vp.x);
        t = MIN(t, osd_vp.x);
        r = MAX(r, osd_vp.x + osd_vp.width);
        b = MAX(b, osd_vp.y + osd_vp.height);
        rb->lcd_update_rect(l, t, r - l, b - t);
    }

    osd_hide_tick = *rb->current_tick + osd_timeout;
}

/* Call periodically to have the OSD timeout and hide itself */
static void osd_monitor_timeout(void)
{
    if (osd_status == OSD_HIDDEN)
        return;

    if (osd_timeout >= 0 && TIME_AFTER(*rb->current_tick, osd_hide_tick))
        osd_hide();
}

/* Set the OSD timeout value. < 0 = never timeout */
static void osd_set_timeout(int timeout)
{
    osd_timeout = timeout;
    osd_monitor_timeout();
}

/* Prepare LCD frambuffer for regular drawing */
static void osd_prepare_draw(void)
{
    if (osd_status == OSD_VISIBLE)
    {
        osd_status = OSD_ERASED;
        osd_lcd_erase();
    }
}

/* Update the whole screen */
static void osd_lcd_update(void)
{
    if (osd_status == OSD_ERASED)
    {
        /* Save the screen image underneath and restore the OSD */
        osd_status = OSD_VISIBLE;
        osd_lcd_update_back_buffer();
        osd_lcd_restore();
    }

    rb->lcd_update();
}

/* Update a part of the screen */
static void osd_lcd_update_rect(int x, int y, int width, int height)
{
    if (osd_status == OSD_ERASED)
    {
        /* Save the screen image underneath and restore the OSD */
        osd_status = OSD_VISIBLE;
        osd_lcd_update_back_buffer();
        osd_lcd_restore();
    }

    rb->lcd_update_rect(x, y, width, height);
}


/** Peaks View **/

static void get_peaks(int *left, int *right)
{
#if CONFIG_CODEC == SWCODEC
    static struct pcm_peaks peaks;
    rb->mixer_channel_calculate_peaks(PCM_MIXER_CHAN_PLAYBACK,
                                      &peaks);
    *left = peaks.left;
    *right = peaks.right;
#elif defined (SIMULATOR)
    *left = rand() % 0x8000;
    *right = rand() % 0x8000;
#elif (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    *left = rb->mas_codec_readreg(0xC);
    *right = rb->mas_codec_readreg(0xD);
#else
    *left = 0;
    *right = 0;
#endif
}

static long get_next_delay(void)
{
    long delay = osc_delay / 100;
    osc_delay_error += osc_delay - delay * 100;

    if (osc_delay_error >= 100)
    {
        delay++;
        osc_delay_error -= 100;
    }

    return delay;
}

static void anim_peaks_draw_cursor_h(int x)
{
#if LCD_DEPTH > 1                 /* cursor bar */    
    rb->lcd_set_foreground(CURSOR_COLOR);
    rb->lcd_vline(x, 0, LCD_HEIGHT-1); 
    rb->lcd_set_foreground(GRAPH_COLOR);
#else
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_vline(x, 0, LCD_HEIGHT-1);
    rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
}

static long anim_peaks_horizontal(void)
{
    long cur_tick = *rb->current_tick;
    int cur_x, x;
    int left, right, dl, dr;
    long d = (cur_tick - last_tick) / last_delay;
    bool full_update = false;

    if (paused)
    {
        one_frame_paused = false;
        osd_prepare_draw();
        anim_peaks_draw_cursor_h(0);
        osd_lcd_update_rect(0, 0, 1, LCD_HEIGHT);
        last_pos = -1;
        return cur_tick + HZ/5;
    }

    if (d <= 0)  /* too early, bail out */
        return last_tick + last_delay;

    last_tick = cur_tick;

    int cur_left, cur_right;
    get_peaks(&cur_left, &cur_right);

    if (d > HZ) /* first call or too much delay, (re)start */
    {
        last_left = cur_left;
        last_right = cur_right;
        return cur_tick + last_delay;
    }

    one_frame_paused = false;

    osd_prepare_draw();

    cur_x = last_pos + d;

    if (cur_x >= LCD_WIDTH)
    {
        if (osc.advance == ADV_SCROLL)
        {
            int shift = cur_x - (LCD_WIDTH-1);
            xlcd_scroll_left(shift);
            full_update = true;
            cur_x -= shift;
            last_pos -= shift;
        }
        else
        {
            cur_x -= LCD_WIDTH;
        }
    }
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);

    if (cur_x > last_pos)
    {
        rb->lcd_fillrect(last_pos + 1, 0, d, LCD_HEIGHT);
    }
    else
    {
        rb->lcd_fillrect(last_pos + 1, 0, LCD_WIDTH - last_pos, LCD_HEIGHT);
        rb->lcd_fillrect(0, 0, cur_x + 1, LCD_HEIGHT);
    }
    rb->lcd_set_drawmode(DRMODE_SOLID);

    switch (osc.draw)
    {
        case DRAW_FILLED:
            left = last_left;
            right = last_right;
            dl = (cur_left  - left)  / d;
            dr = (cur_right - right) / d;

            for (x = last_pos + 1; d > 0; x++, d--)
            {
                if (x == LCD_WIDTH)
                    x = 0;

                left  += dl;
                right += dr;

                rb->lcd_vline(x, LCD_HEIGHT/2-1,
                              LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * left) >> 16));
                rb->lcd_vline(x, LCD_HEIGHT/2+1,
                              LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * right) >> 16));
            }
            break;

        case DRAW_LINE:
            if (cur_x > last_pos)
            {
                rb->lcd_drawline(
                    last_pos, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * last_left) >> 16),
                    cur_x, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * cur_left) >> 16)
                );
                rb->lcd_drawline(
                    last_pos, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * last_right) >> 16),
                    cur_x, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * cur_right) >> 16)
                );
            }
            else
            {
                left  = last_left 
                      + (LCD_WIDTH - last_pos) * (last_left - cur_left) / d;
                right = last_right 
                      + (LCD_WIDTH - last_pos) * (last_right - cur_right) / d;

                rb->lcd_drawline(
                    last_pos, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * last_left) >> 16),
                    LCD_WIDTH, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * left) >> 16)
                );
                rb->lcd_drawline(
                    last_pos, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * last_right) >> 16),
                    LCD_WIDTH, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * right) >> 16)
                );
                if (cur_x > 0)
                {
                    rb->lcd_drawline(
                        0, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * left) >> 16),
                        cur_x, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * cur_left) >> 16)
                    );
                    rb->lcd_drawline(
                        0, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * right) >> 16),
                        cur_x, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * cur_right) >> 16)
                    );
                }
            }
            break;
            
        case DRAW_PIXEL:
            left = last_left;
            right = last_right;
            dl = (cur_left - left) / d;
            dr = (cur_right - right) / d;

            for (x = last_pos + 1; d > 0; x++, d--)
            {
                if (x == LCD_WIDTH)
                    x = 0;

                left  += dl;
                right += dr;

                rb->lcd_drawpixel(x, LCD_HEIGHT/2-1 - (((LCD_HEIGHT-2) * left) >> 16));
                rb->lcd_drawpixel(x, LCD_HEIGHT/2+1 + (((LCD_HEIGHT-2) * right) >> 16));
            }
            break;

    }

    last_left  = cur_left;
    last_right = cur_right;
    
    if (full_update)
    {
        osd_lcd_update();
    }
    else
    {
        anim_peaks_draw_cursor_h(cur_x + 1); /* cursor bar */    

        if (cur_x > last_pos)
        {
            osd_lcd_update_rect(last_pos, 0, cur_x - last_pos + 2, LCD_HEIGHT);
        }
        else
        {
            osd_lcd_update_rect(last_pos, 0, LCD_WIDTH - last_pos, LCD_HEIGHT);
            osd_lcd_update_rect(0, 0, cur_x + 2, LCD_HEIGHT);
        }
    }
    last_pos = cur_x;

    last_delay = get_next_delay();
    return cur_tick + last_delay;
}

static void anim_peaks_draw_cursor_v(int y)
{
#if LCD_DEPTH > 1               /* cursor bar */
    rb->lcd_set_foreground(CURSOR_COLOR);
    rb->lcd_hline(0, LCD_WIDTH-1, y); 
    rb->lcd_set_foreground(GRAPH_COLOR);
#else
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_hline(0, LCD_WIDTH-1, y);
    rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
}

static long anim_peaks_vertical(void)
{
    long cur_tick = *rb->current_tick;
    int cur_y, y;
    int left, right, dl, dr;
    bool full_update = false;
    long d = (cur_tick - last_tick) / last_delay;

    if (paused)
    {
        one_frame_paused = false;
        osd_prepare_draw();
        anim_peaks_draw_cursor_v(0);
        osd_lcd_update_rect(0, 0, LCD_WIDTH, 1);
        last_pos = -1;
        return cur_tick + HZ/5;
    }

    if (d == 0)  /* too early, bail out */
        return last_tick + last_delay;

    last_tick = cur_tick;

    int cur_left, cur_right;
    get_peaks(&cur_left, &cur_right);

    if (d > HZ) /* first call or too much delay, (re)start */
    {
        last_left = cur_left;
        last_right = cur_right;
        return cur_tick + last_delay;
    }

    one_frame_paused = false;

    osd_prepare_draw();

    cur_y = last_pos + d;

    if (cur_y >= LCD_HEIGHT)
    {
        if (osc.advance == ADV_SCROLL)
        {
            int shift = cur_y - (LCD_HEIGHT-1);
            xlcd_scroll_up(shift);
            full_update = true;
            cur_y -= shift;
            last_pos -= shift;
        }
        else 
        {
            cur_y -= LCD_HEIGHT;
        }
    }
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);

    if (cur_y > last_pos)
    {
        rb->lcd_fillrect(0, last_pos + 1, LCD_WIDTH, d);
    }
    else
    {
        rb->lcd_fillrect(0, last_pos + 1, LCD_WIDTH, LCD_HEIGHT - last_pos);
        rb->lcd_fillrect(0, 0, LCD_WIDTH, cur_y + 1);
    }
    rb->lcd_set_drawmode(DRMODE_SOLID);

    switch (osc.draw)
    {
        case DRAW_FILLED:
            left = last_left;
            right = last_right;
            dl = (cur_left  - left)  / d;
            dr = (cur_right - right) / d;

            for (y = last_pos + 1; d > 0; y++, d--)
            {
                if (y == LCD_HEIGHT)
                    y = 0;

                left  += dl;
                right += dr;

                rb->lcd_hline(LCD_WIDTH/2-1,
                              LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * left) >> 16), y);
                rb->lcd_hline(LCD_WIDTH/2+1,
                              LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * right) >> 16), y);
            }
            break;

        case DRAW_LINE:
            if (cur_y > last_pos)
            {
                rb->lcd_drawline(
                    LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * last_left) >> 16), last_pos,
                    LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * cur_left) >> 16), cur_y
                );
                rb->lcd_drawline(
                    LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * last_right) >> 16), last_pos,
                    LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * cur_right) >> 16), cur_y
                );
            }
            else
            {
                left  = last_left 
                      + (LCD_HEIGHT - last_pos) * (last_left - cur_left) / d;
                right = last_right
                      + (LCD_HEIGHT - last_pos) * (last_right - cur_right) / d;

                rb->lcd_drawline(
                    LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * last_left) >> 16), last_pos,
                    LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * left) >> 16), LCD_HEIGHT
                );
                rb->lcd_drawline(
                    LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * last_right) >> 16), last_pos,
                    LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * right) >> 16), LCD_HEIGHT
                );
                if (cur_y > 0)
                {
                    rb->lcd_drawline(
                        LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * left) >> 16), 0,
                        LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * cur_left) >> 16), cur_y
                    );
                    rb->lcd_drawline(
                        LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * right) >> 16), 0,
                        LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * cur_right) >> 16), cur_y
                    );
                }
            }
            break;
            
        case DRAW_PIXEL:
            left = last_left;
            right = last_right;
            dl = (cur_left - left) / d;
            dr = (cur_right - right) / d;

            for (y = last_pos + 1; d > 0; y++, d--)
            {
                if (y == LCD_HEIGHT)
                    y = 0;

                left  += dl;
                right += dr;

                rb->lcd_drawpixel(LCD_WIDTH/2-1 - (((LCD_WIDTH-2) * left) >> 16), y);
                rb->lcd_drawpixel(LCD_WIDTH/2+1 + (((LCD_WIDTH-2) * right) >> 16), y);
            }
            break;

    }

    last_left  = cur_left;
    last_right = cur_right;
    
    if (full_update)
    {
        osd_lcd_update();
    }
    else
    {
        anim_peaks_draw_cursor_v(cur_y + 1); /* cursor bar */

        if (cur_y > last_pos)
        {
            osd_lcd_update_rect(0, last_pos, LCD_WIDTH, cur_y - last_pos + 2);
        }
        else
        {
            osd_lcd_update_rect(0, last_pos, LCD_WIDTH, LCD_HEIGHT - last_pos);
            osd_lcd_update_rect(0, 0, LCD_WIDTH, cur_y + 2);
        }
    }
    last_pos = cur_y;

    last_delay = get_next_delay();
    return cur_tick + last_delay;
}


/** Waveform View **/

#ifdef OSCILLOSCOPE_GRAPHMODE
static int16_t waveform_buffer[2*NATIVE_FREQUENCY+2047/2048] MEM_ALIGN_ATTR;
static size_t waveform_buffer_threshold = 0;
static size_t volatile waveform_buffer_have = 0;
static size_t waveform_buffer_break = 0;
#define PCM_SAMPLESIZE (2*sizeof(int16_t))
#define PCM_BYTERATE (NATIVE_FREQUENCY*PCM_SAMPLESIZE)

#define WAVEFORM_SCALE_PCM(full_scale, sample) \
        ((((full_scale) * (sample)) + (1 << 14)) >> 15)

static void waveform_buffer_set_threshold(size_t threshold)
{
    if (threshold > sizeof (waveform_buffer))
        threshold = sizeof (waveform_buffer);

    if (threshold == waveform_buffer_threshold)
        return;

    /* Avoid changing it in the middle of buffer callback */
    rb->pcm_play_lock();

    waveform_buffer_threshold = threshold;

    rb->pcm_play_unlock();
}

static inline bool waveform_buffer_have_enough(void)
{
    return waveform_buffer_have >= waveform_buffer_threshold;
}

static void waveform_buffer_done(void)
{
    /* This is safe because buffer callback won't add more data unless the
       treshold is changed or data is removed below the threshold. This isn't
       called until after the threshold is already met. */
    size_t have = waveform_buffer_have;
    size_t threshold = waveform_buffer_threshold;
    size_t slide = (have - 1) / threshold * threshold;

    if (slide == 0)
        slide = threshold;

    have -= slide;

    if (have > 0)
        memcpy(waveform_buffer, (void *)waveform_buffer + slide, have);

    waveform_buffer_have = have;
}
    
/* where the samples are obtained and buffered */
static void waveform_buffer_callback(const void *start, size_t size)
{
    size_t threshold = waveform_buffer_threshold;
    size_t have = waveform_buffer_have;

    if (have >= threshold)
    {
        waveform_buffer_break += size;
        return;
    }

    if (waveform_buffer_break > 0)
    {
        /* Previosly missed a part or all of a frame and the break would
           happen within the data threshold area. Start where frame would
           end up if all had been processed fully. This might mean a period
           of resynchronization will have to happen first before the buffer
           is filled to the threshold or even begins filling. Maintaining
           scan phase relationship is important to proper appearance or else
           the waveform display looks sloppy. */
        size_t brk = have + waveform_buffer_break;
        waveform_buffer_have = have = 0;

        brk %= threshold;

        if (brk != 0)
        {
            brk += size;

            if (brk <= threshold)
            {
                waveform_buffer_break = brk;
                return;
            }

            brk -= threshold;
            start += size - brk;
            size = brk;
        }

        waveform_buffer_break = 0;
    }

    size_t remaining = sizeof (waveform_buffer) - have;
    size_t copy = size;

    if (copy > remaining)
    {
        waveform_buffer_break = copy - remaining;
        copy = remaining;
    }

    memcpy((void *)waveform_buffer + have, start, copy);

    waveform_buffer_have = have + copy;
}

static void waveform_buffer_reset(void)
{
    /* only called when callback is off */
    waveform_buffer_have = 0;
    waveform_buffer_threshold = 0;
    waveform_buffer_break = 0;
}

static void anim_waveform_plot_filled_h(int x, int x_prev,
                                        int vmin, int vmax,
                                        int vmin_prev, int vmax_prev)
{
    /* filltriangle is a bit dodgey when things are tiny so do
       our best to deal with it */
    if (vmin_prev != vmax_prev)
    {
        xlcd_filltriangle(x_prev, vmin_prev, x_prev, vmax_prev,
                          x, vmin);

        if (vmin != vmax)
            xlcd_filltriangle(x_prev, vmax_prev, x, vmin, x, vmax);                        
    }
    else if (vmin != vmax)
    {
        xlcd_filltriangle(x_prev, vmax_prev, x, vmin, x, vmax);
    }
    else
    {
        rb->lcd_drawline(x_prev, vmin_prev, x, vmin);
    }
}

static void anim_waveform_plot_lines_h(int x, int x_prev,
                                       int vmin, int vmax,
                                       int vmin_prev, int vmax_prev)
{
    rb->lcd_drawline(x_prev, vmin_prev, x, vmin);

    if (vmax_prev != vmin_prev || vmax != vmin)
        rb->lcd_drawline(x_prev, vmax_prev, x, vmax);
}

static void anim_waveform_plot_pixel_h(int x, int x_prev,
                                       int vmin, int vmax,
                                       int vmin_prev, int vmax_prev)
{
    rb->lcd_drawpixel(x, vmin);

    if (vmax != vmin)
        rb->lcd_drawpixel(x, vmax);

    (void)x_prev; (void)vmin_prev; (void)vmax_prev;
}

static long anim_waveform_horizontal(void)
{
    static void (* const plot[MAX_DRAW])(int, int, int, int, int, int) =
    {
        [DRAW_FILLED] = anim_waveform_plot_filled_h,
        [DRAW_LINE]   = anim_waveform_plot_lines_h,
        [DRAW_PIXEL]  = anim_waveform_plot_pixel_h,
    };

    long cur_tick = *rb->current_tick;

    if (rb->mixer_channel_status(PCM_MIXER_CHAN_PLAYBACK) != CHANNEL_PLAYING)
    {
        osd_prepare_draw();
        rb->lcd_hline(0, LCD_WIDTH-1, 1*LCD_HEIGHT/4);
        rb->lcd_hline(0, LCD_WIDTH-1, 3*LCD_HEIGHT/4);
        osd_lcd_update();
        one_frame_paused = false;
        return cur_tick + HZ/5;
    }

    int count = (NATIVE_FREQUENCY*osc_delay + 100*HZ - 1) / (100*HZ);

    waveform_buffer_set_threshold(count*PCM_SAMPLESIZE);

    if (!waveform_buffer_have_enough())
        return cur_tick + HZ/100;

    one_frame_paused = false;

    osd_prepare_draw();

    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, 0, LCD_WIDTH, LCD_HEIGHT);
    rb->lcd_set_drawmode(DRMODE_SOLID);
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(GRAPH_COLOR);
#endif

    int x = 0, x_prev = count - 1;
    int x_step = (LCD_WIDTH-1) / x_prev;
    int x_a_step = (LCD_WIDTH-1) - x_step * x_prev;
    int x_a = 0;

    int idx_step = (count / LCD_WIDTH) * 2;
    int idx_a_step = count - idx_step * (LCD_WIDTH/2);
    int idx = idx_step, idx_prev = 0;
    int idx_a = idx_a_step;

    int a_lim = MIN(x_prev, LCD_WIDTH-1);

    int lmin, lmin_prev;
    int rmin, rmin_prev;
    int lmax, lmax_prev;
    int rmax, rmax_prev;

    if (osc.draw == DRAW_PIXEL)
        goto plot_start_noprev; /* Doesn't need previous points */

    lmax = lmin = waveform_buffer[0];
    rmax = rmin = waveform_buffer[1];

    /* Find min-max envelope for interval */
    for (int i = 2; i < idx; i += 2)
    {
        int sl = waveform_buffer[i + 0];
        int sr = waveform_buffer[i + 1];

        if (sl < lmin)
            lmin = sl;
        if (sl > lmax)
            lmax = sl;

        if (sr < rmin)
            rmin = sr;
        if (sr > rmax)
            rmax = sr;
    }

    lmin = (1*LCD_HEIGHT/4) + WAVEFORM_SCALE_PCM(LCD_HEIGHT/4, lmin);
    lmax = (1*LCD_HEIGHT/4) + WAVEFORM_SCALE_PCM(LCD_HEIGHT/4, lmax);
    rmin = (3*LCD_HEIGHT/4) + WAVEFORM_SCALE_PCM(LCD_HEIGHT/4, rmin);
    rmax = (3*LCD_HEIGHT/4) + WAVEFORM_SCALE_PCM(LCD_HEIGHT/4, rmax);

    while (1)
    {
        x_prev = x;
        x += x_step;
        x_a += x_a_step;

        if (x_a >= a_lim)
        {
            x_a -= a_lim;
            x++;
        }

        if (x >= LCD_WIDTH)
            break;

        idx_prev = idx;
        idx += idx_step;
        idx_a += idx_a_step;

        if (idx_a > a_lim)
        {
            idx_a -= a_lim + 1;
            idx += 2;
        }

        lmin_prev = lmin, lmax_prev = lmax;
        rmin_prev = rmin, rmax_prev = rmax;

    plot_start_noprev:
        lmax = lmin = waveform_buffer[idx_prev + 0];
        rmax = rmin = waveform_buffer[idx_prev + 1];

        /* Find min-max envelope for interval */
        for (int i = idx_prev + 2; i < idx; i += 2)
        {
            int sl = waveform_buffer[i + 0];
            int sr = waveform_buffer[i + 1];

            if (sl < lmin)
                lmin = sl;
            if (sl > lmax)
                lmax = sl;

            if (sr < rmin)
                rmin = sr;
            if (sr > rmax)
                rmax = sr;
        }

        lmin = (1*LCD_HEIGHT/4) + WAVEFORM_SCALE_PCM(LCD_HEIGHT/4, lmin);
        lmax = (1*LCD_HEIGHT/4) + WAVEFORM_SCALE_PCM(LCD_HEIGHT/4, lmax);
        rmin = (3*LCD_HEIGHT/4) + WAVEFORM_SCALE_PCM(LCD_HEIGHT/4, rmin);
        rmax = (3*LCD_HEIGHT/4) + WAVEFORM_SCALE_PCM(LCD_HEIGHT/4, rmax);

        plot[osc.draw](x, x_prev, lmin, lmax, lmin_prev, lmax_prev);
        plot[osc.draw](x, x_prev, rmin, rmax, rmin_prev, rmax_prev);
    }

    waveform_buffer_done();

    osd_lcd_update();

    long delay = get_next_delay();
    return cur_tick + delay - waveform_buffer_have * HZ / PCM_BYTERATE;
}

static void anim_waveform_plot_filled_v(int y, int y_prev,
                                        int vmin, int vmax,
                                        int vmin_prev, int vmax_prev)
{
    /* filltriangle is a bit dodgey when things are tiny so do
       our best to deal with it */
    if (vmin_prev != vmax_prev)
    {
        xlcd_filltriangle(vmin_prev, y_prev, vmax_prev, y_prev,
                          vmin, y);

        if (vmin != vmax)
            xlcd_filltriangle(vmax_prev, y_prev, vmin, y, vmax, y);
    }
    else if (vmin != vmax)
    {
        xlcd_filltriangle(vmax_prev, y_prev, vmin, y, vmax, y);
    }
    else
    {
        rb->lcd_drawline(vmin_prev, y_prev, vmin, y);
    }
}

static void anim_waveform_plot_lines_v(int y, int y_prev,
                                       int vmin, int vmax,
                                       int vmin_prev, int vmax_prev)
{
    rb->lcd_drawline(vmin_prev, y_prev, vmin, y);

    if (vmax_prev != vmin_prev || vmax != vmin)
        rb->lcd_drawline(vmax_prev, y_prev, vmax, y);
}

static void anim_waveform_plot_pixel_v(int y, int y_prev,
                                       int vmin, int vmax,
                                       int vmin_prev, int vmax_prev)
{
    rb->lcd_drawpixel(vmin, y);

    if (vmax != vmin)
        rb->lcd_drawpixel(vmax, y);

    (void)y_prev; (void)vmin_prev; (void)vmax_prev;
}

static long anim_waveform_vertical(void)
{
    static void (* const plot[MAX_DRAW])(int, int, int, int, int, int) =
    {
        [DRAW_FILLED] = anim_waveform_plot_filled_v,
        [DRAW_LINE]   = anim_waveform_plot_lines_v,
        [DRAW_PIXEL]  = anim_waveform_plot_pixel_v,
    };

    long cur_tick = *rb->current_tick;

    if (rb->mixer_channel_status(PCM_MIXER_CHAN_PLAYBACK) != CHANNEL_PLAYING)
    {
        osd_prepare_draw();
        rb->lcd_vline(1*LCD_WIDTH/4, 0, LCD_HEIGHT-1);
        rb->lcd_vline(3*LCD_WIDTH/4, 0, LCD_HEIGHT-1);
        osd_lcd_update();
        one_frame_paused = false;
        return cur_tick + HZ/5;
    }

    int count = (NATIVE_FREQUENCY*osc_delay + 100*HZ - 1) / (100*HZ);

    waveform_buffer_set_threshold(count*PCM_SAMPLESIZE);

    if (!waveform_buffer_have_enough())
        return cur_tick + HZ/100;

    one_frame_paused = false;

    osd_prepare_draw();

    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(0, 0, LCD_WIDTH, LCD_HEIGHT);
    rb->lcd_set_drawmode(DRMODE_SOLID);
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(GRAPH_COLOR);
#endif

    int y = 0, y_prev = count - 1;
    int y_step = (LCD_HEIGHT-1) / y_prev;
    int y_a_step = (LCD_HEIGHT-1) - y_step * y_prev;
    int y_a = 0;

    int idx_step = (count / LCD_HEIGHT) * 2;
    int idx_a_step = count - idx_step * (LCD_HEIGHT/2);
    int idx = idx_step, idx_prev = 0;
    int idx_a = idx_a_step;

    int a_lim = MIN(y_prev, LCD_HEIGHT-1);

    int lmin, lmin_prev;
    int rmin, rmin_prev;
    int lmax, lmax_prev;
    int rmax, rmax_prev;

    if (osc.draw == DRAW_PIXEL)
        goto plot_start_noprev; /* Doesn't need previous points */

    lmax = lmin = waveform_buffer[0];
    rmax = rmin = waveform_buffer[1];

    /* Find min-max envelope for interval */
    for (int i = 2; i < idx; i += 2)
    {
        int sl = waveform_buffer[i + 0];
        int sr = waveform_buffer[i + 1];

        if (sl < lmin)
            lmin = sl;
        if (sl > lmax)
            lmax = sl;

        if (sr < rmin)
            rmin = sr;
        if (sr > rmax)
            rmax = sr;
    }

    lmin = (1*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4, lmin);
    lmax = (1*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4, lmax);
    rmin = (3*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4, rmin);
    rmax = (3*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4, rmax);

    while (1)
    {
        y_prev = y;
        y += y_step;
        y_a += y_a_step;

        if (y_a >= a_lim)
        {
            y_a -= a_lim;
            y++;
        }

        if (y >= LCD_HEIGHT)
            break;

        idx_prev = idx;
        idx += idx_step;
        idx_a += idx_a_step;

        if (idx_a > a_lim)
        {
            idx_a -= a_lim + 1;
            idx += 2;
        }

        lmin_prev = lmin, lmax_prev = lmax;
        rmin_prev = rmin, rmax_prev = rmax;

    plot_start_noprev:
        lmax = lmin = waveform_buffer[idx_prev + 0];
        rmax = rmin = waveform_buffer[idx_prev + 1];

        /* Find min-max envelope for interval */
        for (int i = idx_prev + 2; i < idx; i += 2)
        {
            int sl = waveform_buffer[i + 0];
            int sr = waveform_buffer[i + 1];

            if (sl < lmin)
                lmin = sl;
            if (sl > lmax)
                lmax = sl;

            if (sr < rmin)
                rmin = sr;
            if (sr > rmax)
                rmax = sr;
        }

        lmin = (1*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4, lmin);
        lmax = (1*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4, lmax);
        rmin = (3*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4, rmin);
        rmax = (3*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4, rmax);

        plot[osc.draw](y, y_prev, lmin, lmax, lmin_prev, lmax_prev);
        plot[osc.draw](y, y_prev, rmin, rmax, rmin_prev, rmax_prev);
    }

    waveform_buffer_done();

    osd_lcd_update();

    long delay = get_next_delay();
    return cur_tick + delay - waveform_buffer_have * HZ / PCM_BYTERATE;
}

static void anim_waveform_exit(void)
{
    /* Remove any buffer hook */
    rb->mixer_channel_set_buffer_hook(PCM_MIXER_CHAN_PLAYBACK, NULL);
#ifdef HAVE_SCHEDULER_BOOSTCTRL
    /* Remove our boost */
    rb->cancel_cpu_boost();
#endif
}
#endif /* OSCILLOSCOPE_GRAPHMODE */

static void graphmode_reset(void)
{
    last_left = 0;
    last_right = 0;
    last_pos = 0;
    last_tick = *rb->current_tick;
    last_delay = 1;
    osd_prepare_draw();
    rb->lcd_clear_display();
    osd_lcd_update();

    one_frame_paused = true;
    osd_set_timeout(paused ? 4*HZ : HZ);
}

static void graphmode_pause_unpause(bool paused)
{
    if (!paused)
    {
        last_tick = *rb->current_tick;
        osd_set_timeout(HZ);
        one_frame_paused = false;
    }
}

static void update_osc_delay(int value)
{
    /*
     *                   / 99*[e ^ ((100 - x) / z) - 1]    \
     * osc_delay = HZ * |  ---------------------------- + 1 |
     *                   \      [e ^ (99 / z) - 1]         /
     *
     * x: 1 .. 100, z: 15, delay: 100.00 .. 1.00
     */
    int z = 15;
    int f = fp16_exp(fp_div(100 - value, z, 16)) - (1 << 16);
    int g = fp16_exp(fp_div(99, z, 16)) - (1 << 16);
    osc_delay = (HZ * 99ll * f + (g / 2)) / g + HZ;
    osc_delay_error = 0;
}

static void graphmode_setup(void)
{
    int *value;

#ifdef OSCILLOSCOPE_GRAPHMODE
    if (osc.graphmode == GRAPH_WAVEFORM)
    {
        rb->mixer_channel_set_buffer_hook(PCM_MIXER_CHAN_PLAYBACK,
                                          waveform_buffer_callback);
#ifdef HAVE_SCHEDULER_BOOSTCTRL
        rb->trigger_cpu_boost(); /* Just looks better */
#endif
        value = &osc.wavespeed;
    }
    else
#endif /* OSCILLOSCOPE_GRAPHMODE */
    {
#ifdef OSCILLOSCOPE_GRAPHMODE
        rb->mixer_channel_set_buffer_hook(PCM_MIXER_CHAN_PLAYBACK,
                                          NULL);
#ifdef HAVE_SCHEDULER_BOOSTCTRL
        rb->cancel_cpu_boost();
#endif
        waveform_buffer_reset();
#endif /* OSCILLOSCOPE_GRAPHMODE */
        value = &osc.speed;
    }

    update_osc_delay(*value);
    graphmode_reset();
}

static long oscilloscope_draw(void)
{
    if (message != -1)
    {
        osd_show_message((enum osc_message_ids)message, msgval);
        message = -1;
    }
    else
    {
        osd_monitor_timeout();
    }

    long delay = HZ/5;

    if (!paused || one_frame_paused)
    {
        long tick;

#ifdef OSCILLOSCOPE_GRAPHMODE
        if (osc.graphmode == GRAPH_WAVEFORM)
        {
            tick = osc.orientation == OSC_HORIZ ?
                        anim_waveform_horizontal() :
                        anim_waveform_vertical();
        }
        else
#endif /* OSCILLOSCOPE_GRAPHMODE */
        {
            tick = osc.orientation == OSC_HORIZ ?
                        anim_peaks_horizontal() :
                        anim_peaks_vertical();
        }

        delay = tick - *rb->current_tick;

        if (delay > HZ/5)
            delay = HZ/5;
    }

    return delay;
}

static void cleanup(void)
{
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_DEFAULT_FG);
    rb->lcd_set_background(LCD_DEFAULT_BG);
#endif
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();

#ifdef OSCILLOSCOPE_GRAPHMODE
    anim_waveform_exit();
#endif

    /* save settings if changed */
    if (rb->memcmp(&osc, &osc_disk, sizeof(osc)))
    {
        rb->memcpy(&osc_disk, &osc, sizeof(osc));
        configfile_save(cfg_filename, disk_config, ARRAYLEN(disk_config),
                        CFGFILE_VERSION);
    }
}

static void setup(void)
{
    atexit(cleanup);
    configfile_load(cfg_filename, disk_config, ARRAYLEN(disk_config),
                    CFGFILE_MINVERSION);
    osc = osc_disk; /* copy to running config */

    osd_init();

#if LCD_DEPTH > 1
    osd_prepare_draw();
    rb->lcd_set_foreground(GRAPH_COLOR);
    rb->lcd_set_background(BACKG_COLOR);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_clear_display();
    osd_lcd_update();
#endif

    /* Turn off backlight timeout */
    backlight_ignore_timeout();
    graphmode_setup();
}

enum plugin_status plugin_start(const void* parameter)
{
    bool exit = false;
#ifdef NEED_LASTBUTTON
    int lastbutton = BUTTON_NONE;
#endif
    int button, vol;

    setup();

    while (!exit)
    {
        long delay = oscilloscope_draw();

        if (delay <= 0)
        {
            delay = 0;
            rb->yield(); /* tmo = 0 won't yield */
        }

        button = rb->button_get_w_tmo(delay);

        switch (button)
        {
#ifdef OSCILLOSCOPE_RC_QUIT
            case OSCILLOSCOPE_RC_QUIT:
#endif
            case OSCILLOSCOPE_QUIT:
                exit = true;
                break;

            case OSCILLOSCOPE_ADVMODE:
#ifdef OSCILLOSCOPE_GRAPHMODE
                if (osc.graphmode == GRAPH_WAVEFORM)
                    break; /* Not applicable */
#endif
                if (++osc.advance >= MAX_ADV)
                    osc.advance = 0;

                osc_popupmsg(OSC_MSG_ADVMODE, osc.advance);
                break;

            case OSCILLOSCOPE_DRAWMODE:
#ifdef OSCILLOSCOPE_DRAWMODE_PRE
                if (lastbutton != OSCILLOSCOPE_DRAWMODE_PRE)
                    break;
#endif
                if (++osc.draw >= MAX_DRAW)
                    osc.draw = 0;

                osc_popupmsg(OSC_MSG_DRAWMODE, osc.draw);
                break;

            /* Need all keymaps for this (remove extra condition when
               completed) */
#ifdef OSCILLOSCOPE_GRAPHMODE
            case OSCILLOSCOPE_GRAPHMODE:
#ifdef OSCILLOSCOPE_GRAPHMODE_PRE
                if (lastbutton != OSCILLOSCOPE_GRAPHMODE_PRE)
                    break;
#endif
                if (++osc.graphmode >= MAX_GRAPH)
                    osc.graphmode = 0;

                graphmode_setup();

                osc_popupmsg(OSC_MSG_GRAPHMODE, osc.graphmode);
                break;
#endif /* OSCILLOSCOPE_GRAPHMODE */
                
            case OSCILLOSCOPE_ORIENTATION:
#ifdef OSCILLOSCOPE_ORIENTATION_PRE
                if (lastbutton != OSCILLOSCOPE_ORIENTATION_PRE)
                    break;
#endif
                if (++osc.orientation >= MAX_OSC)
                    osc.orientation = 0;

                graphmode_reset();
                osc_popupmsg(OSC_MSG_ORIENTATION, osc.orientation);
                break;

            case OSCILLOSCOPE_PAUSE:
#ifdef OSCILLOSCOPE_PAUSE_PRE
                if (lastbutton != OSCILLOSCOPE_PAUSE_PRE)
                    break;
#endif
                paused = !paused;
                graphmode_pause_unpause(paused);
                osc_popupmsg(OSC_MSG_PAUSED, paused ? 1 : 0);
                break;
                
            case OSCILLOSCOPE_SPEED_UP:
            case OSCILLOSCOPE_SPEED_UP | BUTTON_REPEAT:
            {
                int *val = &osc.speed;
#ifdef OSCILLOSCOPE_GRAPHMODE
                if (osc.graphmode == GRAPH_WAVEFORM)
                    val = &osc.wavespeed;
#endif
                if (++*val > 100)
                    *val = 100;

                update_osc_delay(*val);
                osc_popupmsg(OSC_MSG_SPEED, *val);
                break;
            }
                
            case OSCILLOSCOPE_SPEED_DOWN:
            case OSCILLOSCOPE_SPEED_DOWN | BUTTON_REPEAT:
            {
                int *val = &osc.speed;
#ifdef OSCILLOSCOPE_GRAPHMODE
                if (osc.graphmode == GRAPH_WAVEFORM)
                    val = &osc.wavespeed;
#endif
                if (--*val < 1)
                    *val = 1;

                update_osc_delay(*val);
                osc_popupmsg(OSC_MSG_SPEED, *val);
                break;
            }

            case OSCILLOSCOPE_VOL_UP:
            case OSCILLOSCOPE_VOL_UP | BUTTON_REPEAT:
                vol = rb->global_settings->volume;
                if (vol < rb->sound_max(SOUND_VOLUME))
                {
                    vol++;
                    rb->sound_set(SOUND_VOLUME, vol);
                    rb->global_settings->volume = vol;
                }

                osc_popupmsg(OSC_MSG_VOLUME, vol);
                break;

            case OSCILLOSCOPE_VOL_DOWN:
            case OSCILLOSCOPE_VOL_DOWN | BUTTON_REPEAT:
                vol = rb->global_settings->volume;
                if (vol > rb->sound_min(SOUND_VOLUME))
                {
                    vol--;
                    rb->sound_set(SOUND_VOLUME, vol);
                    rb->global_settings->volume = vol;
                }

                osc_popupmsg(OSC_MSG_VOLUME, vol);
                break;

            default:
                exit_on_usb(button);
                break;
        }

#ifdef NEED_LASTBUTTON
        if (button != BUTTON_NONE)
            lastbutton = button;
#endif
    } /* while */

    return PLUGIN_OK;
    (void)parameter;
}
