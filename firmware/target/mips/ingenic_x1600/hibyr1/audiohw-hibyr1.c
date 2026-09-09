/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#include "audiohw.h"
#include "cs43131.h"
#include "system.h"
#include "kernel.h"
#include "pcm_sampr.h"
#include "generic_i2c.h"
#include "aic-x1600.h"
#include "clk-x1600.h"
#include "gpio-x1600.h"

/* X1600 AIC to CS43131 over I2S. The SoC supplies MCLK, 64fs BCLK and
 * LRCLK; the DAC is a slave. */

/* Use software I2C on PB30/PB31 because I2C0 is assigned to the PMU bus.
 * Match the vendor 5 us delay. SDA is driven during writes and released
 * for ACKs and reads. */
#define CODEC_I2C_DELAY_US 5

static void codec_i2c_scl_dir(bool out)
{
    gpio_set_function(GPIO_CODEC_I2C_SCL,
                      out ? GPIOF_OUTPUT(1) : GPIOF_INPUT);
}

static void codec_i2c_sda_dir(bool out)
{
    gpio_set_function(GPIO_CODEC_I2C_SDA,
                      out ? GPIOF_OUTPUT(1) : GPIOF_INPUT);
}

static void codec_i2c_scl_out(bool high)
{
    gpio_set_level(GPIO_CODEC_I2C_SCL, high ? 1 : 0);
}

static void codec_i2c_sda_out(bool high)
{
    gpio_set_level(GPIO_CODEC_I2C_SDA, high ? 1 : 0);
}

static bool codec_i2c_scl_in(void)
{
    return gpio_get_level(GPIO_CODEC_I2C_SCL) ? true : false;
}

static bool codec_i2c_sda_in(void)
{
    return gpio_get_level(GPIO_CODEC_I2C_SDA) ? true : false;
}

static void codec_i2c_delay(int d)
{
    udelay(d);
}

static const struct i2c_interface codec_i2c_iface = {
    .scl_dir        = codec_i2c_scl_dir,
    .sda_dir        = codec_i2c_sda_dir,
    .scl_out        = codec_i2c_scl_out,
    .sda_out        = codec_i2c_sda_out,
    .scl_in         = codec_i2c_scl_in,
    .sda_in         = codec_i2c_sda_in,
    .delay          = codec_i2c_delay,

    .delay_hd_sta   = CODEC_I2C_DELAY_US,
    .delay_hd_dat   = CODEC_I2C_DELAY_US,
    .delay_su_dat   = CODEC_I2C_DELAY_US,
    .delay_su_sto   = CODEC_I2C_DELAY_US,
    .delay_su_sta   = CODEC_I2C_DELAY_US,
    .delay_thigh    = CODEC_I2C_DELAY_US,
};

/* audiohw interface */

static int cur_fsel = HW_FREQ_44;
static int cur_vol_l = -400, cur_vol_r = -400;

void audiohw_init(void)
{
    /* AIC: I2S, SoC is master for BCLK and LRCLK */
    aic_set_i2s_mode(AIC_I2S_MASTER_MODE);
    aic_enable_i2s_bit_clock(true);

    /* Bring up the software I2C bus and hand the DAC driver its index */
    int bus = i2c_add_node(&codec_i2c_iface);
    cs43131_init(bus);

    cs43131_set_volume(cur_vol_l, cur_vol_r);
    audiohw_set_frequency(cur_fsel);
}

void audiohw_postinit(void)
{
    cs43131_mute(false);
}

void audiohw_close(void)
{
    cs43131_close();
}

void audiohw_set_frequency(int fsel)
{
    int sampr = hw_freq_sampr[fsel];

    /* The DAC must not use MCLK while its source is being reprogrammed. */
    cs43131_close();
    aic_enable_i2s_bit_clock(false);
    if(aic_set_i2s_clock(X1600_CLK_EPLL, sampr, AIC_I2S_MCLK_MULT) < 0)
        return;
    aic_enable_i2s_bit_clock(true);

    cs43131_set_frequency(sampr);
    cs43131_open();

    cur_fsel = fsel;
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    cur_vol_l = vol_l;
    cur_vol_r = vol_r;
    cs43131_set_volume(vol_l, vol_r);
}

/* Target-local mute control. */
void audiohw_mute(bool mute)
{
    cs43131_mute(mute);
}

/* CS43131 target hooks */

void cs43131_set_power_pin(int level)
{
    /* PB02, active high (vendor cs43131_pwr_en_level = 1) */
    gpio_set_level(GPIO_CS43131_POWER, level ? 1 : 0);
}

void cs43131_set_reset_pin(int level)
{
    /* PB21, active high (vendor cs43131_rst_en_level = 1) */
    gpio_set_level(GPIO_CS43131_RESET, level ? 1 : 0);
}

/* Anti-pop pre-roll is not implemented. */
