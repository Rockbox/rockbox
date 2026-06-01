/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Aidan MacDonald
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
#include "i2c-stm32h7.h"
#include "clock-echoplayer.h"
#include "nvic-arm.h"
#include "regs/stm32h743/i2c.h"

static const struct stm32_i2c_config i2c1_conf INITDATA_ATTR = {
    .instance = ITA_I2C1,
    .ker_clock = &i2c1_ker_clock,
    .bus_freq_hz = 400000,
    .scl_low_min_ns = 1300,
    .scl_high_min_ns = 600,
    .t_vd_dat_max_ns = 900,
    .t_su_dat_max_ns = 100,
    .rise_time_max_ns = 300,
    .fall_time_max_ns = 300,
};

struct stm32_i2c_controller i2c1_ctl;

void i2c_init(void)
{
    stm32_i2c_init(&i2c1_ctl, &i2c1_conf);
    nvic_enable_irq(NVIC_IRQN_I2C1_EV);
    nvic_enable_irq(NVIC_IRQN_I2C1_ER);
}

void i2c1_irq_handler(void)
{
    stm32_i2c_irq_handler(&i2c1_ctl);
}
