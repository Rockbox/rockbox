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

#define I2C_FREQ_100K 100000
#define I2C_FREQ_400K 400000

#define I2C_CONTINUE 0
#define I2C_STOP     1
#define I2C_RESTART  2

#define I2C_10BITADDR (1 << 12)

#define I2C_ERR_BUS     (-1)
#define I2C_ERR_DRIVER  (-2)
#define I2C_ERR_TIMEOUT (-3)

extern void i2c_init(void);

/* Configure the I2C controller prior to use.
 *
 * - freq: frequency of SCL, should be <= 400 KHz and >= 25 KHz
 *   - use I2C_FREQ_100K for 100 KHz
 *   - use I2C_FREQ_400K for 400 Khz
 *   - use 0 to disable the controller completely
 *   - frequencies below 25 KHz will violate timing constraints
 */
extern void i2c_set_freq(int chn, int freq);

/* I2C bus lock. You need to hold the lock before issuing a read or write. */
extern void i2c_lock(int chn);
extern void i2c_unlock(int chn);

/* I2C read/write commands. Blocks until the transfer completes.
 * Returns 0 on success, and a negative number on error.
 *
 * - addr: slave address
 *   - for 7-bit addresses, just specify them directly
 *   - for 10-bit addresses, OR them with I2C_10BITADDR
 *
 * - flag: action to take after transmitting
 *   - I2C_CONTINUE: hold SCL low after transmission
 *   - I2C_STOP: send STOP condition after transmission
 *   - I2C_RESTART: send RESTART before first byte
 */
extern int i2c_read(int chn, int addr, int flag,
                    int count, unsigned char* data);
extern int i2c_write(int chn, int addr, int flag,
                     int count, const unsigned char* data);

#endif /* __I2C_X1000_H__ */
