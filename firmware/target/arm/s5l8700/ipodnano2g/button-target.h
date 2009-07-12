/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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

bool button_hold(void);
void button_init_device(void);
int button_read_device(void);

void ipod_mini_button_int(void);
void ipod_3g_button_int(void);
void ipod_4g_button_int(void);

/* iPod specific button codes */

#define BUTTON_SELECT       0x00000001
#define BUTTON_MENU         0x00000002

#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_SCROLL_FWD   0x00000010
#define BUTTON_SCROLL_BACK  0x00000020

#define BUTTON_PLAY         0x00000040

#define BUTTON_MAIN (BUTTON_SELECT|BUTTON_MENU\
                |BUTTON_LEFT|BUTTON_RIGHT|BUTTON_SCROLL_FWD\
                |BUTTON_SCROLL_BACK|BUTTON_PLAY)

    /* Remote control's buttons */
#ifdef IPOD_ACCESSORY_PROTOCOL
#define BUTTON_RC_PLAY      0x00100000
#define BUTTON_RC_STOP      0x00080000

#define BUTTON_RC_LEFT      0x00040000
#define BUTTON_RC_RIGHT     0x00020000
#define BUTTON_RC_VOL_UP    0x00010000
#define BUTTON_RC_VOL_DOWN  0x00008000

#define BUTTON_REMOTE (BUTTON_RC_PLAY|BUTTON_RC_STOP\
                |BUTTON_RC_LEFT|BUTTON_RC_RIGHT\
                |BUTTON_RC_VOL_UP|BUTTON_RC_VOL_DOWN)
#else
#define BUTTON_REMOTE 0
#endif

/* This is for later
#define  BUTTON_SCROLL_TOUCH 0x00000200
*/


#define POWEROFF_BUTTON BUTTON_PLAY
#define POWEROFF_COUNT 40

#endif /* _BUTTON_TARGET_H_ */
