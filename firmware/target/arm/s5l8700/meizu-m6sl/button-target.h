/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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

/* Toshiba Gigabeat specific button codes */

#define BUTTON_LEFT         0x00000001
#define BUTTON_RIGHT        0x00000002
#define BUTTON_UP           0x00000004
#define BUTTON_DOWN         0x00000008

#define BUTTON_SELECT       0x00000010

#define BUTTON_MENU         0x00000020
#define BUTTON_PLAY         0x00000040


#define BUTTON_MAIN (BUTTON_MENU|BUTTON_LEFT|BUTTON_RIGHT\
                |BUTTON_UP|BUTTON_DOWN|BUTTON_SELECT|BUTTON_PLAY)

#define BUTTON_REMOTE 0

#define POWEROFF_BUTTON BUTTON_PLAY
#define POWEROFF_COUNT 10

#endif /* _BUTTON_TARGET_H_ */
