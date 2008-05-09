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
void button_power_set_state(bool pressed);
void set_headphones_inserted(bool inserted);
bool headphones_inserted(void);

/* Toshiba Gigabeat S-specific button codes */

/* These shifts are selected to optimize scanning of the keypad port */
#define BUTTON_LEFT         (1 << 0)
#define BUTTON_UP           (1 << 1)
#define BUTTON_DOWN         (1 << 2)
#define BUTTON_RIGHT        (1 << 3)
#define BUTTON_SELECT       (1 << 4)
#define BUTTON_BACK         (1 << 5)
#define BUTTON_MENU         (1 << 6)
#define BUTTON_VOL_UP       (1 << 7)
#define BUTTON_VOL_DOWN     (1 << 8)
#define BUTTON_PREV         (1 << 9)
#define BUTTON_PLAY         (1 << 10)
#define BUTTON_NEXT         (1 << 11)
#define BUTTON_POWER        (1 << 12) /* Read from PMIC */

#define BUTTON_REMOTE       0

#define POWEROFF_BUTTON     BUTTON_POWER
#define POWEROFF_COUNT      10

#endif /* _BUTTON_TARGET_H_ */
