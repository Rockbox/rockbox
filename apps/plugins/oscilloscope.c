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
#include "fixedpoint.h"
#include "lib/helper.h"
#include "lib/pluginlib_exit.h"
#include "lib/xlcd.h"
#include "lib/configfile.h"
#include "lib/osd.h"

/* variable button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
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
/* Not enough plugin RAM for waveform view */

#elif (CONFIG_KEYPAD == SANSA_M200_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_SELECT | BUTTON_REL)
#define OSCILLOSCOPE_ADVMODE         BUTTON_DOWN
#define OSCILLOSCOPE_ORIENTATION     BUTTON_UP
#define OSCILLOSCOPE_PAUSE           (BUTTON_SELECT | BUTTON_UP)
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
/* Not enough plugin RAM for waveform view */

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

#elif CONFIG_KEYPAD == CREATIVE_ZENXFI3_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_MENU | BUTTON_UP)
#define OSCILLOSCOPE_ADVMODE         (BUTTON_MENU | BUTTON_DOWN)
#define OSCILLOSCOPE_GRAPHMODE_PRE   BUTTON_PLAY
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_PLAY | BUTTON_REPEAT)
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_MENU | BUTTON_BACK)
#define OSCILLOSCOPE_SPEED_UP        (BUTTON_BACK | BUTTON_UP)
#define OSCILLOSCOPE_SPEED_DOWN      (BUTTON_BACK | BUTTON_DOWN)
#define OSCILLOSCOPE_PAUSE           (BUTTON_PLAY | BUTTON_REL)
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN

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

#elif CONFIG_KEYPAD == ONDAVX747_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN

#elif CONFIG_KEYPAD == ONDAVX777_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER

#elif CONFIG_KEYPAD == MROBE500_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YH92X_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_REW
#define OSCILLOSCOPE_DRAWMODE        BUTTON_FFWD
#define OSCILLOSCOPE_ADVMODE         (BUTTON_PLAY|BUTTON_RIGHT)
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_PLAY|BUTTON_UP)
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_PLAY|BUTTON_LEFT)
#define OSCILLOSCOPE_PAUSE_PRE       BUTTON_PLAY
#define OSCILLOSCOPE_PAUSE           (BUTTON_PLAY|BUTTON_REL)
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN

#elif CONFIG_KEYPAD == SAMSUNG_YH820_PAD
#define OSCILLOSCOPE_QUIT            BUTTON_REW
#define OSCILLOSCOPE_DRAWMODE        BUTTON_FFWD
#define OSCILLOSCOPE_ADVMODE         (BUTTON_REC|BUTTON_RIGHT)
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_REC|BUTTON_UP)
#define OSCILLOSCOPE_GRAPHMODE       (BUTTON_REC|BUTTON_LEFT)
#define OSCILLOSCOPE_PAUSE           BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP        BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN

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
#define OSCILLOSCOPE_ORIENTATION     BUTTON_MENU
#define OSCILLOSCOPE_PAUSE           BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP        BUTTON_FF
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_REW
#define OSCILLOSCOPE_VOL_UP          BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_DOWN
/* Need GRAPHMODE */

#elif CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD
#define OSCILLOSCOPE_QUIT         BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE     BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE      BUTTON_BACK
#define OSCILLOSCOPE_ORIENTATION  BUTTON_UP
#define OSCILLOSCOPE_PAUSE        BUTTON_PLAYPAUSE
#define OSCILLOSCOPE_SPEED_UP     BUTTON_LEFT
#define OSCILLOSCOPE_SPEED_DOWN   BUTTON_RIGHT
#define OSCILLOSCOPE_VOL_UP       BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN     BUTTON_VOL_DOWN
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

#elif CONFIG_KEYPAD == SONY_NWZ_PAD
#define OSCILLOSCOPE_QUIT           (BUTTON_BACK|BUTTON_REPEAT)
#define OSCILLOSCOPE_DRAWMODE       BUTTON_BACK
#define OSCILLOSCOPE_ADVMODE        (BUTTON_POWER|BUTTON_REPEAT)
#define OSCILLOSCOPE_ORIENTATION    BUTTON_POWER
#define OSCILLOSCOPE_PAUSE          BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP       BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN     BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP         BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN       BUTTON_DOWN

