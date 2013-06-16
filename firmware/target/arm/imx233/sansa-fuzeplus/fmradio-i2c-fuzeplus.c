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
#include "rds.h"
#include "si4700.h"

/**
 * Sansa Fuze+ fmradio uses the following pins:
 * - B0P29 as CE apparently (active high)
 * - B1P24 as SDA
 * - B1P22 as SCL
 * - B2P27 as STC/RDS
 */
static int fmradio_i2c_bus = -1;

static void i2c_scl_dir(bool out)
{
    imx233_pinctrl_enable_gpio(1, 22, out);
}

static void i2c_sda_dir(bool out)
{
    imx233_pinctrl_enable_gpio(1, 24, out);
}

static void i2c_scl_out(bool high)
{
    imx233_pinctrl_set_gpio(1, 22, high);
}

static void i2c_sda_out(bool high)
{
    imx233_pinctrl_set_gpio(1, 24, high);
}

static bool i2c_scl_in(void)
{
    return imx233_pinctrl_get_gpio_mask(1, 1 << 22);
}

static bool i2c_sda_in(void)
{
    return imx233_pinctrl_get_gpio_mask(1, 1 << 24);
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
    imx233_pinctrl_acquire(1, 24, "fmradio i2c");
    imx233_pinctrl_acquire(1, 22, "fmradio i2c");
    imx233_pinctrl_set_function(1, 24, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_function(1, 22, PINCTRL_FUNCTION_GPIO);
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

#ifdef HAVE_RDS_CAP
/* Low-level RDS Support */
static struct semaphore rds_sema;
static uint32_t rds_stack[DEFAULT_STACK_SIZE / sizeof(uint32_t)];

/* RDS GPIO interrupt handler */
static void stc_rds_callback(int bank, int pin, intptr_t user)
{
    (void) bank;
    (void) pin;
    (void) user;

    semaphore_release(&rds_sema);
}

/* Captures RDS data and processes it */
static void NORETURN_ATTR rds_thread(void)
{
    uint16_t rds_data[4];

    while(true)
    {
        semaphore_wait(&rds_sema, TIMEOUT_BLOCK);
        if(si4700_rds_read_raw(rds_data) && rds_process(rds_data))
            si4700_rds_set_event();
        /* renable callback */
        imx233_pinctrl_setup_irq(2, 27, true, true, false, &stc_rds_callback, 0);
    }
}

/* true after full radio power up, and false before powering down */
void si4700_rds_powerup(bool on)
{
    if(on)
    {
        imx233_pinctrl_acquire(2, 27, "tuner stc/rds");
        imx233_pinctrl_set_function(2, 27, PINCTRL_FUNCTION_GPIO);
        imx233_pinctrl_enable_gpio(2, 27, false);
        /* pin is set to 0 when an RDS packet has arrived */
        imx233_pinctrl_setup_irq(2, 27, true, true, false, &stc_rds_callback, 0);
    }
    else
    {
        imx233_pinctrl_setup_irq(2, 27, false, false, false, NULL, 0);
        imx233_pinctrl_release(2, 27, "tuner stc/rds");
    }
}

/* One-time RDS init at startup */
void si4700_rds_init(void)
{
    semaphore_init(&rds_sema, 1, 0);
    rds_init();
    create_thread(rds_thread, rds_stack, sizeof(rds_stack), 0, "rds"
        IF_PRIO(, PRIORITY_REALTIME) IF_COP(, CPU));
}
#endif /* HAVE_RDS_CAP */
