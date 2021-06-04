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

#include "audiohw.h"
#include "system.h"
#include "pcm_sampr.h"
#include "aic-x1000.h"
#include "i2c-x1000.h"
#include "gpio-x1000.h"
#include "logf.h"

static int cur_fsel = HW_FREQ_48;
static int cur_power_mode = 0;

static void set_ak_freqmode(void)
{
    int freq = hw_freq_sampr[cur_fsel];
    int mult = freq >= SAMPR_176 ? 128 : 256;

    aic_enable_i2s_bit_clock(false);
    aic_set_i2s_clock(X1000_CLK_SCLK_A, freq, mult);
    ak4376_set_freqmode(cur_fsel, mult, cur_power_mode);
    aic_enable_i2s_bit_clock(true);
}

void audiohw_init(void)
{
    /* Configure AIC */
    aic_set_external_codec(true);
    aic_set_i2s_mode(AIC_I2S_MASTER_MODE);
    aic_enable_i2s_master_clock(true);

    /* Initialize DAC */
    i2c_x1000_set_freq(AK4376_BUS, I2C_FREQ_400K);
    ak4376_open();
    set_ak_freqmode();
}

void audiohw_postinit(void)
{
}

void audiohw_close(void)
{
    ak4376_close();
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    ak4376_set_volume(vol_l, vol_r);
}

void audiohw_set_filter_roll_off(int val)
{
    ak4376_set_filter_roll_off(val);
}

void audiohw_set_frequency(int fsel)
{
    cur_fsel = fsel;
    set_ak_freqmode();
}

void audiohw_set_power_mode(int mode)
{
    cur_power_mode = mode;
    set_ak_freqmode();
}

void ak4376_set_pdn_pin(int level)
{
    gpio_set_level(GPIO_AK4376_POWER, level ? 1 : 0);
}
