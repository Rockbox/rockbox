/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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

/* The NWZ-A860 is completely different, it has a touchscreen and some but not
 * all keys of the other others */
#if defined(SONY_NWZA860)

/* Main unit's buttons */
#define BUTTON_BACK                 0x00000001 /* HOME */
#define BUTTON_PLAY                 0x00000002
#define BUTTON_REW                  0x00000004
#define BUTTON_FF                   0x00000008
#define BUTTON_VOL_DOWN             0x00000010
#define BUTTON_VOL_UP               0x00000020
/* For compatibility */
#define BUTTON_LEFT                 BUTTON_MIDLEFT
#define BUTTON_RIGHT                BUTTON_MIDRIGHT
#define BUTTON_UP                   BUTTON_TOPMIDDLE
#define BUTTON_DOWN                 BUTTON_BOTTOMMIDDLE

/* Touch Screen Area Buttons */
#define BUTTON_TOPLEFT              0x00000040
#define BUTTON_TOPMIDDLE            0x00000080
#define BUTTON_TOPRIGHT             0x00000100
#define BUTTON_MIDLEFT              0x00000200
#define BUTTON_CENTER               0x00000400
#define BUTTON_MIDRIGHT             0x00000800
#define BUTTON_BOTTOMLEFT           0x00001000
#define BUTTON_BOTTOMMIDDLE         0x00002000
#define BUTTON_BOTTOMRIGHT          0x00004000

#define BUTTON_MAIN 0x7fff

#define POWEROFF_BUTTON             BUTTON_BACK

#else /* SONY_NWZA860 */

/* Main unit's buttons */
#define BUTTON_POWER                0x00000001
#define BUTTON_BACK                 0x00000002
#define BUTTON_PLAY                 0x00000004
#define BUTTON_LEFT                 0x00000008
#define BUTTON_UP                   0x00000010
#define BUTTON_DOWN                 0x00000020
#define BUTTON_RIGHT                0x00000040
#define BUTTON_VOL_DOWN             0x00000080
#define BUTTON_VOL_UP               0x00000100

#define BUTTON_MAIN                 0x000001ff

#define POWEROFF_BUTTON             BUTTON_POWER

#endif /* SONY_NWZA860 */

/* Software power-off */
#define POWEROFF_COUNT 10

/* force driver to reload button state (useful after suspend) */
void nwz_button_reload_after_suspend(void);

#endif /* _BUTTON_TARGET_H_ */
