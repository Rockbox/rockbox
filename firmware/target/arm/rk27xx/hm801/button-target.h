/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Andrew Ryabinin
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

#define BUTTON_UP          0x00000001
#define BUTTON_POWER       0x00000002
#define BUTTON_DOWN        0x00000004
#define BUTTON_LEFT        0x00000008
#define BUTTON_RIGHT       0x00000010
#define BUTTON_SELECT      0x00000020
#define BUTTON_NEXT        0x00000040
#define BUTTON_PREV        0x00000080
#define BUTTON_PLAY        0x00000100

#define BUTTON_MAIN        (BUTTON_UP|BUTTON_POWER| \
                            BUTTON_DOWN|BUTTON_LEFT| \
                            BUTTON_RIGHT|BUTTON_SELECT| \
                            BUTTON_NEXT|BUTTON_PREV| \
                            BUTTON_PLAY)

#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT 30

#endif /* _BUTTON_TARGET_H_ */