#elif CONFIG_KEYPAD == CREATIVE_ZEN_PAD
#define OSCILLOSCOPE_QUIT           BUTTON_BACK
#define OSCILLOSCOPE_DRAWMODE       BUTTON_SELECT
#define OSCILLOSCOPE_ADVMODE        BUTTON_MENU
#define OSCILLOSCOPE_ORIENTATION    BUTTON_SHORTCUT
#define OSCILLOSCOPE_PAUSE          BUTTON_PLAYPAUSE
#define OSCILLOSCOPE_SPEED_UP       BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN     BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP         BUTTON_UP
#define OSCILLOSCOPE_VOL_DOWN       BUTTON_DOWN

#elif CONFIG_KEYPAD == DX50_PAD
#define OSCILLOSCOPE_QUIT            (BUTTON_POWER|BUTTON_REL)
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN

#elif CONFIG_KEYPAD == CREATIVE_ZENXFI2_PAD
#define OSCILLOSCOPE_QUIT           BUTTON_POWER
#define OSCILLOSCOPE_PAUSE          BUTTON_MENU
#define OSCILLOSCOPE_ORIENTATION    BUTTON_TOPLEFT
#define OSCILLOSCOPE_GRAPHMODE      BUTTON_BOTTOMLEFT

#elif CONFIG_KEYPAD == AGPTEK_ROCKER_PAD
#define OSCILLOSCOPE_QUIT           BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE       (BUTTON_SELECT|BUTTON_UP)
#define OSCILLOSCOPE_ADVMODE        BUTTON_DOWN
#define OSCILLOSCOPE_ORIENTATION    BUTTON_UP
#define OSCILLOSCOPE_PAUSE          BUTTON_SELECT
#define OSCILLOSCOPE_SPEED_UP       BUTTON_RIGHT
#define OSCILLOSCOPE_SPEED_DOWN     BUTTON_LEFT
#define OSCILLOSCOPE_VOL_UP         BUTTON_VOLUP
#define OSCILLOSCOPE_VOL_DOWN       BUTTON_VOLDOWN

#elif (CONFIG_KEYPAD == XDUOO_X3_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE_PRE    BUTTON_PLAY
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_PLAY | BUTTON_REL)
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_PLAY
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_PLAY | BUTTON_REPEAT)
#define OSCILLOSCOPE_ADVMODE         BUTTON_HOME
#define OSCILLOSCOPE_PAUSE           BUTTON_OPTION
#define OSCILLOSCOPE_SPEED_UP        BUTTON_NEXT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_PREV
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
#define NEED_LASTBUTTON

#elif (CONFIG_KEYPAD == XDUOO_X3II_PAD) || (CONFIG_KEYPAD == XDUOO_X20_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE_PRE    BUTTON_PLAY
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_PLAY | BUTTON_REL)
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_PLAY
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_PLAY | BUTTON_REPEAT)
#define OSCILLOSCOPE_ADVMODE         BUTTON_HOME
#define OSCILLOSCOPE_PAUSE           BUTTON_OPTION
#define OSCILLOSCOPE_SPEED_UP        BUTTON_NEXT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_PREV
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
#define NEED_LASTBUTTON

#elif (CONFIG_KEYPAD == FIIO_M3K_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE_PRE    BUTTON_PLAY
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_PLAY | BUTTON_REL)
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_PLAY
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_PLAY | BUTTON_REPEAT)
#define OSCILLOSCOPE_ADVMODE         BUTTON_HOME
#define OSCILLOSCOPE_PAUSE           BUTTON_OPTION
#define OSCILLOSCOPE_SPEED_UP        BUTTON_NEXT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_PREV
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
#define NEED_LASTBUTTON

