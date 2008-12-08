/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Bertrik Sikken
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
    This is the fmradio_i2c interface, used by the radio driver
    to communicate with the radio tuner chip.
    
    It is implemented using the generic i2c driver, which does "bit-banged"
    I2C with a couple of GPIO pins.
 */

#include "as3525.h"
#include "generic_i2c.h"
#include "fmradio_i2c.h"

#define I2C_SCL_PIN 4
#define I2C_SDA_PIN 5

static void fm_scl_hi(void)
{
    GPIOB_PIN(I2C_SCL_PIN) = 1 << I2C_SCL_PIN;
}

static void fm_scl_lo(void)
{
    GPIOB_PIN(I2C_SCL_PIN) = 0;
}

static void fm_sda_hi(void)
{
    GPIOB_PIN(I2C_SDA_PIN) = 1 << I2C_SDA_PIN;
}

static void fm_sda_lo(void)
{
    GPIOB_PIN(I2C_SDA_PIN) = 0;
}

static void fm_sda_input(void)
{
    GPIOB_DIR &= ~(1 << I2C_SDA_PIN);
}

static void fm_sda_output(void)
{
    GPIOB_DIR |= 1 << I2C_SDA_PIN;
}

static void fm_scl_input(void)
{
    GPIOB_DIR &= ~(1 << I2C_SCL_PIN);
}

static void fm_scl_output(void)
{
    GPIOB_DIR |= 1 << I2C_SCL_PIN;
}

static int fm_sda(void)
{
    return GPIOB_PIN(I2C_SDA_PIN);
}

static int fm_scl(void)
{
    return GPIOB_PIN(I2C_SCL_PIN);
}

/* simple and crude delay, used for all delays in the generic i2c driver */
static void fm_delay(void)
{
    volatile int i;
    
    /* this loop is uncalibrated and could use more sophistication */
    for (i = 0; i < 100; i++) {
    }
}

/* interface towards the generic i2c driver */
static struct i2c_interface fm_i2c_interface = {
    .address = 0x10 << 1,
    
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
    i2c_add_node(&fm_i2c_interface);
}

int fmradio_i2c_write(unsigned char address, const unsigned char* buf, int count)
{
    return i2c_write_data(address, -1, buf, count);
}

int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
    return i2c_read_data(address, -1, buf, count);
}



