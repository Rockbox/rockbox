/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Marcin Bukat
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

#define HAS_BUTTON_HOLD
#define HAS_REMOTE_BUTTON_HOLD

/* HD200 specific button codes */
/* Main unit's buttons  - flags as in original firmware*/
#define BUTTON_PLAY         0x00000001

#define BUTTON_REW          0x00000004
#define BUTTON_FF           0x00000002
#define BUTTON_VOL_UP       0x00000008
#define BUTTON_VOL_DOWN     0x00000010
#define BUTTON_REC          0x00000020
#define BUTTON_FUNC         0x00002000

#define BUTTON_RC_PLAY      0x00010000

#define BUTTON_RC_REW          0x00040000
#define BUTTON_RC_FF           0x00020000
#define BUTTON_RC_VOL_UP       0x00080000
#define BUTTON_RC_VOL_DOWN     0x00100000
#define BUTTON_RC_FUNC         0x20000000

#define BUTTON_LEFT BUTTON_REW
#define BUTTON_RIGHT BUTTON_FF
#define BUTTON_ON BUTTON_PLAY

#define BUTTON_MAIN (BUTTON_PLAY|BUTTON_REW|BUTTON_FF|BUTTON_VOL_UP|\
        BUTTON_VOL_DOWN|BUTTON_REC|BUTTON_FUNC)

#define BUTTON_REMOTE (BUTTON_RC_PLAY|BUTTON_RC_REW|BUTTON_RC_FF|\
        BUTTON_RC_VOL_UP|BUTTON_RC_VOL_DOWN|BUTTON_RC_FUNC)

#define POWEROFF_BUTTON BUTTON_PLAY
#define RC_POWEROFF_BUTTON BUTTON_RC_PLAY
#define POWEROFF_COUNT 30

#endif /* _BUTTON_TARGET_H_ */