#elif (CONFIG_KEYPAD == IHIFI_770_PAD) || (CONFIG_KEYPAD == IHIFI_800_PAD)
#define OSCILLOSCOPE_QUIT            BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE_PRE    BUTTON_PLAY
#define OSCILLOSCOPE_DRAWMODE        (BUTTON_PLAY | BUTTON_REL)
#define OSCILLOSCOPE_ORIENTATION_PRE BUTTON_PLAY
#define OSCILLOSCOPE_ORIENTATION     (BUTTON_PLAY | BUTTON_REPEAT)
#define OSCILLOSCOPE_ADVMODE         BUTTON_HOME
#define OSCILLOSCOPE_PAUSE           (BUTTON_HOME | BUTTON_REPEAT)
#define OSCILLOSCOPE_SPEED_UP        BUTTON_NEXT
#define OSCILLOSCOPE_SPEED_DOWN      BUTTON_PREV
#define OSCILLOSCOPE_VOL_UP          BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN        BUTTON_VOL_DOWN
#define NEED_LASTBUTTON

#elif CONFIG_KEYPAD == EROSQ_PAD
#define OSCILLOSCOPE_QUIT           BUTTON_POWER
#define OSCILLOSCOPE_DRAWMODE       BUTTON_PREV
#define OSCILLOSCOPE_ADVMODE        BUTTON_NEXT
#define OSCILLOSCOPE_ORIENTATION    BUTTON_BACK
#define OSCILLOSCOPE_PAUSE          BUTTON_PLAY
#define OSCILLOSCOPE_SPEED_UP       BUTTON_SCROLL_FWD
#define OSCILLOSCOPE_SPEED_DOWN     BUTTON_SCROLL_BACK
#define OSCILLOSCOPE_VOL_UP         BUTTON_VOL_UP
#define OSCILLOSCOPE_VOL_DOWN       BUTTON_VOL_DOWN

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
#endif /* HAVE_TOUCHSCREEN */

#if defined(OSCILLOSCOPE_DRAWMODE_PRE) || defined(OSCILLOSCOPE_ORIENTATION_PRE) \
    || defined(OSCILLOSCOPE_GRAPHMODE_PRE) || defined(OSCILLOSCOPE_PAUSE_PRE)
#define NEED_LASTBUTTON
#endif

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
#define OSC_OSD_MARGIN_SIZE 0 /* Border and text are different */
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

#ifndef OSC_OSD_MARGIN_SIZE
#define OSC_OSD_MARGIN_SIZE 1
#endif

#define MIN_SPEED 1
#define MAX_SPEED 100

enum { DRAW_FILLED, DRAW_LINE, DRAW_PIXEL, MAX_DRAW };
enum { ADV_SCROLL, ADV_WRAP, MAX_ADV };
enum { OSC_HORIZ, OSC_VERT, MAX_OSC };
enum
{
    GRAPH_PEAKS,
#ifdef OSCILLOSCOPE_GRAPHMODE
    GRAPH_WAVEFORM,
#endif
    MAX_GRAPH
};

#define CFGFILE_VERSION    1  /* Current config file version */
#define CFGFILE_MINVERSION 0  /* Minimum config file version to accept */

/* global variables */

/*
 *                   / 99*[e ^ ((100 - x) / z) - 1]    \
 * osc_delay = HZ * |  ---------------------------- + 1 |
 *                   \      [e ^ (99 / z) - 1]         /
 *
 * x: 1 .. 100, z: 15, delay: 100.00 .. 1.00
 */
static const unsigned short osc_delay_tbl[100] =
{
    10000, 9361, 8763, 8205, 7682, 7192, 6735, 6307, 5906, 5532,
     5181, 4853, 4546, 4259, 3991, 3739, 3504, 3285, 3079, 2886,
     2706, 2538, 2380, 2233, 2095, 1966, 1845, 1732, 1627, 1528,
     1435, 1349, 1268, 1192, 1121, 1055,  993,  935,  880,  830,
      782,  737,  696,  657,  620,  586,  554,  524,  497,  470,
      446,  423,  402,  381,  363,  345,  329,  313,  299,  285,
      273,  261,  250,  239,  230,  221,  212,  204,  197,  190,
      183,  177,  171,  166,  161,  156,  152,  147,  144,  140,
      136,  133,  130,  127,  125,  122,  120,  118,  116,  114,
      112,  110,  108,  107,  106,  104,  103,  102,  101,  100
};

