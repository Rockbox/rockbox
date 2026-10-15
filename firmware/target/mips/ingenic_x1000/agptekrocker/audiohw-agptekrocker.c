/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Marcin Bukat
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
#include "system.h"
#include "pcm_sampr.h"
#include "aic-x1000.h"
#include "i2c-ingenic.h"
#include "gpio-ingenic.h"
#include "axp-pmu.h"
#include "logf.h"

/* X1000 AIC is I2S master and drives MCLK/BCLK/LRCLK, the CS42L51 runs
 * as slave. Codec supply is AXP192 LDO2, the headphone amp has its own
 * supply switch (PB6). HP and line outputs have mute GPIOs (high = muted).
 *
 * TODO: the OF sets I2SCR.RFIRST (right channel first), check if the
 *       channels are swapped on this board.
 */

#define AXP192_LDO2_EN  0x04

static int cur_fsel = HW_FREQ_48;
static int cur_vol_l = 0, cur_vol_r = 0;

static void codec_power(bool on)
{
    if(on) {
        i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                        AXP_REG_PWROUTPUTCTRL2, 0, AXP192_LDO2_EN, NULL);
        gpio_set_level(GPIO_AMP_PWR, 1);
        mdelay(10);
        gpio_set_level(GPIO_CS42L51_RESET, 1);
        mdelay(10);
    } else {
        gpio_set_level(GPIO_CS42L51_RESET, 0);
        gpio_set_level(GPIO_AMP_PWR, 0);
        i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                        AXP_REG_PWROUTPUTCTRL2, AXP192_LDO2_EN, 0, NULL);
    }
}

void audiohw_init(void)
{
    /* Configure AIC */
    aic_set_play_last_sample(true);
    aic_set_external_codec(true);
    aic_set_i2s_mode(AIC_I2S_MASTER_MODE);
    aic_set_i2s_clock(X1000_CLK_SCLK_A, hw_freq_sampr[cur_fsel], 256);
    aic_enable_i2s_master_clock(true);
    aic_enable_i2s_bit_clock(true);

    /* Power up and configure the codec */
    i2c_x1000_set_freq(CS42L51_BUS, I2C_FREQ_400K);
    codec_power(true);
    if(cs42l51_open())
        logf("audiohw: codec not found");

    cs42l51_set_frequency(hw_freq_sampr[cur_fsel]);
    cs42l51_set_volume(cur_vol_l, cur_vol_r);
}

void audiohw_postinit(void)
{
    cs42l51_set_mute(false);

    /* The OF releases both mutes when playback starts (k1_snd_trigger),
     * so the headphone path probably goes through the line mute too. */
    gpio_set_level(GPIO_LINE_MUTE, 0);
    gpio_set_level(GPIO_HP_MUTE, 0);
}

void audiohw_close(void)
{
    gpio_set_level(GPIO_HP_MUTE, 1);
    gpio_set_level(GPIO_LINE_MUTE, 1);
    cs42l51_close();
    codec_power(false);
}

void audiohw_set_frequency(int fsel)
{
    int sampr = hw_freq_sampr[fsel];

    /* MCLK = 256 fs */
    aic_enable_i2s_bit_clock(false);
    aic_set_i2s_clock(X1000_CLK_SCLK_A, sampr, 256);
    aic_enable_i2s_bit_clock(true);

    cs42l51_set_frequency(sampr);
    cur_fsel = fsel;
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    cur_vol_l = vol_l;
    cur_vol_r = vol_r;
    cs42l51_set_volume(vol_l, vol_r);
}
