/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Jerome Kuptz
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _WPS_H
#define _WPS_H
#include "id3.h"
#include "playlist.h" 

/* button definitions */
#if CONFIG_KEYPAD == IRIVER_H100_PAD
#define WPS_NEXT       (BUTTON_RIGHT | BUTTON_REL)
#define WPS_NEXT_PRE   BUTTON_RIGHT
#define WPS_PREV       (BUTTON_LEFT | BUTTON_REL)
#define WPS_PREV_PRE   BUTTON_LEFT
#define WPS_FFWD       (BUTTON_RIGHT | BUTTON_REPEAT)
#define WPS_REW        (BUTTON_LEFT | BUTTON_REPEAT)
#define WPS_INCVOL     BUTTON_UP
#define WPS_DECVOL     BUTTON_DOWN
#define WPS_PAUSE      BUTTON_ON
#define WPS_MENU       (BUTTON_MODE | BUTTON_REL)
#define WPS_MENU_PRE   BUTTON_MODE
#define WPS_BROWSE     (BUTTON_SELECT | BUTTON_REL)
#define WPS_BROWSE_PRE BUTTON_SELECT
#define WPS_EXIT       BUTTON_OFF
#define WPS_KEYLOCK    (BUTTON_MODE | BUTTON_DOWN)
#define WPS_ID3        (BUTTON_MODE | BUTTON_ON)

#define WPS_RC_NEXT    BUTTON_RC_FF
#define WPS_RC_PREV    BUTTON_RC_REW
#define WPS_RC_PAUSE   BUTTON_RC_ON
#define WPS_RC_INCVOL  BUTTON_RC_VOL_UP
#define WPS_RC_DECVOL  BUTTON_RC_VOL_DOWN
#define WPS_RC_EXIT    BUTTON_RC_STOP
#define WPS_RC_MENU    (BUTTON_RC_MENU | BUTTON_REL)
#define WPS_RC_MENU_PRE    BUTTON_RC_MENU
#define WPS_RC_BROWSE     (BUTTON_RC_VOL | BUTTON_REL)
#define WPS_RC_BROWSE_PRE BUTTON_RC_VOL

#elif CONFIG_KEYPAD == RECORDER_PAD
#define WPS_NEXT       (BUTTON_RIGHT | BUTTON_REL)
#define WPS_NEXT_PRE   BUTTON_RIGHT
#define WPS_PREV       (BUTTON_LEFT | BUTTON_REL)
#define WPS_PREV_PRE   BUTTON_LEFT
#define WPS_FFWD       (BUTTON_RIGHT | BUTTON_REPEAT)
#define WPS_REW        (BUTTON_LEFT | BUTTON_REPEAT)
#define WPS_INCVOL     BUTTON_UP
#define WPS_DECVOL     BUTTON_DOWN
#define WPS_PAUSE      BUTTON_PLAY
#define WPS_MENU       (BUTTON_F1 | BUTTON_REL)
#define WPS_MENU_PRE   BUTTON_F1
#define WPS_BROWSE     (BUTTON_ON | BUTTON_REL)
#define WPS_BROWSE_PRE BUTTON_ON
#define WPS_EXIT       BUTTON_OFF
#define WPS_KEYLOCK    (BUTTON_F1 | BUTTON_DOWN)
#define WPS_ID3        (BUTTON_F1 | BUTTON_ON)

#define WPS_RC_NEXT    BUTTON_RC_RIGHT
#define WPS_RC_PREV    BUTTON_RC_LEFT
#define WPS_RC_PAUSE   BUTTON_RC_PLAY
#define WPS_RC_INCVOL  BUTTON_RC_VOL_UP
#define WPS_RC_DECVOL  BUTTON_RC_VOL_DOWN
#define WPS_RC_EXIT    BUTTON_RC_STOP

