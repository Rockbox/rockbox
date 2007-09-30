/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: i2c-meg-fx.h 13720 2007-06-26 02:11:30Z jethead71 $
 *
 * Copyright (C) 2007 by Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

