/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __CW2015_H__
#define __CW2015_H__

#include <stdint.h>
#include <stdbool.h>

/* Driver for I2C battery fuel gauge IC CW2015. */

#define CW2015_REG_VERSION      0x00
#define CW2015_REG_VCELL        0x02 /* 14 bits, registers 0x02 - 0x03 */
#define CW2015_REG_SOC          0x04 /* 16 bits, registers 0x04 - 0x05 */
#define CW2015_REG_RRT_ALERT    0x06 /* 13 bit RRT + alert flag, 0x06-0x07 */
#define CW2015_REG_CONFIG       0x08
#define CW2015_REG_MODE         0x0a
#define CW2015_REG_BATINFO      0x10 /* cf. mainline Linux CW2015 driver */
#define CW2015_SIZE_BATINFO     64

extern void cw2015_init(void);

/* Read the battery terminal voltage, converted to millivolts. */
extern int cw2015_get_vcell(void);

/* Read the SOC, in percent (0-100%). */
extern int cw2015_get_soc(void);

/* Get the estimated remaining run time, in minutes.
 * Not a linearly varying quantity, according to the datasheet. */
extern int cw2015_get_rrt(void);

/* Read the current battery profile */
extern const uint8_t* cw2015_get_bat_info(void);

/* Debug screen */
extern bool cw2015_debug_menu(void);

#endif /* __CW2015_H__ */
