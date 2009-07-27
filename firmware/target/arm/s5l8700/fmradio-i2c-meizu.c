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

/*
    FM radio i2c interface, allows the radio driver to talk to the tuner chip.

    It is implemented using the generic i2c driver, which does "bit-banged"
    I2C with a couple of GPIO pins.
 */

#include "config.h"

#include "inttypes.h"
#include "s5l8700.h"
#include "generic_i2c.h"
#include "fmradio_i2c.h"

#define I2C_GPIO        PDAT3
#define I2C_GPIO_DIR    PCON3
#define I2C_SCL_PIN     4
#define I2C_SDA_PIN     2

#define SCL_DIR_MASK    (0xF<<(4*I2C_SCL_PIN))
#define SCL_DIR_OUT     (1<<(4*I2C_SCL_PIN))
#define SCL_DIR_IN      0
#define SDA_DIR_MASK    (0xF<<(4*I2C_SDA_PIN))
#define SDA_DIR_OUT     (1<<(4*I2C_SDA_PIN))
#define SDA_DIR_IN      0

static int fm_i2c_bus;

static void fm_scl_hi(void)
{
    I2C_GPIO |= 1 << I2C_SCL_PIN;
}

static void fm_scl_lo(void)
{
    I2C_GPIO &= ~(1 << I2C_SCL_PIN);
}

static void fm_sda_hi(void)
{
    I2C_GPIO |= 1 << I2C_SDA_PIN;
}

static void fm_sda_lo(void)
{
    I2C_GPIO &= ~(1 << I2C_SDA_PIN);
}

static void fm_sda_input(void)
{
    I2C_GPIO_DIR = (I2C_GPIO_DIR & ~SDA_DIR_MASK) | SDA_DIR_IN;
}

static void fm_sda_output(void)
{
    I2C_GPIO_DIR = (I2C_GPIO_DIR & ~SDA_DIR_MASK) | SDA_DIR_OUT;
}

static void fm_scl_input(void)
{
    I2C_GPIO_DIR = (I2C_GPIO_DIR & ~SCL_DIR_MASK) | SCL_DIR_IN;
}

static void fm_scl_output(void)
{
    I2C_GPIO_DIR = (I2C_GPIO_DIR & ~SCL_DIR_MASK) | SCL_DIR_OUT;
}

static int fm_sda(void)
{
    return (I2C_GPIO & (1 << I2C_SDA_PIN));
}

static int fm_scl(void)
{
    return (I2C_GPIO & (1 << I2C_SCL_PIN));
}

/* simple and crude delay, used for all delays in the generic i2c driver */
static void fm_delay(void)
{
    volatile int i;

    /* this loop is uncalibrated and could use more sophistication */
    for (i = 0; i < 20; i++) {
    }
}

/* interface towards the generic i2c driver */
static const struct i2c_interface fm_i2c_interface = {
    .scl_hi = fm_scl_hi,
    .scl_lo = fm_scl_lo,
    .sda_hi = fm_sda_hi,
    .sda_lo = fm_sda_lo,
    .sda_input = fm_sda_input,
    .sda_output = fm_sda_output,
    .scl_input = fm_scl_input,
    .scl_output = fm_scl_output,
    .scl = fm_scl,
    .sda = fm_sda,

    .delay_hd_sta = fm_delay,
    .delay_hd_dat = fm_delay,
    .delay_su_dat = fm_delay,
    .delay_su_sto = fm_delay,
    .delay_su_sta = fm_delay,
    .delay_thigh = fm_delay
};

/* initialise i2c for fmradio */
void fmradio_i2c_init(void)
{
    fm_i2c_bus = i2c_add_node(&fm_i2c_interface);
}

int fmradio_i2c_write(unsigned char address, const unsigned char* buf, int count)
{
    return i2c_write_data(fm_i2c_bus, address, -1, buf, count);
}

int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
    return i2c_read_data(fm_i2c_bus, address, -1, buf, count);
}

