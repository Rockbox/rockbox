/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Jens Arnold
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

#define HAS_SERIAL_REMOTE

void button_init_device(void);
int button_read_device(void);

      /* Main unit's buttons */
#define BUTTON_ON           0x00000001
#define BUTTON_OFF          0x00000002

#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_UP           0x00000010
#define BUTTON_DOWN         0x00000020

#define BUTTON_PLAY         0x00000040

#define BUTTON_F1           0x00000080
#define BUTTON_F2           0x00000100
#define BUTTON_F3           0x00000200

#define BUTTON_MAIN (BUTTON_ON|BUTTON_OFF|BUTTON_LEFT|BUTTON_RIGHT\
                |BUTTON_UP|BUTTON_DOWN|BUTTON_PLAY\
                |BUTTON_F1|BUTTON_F2|BUTTON_F3)
                
    /* Remote control's buttons */
#define BUTTON_RC_PLAY      0x00100000
#define BUTTON_RC_STOP      0x00080000

#define BUTTON_RC_LEFT      0x00040000
#define BUTTON_RC_RIGHT     0x00020000
#define BUTTON_RC_VOL_UP    0x00010000
#define BUTTON_RC_VOL_DOWN  0x00008000

#define BUTTON_REMOTE (BUTTON_RC_PLAY|BUTTON_RC_STOP\
                |BUTTON_RC_LEFT|BUTTON_RC_RIGHT\
                |BUTTON_RC_VOL_UP|BUTTON_RC_VOL_DOWN)

#endif /* _BUTTON_TARGET_H_ */
