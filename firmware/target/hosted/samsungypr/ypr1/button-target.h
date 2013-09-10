/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 by Lorenzo Miori
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

void button_close_device(void);

/* Logical buttons key codes */
#define BUTTON_VOL_UP           0x00000001
#define BUTTON_VOL_DOWN         0x00000002
#define BUTTON_POWER            0x00000004
#define BUTTON_RIGHT            0x00000008
#define BUTTON_LEFT             0x00000012

/* Touch Screen Area Buttons */
#define BUTTON_TOPLEFT          0x00000010
#define BUTTON_TOPMIDDLE        0x00000020
#define BUTTON_TOPRIGHT         0x00000040
#define BUTTON_MIDLEFT          0x00000080
#define BUTTON_CENTER           0x00000100
#define BUTTON_MIDRIGHT         0x00000200
#define BUTTON_BOTTOMLEFT       0x00000400
#define BUTTON_BOTTOMMIDDLE     0x00000800
#define BUTTON_BOTTOMRIGHT      0x00001000

/* All the buttons */
#define BUTTON_MAIN         (BUTTON_VOL_UP | BUTTON_VOL_DOWN | BUTTON_POWER | \
                            BUTTON_RIGHT|BUTTON_LEFT| BUTTON_TOPLEFT | BUTTON_TOPMIDDLE    | \
                            BUTTON_TOPRIGHT | BUTTON_MIDLEFT    | BUTTON_CENTER       | \
                            BUTTON_MIDRIGHT | BUTTON_BOTTOMLEFT | BUTTON_BOTTOMMIDDLE | \
                            BUTTON_BOTTOMRIGHT) /* all buttons */

/* Default touchscreen calibration
 * TODO this is not 100% accurate. X-AXIS must be slightly moved to the right
 */
#define DEFAULT_TOUCHSCREEN_CALIBRATION {.A=0x0000AD7C, .B=0xFFFFFC70, \
                                         .C=0xFF6632E0,  .D=0xFFFFFF1A, .E=0xFFFF5D08, \
                                         .F=0xFFFC4230, .divider=0xFFFF54E0}

/* Software power-off */
#define POWEROFF_BUTTON         BUTTON_POWER
/* About 3 seconds */
#define POWEROFF_COUNT          10

#endif /* _BUTTON_TARGET_H_ */
