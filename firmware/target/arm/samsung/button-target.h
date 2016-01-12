/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Mark Arigo
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

/* Button codes for Samsung YH-820, 920, 925 */

#if defined(SAMSUNG_YH920) || defined(SAMSUNG_YH925)
void remote_int(void);
#endif /* (SAMSUNG_YH920) || (SAMSUNG_YH925) */

/* Main unit's buttons */
/* Left = Menu, Right = Sel */
#define BUTTON_LEFT         0x00000001
#define BUTTON_RIGHT        0x00000002
#define BUTTON_UP           0x00000004
#define BUTTON_DOWN         0x00000008
#define BUTTON_PLAY         0x00000010
#define BUTTON_REW          0x00000020
#define BUTTON_FFWD         0x00000040
#if defined(SAMSUNG_YH820) /* YH820 has record button */
#define BUTTON_REC          0x00000080
#else /* virtual buttons for record switch state change on YH92x */
#define BUTTON_REC_SW_ON    0x00000080
#define BUTTON_REC_SW_OFF   0x00000100
#endif

#if defined(SAMSUNG_YH820)
#define BUTTON_MAIN         0x000000ff
#else
#define BUTTON_MAIN         0x000001ff
#endif

#define BUTTON_RC_PLUS      BUTTON_UP
#define BUTTON_RC_MINUS     BUTTON_DOWN
#define BUTTON_RC_PLAY      BUTTON_PLAY
#define BUTTON_RC_REW       BUTTON_REW
#define BUTTON_RC_FFWD      BUTTON_FFWD

#define POWEROFF_BUTTON BUTTON_PLAY
#define POWEROFF_COUNT  15

#endif /* _BUTTON_TARGET_H_ */