#define GRAPH_PEAKS_DEFAULT 68
#define GRAPH_WAVEFORM_DEFAULT 50

/* settings */
static struct osc_config
{
    int speed[MAX_GRAPH];
    int draw;
    int advance;
    int orientation;
    int graphmode;
} osc_disk =
{
    .speed =
    {
        [GRAPH_PEAKS] = GRAPH_PEAKS_DEFAULT,
#ifdef OSCILLOSCOPE_GRAPHMODE
        [GRAPH_WAVEFORM] = GRAPH_WAVEFORM_DEFAULT,
#endif
    },
    .draw = DRAW_FILLED,
    .advance = ADV_SCROLL,
    .orientation = OSC_HORIZ,
#ifdef OSCILLOSCOPE_GRAPHMODE
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
   { TYPE_INT,  MIN_SPEED, MAX_SPEED,
     { .int_p = &osc_disk.speed[GRAPH_PEAKS] }, "speed", NULL },
   { TYPE_ENUM, 0, MAX_DRAW, { .int_p = &osc_disk.draw }, "draw", draw_str },
   { TYPE_ENUM, 0, MAX_ADV, { .int_p = &osc_disk.advance }, "advance",
     advance_str },
   { TYPE_ENUM, 0, MAX_OSC, { .int_p = &osc_disk.orientation }, "orientation",
     orientation_str },
#ifdef OSCILLOSCOPE_GRAPHMODE
   { TYPE_INT, MIN_SPEED, MAX_SPEED,
     { .int_p = &osc_disk.speed[GRAPH_WAVEFORM] }, "wavespeed", NULL },
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
};

static long (*anim_draw)(void);
static int message = -1, msgval = 0; /* popup message */
static bool paused = false;
static bool one_frame_paused = false; /* Allow one frame to be drawn when paused if
                                         view is erased */
static long osc_delay;       /* delay in 100ths of a tick */
static long osc_delay_error; /* delay fraction error accumulator */

/* implementation */


/** Messages **/

static unsigned char osc_osd_message[16]; /* message to display (formatted) */

static inline void osc_popupmsg(int msg, int val)
{
    message = msg; msgval = val;
}

/* Format a message to display */
static void osc_osd_format_message(enum osc_message_ids id, int val)
{
    const char *msg = "";

    switch (id)
    {
    case OSC_MSG_ADVMODE:
        msg = val == ADV_WRAP ? "Wrap" : "Scroll";
        break;

    case OSC_MSG_DRAWMODE:
        switch (val)
        {
            case DRAW_PIXEL: msg = "Pixel"; break;
            case DRAW_LINE:  msg = "Line";  break;
            default:         msg = "Filled";
        }
        break;

#ifdef OSCILLOSCOPE_GRAPHMODE
    case OSC_MSG_GRAPHMODE:
        msg = val == GRAPH_WAVEFORM ? "Waveform" : "Peaks";
        break;
#endif

    case OSC_MSG_ORIENTATION:
        msg = val == OSC_VERT ? "Vertical" : "Horizontal";
        break;

    case OSC_MSG_PAUSED:
        msg = val ? "Paused" : "Running";
        break;

    case OSC_MSG_SPEED:
        msg = "Speed: %d";
        break;

    case OSC_MSG_VOLUME:
        rb->snprintf(osc_osd_message, sizeof (osc_osd_message),
                     "Volume: %d%s",
                     rb->sound_val2phys(SOUND_VOLUME, val),
                     rb->sound_unit(SOUND_VOLUME));
        return;
    }

    /* Default action: format integer */
    rb->snprintf(osc_osd_message, sizeof (osc_osd_message), msg, val);
}


/** OSD **/

