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

#ifndef __I2C_X1000_H__
#define __I2C_X1000_H__

#include "i2c-async.h"

#define I2C_FREQ_100K 100000
#define I2C_FREQ_400K 400000

extern void i2c_init(void);

/* Configure the I2C controller prior to use.
 *
 * - freq: frequency of SCL, should be <= 400 KHz and >= 25 KHz
 *   - use I2C_FREQ_100K for 100 KHz
 *   - use I2C_FREQ_400K for 400 Khz
 *   - use 0 to disable the controller completely
 *   - frequencies below 25 KHz will violate timing constraints
 *
 * TODO: move this to the i2c-async API, it's simple enough
 */
extern void i2c_x1000_set_freq(int chn, int freq);

#endif /* __I2C_X1000_H__ */
