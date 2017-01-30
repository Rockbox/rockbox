/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Bertrik Sikken
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

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "tuner.h"
#include "generic_i2c.h"
#include "fmradio_i2c.h"
#include "thread.h"

#if     defined(SANSA_CLIP) || defined(SANSA_C200V2)
#define I2C_SCL_GPIO(x) GPIOB_PIN(x)
#define I2C_SDA_GPIO(x) GPIOB_PIN(x)
#define I2C_SCL_GPIO_DIR GPIOB_DIR
#define I2C_SDA_GPIO_DIR GPIOB_DIR
#define I2C_SCL_PIN 4
#define I2C_SDA_PIN 5

#elif   defined(SANSA_CLIPV2) || defined(SANSA_CLIPPLUS) \
     || defined(SANSA_CLIPZIP)
#define I2C_SCL_GPIO(x) GPIOB_PIN(x)
#define I2C_SDA_GPIO(x) GPIOB_PIN(x)
#define I2C_SCL_GPIO_DIR GPIOB_DIR
#define I2C_SDA_GPIO_DIR GPIOB_DIR
#define I2C_SCL_PIN 6
#define I2C_SDA_PIN 7

#elif   defined(SANSA_M200V4)
#define I2C_SCL_GPIO(x) GPIOD_PIN(x)
#define I2C_SDA_GPIO(x) GPIOD_PIN(x)
#define I2C_SCL_GPIO_DIR GPIOD_DIR
#define I2C_SDA_GPIO_DIR GPIOD_DIR
#define I2C_SCL_PIN 7
#define I2C_SDA_PIN 6

#elif   defined(SANSA_FUZE) || defined(SANSA_E200V2)
#define I2C_SCL_GPIO(x) GPIOA_PIN(x)
#define I2C_SDA_GPIO(x) GPIOA_PIN(x)
#define I2C_SCL_GPIO_DIR GPIOA_DIR
#define I2C_SDA_GPIO_DIR GPIOA_DIR
#define I2C_SCL_PIN 6
#define I2C_SDA_PIN 7

#elif   defined(SANSA_FUZEV2)
#define I2C_SCL_GPIO(x) GPIOB_PIN(x)
#define I2C_SDA_GPIO(x) GPIOA_PIN(x)
#define I2C_SCL_GPIO_DIR GPIOB_DIR
#define I2C_SDA_GPIO_DIR GPIOA_DIR
#define I2C_SCL_PIN 1
#define I2C_SDA_PIN 0

#else
#error no FM I2C GPIOPIN defines
#endif

static int fm_i2c_bus;

static void fm_scl_dir(bool out)
{
    if (out) {
        I2C_SCL_GPIO_DIR |= 1 << I2C_SCL_PIN;
    } else {
        I2C_SCL_GPIO_DIR &= ~(1 << I2C_SCL_PIN);
    }
}

static void fm_sda_dir(bool out)
{
    if (out) {
        I2C_SDA_GPIO_DIR |= 1 << I2C_SDA_PIN;
    } else {
        I2C_SDA_GPIO_DIR &= ~(1 << I2C_SDA_PIN);
    }
}

static void fm_scl_out(bool level)
{
    if (level) {
        I2C_SCL_GPIO(I2C_SCL_PIN) = 1 << I2C_SCL_PIN;
    } else {
        I2C_SCL_GPIO(I2C_SCL_PIN) = 0;
    }
}

static void fm_sda_out(bool level)
{
    if (level) {
        I2C_SDA_GPIO(I2C_SDA_PIN) = 1 << I2C_SDA_PIN;
    } else {
        I2C_SDA_GPIO(I2C_SDA_PIN) = 0;
    }
}

static bool fm_scl_in(void)
{
    return I2C_SCL_GPIO(I2C_SCL_PIN);
}

static bool fm_sda_in(void)
{
    return I2C_SDA_GPIO(I2C_SDA_PIN);
}

static void fm_delay(int delay)
{
    if (delay != 0) {
        udelay(delay);
    }
}

/* interface towards the generic i2c driver */
static const struct i2c_interface fm_i2c_interface = {
    .scl_out = fm_scl_out,
    .scl_dir = fm_scl_dir,
    .sda_out = fm_sda_out,
    .sda_dir = fm_sda_dir,
    .sda_in = fm_sda_in,
    .scl_in = fm_scl_in,
    .delay = fm_delay,

    .delay_hd_sta = 1,
    .delay_hd_dat = 0,
    .delay_su_dat = 1,
    .delay_su_sto = 1,
    .delay_su_sta = 1,
    .delay_thigh = 2
};

/* initialise i2c for fmradio */
void fmradio_i2c_init(void)
{
    fm_i2c_bus = i2c_add_node(&fm_i2c_interface);
}

int fmradio_i2c_write(unsigned char address, const unsigned char* buf, int count)
{
#ifdef SANSA_FUZEV2
    bitclr32(&CCU_IO, 1<<12);
#endif
    int ret = i2c_write_data(fm_i2c_bus, address, -1, buf, count);
#ifdef SANSA_FUZEV2
    bitset32(&CCU_IO, 1<<12);
#endif
    return ret;
}

int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
#ifdef SANSA_FUZEV2
    bitclr32(&CCU_IO, 1<<12);
#endif
    int ret = i2c_read_data(fm_i2c_bus, address, -1, buf, count);
#ifdef SANSA_FUZEV2
    bitset32(&CCU_IO, 1<<12);
#endif
    return ret;
}

#ifdef HAVE_RDS_CAP
/* Low-level RDS Support */
static struct semaphore rds_sema;
static uint32_t rds_stack[DEFAULT_STACK_SIZE/sizeof(uint32_t)];

/* RDS GPIO interrupt handler */
void tuner_isr(void)
{
    /* read and clear the interrupt */
    if (GPIOA_MIS & (1<<4)) {
        semaphore_release(&rds_sema);
        GPIOA_IC = (1<<4);
    }
}

/* Captures RDS data and processes it */
static void NORETURN_ATTR rds_thread(void)
{
    while (true) {
        semaphore_wait(&rds_sema, TIMEOUT_BLOCK);
        si4700_rds_process();
    }
}

/* Called with on=true after full radio power up, and with on=false before
   powering down */
void si4700_rds_powerup(bool on)
{
    GPIOA_IE &= ~(1<<4);    /* disable GPIO interrupt */

    if (on) {
        GPIOA_DIR &= ~(1<<4);   /* input */
        GPIOA_IS &= ~(1<<4);    /* edge detect */
        GPIOA_IBE &= ~(1<<4);   /* only one edge */
        GPIOA_IEV &= ~(1<<4);   /* falling edge */
        GPIOA_IC = (1<<4);      /* clear any pending interrupt */
        GPIOA_IE |= (1<<4);     /* enable GPIO interrupt */
    }
}

/* One-time RDS init at startup */
void si4700_rds_init(void)
{
    semaphore_init(&rds_sema, 1, 0);
    create_thread(rds_thread, rds_stack, sizeof(rds_stack), 0, "rds"
                  IF_PRIO(, PRIORITY_REALTIME) IF_COP(, CPU));
}
#endif /* HAVE_RDS_CAP */
