/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Dave Chapman
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

/* 

Results of button testing:

HOLD:  GPIOA & 0x0002 (0=pressed, 0x0002 = released)
POWER: GPIOA & 0x8000 (0=pressed, 0x8000 = released)

ADC[0]: (approx values)

RIGHT - 0x37
LEFT  - 0x7f
JOYSTICK PRESS - 0xc7
UP             - 0x11e
DOWN           - 0x184
MODE           - 0x1f0/0x1ff
PRESET         - 0x268/0x269
TIMESHIFT      - 0x2dd

Values of ADC[0] tested in OF disassembly: 0x50, 0x96, 0xdc, 0x208, 0x384

*/


void button_init_device(void);
int button_read_device(void);

/* Main unit's buttons */
#define BUTTON_POWERPLAY    0x00000001
#define BUTTON_MODE         0x00000002
#define BUTTON_HOLD         0x00000004
#define BUTTON_REC          0x00000008
#define BUTTON_PRESET       0x00000010
#define BUTTON_LEFT         0x00000020
#define BUTTON_RIGHT        0x00000040
#define BUTTON_UP           0x00000080
#define BUTTON_DOWN         0x00000100
#define BUTTON_SELECT       0x00000200

#define BUTTON_MAIN (BUTTON_POWERPLAY|BUTTON_MODE|BUTTON_HOLD\
                    |BUTTON_REC|BUTTON_PRESET|BUTTON_LEFT\
                    |BUTTON_RIGHT|BUTTON_UP|BUTTON_DOWN|BUTTON_SELECT)

#define BUTTON_REMOTE 0
                
#endif /* _BUTTON_TARGET_H_ */
