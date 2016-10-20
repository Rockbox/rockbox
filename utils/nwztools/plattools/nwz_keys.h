/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Amaury Pouly
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
#ifndef __NWZ_KEYS_H__
#define __NWZ_KEYS_H__

#define NWZ_KEY_NAME    "icx_key"

/* The Sony icx_key driver reports keys via the /dev/input/event0 device and
 * abuses the standard struct input_event. The input_event.code is split into
 * two parts:
 * - one part giving the code of the key just pressed/released
 * - one part is a bitmap of various keys (such as HOLD)
 * The status of the HOLD can be queried at any time by reading the state of
 * the first LED.
 */

/* key code and mask */
#define NWZ_KEY_MASK        0x1f
#define NWZ_KEY_PLAY        0
#define NWZ_KEY_RIGHT       1
#define NWZ_KEY_LEFT        2
#define NWZ_KEY_UP          3
#define NWZ_KEY_DOWN        4
#define NWZ_KEY_ZAPPIN      5
#define NWZ_KEY_AD0_6       6
#define NWZ_KEY_AD0_7       7
/* special "key" when event is for bitmap key, like HOLD */
#define NWZ_KEY_NONE        15
#define NWZ_KEY_VOL_DOWN    16
#define NWZ_KEY_VOL_UP      17
#define NWZ_KEY_BACK        18
#define NWZ_KEY_OPTION      19
#define NWZ_KEY_BT          20
#define NWZ_KEY_AD1_5       21
#define NWZ_KEY_AD1_6       22
#define NWZ_KEY_AD1_7       23

/* bitmap of other things */
#define NWZ_KEY_NMLHP_MASK  0x20
#define NWZ_KEY_NCHP_MASK   0x40
#define NWZ_KEY_HOLD_MASK   0x80
#define NWZ_KEY_NC_MASK     0x100

#endif /* __NWZ_KEYS_H__ */
