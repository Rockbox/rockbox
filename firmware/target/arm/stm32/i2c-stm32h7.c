/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 by Aidan MacDonald
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
#include "kernel.h"
#include "panic.h"
#include "regs/stm32h743/rcc.h"
#include "regs/stm32h743/i2c.h"

#define FLAG_WRITE_REG_ADDR     0x01
#define FLAG_WAIT_REG_ADDR      0x02
#define FLAG_READ_DATA          0x04
#define FLAG_WRITE_DATA         0x08

static uint32_t stm32_i2c_get_timingr(const struct stm32_i2c_config *cfg)
{
    /* Kernel clock period and target I2C bus clock period */
    int t_ker = 1000000 / (stm32_clock_get_frequency(cfg->ker_clock) / 1000);
    int t_scl = 1000000 / (cfg->bus_freq_hz / 1000);

    /* Analog filter delays */
    int af_min = 50;
    int af_max = 260;

    /* Digital filter delays */
    int df = 0;

    /* Synchronization delays excluding rise/fall time */
    int sync_min = af_min + df + (2 * t_ker);
    int sync_max = af_max + df + (4 * (t_ker + 1));

    /* Calculate bounds for SCLDEL/SDADEL */
    int min_scldel_ns = cfg->rise_time_max_ns + cfg->t_su_dat_max_ns;
    int min_sdadel_ns = cfg->fall_time_max_ns - sync_min;
    int max_sdadel_ns = cfg->t_vd_dat_max_ns - cfg->rise_time_max_ns - sync_max;

    /* Find prescaler setting which produces the least error */
    uint32_t best_timingr = 0;
    int best_error = INT_MAX;

    for (int presc = 16; presc > 0; --presc)
    {
        /* Prescaler clock period */
        int t_presc_min = presc * t_ker;
        int t_presc_max = presc * (t_ker + 1);
        int t_presc_avg = (t_presc_min + t_presc_max) / 2;

        /* Compute SCLDEL setting */
        int scldel = (min_scldel_ns + t_presc_min - 1) / t_presc_min;
        if (scldel > 16)
            continue;

        /* Compute SDADEL setting */
        int min_sdadel = (min_sdadel_ns + t_presc_min - 1) / t_presc_min;
        int max_sdadel = max_sdadel_ns / t_presc_max;
        int sdadel = (min_sdadel + max_sdadel) / 2;
        if (min_sdadel > 15 || max_sdadel == 0 || min_sdadel > max_sdadel)
            continue;

        /* Find minimum SCLL and SCLH settings */
        int scll_min = (cfg->scl_low_min_ns + t_presc_min - 1) / t_presc_min;
        int sclh_min = (cfg->scl_high_min_ns + t_presc_min - 1) / t_presc_min;
        if (scll_min > 256 || scll_min > 256)
            continue;

        /* Find slack time to add to SCLL/SCLH to meet target clock */
        int t_scl_slack = t_scl;
        t_scl_slack -= sync_min * 2;
        t_scl_slack -= cfg->rise_time_max_ns + cfg->fall_time_max_ns;
        t_scl_slack -= (scll_min + sclh_min) * t_presc_min;

        /* Compute real SCLL and SCLH */
        int scll = scll_min;
        int sclh = sclh_min;

        /* Divide slack between SCLL/SCLH evenly if possible */
        if (t_scl_slack >= 2*t_presc_avg)
        {
            int add_clocks = t_scl_slack / (2 * t_presc_avg);

            scll += add_clocks;
            sclh += add_clocks;
            t_scl_slack -= add_clocks * t_presc_avg;
        }

        /* Add leftover clock to SCLH if would reduce error */
        if (t_scl_slack >= t_presc_avg/2)
            sclh += 1;

        /* Determine SCL clock period from these settings */
        int approx_scl = (scll + sclh) * t_presc_avg + (2 * sync_min);
        approx_scl += cfg->rise_time_max_ns + cfg->fall_time_max_ns;

        /* Update best timing value */
        int err = t_scl - approx_scl;
        if (err < 0)
            err = -err;

        if (err < best_error)
        {
            best_timingr = __reg_orf(I2C_TIMINGR,
                                     PRESC(presc - 1),
                                     SDADEL(sdadel - 0),
                                     SCLDEL(scldel - 1),
                                     SCLL(scll - 1),
                                     SCLH(sclh - 1));
            best_error = err;
        }
    }

    if (best_error == INT_MAX)
        panicf("%s", __func__);

    return best_timingr;
}

static void stm32_i2c_transfer_complete(struct stm32_i2c_controller *ctl, int err)
{
    ctl->error = err;

    reg_writelf(ctl->regs, I2C_CR1, PE(0));
    semaphore_release(&ctl->bus_sem);
}

void stm32_i2c_init(struct stm32_i2c_controller *ctl,
                    const struct stm32_i2c_config *cfg)
{
    mutex_init(&ctl->bus_lock);
    semaphore_init(&ctl->bus_sem, 1, 0);

    ctl->regs = cfg->instance;
    ctl->ker_clock = cfg->ker_clock;

    stm32_clock_enable(ctl->ker_clock);

    reg_writelf(ctl->regs, I2C_CR1,
                ANFOFF(0),
                ERRIE(1),
                TCIE(1),
                STOPIE(1),
                NACKIE(1),
                RXIE(1),
                TXIE(1));
    reg_varl(ctl->regs, I2C_TIMINGR) = stm32_i2c_get_timingr(cfg);

    stm32_clock_disable(ctl->ker_clock);
}

