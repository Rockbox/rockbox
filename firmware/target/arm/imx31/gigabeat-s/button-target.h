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
#include "config.h"

#define HAS_BUTTON_HOLD

#ifdef BOOTLOADER
#define BUTTON_DRIVER_CLOSE
#endif

bool button_hold(void);
void button_init_device(void);
void button_close_device(void);
int button_read_device(void);
void button_power_event(void);
void headphone_detect_event(void);
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

#define BUTTON_MAIN         (0x1fff)

#define BUTTON_REMOTE       0

#define POWEROFF_BUTTON     BUTTON_POWER
#define POWEROFF_COUNT      10

#endif /* _BUTTON_TARGET_H_ */
