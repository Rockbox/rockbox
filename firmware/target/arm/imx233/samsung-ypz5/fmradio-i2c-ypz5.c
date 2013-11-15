/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Lorenzo Miori
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
#include "fmradio_i2c.h"
#include "pinctrl-imx233.h"
#include "generic_i2c.h"
#include "rds.h"
//#include "i2c.h"

#warning fixme radio hangs at startup if using I2C proper interface

/* I2C proper interface is disabled for the moment due to some problems */

#if 0
void fmradio_i2c_init(void)
{
}

int fmradio_i2c_write(unsigned char address, const unsigned char* buf, int count)
{
//    return -1;
    return i2c_write(address, buf, count);
}
 
int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
//    return -1;
    return i2c_read(address, buf, count);
}
#endif

static int fmradio_i2c_bus = -1;

static void i2c_scl_dir(bool out)
{
    imx233_pinctrl_enable_gpio(3, 17, out);
}

static void i2c_sda_dir(bool out)
{
    imx233_pinctrl_enable_gpio(3, 18, out);
}

static void i2c_scl_out(bool high)
{
    imx233_pinctrl_set_gpio(3, 17, high);
}

static void i2c_sda_out(bool high)
{
    imx233_pinctrl_set_gpio(3, 18, high);
}

static bool i2c_scl_in(void)
{
    return imx233_pinctrl_get_gpio(3, 17);
}

static bool i2c_sda_in(void)
{
    return imx233_pinctrl_get_gpio(3, 18);
}

static void i2c_delay(int d)
{
    udelay(d);
}

struct i2c_interface fmradio_i2c =
{
    .scl_dir = i2c_scl_dir,
    .sda_dir = i2c_sda_dir,
    .scl_out = i2c_scl_out,
    .sda_out = i2c_sda_out,
    .scl_in = i2c_scl_in,
    .sda_in = i2c_sda_in,
    .delay = i2c_delay,
    .delay_hd_sta = 4,
    .delay_hd_dat = 5,
    .delay_su_dat = 1,
    .delay_su_sto = 4,
    .delay_su_sta = 5,
    .delay_thigh = 4
};

void fmradio_i2c_init(void)
{
    imx233_pinctrl_acquire(3, 17, "i2c scl");
    imx233_pinctrl_acquire(3, 18, "i2c sda");
    imx233_pinctrl_set_function(3, 17, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_drive(3, 17, PINCTRL_DRIVE_4mA);
    imx233_pinctrl_set_function(3, 18, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_drive(3, 18, PINCTRL_DRIVE_4mA);
    fmradio_i2c_bus = i2c_add_node(&fmradio_i2c);
}

int fmradio_i2c_write(unsigned char address, const unsigned char* buf, int count)
{
    return i2c_write_data(fmradio_i2c_bus, address, -1, buf, count);
}

int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
    return i2c_read_data(fmradio_i2c_bus, address, -1, buf, count);
}
