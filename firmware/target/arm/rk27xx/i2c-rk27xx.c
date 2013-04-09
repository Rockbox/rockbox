/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Marcin Bukat
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

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "i2c-rk27xx.h"

/*  Driver for the rockchip rk27xx built-in I2C controller in master mode
    
    Both the i2c_read and i2c_write function take the following arguments:
    * slave, the address of the i2c slave device to read from / write to
    * address, optional sub-address in the i2c slave (unused if -1)
    * len, number of bytes to be transfered
    * data, pointer to data to be transfered
    A return value other than 0 indicates an error.
*/

static struct mutex i2c_mtx;

static bool i2c_stop(void)
{
    long timeout = current_tick + HZ/50;

    I2C_CONR |= (1<<4); /* NACK */
    I2C_LCMR |= (1<<2) | (1<<1); /* resume op, stop */

    while (I2C_LCMR & (1<<1))
        if (TIME_AFTER(current_tick, timeout))
            return false;

    return true;
}

static bool i2c_write_byte(uint8_t data, bool start)
{
    long timeout = current_tick + HZ/50;
    unsigned int isr_status;

    I2C_CONR = (1<<3) | (1<<2); /* master port enable, MTX mode, ACK enable */
    I2C_MTXR = data;

    if (start)
        I2C_LCMR = (1<<2) | (1<<0); /* resume op, start bit */
    else
        I2C_LCMR = (1<<2); /* resume op */

    /* wait for ACK from slave */
    do
    {
        isr_status = I2C_ISR;

        if (isr_status & (1<<7))
        {
            i2c_stop();
            I2C_ISR = 0;
            return false;
        }

        if (TIME_AFTER(current_tick, timeout))
            return false;

    } while ((isr_status & (1<<0)) == 0);

    /* clear status bit */
    I2C_ISR &= ~(1<<0);

    return true;
}

static bool i2c_read_byte(unsigned char *data)
{
    long timeout = current_tick + HZ/50;

    I2C_LCMR = (1<<2); /* resume op */

    while (!(I2C_ISR & (1<<1)))
        if (TIME_AFTER(current_tick, timeout))
            return false;

    *data = I2C_MRXR;

    /* clear status bit */
    I2C_ISR &= ~(1<<1);

    return true;
}



/* route i2c bus to internal codec or external bus
 * internal codec has 0x4e i2c slave address so
 * access to this address is routed to internal bus.
 * All other addresses are routed to external pads
 */
static void i2c_iomux(unsigned char slave)
{
    unsigned long muxa = SCU_IOMUXA_CON & ~(0x1f<<14);

    if ((slave & 0xfe) == (0x27<<1))
    {
        /* internal codec */
        SCU_IOMUXA_CON = (muxa | (1<<16) | (1<<14));
    }
    else
    {
        /* external I2C bus */
        SCU_IOMUXA_CON = (muxa | (1<<18));
    }
}

void i2c_init(void)
{
    mutex_init(&i2c_mtx);


    /* ungate i2c module clock */
    SCU_CLKCFG &= ~CLKCFG_I2C;

    I2C_OPR |= (1<<7); /* reset state machine */
    sleep(HZ/100);
    I2C_OPR &= ~((1<<7) | (1<<6)); /* clear ENABLE bit, deasert reset */

    /* set I2C divider to stay within allowed SCL freq limit
     * APBfreq = 50Mhz
     * I2C_div = (I2CCDVR[5:3] + 1) * 2^((I2CCDVR[2:0] + 1))
     * SCLfreq = APBfreq/(5*I2C_div)
     *
     * (5<<3) | (1<<0)    416.7 KHz (above spec)
     * (6<<3) | (1<<0)    357.1 kHz
     * (7<<3) | (1<<0)    312.4 kHz
     * (6<<3) | (2<<0)    178.6 kHz
     * (7<<3) | (2<<0)    156.3 kHz
     */
    I2C_OPR = (I2C_OPR & ~(0x3F)) | (6<<3) | (1<<0);

    I2C_IER = 0x00;

    I2C_OPR |= (1<<6); /* enable i2c core */

    /* turn off i2c module clock until we need to comunicate */
    SCU_CLKCFG |= CLKCFG_I2C;
}

int i2c_write(unsigned char slave, int address, int len,
              const unsigned char *data)
{
    int ret = 0;

    mutex_lock(&i2c_mtx);

    i2c_iomux(slave);

    /* ungate i2c clock */
    SCU_CLKCFG &= ~CLKCFG_I2C;

    /* clear all flags */
    I2C_ISR = 0x00;
    I2C_IER = 0x00;

    /* START */
    if (! i2c_write_byte(slave & ~1, true))
    {
        ret = 1;
        goto end;
    }

    if (address >= 0)
    {
        if (! i2c_write_byte(address, false))
        {
            ret = 2;
            goto end;
        }
    }

    /* write data */
    while (len--)
    {
        if (! i2c_write_byte(*data++, false))
        {
            ret = 4;
            goto end;
        }
    }

    /* STOP */
    if (! i2c_stop())
    {
        ret = 5;
        goto end;
    }

end:
    mutex_unlock(&i2c_mtx);
    SCU_CLKCFG |= CLKCFG_I2C;

    return ret;
}

int i2c_read(unsigned char slave, int address, int len, unsigned char *data)
{
    int ret = 0;

    mutex_lock(&i2c_mtx);

    i2c_iomux(slave);

    /* ungate i2c module clock */
    SCU_CLKCFG &= ~CLKCFG_I2C;

    /* clear all flags */
    I2C_ISR = 0x00;
    I2C_IER = 0x00;

    if (address >= 0)
    {
        /* START */
        if (! i2c_write_byte(slave & ~1, true))
        {
            ret = 1;
            goto end;
        }

        /* write address */
        if (! i2c_write_byte(address, false))
        {
            ret = 2;
            goto end;
        }
    }

    /* (repeated) START */
    if (! i2c_write_byte(slave | 1, true))
    {
        ret = 3;
        goto end;
    }

    I2C_CONR = (1<<2); /* master port enable, MRX mode, ACK enable */

    while (len)
    {
        if (! i2c_read_byte(data++))
        {
            ret = 4;
            goto end;
        }

        if (len == 1)
            I2C_CONR |= (1<<4); /* NACK */
        else
            I2C_CONR &= ~(1<<4); /* ACK */

        len--;
    }

   /* STOP */
    if (! i2c_stop())
    {
        ret = 5;
        goto end;
    }

end:
    mutex_unlock(&i2c_mtx);
    SCU_CLKCFG |= CLKCFG_I2C;

    return ret;
}