/* Actually draw the OSD within the OSD viewport */
extern void osd_lcd_draw_cb(int x, int y, int width, int height)
{
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(OSD_TEXT_COLOR);
    rb->lcd_set_background(OSD_BACKG_COLOR);
#endif
#if OSC_OSD_MARGIN_SIZE != 0
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(1, 1, width - 2, height - 2);
    rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
    rb->lcd_putsxy(1+OSC_OSD_MARGIN_SIZE, 1+OSC_OSD_MARGIN_SIZE,
                   osc_osd_message);
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(OSD_BORDER_COLOR);
#endif
    rb->lcd_drawrect(0, 0, width, height);

    (void)x; (void)y;
}

/* Perform all necessary basic setup tasks for the OSD */
static void osc_osd_init(void)
{
    /* Grab the plugin buffer for the OSD back buffer */
    size_t bufsize;
    void *buf = rb->plugin_get_buffer(&bufsize);

    int width, height;
    rb->lcd_setfont(FONT_UI);
    rb->lcd_getstringsize("M", NULL, &height);
    width = LCD_WIDTH;
    height += 2 + 2*OSC_OSD_MARGIN_SIZE;
    osd_init(OSD_INIT_MAJOR_HEIGHT | OSD_INIT_MINOR_MAX, buf, bufsize,
             osd_lcd_draw_cb, &width, &height, NULL);
}

/* Format a message by ID and show the OSD */
static void osc_osd_show_message(int id, int val)
{
    osc_osd_format_message(id, val);

    if (!osd_enabled())
        return;

    int width, height;
    int maxwidth, maxheight;

    struct viewport *last_vp = rb->lcd_set_viewport(osd_get_viewport());
    osd_get_max_dims(&maxwidth, &maxheight);
    rb->lcd_getstringsize(osc_osd_message, &width, &height);
    rb->lcd_set_viewport(last_vp); /* to regular viewport */

    width += 2 + 2*OSC_OSD_MARGIN_SIZE;
    if (width > maxwidth)
        width = maxwidth;

    height += 2 + 2*OSC_OSD_MARGIN_SIZE;
    if (height > maxheight)
        height = maxheight;

    /* Center it on the screen */
    bool drawn = osd_update_pos((LCD_WIDTH - width) / 2,
                                (LCD_HEIGHT - height) / 2,
                                width, height);

    osd_show(OSD_SHOW | (drawn ? 0 : OSD_UPDATENOW));
}

/* Draw cursor bar for horizontal views */
static void anim_draw_cursor_h(int x)
{
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(CURSOR_COLOR);
    rb->lcd_vline(x, 0, LCD_HEIGHT-1);
    rb->lcd_set_foreground(GRAPH_COLOR);
#else
    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
    rb->lcd_vline(x, 0, LCD_HEIGHT-1);
    rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
}

/* Draw cursor bar for vertical views */
static void anim_draw_cursor_v(int y)
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


/** Peaks View **/

static long last_tick = 0;  /* time of last drawing */
static long last_delay = 0; /* last delay value used */
static int  last_pos = 0;   /* last x or y drawing position. Reset for aspect switch. */
static int  last_left;      /* last channel values */
static int  last_right;

