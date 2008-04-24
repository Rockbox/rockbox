/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2008 by Maurus Cuelenaere
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

#define BUTTON_BACK         (1 << 0)
#define BUTTON_CUSTOM       (1 << 1)
#define BUTTON_MENU         (1 << 2)

#define BUTTON_LEFT         (1 << 3)
#define BUTTON_RIGHT        (1 << 4)
#define BUTTON_UP           (1 << 5)
#define BUTTON_DOWN         (1 << 6)
#define BUTTON_SELECT       (1 << 7)

#define BUTTON_POWER        (1 << 8)
#define BUTTON_PLAY         (1 << 9)

#define BUTTON_HOLD         (1 << 10)

#define BUTTON_REMOTE       0

#define BUTTON_MAIN         ( BUTTON_BACK | BUTTON_MENU | BUTTON_LEFT   | BUTTON_RIGHT \
                            | BUTTON_UP   | BUTTON_DOWN | BUTTON_SELECT | BUTTON_POWER \
                            | BUTTON_PLAY | BUTTON_HOLD | BUTTON_CUSTOM                )

#define POWEROFF_BUTTON     BUTTON_POWER
#define POWEROFF_COUNT      10

void button_init_device(void);
int button_read_device(void);
bool headphones_inserted(void);
bool button_hold(void);
bool button_usb_connected(void);

int get_debug_info(int choice);

#endif /* _BUTTON_TARGET_H_ */
