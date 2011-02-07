/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: i2c-s5l8700.c 28589 2010-11-14 15:19:30Z theseven $
 *
 * Copyright (C) 2009 by Bertrik Sikken
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
#include "i2c-s5l8702.h"

/*  Driver for the s5l8700 built-in I2C controller in master mode
    
    Both the i2c_read and i2c_write function take the following arguments:
    * slave, the address of the i2c slave device to read from / write to
    * address, optional sub-address in the i2c slave (unused if -1)
    * len, number of bytes to be transfered
    * data, pointer to data to be transfered
    A return value < 0 indicates an error.
    
    Note:
    * blocks the calling thread for the entire duraton of the i2c transfer but
      uses wakeup_wait/wakeup_signal to allow other threads to run.
    * ACK from slave is not checked, so functions never return an error
*/

static struct mutex i2c_mtx[2];

static void i2c_on(int bus)
{
    /* enable I2C clock */
    PWRCON(1) &= ~(1 << 4);

    IICCON(bus) = (1 << 7) | /* ACK_GEN */
                  (0 << 6) | /* CLKSEL = PCLK/16 */
                  (1 << 5) | /* INT_EN */
                  (1 << 4) | /* IRQ clear */
                  (7 << 0);  /* CK_REG */

    /* serial output on */
    IICSTAT(bus) = (1 << 4);
}

static void i2c_off(int bus)
{
    /* serial output off */
    IICSTAT(bus) = 0;

    /* disable I2C clock */
    PWRCON(1) |= (1 << 4);
}

void i2c_init()
{
    mutex_init(&i2c_mtx[0]);
    mutex_init(&i2c_mtx[1]);
}

int i2c_write(int bus, unsigned char slave, int address, int len, const unsigned char *data)
{
    mutex_lock(&i2c_mtx[bus]);
	i2c_on(bus);
    long timeout = current_tick + HZ / 50;

    /* START */
    IICDS(bus) = slave & ~1;
    IICSTAT(bus) = 0xF0;
    IICCON(bus) = 0xB3;
    while ((IICCON(bus) & 0x10) == 0)
        if (TIME_AFTER(current_tick, timeout))
        {
            mutex_unlock(&i2c_mtx[bus]);
            return 1;
        }

    
    if (address >= 0) {
        /* write address */
        IICDS(bus) = address;
        IICCON(bus) = 0xB3;
        while ((IICCON(bus) & 0x10) == 0)
            if (TIME_AFTER(current_tick, timeout))
            {
                mutex_unlock(&i2c_mtx[bus]);
                return 2;
            }
    }
    
    /* write data */
    while (len--) {
        IICDS(bus) = *data++;
        IICCON(bus) = 0xB3;
        while ((IICCON(bus) & 0x10) == 0)
            if (TIME_AFTER(current_tick, timeout))
            {
                mutex_unlock(&i2c_mtx[bus]);
                return 4;
            }
    }

    /* STOP */
    IICSTAT(bus) = 0xD0;
    IICCON(bus) = 0xB3;
    while ((IICSTAT(bus) & (1 << 5)) != 0)
        if (TIME_AFTER(current_tick, timeout))
        {
            mutex_unlock(&i2c_mtx[bus]);
            return 5;
        }
    
	i2c_off(bus);
    mutex_unlock(&i2c_mtx[bus]);
    return 0;
}

int i2c_read(int bus, unsigned char slave, int address, int len, unsigned char *data)
{
    mutex_lock(&i2c_mtx[bus]);
	i2c_on(bus);
    long timeout = current_tick + HZ / 50;

    if (address >= 0) {
        /* START */
        IICDS(bus) = slave & ~1;
        IICSTAT(bus) = 0xF0;
        IICCON(bus) = 0xB3;
        while ((IICCON(bus) & 0x10) == 0)
            if (TIME_AFTER(current_tick, timeout))
            {
                mutex_unlock(&i2c_mtx[bus]);
                return 1;
            }

        /* write address */
        IICDS(bus) = address;
        IICCON(bus) = 0xB3;
        while ((IICCON(bus) & 0x10) == 0)
            if (TIME_AFTER(current_tick, timeout))
            {
                mutex_unlock(&i2c_mtx[bus]);
                return 2;
            }
    }
    
    /* (repeated) START */
    IICDS(bus) = slave | 1;
    IICSTAT(bus) = 0xB0;
    IICCON(bus) = 0xB3;
    while ((IICCON(bus) & 0x10) == 0)
        if (TIME_AFTER(current_tick, timeout))
        {
            mutex_unlock(&i2c_mtx[bus]);
            return 3;
        }
    
    while (len--) {
        IICCON(bus) = (len == 0) ? 0x33 : 0xB3; /* NAK or ACK */
        while ((IICCON(bus) & 0x10) == 0)
            if (TIME_AFTER(current_tick, timeout))
            {
                mutex_unlock(&i2c_mtx[bus]);
                return 4;
            }
        *data++ = IICDS(bus);
    }

    /* STOP */
    IICSTAT(bus) = 0x90;
    IICCON(bus) = 0xB3;
    while ((IICSTAT(bus) & (1 << 5)) != 0)
        if (TIME_AFTER(current_tick, timeout))
        {
            mutex_unlock(&i2c_mtx[bus]);
            return 5;
        }
    
	i2c_off(bus);
    mutex_unlock(&i2c_mtx[bus]);
    return 0;
}