static void get_peaks(int *left, int *right)
{
    static struct pcm_peaks peaks;
    rb->mixer_channel_calculate_peaks(PCM_MIXER_CHAN_PLAYBACK,
                                      &peaks);
    *left = peaks.left;
    *right = peaks.right;
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
        osd_lcd_update_prepare();
        anim_draw_cursor_h(0);
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

    osd_lcd_update_prepare();

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
        anim_draw_cursor_h(cur_x + 1); /* cursor bar */

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
        osd_lcd_update_prepare();
        anim_draw_cursor_v(0);
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

    osd_lcd_update_prepare();

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
        anim_draw_cursor_v(cur_y + 1); /* cursor bar */

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
static int16_t waveform_buffer[2*ALIGN_UP(PLAY_SAMPR_MAX, 2048)+2*2048]
    MEM_ALIGN_ATTR;
static size_t waveform_buffer_threshold = 0;
static size_t volatile waveform_buffer_have = 0;
static size_t waveform_buffer_break = 0;
static unsigned long mixer_sampr = PLAY_SAMPR_DEFAULT;
#define PCM_SAMPLESIZE (2*sizeof(int16_t))
#define PCM_BYTERATE(sampr) ((sampr)*PCM_SAMPLESIZE)

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
    size_t threshold = waveform_buffer_threshold;
    size_t have = 0;

    if (threshold != 0)
    {
        have = waveform_buffer_have;
        size_t slide = (have - 1) / threshold * threshold;

        if (slide == 0)
            slide = threshold;

        have -= slide;

        if (have > 0)
        {
            /* memcpy: the slide >= threshold and have <= threshold */
            memcpy(waveform_buffer, (void *)waveform_buffer + slide, have);
        }
    }

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
    if (vmin != vmax || vmin_prev != vmax_prev)
    {
        /* Graph compression */
        if (vmax > vmin_prev)
            vmax = vmin_prev;
        else if (vmin < vmax_prev)
            vmin = vmax_prev;

        rb->lcd_vline(x, vmax, vmin);
    }
    else
    {
        /* 1:1 or stretch */
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
        osd_lcd_update_prepare();
        rb->lcd_hline(0, LCD_WIDTH-1, 1*LCD_HEIGHT/4);
        rb->lcd_hline(0, LCD_WIDTH-1, 3*LCD_HEIGHT/4);
        osd_lcd_update();
        one_frame_paused = false;
        return cur_tick + HZ/5;
    }

    int count = (mixer_sampr*osc_delay + 100*HZ - 1) / (100*HZ);

    waveform_buffer_set_threshold(count*PCM_SAMPLESIZE);

    if (!waveform_buffer_have_enough())
        return cur_tick + HZ/100;

    one_frame_paused = false;

    osd_lcd_update_prepare();

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

    int lmin, lmin_prev = 0;
    int rmin, rmin_prev = 0;
    int lmax, lmax_prev = 0;
    int rmax, rmax_prev = 0;

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

    lmin = (1*LCD_HEIGHT/4) - WAVEFORM_SCALE_PCM(LCD_HEIGHT/4-1, lmin);
    lmax = (1*LCD_HEIGHT/4) - WAVEFORM_SCALE_PCM(LCD_HEIGHT/4-1, lmax);
    rmin = (3*LCD_HEIGHT/4) - WAVEFORM_SCALE_PCM(LCD_HEIGHT/4-1, rmin);
    rmax = (3*LCD_HEIGHT/4) - WAVEFORM_SCALE_PCM(LCD_HEIGHT/4-1, rmax);

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

        lmin = (1*LCD_HEIGHT/4) - WAVEFORM_SCALE_PCM(LCD_HEIGHT/4-1, lmin);
        lmax = (1*LCD_HEIGHT/4) - WAVEFORM_SCALE_PCM(LCD_HEIGHT/4-1, lmax);
        rmin = (3*LCD_HEIGHT/4) - WAVEFORM_SCALE_PCM(LCD_HEIGHT/4-1, rmin);
        rmax = (3*LCD_HEIGHT/4) - WAVEFORM_SCALE_PCM(LCD_HEIGHT/4-1, rmax);

        plot[osc.draw](x, x_prev, lmin, lmax, lmin_prev, lmax_prev);
        plot[osc.draw](x, x_prev, rmin, rmax, rmin_prev, rmax_prev);
    }

    waveform_buffer_done();

    osd_lcd_update();

    long delay = get_next_delay();
    return cur_tick + delay - waveform_buffer_have * HZ /
                PCM_BYTERATE(mixer_sampr);
}

