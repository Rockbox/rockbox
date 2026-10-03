/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#include "clocking-s5l8702.h"

/*  Driver for the s5l8702 built-in I2C controller in master mode

    Both the i2c_read and i2c_write function take the following arguments:
    * slave, the address of the i2c slave device to read from / write to
    * address, optional sub-address in the i2c slave (unused if -1)
    * len, number of bytes to be transfered
    * data, pointer to data to be transfered
    A return value > 0 indicates an error.

    Note:
    * blocks the calling thread for the entire duraton of the i2c transfer.
*/

static struct mutex i2c_mtx[2];

void i2c_init()
{
    mutex_init(&i2c_mtx[0]);
    mutex_init(&i2c_mtx[1]);
}

static bool wait_rdy(int bus)
{
    for (int i = 0; i < 100000 && IICUNK10(bus); i++)
        ;
    return IICUNK10(bus) == 0;
}

static void i2c_on(int bus)
{
    /* enable I2C clock */
    clockgate_enable(I2CCLKGATE(bus), true);
#if CONFIG_CPU == S5L8720
    /* this clockgate is needed when ECLK is used as source clock */
    clockgate_enable(I2CCLKGATE_2(bus), true);
    udelay(5);
#endif
}

static bool i2c_off(int bus)
{
    /* serial output off */
    if (!wait_rdy(bus))
        return false;
    IICSTAT(bus) = 0;
    /* disable I2C clock */
    if (!wait_rdy(bus))
        return false;
    clockgate_enable(I2CCLKGATE(bus), false);
#if CONFIG_CPU == S5L8720
    clockgate_enable(I2CCLKGATE_2(bus), false);
#endif
    return true;
}

/* wait for bus not busy, or tx/rx byte (should return once
   8 data + 1 ack clocks are generated), or STOP. */
static bool i2c_wait_io(int bus)
{
    for (int i = 0; i < 100000; i++) {
        if ((IICSTAT(bus) & (1 << 5)) == 0 ||
            (IICSTA2(bus) & ((1 << 8)|(1 << 13))) != 0) {
            IICSTA2(bus) |= (1 << 8)|(1 << 13);
            return true;
        }
        if (!wait_rdy(bus))
            return false;
    }
    return false;
}

static int i2c_start(int bus, unsigned char slave, bool rd)
{
    /* configure port */
    if (!wait_rdy(bus))
        return 2;
    IICCON(bus) = (0 << 8) | /* INT_EN = disabled */
                  (1 << 7) | /* ACK_GEN */
                  (0 << 6) | /* CLKSEL = SRCCLK/32 (TBC) */
#if CONFIG_CPU == S5L8702
                  (0 << 0);  /* CK_REG */
#elif CONFIG_CPU == S5L8720
                  (1 << 0);  /* CK_REG */
#endif

    /* START */
    int mode = rd ? 0x80 : 0xC0;
    if (!wait_rdy(bus))
        return 2;
    IICSTAT(bus) = mode;
    if (!wait_rdy(bus))
        return 2;
    IICDS(bus) = slave | rd;
    if (!wait_rdy(bus))
        return 2;
    IICSTAT(bus) = mode | 0x30;

#if CONFIG_CPU == S5L8702
    /* Nano 3G needs this synchronization when ECLK is used. */
    if (!wait_rdy(bus))
        return 2;
#endif
    if (!i2c_wait_io(bus))
        return 2;

    /* check ACK */
    if (IICSTAT(bus) & 1)
        return 1;

    return 0;
}

static bool i2c_stop(int bus)
{
    /* STOP */
    if (!wait_rdy(bus))
        return false;
    IICSTAT(bus) &= ~0x20;
    if (!wait_rdy(bus))
        return false;
    IICCON(bus) = 0x10;
    return i2c_wait_io(bus);
}

