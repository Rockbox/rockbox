/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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

#define MEP_BUTTON_HEADER   0x19
#define MEP_BUTTON_ID       0x9
#define MEP_ABSOLUTE_HEADER 0x0b
#define MEP_FINGER          0x01
#define MEP_GESTURE         0x02

#define HAS_BUTTON_HOLD

#ifndef BOOTLOADER
void button_int(void);
#endif

/* Main unit's buttons */
#define BUTTON_POWER        0x00000001
#define BUTTON_MENU         0x00000002
#define BUTTON_VIEW         0x00000004
#define BUTTON_VOL_UP       0x00000008
#define BUTTON_VOL_DOWN     0x00000010
#define BUTTON_LEFT         0x00000020
#define BUTTON_RIGHT        0x00000040
#define BUTTON_UP           0x00000080
#define BUTTON_DOWN         0x00000100
#define BUTTON_NEXT         0x00000200
#define BUTTON_PREV         0x00000400
#define BUTTON_PLAY         0x00000800
#define BUTTON_MAIN         0x00000fff

#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT 10

/*
 * scrollstrip defines
 */

/* The maximum value that is returned by button_read_device(). Note that the
   hardware maximum is scaled up to minimize rounding errors in strip driver. */
#define SCROLLSTRIP_MAX_VAL     (4096<<4)
/* areas for button up/down-tap-detection, with an unused zone between them */
#define STRIP_POS_IS_BTN_UP     (SCROLLSTRIP_MAX_VAL*45/100)
#define STRIP_POS_IS_BTN_DOWN   (SCROLLSTRIP_MAX_VAL*55/100)

/* a single tap on the scrollstrip shorter than this
   will produce a single button press event                                 */
#define TAP_EVENT_TIME          (40*HZ/100)

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


#endif /* _BUTTON_TARGET_H_ */
