/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Aidan MacDonald
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
#ifndef __I2C_STM32H743_H__
#define __I2C_STM32H743_H__

#include "mutex.h"
#include "semaphore.h"
#include "clock-stm32h7.h"

struct stm32_i2c_config
{
    /* I2C instance address */
    uint32_t instance;

    /* I2C kernel clock */
    const struct stm32_clock *ker_clock;

    int bus_freq_hz;        /* Desired I2C bus frequency */
    int scl_low_min_ns;     /* Minimum SCL low time */
    int scl_high_min_ns;    /* Minimum SCL high time */
    int t_vd_dat_max_ns;    /* Maximum SDA valid time */
    int t_su_dat_max_ns;    /* Minimum SDA setup time */
    int rise_time_max_ns;   /* Worst-case rise time for SCL & SDA */
    int fall_time_max_ns;   /* Worst-case fall time for SCL & SDA */
};

struct stm32_i2c_controller
{
    uint32_t regs;
    const struct stm32_clock *ker_clock;

    struct mutex bus_lock;
    struct semaphore bus_sem;

    int error;

    uint8_t *xfer_buf;
    size_t xfer_count;

    uint8_t reg_addr;
    uint8_t flags;
};

void stm32_i2c_init(struct stm32_i2c_controller *ctl,
                    const struct stm32_i2c_config *cfg);

int stm32_i2c_read_mem(struct stm32_i2c_controller *ctl,
                       uint8_t dev_addr,
                       uint8_t reg_addr,
                       void *data, size_t nbytes);
int stm32_i2c_write_mem(struct stm32_i2c_controller *ctl,
                        uint8_t dev_addr,
                        uint8_t reg_addr,
                        const void *data, size_t nbytes);

void stm32_i2c_irq_handler(struct stm32_i2c_controller *ctl);

#endif /* __I2C_STM32H743_H__ */
