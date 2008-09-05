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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef BUTTON_TARGET_H
#define BUTTON_TARGET_H

#include <stdbool.h>
#include "config.h"

#define HAS_BUTTON_HOLD

bool button_hold(void);
void button_init_device(void);
int button_read_device(void);

/* Main unit's buttons */
#define BUTTON_POWER      0x00000001
#define BUTTON_VOL_UP     0x00000002
#define BUTTON_VOL_DOWN   0x00000004
#define BUTTON_MENU       0x00000008

/* Compatibility hacks for flipping. Needs a somewhat better fix. */
#define BUTTON_LEFT  0
#define BUTTON_RIGHT 0
#define BUTTON_UP    0
#define BUTTON_DOWN  0

#define BUTTON_MAIN  (BUTTON_POWER | BUTTON_VOL_UP | BUTTON_VOL_DOWN | BUTTON_MENU)

/* No remote */
#define BUTTON_REMOTE 0

/* Software power-off */
#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT 10

#endif /* BUTTON_TARGET_H */
