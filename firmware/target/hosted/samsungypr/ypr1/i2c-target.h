/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Module wrapper for SI4709 FM Radio Chip, using /dev/si470x (si4709.ko)
 *      Samsung YP-R0 & Samsung YP-R1
 *
 * Copyright (c) 2017 Lorenzo Miori
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

#ifndef I2C_TARGET_H_
#define I2C_TARGET_H_

/** Define the FM Radio Chip I2C bus */
#define I2C_BUS_FM_RADIO        ("/dev/i2c-1")

#endif /* I2C_TARGET_H_ */
