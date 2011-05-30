/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Marcin Bukat
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

void button_init_device(void);
int button_read_device(void);

/* Main unit's buttons */
/* values assigned corespond to GPIOs numbers */
#define BUTTON_PLAY         0x00000002

#define BUTTON_REW          0x00000008
#define BUTTON_FF           0x00000004
#define BUTTON_VOL          0x00000040
#define BUTTON_M            0x00000010

#define BUTTON_LEFT BUTTON_REW
#define BUTTON_RIGHT BUTTON_FF
#define BUTTON_ON BUTTON_PLAY

#define BUTTON_REMOTE       0

#define BUTTON_MAIN (BUTTON_PLAY|BUTTON_REW|BUTTON_FF|\
                     BUTTON_VOL|BUTTON_M)

#define POWEROFF_BUTTON BUTTON_PLAY
#define POWEROFF_COUNT 30

#endif /* _BUTTON_TARGET_H_ */