static void anim_waveform_plot_filled_v(int y, int y_prev,
                                        int vmin, int vmax,
                                        int vmin_prev, int vmax_prev)
{
    if (vmin != vmax || vmin_prev != vmax_prev)
    {
        /* Graph compression */
        if (vmax < vmin_prev)
            vmax = vmin_prev;
        else if (vmin > vmax_prev)
            vmin = vmax_prev;

        rb->lcd_hline(vmin, vmax, y);
    }
    else
    {
        /* 1:1 or stretch */
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
        osd_lcd_update_prepare();
        rb->lcd_vline(1*LCD_WIDTH/4, 0, LCD_HEIGHT-1);
        rb->lcd_vline(3*LCD_WIDTH/4, 0, LCD_HEIGHT-1);
        osd_lcd_update();
        one_frame_paused = false;
        return cur_tick + HZ/5;
    }

    int count = (mixer_sampr*osc_delay + 100*HZ - 1) / (100*HZ);

    waveform_buffer_set_threshold(count*PCM_SAMPLESIZE);

    if (!waveform_buffer_have_enough())
        return cur_tick + HZ/100;

    one_frame_paused = false;

    osd_lcd_update_prepare();

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

    int lmin, lmin_prev = 0;
    int rmin, rmin_prev = 0;
    int lmax, lmax_prev = 0;
    int rmax, rmax_prev = 0;

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

    lmin = (1*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4-1, lmin);
    lmax = (1*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4-1, lmax);
    rmin = (3*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4-1, rmin);
    rmax = (3*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4-1, rmax);

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

        lmin = (1*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4-1, lmin);
        lmax = (1*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4-1, lmax);
        rmin = (3*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4-1, rmin);
        rmax = (3*LCD_WIDTH/4) + WAVEFORM_SCALE_PCM(LCD_WIDTH/4-1, rmax);

        plot[osc.draw](y, y_prev, lmin, lmax, lmin_prev, lmax_prev);
        plot[osc.draw](y, y_prev, rmin, rmax, rmin_prev, rmax_prev);
    }

    waveform_buffer_done();

    osd_lcd_update();

    long delay = get_next_delay();
    return cur_tick + delay - waveform_buffer_have * HZ
                / PCM_BYTERATE(mixer_sampr);
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
    static long (* const fns[MAX_GRAPH][MAX_OSC])(void) =
    {
        [GRAPH_PEAKS] =
        {
            [OSC_HORIZ] = anim_peaks_horizontal,
            [OSC_VERT] = anim_peaks_vertical,
        },
#ifdef OSCILLOSCOPE_GRAPHMODE
        [GRAPH_WAVEFORM] =
        {
            [OSC_HORIZ] = anim_waveform_horizontal,
            [OSC_VERT] = anim_waveform_vertical,
        },
#endif /* OSCILLOSCOPE_GRAPHMODE */
    };

    /* For peaks view */
    last_left = 0;
    last_right = 0;
    last_pos = 0;
    last_tick = *rb->current_tick;
    last_delay = 1;

    /* General */
    osd_lcd_update_prepare();
    rb->lcd_clear_display();
    osd_lcd_update();

    one_frame_paused = true;
    osd_set_timeout(paused ? 4*HZ : HZ);
    anim_draw = fns[osc.graphmode][osc.orientation];
}

static void graphmode_pause_unpause(bool paused)
{
    if (paused)
        return;

    last_tick = *rb->current_tick;
    osd_set_timeout(HZ);
    one_frame_paused = false;
#ifdef OSCILLOSCOPE_GRAPHMODE
    if (osc.graphmode == GRAPH_WAVEFORM)
        waveform_buffer_done();
#endif /* OSCILLOSCOPE_GRAPHMODE */
}

static void update_osc_delay(int value)
{
    osc_delay = osc_delay_tbl[value - 1];
    osc_delay_error = 0;
}

static void graphmode_setup(void)
{
#ifdef OSCILLOSCOPE_GRAPHMODE
    if (osc.graphmode == GRAPH_WAVEFORM)
    {
        rb->mixer_channel_set_buffer_hook(PCM_MIXER_CHAN_PLAYBACK,
                                          waveform_buffer_callback);
#ifdef HAVE_SCHEDULER_BOOSTCTRL
        rb->trigger_cpu_boost(); /* Just looks better */
#endif
    }
    else
    {
        rb->mixer_channel_set_buffer_hook(PCM_MIXER_CHAN_PLAYBACK,
                                          NULL);
#ifdef HAVE_SCHEDULER_BOOSTCTRL
        rb->cancel_cpu_boost();
#endif
        waveform_buffer_reset();
    }
#endif /* OSCILLOSCOPE_GRAPHMODE */

    update_osc_delay(osc.speed[osc.graphmode]);
    graphmode_reset();
}

