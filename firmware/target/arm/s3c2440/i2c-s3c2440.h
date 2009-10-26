/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
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

/* chip-specific i2c functions */

/* IICCON */
#define I2C_ACKGEN          (1 << 7)
#define I2C_TXCLK_512       (1 << 6)
#define I2C_TXRX_INTENB     (1 << 5)
#define I2C_TXRX_INTPND     (1 << 4)

/* IICSTAT */
#define I2C_MODE_MASTER     (2 << 6)
#define I2C_MODE_TX         (1 << 6)
#define I2C_BUSY            (1 << 5)
#define I2C_START           (1 << 5)
#define I2C_RXTX_ENB        (1 << 4)
#define I2C_BUS_ARB_FAILED  (1 << 3)
#define I2C_S_ADDR_STAT     (1 << 2)
#define I2C_S_ADDR_MATCH    (1 << 1)
#define I2C_ACK_L           (1 << 0)

/* IICLC */
#define I2C_FLT_ENB         (1 << 2)

void i2c_init(void);
void i2c_write(int addr, const unsigned char *data, int count);

