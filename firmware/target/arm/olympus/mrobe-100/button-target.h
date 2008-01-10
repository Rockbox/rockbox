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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _BUTTON_TARGET_H_
#define _BUTTON_TARGET_H_

#include <stdbool.h>
#include "config.h"

#define HAS_BUTTON_HOLD

bool button_hold(void);
void button_init_device(void);
int button_read_device(void);

#define POWEROFF_BUTTON     BUTTON_POWER
#define POWEROFF_COUNT      10

/* FIXME: Until the buttons are figured out, we use the button definitions
   for the H10 keypad & remote. THESE ARE NOT CORRECT! */

/* Main unit's buttons */
#define BUTTON_POWER        0x00000001
#define BUTTON_LEFT         0x00000002
#define BUTTON_RIGHT        0x00000004
#define BUTTON_REW          0x00000008
#define BUTTON_PLAY         0x00000010
#define BUTTON_FF           0x00000020
#define BUTTON_SCROLL_UP    0x00000040
#define BUTTON_SCROLL_DOWN  0x00000080
#define BUTTON_MAIN         (BUTTON_POWER|BUTTON_O|BUTTON_BACK|BUTTON_REW\
                            |BUTTON_PLAY|BUTTON_FF)

/* Remote control's buttons */
#define BUTTON_RC_REW       0x00080000
#define BUTTON_RC_PLAY      0x00100000
#define BUTTON_RC_FF        0x00200000
#define BUTTON_RC_VOL_UP    0x00400000
#define BUTTON_RC_VOL_DOWN  0x00800000
#define BUTTON_REMOTE (BUTTON_RC_PLAY|BUTTON_RC_VOL_UP|BUTTON_RC_VOL_DOWN\
                |BUTTON_RC_REW|BUTTON_RC_FF)
#define RC_POWEROFF_BUTTON  BUTTON_RC_PLAY

#endif /* _BUTTON_TARGET_H_ */
