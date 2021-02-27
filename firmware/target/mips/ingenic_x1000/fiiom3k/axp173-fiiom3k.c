/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "system.h"
#include "axp173.h"
#include "i2c-x1000.h"
#include "gpio-x1000.h"

#define AXP173_CHN  2
#define AXP173_ADDR 0x34

#define AXP173_IRQ_PORT GPIO_B
#define AXP173_IRQ_PIN  (1 << 10)

void GPIOB10(void)
{
    /* TODO this will also benefit from asynchronous I2C bus */
}

void axp173_init(void)
{
    /* Configure I2C bus */
    i2c_set_freq(2, I2C_FREQ_400K);

    /* Turn on all power outputs */
    axp173_reg_set(0x12, 0x5f);
    axp173_reg_set(0x80, 0xc0);

    /* Set ADC sample rate to 25 Hz (slowest rate supported) */
    axp173_reg_clrset(0x84, 0xc0, 0);

    /* Configure used ADCs: battery voltage and current */
    axp173_reg_write(0x82, 0xc1);

    /* Setup IRQ line */
    gpio_config(AXP173_IRQ_PORT, AXP173_IRQ_PIN, GPIO_IRQ_EDGE(0));

    /* Short delay to give power outputs time to stabilize */
    mdelay(5);
}

void axp173_irq_enable(bool en)
{
    if(en)
        gpio_enable_irq(AXP173_IRQ_PORT, AXP173_IRQ_PIN);
    else
        gpio_disable_irq(AXP173_IRQ_PORT, AXP173_IRQ_PIN);
}

int axp173_reg_clrset(int reg, int clr, int set)
{
    unsigned char buf[2];
    int ret;

    i2c_lock(AXP173_CHN);

    buf[0] = reg & 0xff;
    if(ret = i2c_write(AXP173_CHN, AXP173_ADDR,
                       I2C_CONTINUE, 1, &buf[0]))
        goto _end;
    if(ret = i2c_read(AXP173_CHN, AXP173_ADDR,
                      I2C_RESTART|I2C_STOP, 1, &buf[1]))
        goto _end;

    /* Perform the update */
    ret = buf[1];
    buf[1] &= ~clr;
    buf[1] |= set;
    if(buf[1] == ret)
        goto _end;

    if(ret = i2c_write(AXP173_CHN, AXP173_ADDR,
                       I2C_CONTINUE, 2, &buf[0]))
        goto _end;

  _end:
    i2c_unlock(AXP173_CHN);
    return ret;
}

int axp173_reg_read_multi(int reg, int count, unsigned char* buf)
{
    if(count == 0)
        return 0;

    unsigned char addr = reg;
    int ret;

    i2c_lock(AXP173_CHN);

    if(ret = i2c_write(AXP173_CHN, AXP173_ADDR,
                       I2C_CONTINUE, 1, &addr))
        goto _end;
    if(ret = i2c_read(AXP173_CHN, AXP173_ADDR,
                      I2C_RESTART|I2C_STOP, count, buf))
        goto _end;

  _end:
    i2c_unlock(AXP173_CHN);
    return ret;
}
