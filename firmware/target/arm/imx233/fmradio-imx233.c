/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#include "kernel.h"
#include "fmradio-imx233.h"
#include "fmradio-target.h"
#include "pinctrl-imx233.h"
#include "i2c-imx233.h"
#include "generic_i2c.h"

#ifndef IMX233_FMRADIO_I2C
#error You must define IMX233_FMRADIO_I2C in fmradio-target.h
#endif

#ifndef IMX233_FMRADIO_POWER
#error You must define IMX233_FMRADIO_POWER in fmradio-target.h
#endif

/** Hardware I2C */
#if IMX233_FMRADIO_I2C == FMI_HW
void fmradio_i2c_init(void)
{
}

int fmradio_i2c_write(unsigned char address, const unsigned char* buf, int count)
{
    return i2c_write(address, buf, count);
}

int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
    return i2c_read(address, buf, count);
}
/** Software I2C */
#elif IMX233_FMRADIO_I2C == FMI_SW
#if !defined(FMI_SW_SDA_BANK) || !defined(FMI_SW_SDA_PIN) || \
    !defined(FMI_SW_SCL_BANK) || !defined(FMI_SW_SCL_PIN)
#error You must define FMI_SW_SDA_BANK, FMI_SW_SDA_PIN, FMI_SW_SCL_BANK and FMI_SW_SCL_PIN
#endif
static int fmradio_i2c_bus = -1;

static void i2c_scl_dir(bool out)
{
    imx233_pinctrl_enable_gpio(FMI_SW_SCL_BANK, FMI_SW_SCL_PIN, out);
}

static void i2c_sda_dir(bool out)
{
    imx233_pinctrl_enable_gpio(FMI_SW_SDA_BANK, FMI_SW_SDA_PIN, out);
}

static void i2c_scl_out(bool high)
{
    imx233_pinctrl_set_gpio(FMI_SW_SCL_BANK, FMI_SW_SCL_PIN, high);
}

static void i2c_sda_out(bool high)
{
    imx233_pinctrl_set_gpio(FMI_SW_SDA_BANK, FMI_SW_SDA_PIN, high);
}

static bool i2c_scl_in(void)
{
    return imx233_pinctrl_get_gpio(FMI_SW_SCL_BANK, FMI_SW_SCL_PIN);
}

static bool i2c_sda_in(void)
{
    return imx233_pinctrl_get_gpio(FMI_SW_SDA_BANK, FMI_SW_SDA_PIN);
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
    imx233_pinctrl_acquire(FMI_SW_SDA_BANK, FMI_SW_SDA_PIN, "fmradio_i2c_sda");
    imx233_pinctrl_acquire(FMI_SW_SCL_BANK, FMI_SW_SCL_BANK, "fmradio_i2c_scl");
    imx233_pinctrl_set_function(FMI_SW_SDA_BANK, FMI_SW_SDA_PIN, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_function(FMI_SW_SCL_BANK, FMI_SW_SCL_BANK, PINCTRL_FUNCTION_GPIO);
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
#else
#error Invalid value for IMX233_FMRADIO_I2C
#endif

static bool tuner_enable = false;

bool tuner_power(bool enable)
{
    if(enable != tuner_enable)
    {
        tuner_enable = enable;
#if IMX233_FMRADIO_POWER == FMP_GPIO
        static bool init = false;
        if(!init)
        {
            /* CE is B0P29 (active high) */
            imx233_pinctrl_acquire(FMP_GPIO_BANK, FMP_GPIO_PIN, "tuner_power");
            imx233_pinctrl_set_function(FMP_GPIO_BANK, FMP_GPIO_PIN, PINCTRL_FUNCTION_GPIO);
            imx233_pinctrl_enable_gpio(FMP_GPIO_BANK, FMP_GPIO_PIN, true);
        }
        imx233_pinctrl_set_gpio(FMP_GPIO_BANK, FMP_GPIO_PIN, enable);
        /* power up delay */
        sleep(FMP_GPIO_DELAY);
#elif IMX233_FMRADIO_POWER == FMP_NONE
#else
#error Invalid value for IMX233_FMRADIO_POWER
#endif
    }
    return tuner_enable;
}

bool tuner_powered(void)
{
    return tuner_enable;
}
