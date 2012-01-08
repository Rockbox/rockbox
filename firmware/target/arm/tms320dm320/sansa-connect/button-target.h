/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id: $
*
* Copyright (C) 2011 by Tomasz Moń
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

/* these definitions match the avr hid reply */
#define BUTTON_LEFT     (1 << 2)
#define BUTTON_UP       (1 << 3)
#define BUTTON_RIGHT    (1 << 4)
#define BUTTON_DOWN     (1 << 5)
#define BUTTON_SELECT   (1 << 6)
#define BUTTON_VOL_UP   (1 << 10)
#define BUTTON_VOL_DOWN (1 << 11)
#define BUTTON_NEXT     (1 << 13)
#define BUTTON_PREV     (1 << 14)

/* following definitions use "free bits" from avr hid reply */
#define BUTTON_POWER       (1 << 0)
#define BUTTON_HOLD        (1 << 1)
#define BUTTON_SCROLL_FWD  (1 << 7)
#define BUTTON_SCROLL_BACK (1 << 8)

#define BUTTON_MAIN (BUTTON_LEFT | BUTTON_UP | BUTTON_RIGHT | BUTTON_DOWN |\
                     BUTTON_SELECT | BUTTON_VOL_UP | BUTTON_VOL_DOWN |\
                     BUTTON_NEXT | BUTTON_PREV | BUTTON_POWER |\
                     BUTTON_SCROLL_FWD | BUTTON_SCROLL_BACK)

#define POWEROFF_BUTTON     BUTTON_POWER
#define POWEROFF_COUNT      5

#define HAS_BUTTON_HOLD

#endif /* _BUTTON_TARGET_H_ */
