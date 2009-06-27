/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "i2c-s5l8700.h"

/*  Driver for the s5l8700 built-in I2C controller in master mode
    
    Both the i2c_read and i2c_write function take the following arguments:
    * slave, the address of the i2c slave device to read from / write to
    * address, optional sub-address in the i2c slave (unused if -1)
    * len, number of bytes to be transfered
    * data, pointer to data to be transfered
    A return value < 0 indicates an error.
    
    Known issues:
    * uses polled mode (not interrupt driven), just like the OF
    * ACK from slave is not checked, so functions never return an error
*/

static struct mutex i2c_mtx;

void i2c_init(void)
{
    mutex_init(&i2c_mtx);

    /* enable I2C pins */
    PCON10 = (2 << 2) |
             (2 << 0);

    /* enable I2C clock */
    PWRCON &= ~(1 << 5);

    /* initial config */
    IICADD = 0;
    IICCON = (1 << 7) | /* ACK_GEN */
             (1 << 6) | /* CLKSEL = PCLK/512 */
             (1 << 5) | /* INT_EN */
             (1 << 4) | /* IRQ clear */
             (3 << 0);  /* CK_REG */

    /* serial output on */
    IICSTAT = (1 << 4);
}

int i2c_write(unsigned char slave, int address, int len, const unsigned char *data)
{
    mutex_lock(&i2c_mtx);

    /* START */
    IICDS = slave & ~1;
    IICSTAT = 0xF0;
    IICCON = 0xF3;
    while ((IICCON & (1 << 4)) == 0);
    
    if (address >= 0) {
        /* write address */
        IICDS = address;
        IICCON = 0xF3;
        while ((IICCON & (1 << 4)) == 0);
    }
    
    /* write data */
    while (len--) {
        IICDS = *data++;
        IICCON = 0xF3;
        while ((IICCON & (1 << 4)) == 0);
    }

    /* STOP */
    IICSTAT = 0xD0;
    IICCON = 0xF3;
    while ((IICSTAT & (1 << 5)) != 0);
    
    mutex_unlock(&i2c_mtx);
    return 0;
}

int i2c_read(unsigned char slave, int address, int len, unsigned char *data)
{
    mutex_lock(&i2c_mtx);

    if (address >= 0) {
        /* START */
        IICDS = slave & ~1;
        IICSTAT = 0xF0;
        IICCON = 0xF3;
        while ((IICCON & (1 << 4)) == 0);

        /* write address */
        IICDS = address;
        IICCON = 0xF3;
        while ((IICCON & (1 << 4)) == 0);
    }
    
    /* (repeated) START */
    IICDS = slave | 1;
    IICSTAT = 0xB0;
    IICCON = 0xF3;
    while ((IICCON & (1 << 4)) == 0);
    
    while (len--) {
        IICCON = (len == 0) ? 0x73 : 0xF3; /* NACK or ACK */
        while ((IICCON & (1 << 4)) == 0);
        *data++ = IICDS;
    }

    /* STOP */
    IICSTAT = 0x90;
    IICCON = 0xF3;
    while ((IICSTAT & (1 << 5)) != 0);
    
    mutex_unlock(&i2c_mtx);
    return 0;
}

