/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *
 * Copyright (C) 2009 by Jorge Pinto
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

#define BUTTON_SELECT       0x00000001
#define BUTTON_MENU         0x00000002
#define BUTTON_PLAY         0x00000004
#define BUTTON_STOP         0x00000008

#define BUTTON_LEFT         0x00000010
#define BUTTON_RIGHT        0x00000020
#define BUTTON_UP           0x00000040
#define BUTTON_DOWN         0x00000080

#define BUTTON_MAIN         (BUTTON_UP|BUTTON_DOWN|BUTTON_RIGHT|BUTTON_LEFT \
                            |BUTTON_SELECT|BUTTON_MENU|BUTTON_PLAY \
                            |BUTTON_STOP)

#endif /* _BUTTON_TARGET_H_ */
