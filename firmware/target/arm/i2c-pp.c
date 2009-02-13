/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * PP502X and PP5002 I2C driver
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in November 2005
 *
 * Original file: linux/arch/armnommu/mach-ipod/hardware.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
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

#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "logf.h"
#include "system.h"
#include "i2c-pp.h"
#include "ascodec.h"
#include "as3514.h"

/* Local functions definitions */
static struct mutex i2c_mtx SHAREDBSS_ATTR;

#define POLL_TIMEOUT (HZ)

static int pp_i2c_wait_not_busy(void)
{
    unsigned long timeout;
    timeout = current_tick + POLL_TIMEOUT;
    while (TIME_BEFORE(current_tick, timeout)) {
         if (!(I2C_STATUS & I2C_BUSY)) {
            return 0;
         }
         yield();
    }

    return -1;
}

static int pp_i2c_read_bytes(unsigned int addr, int len, unsigned char *data)
{
    int i;

    if (len < 1 || len > 4)
    {
        return -1;
    }

    if (pp_i2c_wait_not_busy() < 0)
    {
        return -2;
    }

    {
        int old_irq_level = disable_irq_save();

        /* clear top 15 bits, left shift 1, or in 0x1 for a read */
        I2C_ADDR = ((addr << 17) >> 16) | 0x1;

        I2C_CTRL |= 0x20;

        I2C_CTRL = (I2C_CTRL & ~0x6) | ((len-1) << 1);

        I2C_CTRL |= I2C_SEND;

        restore_irq(old_irq_level);

        if (pp_i2c_wait_not_busy() < 0)
        {
            return -2;
        }

        old_irq_level = disable_irq_save();

        if (data)
        {
            for ( i = 0; i < len; i++ )
                *data++ = I2C_DATA(i);
        }

        restore_irq(old_irq_level);
    }

    return 0;
}

static int pp_i2c_send_bytes(unsigned int addr, int len, unsigned char *data)
{
    int i;

    if (len < 1 || len > 4)
    {
        return -1;
    }

    if (pp_i2c_wait_not_busy() < 0)
    {
        return -2;
    }

    {
        int old_irq_level = disable_irq_save();

        /* clear top 15 bits, left shift 1 */
        I2C_ADDR = (addr << 17) >> 16;

        I2C_CTRL &= ~0x20;

        for ( i = 0; i < len; i++ )
        {
            I2C_DATA(i) = *data++;
        }

        I2C_CTRL = (I2C_CTRL & ~0x6) | ((len-1) << 1);

        I2C_CTRL |= I2C_SEND;

        restore_irq(old_irq_level);
    }

    return 0;
}

static int pp_i2c_send_byte(unsigned int addr, int data0)
{
    unsigned char data[1];

    data[0] = data0;

    return pp_i2c_send_bytes(addr, 1, data);
}

/* Public functions */
void i2c_lock(void)
{
    mutex_lock(&i2c_mtx);
}

void i2c_unlock(void)
{
    mutex_unlock(&i2c_mtx);
}

int i2c_readbytes(unsigned int dev_addr, int addr, int len, unsigned char *data)
{
    int i, n;

    mutex_lock(&i2c_mtx);

    if (addr >= 0)
        pp_i2c_send_byte(dev_addr, addr);

    i = 0;
    while (len > 0)
    {
        n = (len < 4) ? len : 4;

        if (pp_i2c_read_bytes(dev_addr, n, data + i) < 0)
            break;

        len -= n;
        i   += n;
    }

    mutex_unlock(&i2c_mtx);

    return i;
}

int i2c_readbyte(unsigned int dev_addr, int addr)
{
    unsigned char data;

    mutex_lock(&i2c_mtx);
    pp_i2c_send_byte(dev_addr, addr);
    pp_i2c_read_bytes(dev_addr, 1, &data);
    mutex_unlock(&i2c_mtx);

    return (int)data;
}

int pp_i2c_send(unsigned int addr, int data0, int data1)
{
    int retval;
    unsigned char data[2];

    data[0] = data0;
    data[1] = data1;

    mutex_lock(&i2c_mtx);
    retval = pp_i2c_send_bytes(addr, 2, data);
    mutex_unlock(&i2c_mtx);

    return retval;
}

void i2c_init(void)
{
    /* From ipodlinux */
    mutex_init(&i2c_mtx);

#ifdef IPOD_MINI
    /* GPIO port C disable port 0x10 */
    GPIOC_ENABLE &= ~0x10;

    /* GPIO port C disable port 0x20 */
    GPIOC_ENABLE &= ~0x20;
#endif

#if CONFIG_I2C == I2C_PP5002
    DEV_EN |= 0x2;
#else
    DEV_EN |= DEV_I2C;  /* Enable I2C */
#endif
    DEV_RS |= DEV_I2C;  /* Start I2C Reset */
    DEV_RS &=~DEV_I2C;  /* End I2C Reset */

#if CONFIG_I2C == I2C_PP5020
    outl(0x0, 0x600060a4);
    outl(0x80 | (0 << 8), 0x600060a4);
#elif CONFIG_I2C == I2C_PP5024
#if defined(SANSA_E200) || defined(PHILIPS_SA9200)
    /* Sansa OF sets this to 0x20 first, communicates with the AS3514
       then sets it to 0x23 - this still works fine though */
    outl(0x0, 0x600060a4);
    outl(0x23, 0x600060a4);
#elif defined(SANSA_C200)
    /* This is the init sequence from the Sansa c200 bootloader.
       I'm not sure what's really necessary. */
    pp_i2c_wait_not_busy();

    outl(0, 0x600060a4);
    outl(0x64, 0x600060a4);

    outl(0x55, 0x7000c02c);
    outl(0x54, 0x7000c030);

    outl(0, 0x600060a4);
    outl(0x1e, 0x600060a4);

    ascodec_write(AS3514_SUPERVISOR, 5);
#endif
#endif

    i2c_readbyte(0x8, 0);
}