static long oscilloscope_draw(void)
{
    if (message != -1)
    {
        osc_osd_show_message((enum osc_message_ids)message, msgval);
        message = -1;
    }
    else
    {
        osd_monitor_timeout();
    }

    long delay = HZ/5;

    if (!paused || one_frame_paused)
    {
        delay = anim_draw() - *rb->current_tick;

        if (delay > HZ/5)
            delay = HZ/5;
    }

    return delay;
}

static void osc_cleanup(void)
{
    osd_destroy();

#ifdef OSCILLOSCOPE_GRAPHMODE
    anim_waveform_exit();
#endif

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_DEFAULT_FG);
    rb->lcd_set_background(LCD_DEFAULT_BG);
#endif
#ifdef HAVE_BACKLIGHT
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();
#endif

    /* save settings if changed */
    if (rb->memcmp(&osc, &osc_disk, sizeof(osc)))
    {
        rb->memcpy(&osc_disk, &osc, sizeof(osc));
        configfile_save(cfg_filename, disk_config, ARRAYLEN(disk_config),
                        CFGFILE_VERSION);
    }
}

static void osc_setup(void)
{
    atexit(osc_cleanup);
    configfile_load(cfg_filename, disk_config, ARRAYLEN(disk_config),
                    CFGFILE_MINVERSION);
    osc = osc_disk; /* copy to running config */

    osc_osd_init();

#if LCD_DEPTH > 1
    osd_lcd_update_prepare();
    rb->lcd_set_foreground(GRAPH_COLOR);
    rb->lcd_set_background(BACKG_COLOR);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_clear_display();
    osd_lcd_update();
#endif

#ifdef OSCILLOSCOPE_GRAPHMODE
    mixer_sampr = rb->mixer_get_frequency();
#endif

#ifdef HAVE_BACKLIGHT
    /* Turn off backlight timeout */
    backlight_ignore_timeout();
#endif
    graphmode_setup();
}

enum plugin_status plugin_start(const void* parameter)
{
    bool exit = false;
#ifdef NEED_LASTBUTTON
    int lastbutton = BUTTON_NONE;
#endif

    osc_setup();

    while (!exit)
    {
        long delay = oscilloscope_draw();

        if (delay <= 0)
        {
            delay = 0;
            rb->yield(); /* tmo = 0 won't yield */
        }

        int button = rb->button_get_w_tmo(delay);

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
                int *val = &osc.speed[osc.graphmode];

                if (*val < MAX_SPEED)
                    ++*val;

                update_osc_delay(*val);
                osc_popupmsg(OSC_MSG_SPEED, *val);
                break;
            }

            case OSCILLOSCOPE_SPEED_DOWN:
            case OSCILLOSCOPE_SPEED_DOWN | BUTTON_REPEAT:
            {
                int *val = &osc.speed[osc.graphmode];

                if (*val > MIN_SPEED)
                    --*val;

                update_osc_delay(*val);
                osc_popupmsg(OSC_MSG_SPEED, *val);
                break;
            }

            case OSCILLOSCOPE_VOL_UP:
            case OSCILLOSCOPE_VOL_UP | BUTTON_REPEAT:
            {
                int vol = rb->global_settings->volume;

                if (vol < rb->sound_max(SOUND_VOLUME))
                {
                    vol++;
                    rb->sound_set(SOUND_VOLUME, vol);
                    rb->global_settings->volume = vol;
                }

                osc_popupmsg(OSC_MSG_VOLUME, vol);
                break;
            }

            case OSCILLOSCOPE_VOL_DOWN:
            case OSCILLOSCOPE_VOL_DOWN | BUTTON_REPEAT:
            {
                int vol = rb->global_settings->volume;

                if (vol > rb->sound_min(SOUND_VOLUME))
                {
                    vol--;
                    rb->sound_set(SOUND_VOLUME, vol);
                    rb->global_settings->volume = vol;
                }

                osc_popupmsg(OSC_MSG_VOLUME, vol);
                break;
            }

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
