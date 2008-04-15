/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#ifndef _BUTTON_TARGET_H_
#define _BUTTON_TARGET_H_

#include "config.h"

#define HAS_BUTTON_HOLD

bool button_hold(void);
void button_init_device(void);
int button_read_device(int *data);

struct touch_calibration_point {
    short px_x; /* known pixel value */
    short px_y;
    short val_x; /* touchpad value at the known pixel */
    short val_y;
};
void use_calibration(bool enable);

/* M:Robe 500 specific button codes */

#define BUTTON_POWER        0x00000100

/* Remote control buttons */

#define BUTTON_RC_PLAY      0x00000001
#define BUTTON_RC_DOWN      0x00000002
#define BUTTON_RC_FF        0x00000004
#define BUTTON_RC_REW       0x00000008
#define BUTTON_RC_VOL_UP    0x00000010
#define BUTTON_RC_VOL_DOWN  0x00000020
#define BUTTON_RC_MODE      0x00000040
#define BUTTON_RC_HEART     0x00000080

#define BUTTON_TOUCH        0x00000200

/* Touchpad Screen Area Buttons */
#define BUTTON_TOPLEFT      0x00004000
#define BUTTON_TOPMIDDLE    0x00008000
#define BUTTON_TOPRIGHT     0x00010000
#define BUTTON_MIDLEFT      0x00020000
#define BUTTON_CENTER       0x00040000
#define BUTTON_MIDRIGHT     0x00080000
#define BUTTON_BOTTOMLEFT   0x00100000
#define BUTTON_BOTTOMMIDDLE 0x00200000
#define BUTTON_BOTTOMRIGHT  0x00400000

/* compatibility hacks
   not mapped to the touchpad button areas because
   the touchpad is not always in that mode */
#define BUTTON_LEFT     BUTTON_RC_REW
#define BUTTON_RIGHT    BUTTON_RC_FF

#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT  10

#define BUTTON_MAIN (BUTTON_POWER| \
                     BUTTON_TOPLEFT|BUTTON_TOPMIDDLE|BUTTON_TOPRIGHT \
                     BUTTON_MIDLEFT|BUTTON_CENTER|BUTTON_MIDRIGHT \
                     BUTTON_BOTTOMLEFT|BUTTON_BOTTOMMIDDLE|BUTTON_BOTTOMRIGHT)

#define BUTTON_REMOTE (BUTTON_RC_HEART|BUTTON_RC_MODE|      \
                       BUTTON_RC_VOL_DOWN|BUTTON_RC_VOL_UP| \
                       BUTTON_RC_PLAY|BUTTON_RC_DOWN|       \
                       BUTTON_RC_REW|BUTTON_RC_FF)

#endif /* _BUTTON_TARGET_H_ */
