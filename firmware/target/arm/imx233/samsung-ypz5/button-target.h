/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Lorenzo Miori
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

#define HAS_BUTTON_HOLD
#define IMX233_BUTTON_LRADC_CHANNEL     0
#define IMX233_BUTTON_LRADC_HOLD_DET    BLH_GPIO
#define BLH_GPIO_BANK   0
#define BLH_GPIO_PIN    13

#define IMX233_BUTTON_LRADC_CHANNEL 0

/* Main unit's buttons */
#define BUTTON_POWER                0x00000001
#define BUTTON_VOL_UP               0x00000002
#define BUTTON_VOL_DOWN             0x00000004
/* Directional buttons by touchpad */
#define BUTTON_LEFT                 0x00000008
#define BUTTON_UP                   0x00000010
#define BUTTON_RIGHT                0x00000020
#define BUTTON_DOWN                 0x00000040
#define BUTTON_SELECT               0x00000080
#define BUTTON_BACK                 0x00000100
#define BUTTON_REW                  0x00000200
#define BUTTON_FF                   0x00000400


#define BUTTON_MAIN (BUTTON_VOL_UP | BUTTON_VOL_DOWN | BUTTON_POWER | BUTTON_LEFT | \
                     BUTTON_UP | BUTTON_RIGHT | BUTTON_DOWN | BUTTON_SELECT | \
                     BUTTON_BACK | BUTTON_REW | BUTTON_FF)

/* Software power-off */
#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT 10

bool button_debug_screen(void);

#endif /* _BUTTON_TARGET_H_ */
