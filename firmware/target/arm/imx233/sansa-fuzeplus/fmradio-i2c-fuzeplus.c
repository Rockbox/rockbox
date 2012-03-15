/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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

/**
 * Sansa Fuze+ fmradio uses the following pins:
 * - B0P29 as CE apparently (active high)
 * - B1P24 as SDA
 * - B1P22 as SCL
 */
static int fmradio_i2c_bus = -1;

static void i2c_scl_dir(bool out)
{
    imx233_enable_gpio_output(1, 22, out);
}

static void i2c_sda_dir(bool out)
{
    imx233_enable_gpio_output(1, 24, out);
}

static void i2c_scl_out(bool high)
{
    imx233_set_gpio_output(1, 22, high);
}

static void i2c_sda_out(bool high)
{
    imx233_set_gpio_output(1, 24, high);
}

static bool i2c_scl_in(void)
{
    return imx233_get_gpio_input_mask(1, 1 << 22);
}

static bool i2c_sda_in(void)
{
    return imx233_get_gpio_input_mask(1, 1 << 24);
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
    imx233_set_pin_function(1, 24, PINCTRL_FUNCTION_GPIO);
    imx233_set_pin_function(1, 22, PINCTRL_FUNCTION_GPIO);
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
