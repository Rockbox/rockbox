/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Jonathan Gordon
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

/* Custom written for the Hxxx */

#ifndef _BUTTON_TARGET_H_
#define _BUTTON_TARGET_H_

#include <stdbool.h>
#include "config.h"

#define HAS_BUTTON_HOLD
#define HAS_REMOTE_BUTTON_HOLD

bool button_hold(void);
bool remote_button_hold(void);
bool remote_button_hold_only(void);
void button_init_device(void);
int button_read_device(void);
#ifdef IRIVER_H300_SERIES
void button_enable_scan(bool enable);
bool button_scan_enabled(void);
#endif

/* iRiver H100/H300 specific button codes */

        /* Main unit's buttons */
#define BUTTON_ON           0x00000001
#define BUTTON_OFF          0x00000002

#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_UP           0x00000010
#define BUTTON_DOWN         0x00000020

#define BUTTON_REC          0x00000040
#define BUTTON_MODE         0x00000080

#define BUTTON_SELECT       0x00000100

#define BUTTON_MAIN (BUTTON_ON|BUTTON_OFF|BUTTON_LEFT|BUTTON_RIGHT|\
                BUTTON_UP|BUTTON_DOWN|BUTTON_REC|BUTTON_MODE|BUTTON_SELECT)

    /* Remote control's buttons */
#define BUTTON_RC_ON        0x00100000
#define BUTTON_RC_STOP      0x00080000

#define BUTTON_RC_REW       0x00040000
#define BUTTON_RC_FF        0x00020000
#define BUTTON_RC_VOL_UP    0x00010000
#define BUTTON_RC_VOL_DOWN  0x00008000

#define BUTTON_RC_REC       0x00004000
#define BUTTON_RC_MODE      0x00002000

#define BUTTON_RC_MENU      0x00001000

#define BUTTON_RC_BITRATE   0x00000800
#define BUTTON_RC_SOURCE    0x00000400

#define BUTTON_REMOTE (BUTTON_RC_ON|BUTTON_RC_STOP|BUTTON_RC_REW|BUTTON_RC_FF\
                |BUTTON_RC_VOL_UP|BUTTON_RC_VOL_DOWN|BUTTON_RC_REC\
                |BUTTON_RC_MODE|BUTTON_RC_MENU|BUTTON_RC_BITRATE\
                |BUTTON_RC_SOURCE)

#define POWEROFF_BUTTON BUTTON_OFF
#define RC_POWEROFF_BUTTON BUTTON_RC_STOP
#define POWEROFF_COUNT 10

#endif /* _BUTTON_TARGET_H_ */