int stm32_i2c_read_mem(struct stm32_i2c_controller *ctl,
                       uint8_t dev_addr,
                       uint8_t reg_addr,
                       void *data, size_t count)
{
    if (count > 255)
        panicf("%s: count>255 unsupported", __func__);

    mutex_lock(&ctl->bus_lock);
    stm32_clock_enable(ctl->ker_clock);

    ctl->xfer_buf = data;
    ctl->xfer_count = count;
    ctl->reg_addr = reg_addr;
    ctl->flags = FLAG_WRITE_REG_ADDR | FLAG_WAIT_REG_ADDR | FLAG_READ_DATA;
    arm_dsb();

    /* Start first 1-byte transfer of the register address */
    reg_writelf(ctl->regs, I2C_CR1, PE(1));
    reg_writelf(ctl->regs, I2C_CR2,
                NBYTES(1),
                RD_WRN(0),
                AUTOEND(0),
                START(1),
                SADD(dev_addr << 1));

    semaphore_wait(&ctl->bus_sem, TIMEOUT_BLOCK);

    stm32_clock_disable(ctl->ker_clock);
    mutex_unlock(&ctl->bus_lock);
    return ctl->error;
}

int stm32_i2c_write_mem(struct stm32_i2c_controller *ctl,
                        uint8_t dev_addr,
                        uint8_t reg_addr,
                        const void *data, size_t count)
{
    if (count > 254)
        panicf("%s: count>254 unsupported", __func__);

    mutex_lock(&ctl->bus_lock);
    stm32_clock_enable(ctl->ker_clock);

    ctl->xfer_buf = (void *)data;
    ctl->xfer_count = count;
    ctl->reg_addr = reg_addr;
    ctl->flags = FLAG_WRITE_REG_ADDR | FLAG_WRITE_DATA;
    arm_dsb();

    /* Start transfer of register address + data */
    reg_writelf(ctl->regs, I2C_CR1, PE(1));
    reg_writelf(ctl->regs, I2C_CR2,
                NBYTES(ctl->xfer_count + 1),
                RD_WRN(0),
                AUTOEND(1),
                START(1),
                SADD(dev_addr << 1));

    semaphore_wait(&ctl->bus_sem, TIMEOUT_BLOCK);

    stm32_clock_disable(ctl->ker_clock);
    mutex_unlock(&ctl->bus_lock);
    return ctl->error;
}

void stm32_i2c_irq_handler(struct stm32_i2c_controller *ctl)
{
    uint32_t isr = reg_varl(ctl->regs, I2C_ISR);

    /* Detect errors */
    if (reg_vreadf(isr, I2C_ISR, ARLO) ||
        reg_vreadf(isr, I2C_ISR, BERR) ||
        (reg_vreadf(isr, I2C_ISR, STOPF) && ctl->xfer_count != 0) ||
        reg_vreadf(isr, I2C_ISR, NACKF))
    {
        stm32_i2c_transfer_complete(ctl, -1);
        return;
    }

    if (ctl->flags & FLAG_WRITE_REG_ADDR)
    {
        /* Write the register address */
        if (reg_vreadf(isr, I2C_ISR, TXIS))
        {
            reg_varl(ctl->regs, I2C_TXDR) = ctl->reg_addr;
            ctl->flags &= ~FLAG_WRITE_REG_ADDR;
        }
    }
    else if (ctl->flags & FLAG_WAIT_REG_ADDR)
    {
        /* Wait for register address write to complete */
        if (reg_vreadf(isr, I2C_ISR, TC))
        {
            ctl->flags &= ~FLAG_WAIT_REG_ADDR;

            if (ctl->flags & FLAG_READ_DATA)
            {
                /* Start read portion of the transfer */
                reg_writelf(ctl->regs, I2C_CR2,
                            NBYTES(ctl->xfer_count),
                            RD_WRN(1),
                            AUTOEND(1),
                            START(1));
            }
            else
            {
                /* Writes shouldn't get here */
                panicf("%s", __func__);
            }
        }
    }
    else if (ctl->flags & FLAG_WRITE_DATA)
    {
        if (ctl->xfer_count > 0 && reg_vreadf(isr, I2C_ISR, TXIS))
        {
            /* Transmit bytes from buffer */
            reg_varl(ctl->regs, I2C_TXDR) = *ctl->xfer_buf++;
            ctl->xfer_count--;
        }
        else if (ctl->xfer_count == 0 && reg_vreadf(isr, I2C_ISR, STOPF))
        {
            /* Transfer complete without error */
            ctl->flags &= ~FLAG_WRITE_DATA;
            stm32_i2c_transfer_complete(ctl, 0);
        }
    }
    else if (ctl->flags & FLAG_READ_DATA)
    {
        if (ctl->xfer_count > 0 && reg_vreadf(isr, I2C_ISR, RXNE))
        {
            /* Read bytes into buffer */
            *ctl->xfer_buf++ = reg_varl(ctl->regs, I2C_RXDR);
            ctl->xfer_count--;
        }
        else if (ctl->xfer_count == 0 && reg_vreadf(isr, I2C_ISR, STOPF))
        {
            /* Transfer complete without error */
            ctl->flags &= ~FLAG_READ_DATA;
            stm32_i2c_transfer_complete(ctl, 0);
        }
    }
}
