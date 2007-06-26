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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "system.h"
#include "i2c-meg-fx.h"

/* Only implements sending bytes for now. Adding receiving bytes should be
   straightforward if needed. No yielding is present since the calls only
   involve setting audio codec registers - a very rare event. */

/* Wait for a condition on the bus, optionally returning it */
#define COND_RET    _c;
#define COND_VOID
#define WAIT_COND(cond, ret)                        \
    ({                                              \
        int _t = current_tick + 2;                  \
        bool _c;                                    \
        while (1) {                                 \
            _c = !!(cond);                          \
            if (_c || TIME_AFTER(current_tick, _t)) \
                break;                              \
        }                                           \
        ret                                         \
    })

static int i2c_getack(void)
{
    /* Wait for ACK: 0 = ack received, 1 = ack not received */
    WAIT_COND(IICCON & I2C_TXRX_INTPND, COND_VOID);
    return IICSTAT & I2C_ACK_L;
}

static int i2c_start(void)
{
    /* Generate START */
    IICSTAT = I2C_MODE_MASTER | I2C_MODE_TX | I2C_START | I2C_RXTX_ENB;
    return i2c_getack();
}

static void i2c_stop(void)
{
    /* Generate STOP */
    IICSTAT = I2C_MODE_MASTER | I2C_MODE_TX | I2C_RXTX_ENB;
    /* Clear pending interrupt to continue */
    IICCON &= ~I2C_TXRX_INTPND;
}

static int i2c_outb(unsigned char byte)
{
    /* Write byte to shift register */
    IICDS = byte;
    /* Clear pending interrupt to continue */
    IICCON &= ~I2C_TXRX_INTPND;
    return i2c_getack();
}

void i2c_write(int addr, const unsigned char *buf, int count)
{
    /* Turn on I2C clock */
    CLKCON |= (1 << 16);

    /* Set mode to master transmitter and enable lines */
    IICSTAT = I2C_MODE_MASTER | I2C_MODE_TX | I2C_RXTX_ENB;

    /* Wait for bus to be available */
    if (WAIT_COND(!(IICSTAT & I2C_BUSY), COND_RET))
    {
        /* Send slave address and then data */
        IICCON &= ~I2C_TXRX_INTPND;
        IICCON |= I2C_TXRX_INTENB;

        IICDS = addr & 0xfe;

        if (i2c_start() == 0)
            while (count-- > 0 && i2c_outb(*buf++) == 0);

        i2c_stop();

        IICCON &= ~I2C_TXRX_INTENB;
    }

    /* Go back to slave receive mode and disable lines */
    IICSTAT = 0;

    /* Turn off I2C clock */
    CLKCON &= ~(1 << 16);
}

void i2c_init(void)
{
    /* We poll I2C interrupts */
    INTMSK |= (1 << 27);

    /* Turn on I2C clock */
    CLKCON |= (1 << 16);

    /* Set GPE15 (IICSDA) and GPE14 (IICSCL) to IIC */
    GPECON = (GPECON & ~((3 << 30) | (3 << 28))) |
                ((2 << 30) | (2 << 28));

    /* Bus ACK, IICCLK: fPCLK / 16, Rx/Tx Int: Disable, Tx clock: IICCLK/8 */
    /* OF PCLK: 49.1568MHz / 16 / 8 = 384.0375 kHz */
    IICCON = (7 << 0);

    /* SDA line delayed 0 PCLKs */
    IICLC = (0 << 0);

    /* Turn off I2C clock */
    CLKCON &= ~(1 << 16);
}
