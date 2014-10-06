 /***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Port of xrick, a Rick Dangerous clone, to Rockbox.
 * See http://www.bigorno.net/xrick/
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza
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

#ifndef _ROCKBOXCODES_H
#define _ROCKBOXCODES_H

/* keypad mappings */
#include "plugin.h"

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define XRICK_BTN_UP       BUTTON_UP | BUTTON_REC
#define XRICK_BTN_DOWN     BUTTON_DOWN | BUTTON_MODE
#define XRICK_BTN_LEFT     BUTTON_LEFT
#define XRICK_BTN_RIGHT    BUTTON_RIGHT
#define XRICK_BTN_FIRE     BUTTON_ON
#define XRICK_BTN_PAUSE    BUTTON_SELECT
#define XRICK_BTN_MENU     BUTTON_OFF

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define XRICK_BTN_MENU      BUTTON_POWER
#define XRICK_BTN_FIRE      BUTTON_PLAY
#define XRICK_BTN_PAUSE     BUTTON_REW
#define XRICK_BTN_UP        BUTTON_SCROLL_UP
#define XRICK_BTN_DOWN      BUTTON_SCROLL_DOWN
#define XRICK_BTN_LEFT      BUTTON_LEFT
#define XRICK_BTN_RIGHT     BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define XRICK_BTN_UP       BUTTON_MENU
#define XRICK_BTN_DOWN     BUTTON_PLAY
#define XRICK_BTN_LEFT     BUTTON_LEFT
#define XRICK_BTN_RIGHT    BUTTON_RIGHT
#define XRICK_BTN_FIRE     BUTTON_SELECT
#define XRICK_BTN_PAUSE    BUTTON_SCROLL_BACK
#define XRICK_BTN_MENU     BUTTON_SCROLL_FWD

/* place other keypad mappings here
#elif CONFIG_KEYPAD ==...
#define XRICK_BTN...
*/

#else
#error Unsupported keypad
#endif

#endif /* ndef _ROCKBOXCODES_H */

/* eof */
