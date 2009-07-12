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
    
    Note:
    * blocks the calling thread for the entire duraton of the i2c transfer but
      uses wakeup_wait/wakeup_signal to allow other threads to run.
    * ACK from slave is not checked, so functions never return an error
*/

static struct mutex i2c_mtx;
static struct wakeup i2c_wakeup;

void INT_IIC(void)
{
    /* disable interrupt (but don't clear it yet) */
    IICCON &= ~((1 << 4) | (1 << 5));

    wakeup_signal(&i2c_wakeup);
}

void i2c_init(void)
{
    mutex_init(&i2c_mtx);
    wakeup_init(&i2c_wakeup);

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
    
    /* enable interrupt */
    INTMSK |= (1 << 27);
}

int i2c_write(unsigned char slave, int address, int len, const unsigned char *data)
{
    mutex_lock(&i2c_mtx);

    /* START */
    IICDS = slave & ~1;
    IICSTAT = 0xF0;
    IICCON = 0xF3;
    wakeup_wait(&i2c_wakeup, TIMEOUT_BLOCK);
    
    if (address >= 0) {
        /* write address */
        IICDS = address;
        IICCON = 0xF3;
        wakeup_wait(&i2c_wakeup, TIMEOUT_BLOCK);
    }
    
    /* write data */
    while (len--) {
        IICDS = *data++;
        IICCON = 0xF3;
        wakeup_wait(&i2c_wakeup, TIMEOUT_BLOCK);
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
        wakeup_wait(&i2c_wakeup, TIMEOUT_BLOCK);

        /* write address */
        IICDS = address;
        IICCON = 0xF3;
        wakeup_wait(&i2c_wakeup, TIMEOUT_BLOCK);
    }
    
    /* (repeated) START */
    IICDS = slave | 1;
    IICSTAT = 0xB0;
    IICCON = 0xF3;
    wakeup_wait(&i2c_wakeup, TIMEOUT_BLOCK);
    
    while (len--) {
        IICCON = (len == 0) ? 0x73 : 0xF3; /* NACK or ACK */
        wakeup_wait(&i2c_wakeup, TIMEOUT_BLOCK);
        *data++ = IICDS;
    }

    /* STOP */
    IICSTAT = 0x90;
    IICCON = 0xF3;
    while ((IICSTAT & (1 << 5)) != 0);
    
    mutex_unlock(&i2c_mtx);
    return 0;
}

