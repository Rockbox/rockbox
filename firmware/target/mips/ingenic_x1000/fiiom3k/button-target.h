/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __BUTTON_TARGET_H__
#define __BUTTON_TARGET_H__

#include <stdbool.h>

#define BUTTON_POWER        0x00000001
#define BUTTON_PLAY         0x00000002
#define BUTTON_VOL_UP       0x00000004
#define BUTTON_VOL_DOWN     0x00000008
#define BUTTON_UP           0x00000010
#define BUTTON_DOWN         0x00000020
#define BUTTON_LEFT         0x00000040
#define BUTTON_RIGHT        0x00000080
#define BUTTON_SELECT       0x00000100
#define BUTTON_BACK         0x00000200
#define BUTTON_MENU         0x00000400
#define BUTTON_SCROLL_FWD   0x00000800
#define BUTTON_SCROLL_BACK  0x00001000

#define BUTTON_MAIN (BUTTON_POWER|BUTTON_VOL_UP|BUTTON_VOL_DOWN|\
                     BUTTON_PLAY|BUTTON_TOUCHPAD)

#define BUTTON_TOUCHPAD (BUTTON_UP|BUTTON_DOWN|BUTTON_LEFT|BUTTON_RIGHT|\
                         BUTTON_SELECT|BUTTON_BACK|BUTTON_MENU|\
                         BUTTON_SCROLL_FWD|BUTTON_SCROLL_BACK)

#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT  30

extern void touchpad_set_sensitivity(int level);
extern void touchpad_enable_device(bool en);

#endif /* __BUTTON_TARGET_H__ */
