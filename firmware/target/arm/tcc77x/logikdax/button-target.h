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

/* Software power-off */
#define POWEROFF_BUTTON BUTTON_POWERPLAY
#define POWEROFF_COUNT 40
                
#endif /* _BUTTON_TARGET_H_ */
