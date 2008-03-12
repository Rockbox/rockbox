/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#define HAS_REMOTE_BUTTON_HOLD

bool button_hold(void);
bool remote_button_hold(void);
void button_init_device(void);
int button_read_device(void);

/* iaudio M3 specific button codes */

    /* Main unit's buttons */
#define BUTTON_PLAY         0x00000001

#define BUTTON_LEFT         0x00000002
#define BUTTON_RIGHT        0x00000004
#define BUTTON_VOL_UP       0x00000008
#define BUTTON_VOL_DOWN     0x00000010

#define BUTTON_MODE         0x00000020
#define BUTTON_REC          0x00000040


#define BUTTON_MAIN (BUTTON_PLAY|BUTTON_LEFT|BUTTON_RIGHT\
                |BUTTON_VOL_UP|BUTTON_VOL_DOWN|BUTTON_MODE|BUTTON_REC)

    /* Remote control's buttons */
#define BUTTON_RC_PLAY      0x00100000

#define BUTTON_RC_REW       0x00080000
#define BUTTON_RC_FF        0x00040000
#define BUTTON_RC_VOL_UP    0x00020000
#define BUTTON_RC_VOL_DOWN  0x00010000

#define BUTTON_RC_REC       0x00008000
#define BUTTON_RC_MENU      0x00004000
#define BUTTON_RC_MODE      0x00002000

#define BUTTON_REMOTE (BUTTON_RC_PLAY|BUTTON_RC_VOL_UP|BUTTON_RC_VOL_DOWN\
                |BUTTON_RC_REW|BUTTON_RC_FF\
                |BUTTON_RC_REC|BUTTON_RC_MENU|BUTTON_RC_MODE)

#define POWEROFF_BUTTON     BUTTON_PLAY
#define RC_POWEROFF_BUTTON  BUTTON_RC_PLAY
#define POWEROFF_COUNT      30

#endif /* _BUTTON_TARGET_H_ */
