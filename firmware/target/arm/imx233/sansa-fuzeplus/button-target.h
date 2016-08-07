/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#ifndef _BUTTON_TARGET_H_
#define _BUTTON_TARGET_H_

#include <stdbool.h>
bool button_debug_screen(void);
void touchpad_set_sensitivity(int level);
void touchpad_set_deadzone(int touchpad_deadzone);
void touchpad_enable_device(bool en);

/* Main unit's buttons */
#define BUTTON_POWER                0x00000001
#define BUTTON_VOL_UP               0x00000002
#define BUTTON_VOL_DOWN             0x00000004
/* Virtual buttons */
#define BUTTON_LEFT                 0x00000008
#define BUTTON_UP                   0x00000010
#define BUTTON_RIGHT                0x00000020
#define BUTTON_DOWN                 0x00000040
#define BUTTON_SELECT               0x00000080
#define BUTTON_PLAYPAUSE            0x00000100
#define BUTTON_BACK                 0x00000200
#define BUTTON_BOTTOMLEFT           0x00000400
#define BUTTON_BOTTOMRIGHT          0x00000800


#define BUTTON_MAIN (BUTTON_VOL_UP|BUTTON_VOL_DOWN|BUTTON_POWER|BUTTON_LEFT| \
                     BUTTON_UP|BUTTON_RIGHT|BUTTON_DOWN|BUTTON_SELECT| \
                     BUTTON_PLAYPAUSE|BUTTON_BACK| \
                     BUTTON_BOTTOMRIGHT|BUTTON_BOTTOMLEFT)

#define BUTTON_TOUCHPAD (BUTTON_LEFT|BUTTON_UP|BUTTON_RIGHT|BUTTON_DOWN| \
                         BUTTON_SELECT|BUTTON_PLAYPAUSE|BUTTON_BACK| \
                         BUTTON_BOTTOMRIGHT|BUTTON_BOTTOMLEFT)

/* Software power-off */
#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT 10

#ifdef HAVE_SCROLLSTRIP
/*
 * scrollstrip defines
 */

/* The maximum value that is returned by button_read_device(). Note that the
   hardware maximum is scaled up to minimize rounding errors in strip driver. */
#define SCROLLSTRIP_MAX_VAL     (1975<<5)
/* areas for button up/down-tap-detection, with an unused zone between them */
/* Note: for Fuze+ POC the "unused" area is the select button. TODO:
   make a proper define for the center area */
#define STRIP_POS_IS_BTN_UP     (SCROLLSTRIP_MAX_VAL*34/100)
#define STRIP_POS_IS_BTN_DOWN   (SCROLLSTRIP_MAX_VAL*66/100)

/* a single tap on the scrollstrip shorter than this
   will produce a single button press event                                 */
#define TAP_EVENT_TIME          (40*HZ/100)
/* Time constant for long-button-press variant 1:
   after double-clicking button code is sent */
#define BUTTON_DOUBLECLICK_TIME (40*HZ/100)
/* Time constant for long-button-press variant 2:
   button code is sent after touching for a long enough time */
#define BUTTON_LONG_PRESS_TIME  (100*HZ/100)

/*
 * These parameters control the strip's feel and responsiveness and must be
 * chosen with regard to the target's strip resolution, size and sensibility.
 *
 * Following you find the standard settings; they will be modified according
 * to the speed/afterscroll setting in the menu.
 */
/* maximum distance the finger has to slide for next button event           */
#define BUTTON_DELTA_MAX        (262<<4) // 350 *0.75
/* minimum distance (when sliding fast)                                     */
#define BUTTON_DELTA_MIN        (157<<4) // 210*0.75
/* After generating a button event the delta is reduced.
   The higher this value, the faster the speedup.                           */
#define STRIP_ACCELERATION      (980000/HZ)
/* deceleration (when sliding slow or staying on the same spot) becomes
   stronger and has an earlier influence with higher values.                */
#define STRIP_DECELERATION      (47000/HZ)

/* defines the minimum scrolling speed to achieve afterscroll, and also
   the slowest scrolling speed during afterscrolling                        */
#define AFTERSCROLL_MAX_TICKS   (10*HZ/100)
/* the fastest possible afterscrolling                                      */
#define AFTERSCROLL_MIN_TICKS   (5*HZ/100)
/* decay factor of the afterscroll                                          */
#define AFTERSCROLL_DECAY       (742400/HZ)

/*
 * The following parameters control how the standard parameters change with
 * different speed/afterscroll settings in the menu. They operate as fixed
 * point 24.8 multipliers.
 */
#define SETTING_SPEED_MAX_BUT_DELTA     25
#define SETTING_SPEED_MIN_BUT_DELTA     45
#define SETTING_AFTER_MIN_TICKS         30
#define SETTING_AFTER_DECAY             57
#endif

#endif /* _BUTTON_TARGET_H_ */
