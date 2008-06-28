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
#include "system.h"
#include "i2c-meg-fx.h"

static struct wakeup i2c_wake; /* Transfer completion signal */
static struct mutex i2c_mtx;   /* Mutual exclusion */
static unsigned char *buf_ptr; /* Next byte to transfer */
static int buf_count;          /* Number of bytes remaining to transfer */

static void i2c_stop(void)
{
    /* Generate STOP */
    IICSTAT = I2C_MODE_MASTER | I2C_MODE_TX | I2C_RXTX_ENB;

    /* No more interrupts, clear pending interrupt to continue */
    IICCON &= ~(I2C_TXRX_INTPND | I2C_TXRX_INTENB);
}

void i2c_write(int addr, const unsigned char *buf, int count)
{
    if (count <= 0)
        return;

    mutex_lock(&i2c_mtx);

    /* Turn on I2C clock */
    s3c_regset(&CLKCON, 1 << 16);

    /* Set mode to master transmitter and enable lines */
    IICSTAT = I2C_MODE_MASTER | I2C_MODE_TX | I2C_RXTX_ENB;

    /* Set buffer start and count */
    buf_ptr = (unsigned char *)buf;
    buf_count = count;

    /* Send slave address and then data */
    SRCPND = IIC_MASK;
    INTPND = IIC_MASK;

    IICCON |= I2C_TXRX_INTENB;

    /* Load slave address into shift register */
    IICDS = addr & 0xfe;

    /* Generate START */
    IICSTAT = I2C_MODE_MASTER | I2C_MODE_TX | I2C_START | I2C_RXTX_ENB;

    if (wakeup_wait(&i2c_wake, HZ) != OBJ_WAIT_SUCCEEDED)
    {
        /* Something went wrong - stop transmission */
        int oldlevel = disable_irq_save();
        i2c_stop();
        restore_irq(oldlevel);
    }

    /* Go back to slave receive mode and disable lines */
    IICSTAT = 0;

    /* Turn off I2C clock */
    s3c_regclr(&CLKCON, 1 << 16);

    mutex_unlock(&i2c_mtx);
}

void i2c_init(void)
{
    /* Init kernel objects */
    wakeup_init(&i2c_wake);
    mutex_init(&i2c_mtx);

    /* Clear pending source */
    SRCPND = IIC_MASK;
    INTPND = IIC_MASK;

    /* Enable i2c interrupt in controller */
    s3c_regclr(&INTMOD, IIC_MASK);
    s3c_regclr(&INTMSK, IIC_MASK);

    /* Turn on I2C clock */
    s3c_regset(&CLKCON, 1 << 16);

    /* Set GPE15 (IICSDA) and GPE14 (IICSCL) to IIC */
    GPECON = (GPECON & ~((3 << 30) | (3 << 28))) |
                ((2 << 30) | (2 << 28));

    /* Bus ACK, IICCLK: fPCLK / 16, Rx/Tx Int: Disable, Tx clock: IICCLK/8 */
    /* OF PCLK: 49.1568MHz / 16 / 8 = 384.0375 kHz */
    IICCON = (7 << 0);

    /* SDA line delayed 0 PCLKs */
    IICLC = (0 << 0);

    /* Turn off I2C clock */
    s3c_regclr(&CLKCON, 1 << 16);
}

void IIC(void)
{
    for (;;)
    {
        /* If ack was received from last byte and bytes are remaining */
        if (--buf_count >= 0 && (IICSTAT & I2C_ACK_L) == 0)
        {
            /* Write next byte to shift register */
            IICDS = *buf_ptr++;

            /* Clear pending interrupt to continue */
            IICCON &= ~I2C_TXRX_INTPND;
            break;
        }

        /* Finished */

        /* Generate STOP */
        i2c_stop();

        /* Signal thread */
        wakeup_signal(&i2c_wake);
        break;
    }

    /* Ack */
    SRCPND = IIC_MASK;
    INTPND = IIC_MASK;
}
