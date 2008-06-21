/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: button-target.h 15396 2007-11-01 23:38:57Z dave $
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

#define HAS_BUTTON_HOLD

void button_init_device(void);
int button_read_device(void);
bool button_hold(void);

/* Main unit's buttons */
#define BUTTON_MENU         0x00000001
#define BUTTON_VOLUP        0x00000002
#define BUTTON_VOLDOWN      0x00000004
#define BUTTON_PLAYPAUSE    0x00000008
#define BUTTON_REPEATAB     0x00000010
#define BUTTON_LEFT         0x00000020
#define BUTTON_RIGHT        0x00000040
#define BUTTON_SELECT       0x00000080

#define BUTTON_MAIN (BUTTON_MENU|BUTTON_VOLUP|BUTTON_VOLDOWN\
                    |BUTTON_PLAYPAUSE|BUTTON_REPEATAB|BUTTON_LEFT\
                    |BUTTON_RIGHT|BUTTON_SELECT)

#define BUTTON_REMOTE 0

/* Software power-off */
#define POWEROFF_BUTTON BUTTON_MENU
#define POWEROFF_COUNT 40
                
#endif /* _BUTTON_TARGET_H_ */