#elif CONFIG_KEYPAD == PLAYER_PAD
#define WPS_NEXT       (BUTTON_RIGHT | BUTTON_REL)
#define WPS_NEXT_PRE   BUTTON_RIGHT
#define WPS_PREV       (BUTTON_LEFT | BUTTON_REL)
#define WPS_PREV_PRE   BUTTON_LEFT
#define WPS_FFWD       (BUTTON_RIGHT | BUTTON_REPEAT)
#define WPS_REW        (BUTTON_LEFT | BUTTON_REPEAT)
#define WPS_INCVOL     (BUTTON_MENU | BUTTON_RIGHT)
#define WPS_DECVOL     (BUTTON_MENU | BUTTON_LEFT)
#define WPS_PAUSE      BUTTON_PLAY
#define WPS_MENU       (BUTTON_MENU | BUTTON_REL)
#define WPS_MENU_PRE   BUTTON_MENU
#define WPS_BROWSE     (BUTTON_ON | BUTTON_REL)
#define WPS_BROWSE_PRE BUTTON_ON
#define WPS_EXIT       BUTTON_STOP
#define WPS_KEYLOCK    (BUTTON_MENU | BUTTON_STOP)
#define WPS_ID3        (BUTTON_MENU | BUTTON_ON)

#define WPS_RC_NEXT    BUTTON_RC_RIGHT
#define WPS_RC_PREV    BUTTON_RC_LEFT
#define WPS_RC_PAUSE   BUTTON_RC_PLAY
#define WPS_RC_INCVOL  BUTTON_RC_VOL_UP
#define WPS_RC_DECVOL  BUTTON_RC_VOL_DOWN
#define WPS_RC_EXIT    BUTTON_RC_STOP

#elif CONFIG_KEYPAD == ONDIO_PAD
#define WPS_NEXT       (BUTTON_RIGHT | BUTTON_REL)
#define WPS_NEXT_PRE   BUTTON_RIGHT
#define WPS_PREV       (BUTTON_LEFT | BUTTON_REL)
#define WPS_PREV_PRE   BUTTON_LEFT
#define WPS_FFWD       (BUTTON_RIGHT | BUTTON_REPEAT)
#define WPS_REW        (BUTTON_LEFT | BUTTON_REPEAT)
#define WPS_INCVOL     BUTTON_UP
#define WPS_DECVOL     BUTTON_DOWN
#define WPS_PAUSE      BUTTON_OFF
#define WPS_MENU       (BUTTON_MENU | BUTTON_REPEAT)
#define WPS_BROWSE     (BUTTON_MENU | BUTTON_REL)
#define WPS_BROWSE_PRE BUTTON_MENU
#define WPS_KEYLOCK    (BUTTON_MENU | BUTTON_DOWN)
#define WPS_EXIT       (BUTTON_OFF | BUTTON_REPEAT)

#elif CONFIG_KEYPAD == GMINI100_PAD
#define WPS_NEXT       (BUTTON_RIGHT | BUTTON_REL)
#define WPS_NEXT_PRE   BUTTON_RIGHT
#define WPS_PREV       (BUTTON_LEFT | BUTTON_REL)
#define WPS_PREV_PRE   BUTTON_LEFT
#define WPS_FFWD       (BUTTON_RIGHT | BUTTON_REPEAT)
#define WPS_REW        (BUTTON_LEFT | BUTTON_REPEAT)
#define WPS_INCVOL     BUTTON_UP
#define WPS_DECVOL     BUTTON_DOWN
#define WPS_PAUSE      BUTTON_PLAY
#define WPS_MENU       (BUTTON_MENU | BUTTON_REL)
#define WPS_MENU_PRE   BUTTON_MENU
#define WPS_BROWSE     (BUTTON_ON | BUTTON_REL)
#define WPS_BROWSE_PRE BUTTON_ON
#define WPS_EXIT       BUTTON_OFF
#define WPS_KEYLOCK    (BUTTON_MENU | BUTTON_DOWN)
#define WPS_ID3        (BUTTON_MENU | BUTTON_ON)

#endif

extern bool keys_locked;
extern bool wps_time_countup;

long wps_show(void);
bool refresh_wps(bool refresh_scroll);
void handle_usb(void);

#if CONFIG_KEYPAD == RECORDER_PAD
bool f2_screen(void);
bool f3_screen(void);
#endif

#endif