static int i2c_wr_internal(int bus, unsigned char slave,
                    int address, int len, const unsigned char *data)
{
    int rc = 0;

    if (i2c_start(bus, slave, false) == 0)
    {
        /* write address + data */
        const unsigned char *ptr = data;
        const unsigned char addr = address;
        if (address >= 0) {
            ptr = &addr;
            len++;
        }
        while (len--) {
            if (!wait_rdy(bus)) {
                rc = 3;
                break;
            }
            IICDS(bus) = *ptr;
            udelay(5);
            if (!wait_rdy(bus)) {
                rc = 3;
                break;
            }
            IICCON(bus) = IICCON(bus);
            if (!i2c_wait_io(bus)) {
                rc = 3;
                break;
            }
            /* check ACK */
            if (IICSTAT(bus) & 1) {
                rc = 2;
                break;
            }
            if (ptr == &addr) ptr = data;
            else ptr++;
        }
    }
    else
        rc = 1;

    if (!i2c_stop(bus) && rc == 0)
        rc = 3;
    return rc;
}

static int i2c_rd_internal(int bus, unsigned char slave, int len, unsigned char *data)
{
    int rc = 0;

    if (i2c_start(bus, slave, true) == 0)
    {
        while (len--) {
            if (!wait_rdy(bus)) {
                rc = 4;
                break;
            }
            IICCON(bus) &= ~(len ? 0 : 0x80); /* ACK or NAK */
            if (!i2c_wait_io(bus)) {
                rc = 4;
                break;
            }
            *data++ = IICDS(bus);
        }
    }
    else
        rc = 3;

    if (!i2c_stop(bus) && rc == 0)
        rc = 4;
    return rc;
}

int i2c_wr(int bus, unsigned char slave, int address, int len, const unsigned char *data)
{
    i2c_on(bus);
    int rc = i2c_wr_internal(bus, slave, address, len, data);
    if (!i2c_off(bus) && rc == 0)
        rc = 5;
    return rc;
}

int i2c_rd(int bus, unsigned char slave, int address, int len, unsigned char *data)
{
    i2c_on(bus);
    int rc = i2c_wr_internal(bus, slave, address, 0, NULL);
    if (rc == 0)
        rc = i2c_rd_internal(bus, slave, len, data);
    if (!i2c_off(bus) && rc == 0)
        rc = 5;
    return rc;
}

int i2c_write(int bus, unsigned char slave, int address, int len, const unsigned char *data)
{
    int ret;
    mutex_lock(&i2c_mtx[bus]);
    ret = i2c_wr(bus, slave, address, len, data);
    mutex_unlock(&i2c_mtx[bus]);
    return ret;
}

int i2c_read(int bus, unsigned char slave, int address, int len, unsigned char *data)
{
    int ret;
    mutex_lock(&i2c_mtx[bus]);
    ret = i2c_rd(bus, slave, address, len, data);
    mutex_unlock(&i2c_mtx[bus]);
    return ret;
}

int i2c_preinit(int bus)
{
#if CONFIG_CPU == S5L8702
    if (bus == 0)
        PCON3 = (PCON3 & ~0x00000ff0) | 0x00000220;
#if 0 /* TBC */
    else if (bus == 1)
        PCON6 = (PCON6 & ~0x0ff00000) | 0x02200000;
#endif
#elif CONFIG_CPU == S5L8720
    if (bus == 0) {
        PCON2 = (PCON2 & ~0xf0000000) | 0x20000000;
        PCON3 = (PCON3 & ~0x0000000f) | 0x00000002;
    }
#endif
    i2c_on(bus);
    if (!wait_rdy(bus))
        goto fail;
    IICADD(bus) = 0x40;   /* own slave address */
    if (!wait_rdy(bus))
        goto fail;
    IICUNK14(bus) = 1;    /* SRCCLK = ECLK */
    if (!wait_rdy(bus))
        goto fail;
    IICUNK18(bus) = 0;
    if (!wait_rdy(bus))
        goto fail;
    IICSTAT(bus) = 0x80;  /* master Rx mode, serial output off */
    if (!wait_rdy(bus))
        goto fail;
    IICCON(bus) = 0;
    if (!wait_rdy(bus))
        goto fail;
    IICSTA2(bus) = 0x3f00;
    if (!i2c_off(bus))
        goto fail;
    return 0;

fail:
    clockgate_enable(I2CCLKGATE(bus), false);
#if CONFIG_CPU == S5L8720
    clockgate_enable(I2CCLKGATE_2(bus), false);
#endif
    return -1;
}
