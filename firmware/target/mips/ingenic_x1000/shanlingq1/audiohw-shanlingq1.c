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
#include "x1000/aic.h"
#include "x1000/cpm.h"

/* Codec has an dedicated oscillator connected, so it can operate
 * as i2s master or slave. I can't distinguish any difference in
 * terms of audio quality or power consumption. Code is left here
 * for reference in case it proves useful to change it. */
#define CODEC_MASTER_MODE 0

static int cur_fsel = HW_FREQ_48;
static int cur_vol_l = 0, cur_vol_r = 0;
static int cur_filter = 0;
static enum es9218_amp_mode cur_amp_mode = ES9218_AMP_MODE_1VRMS;

static void codec_start(void)
{
    es9218_open();
    es9218_mute(true);
    es9218_set_iface_role(CODEC_MASTER_MODE ? ES9218_IFACE_ROLE_MASTER
                                            : ES9218_IFACE_ROLE_SLAVE);
    es9218_set_iface_format(ES9218_IFACE_FORMAT_I2S, ES9218_IFACE_BITS_32);
    es9218_set_dpll_bandwidth(10);
    es9218_set_thd_compensation(true);
    es9218_set_thd_coeffs(0, 0);
    audiohw_set_filter_roll_off(cur_filter);
    audiohw_set_frequency(cur_fsel);
    audiohw_set_volume(cur_vol_l, cur_vol_r);
    es9218_set_amp_mode(cur_amp_mode);
}

static void codec_stop(void)
{
    es9218_mute(true);
    es9218_close();
    mdelay(4);
}

void audiohw_init(void)
{
    /* Configure AIC */
    aic_set_external_codec(true);
    aic_set_i2s_mode(CODEC_MASTER_MODE ? AIC_I2S_SLAVE_MODE
                                       : AIC_I2S_MASTER_MODE);
    aic_enable_i2s_bit_clock(true);

    /* Open DAC driver */
    i2c_x1000_set_freq(1, I2C_FREQ_400K);
    codec_start();
}

void audiohw_postinit(void)
{
    es9218_mute(false);
}

void audiohw_close(void)
{
    codec_stop();
}

void audiohw_set_frequency(int fsel)
{
    int sampr = hw_freq_sampr[fsel];

    /* choose clock gear setting, in line with the OF */
    enum es9218_clock_gear clkgear;
    if(sampr <= 48000)
        clkgear = ES9218_CLK_GEAR_4;
    else if(sampr <= 96000)
        clkgear = ES9218_CLK_GEAR_2;
    else
        clkgear = ES9218_CLK_GEAR_1;

    aic_enable_i2s_bit_clock(false);
    es9218_set_clock_gear(clkgear);

    if(CODEC_MASTER_MODE)
        es9218_set_nco_frequency(sampr);
    else
        aic_set_i2s_clock(X1000_CLK_SCLK_A, sampr, 64);

    aic_enable_i2s_bit_clock(true);

    /* save frequency selection */
    cur_fsel = fsel;
}

static int round_step_up(int x, int step)
{
    int rem = x % step;
    if(rem > 0)
        rem -= step;
    return x - rem;
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    /* save volume */
    cur_vol_l = vol_l;
    cur_vol_r = vol_r;

    /* adjust the amp setting first */
    int amp = round_step_up(MAX(vol_l, vol_r), ES9218_AMP_VOLUME_STEP);
    amp = MIN(amp, ES9218_AMP_VOLUME_MAX);
    amp = MAX(amp, ES9218_AMP_VOLUME_MIN);

    /* adjust digital volumes */
    vol_l -= amp;
    vol_l = MIN(vol_l, ES9218_DIG_VOLUME_MAX);
    vol_l = MAX(vol_l, ES9218_DIG_VOLUME_MIN);

    vol_r -= amp;
    vol_r = MIN(vol_r, ES9218_DIG_VOLUME_MAX);
    vol_r = MAX(vol_r, ES9218_DIG_VOLUME_MIN);

    /* program DAC */
    es9218_set_amp_volume(amp);
    es9218_set_dig_volume(vol_l, vol_r);
}

void audiohw_set_filter_roll_off(int value)
{
    cur_filter = value;
    es9218_set_filter(value);
}

void audiohw_set_power_mode(int mode)
{
    enum es9218_amp_mode new_amp_mode;
    if(mode == SOUND_HIGH_POWER)
        new_amp_mode = ES9218_AMP_MODE_2VRMS;
    else
        new_amp_mode = ES9218_AMP_MODE_1VRMS;

    if(new_amp_mode != cur_amp_mode) {
        codec_stop();
        cur_amp_mode = new_amp_mode;
        codec_start();
        es9218_mute(false);
    }
}

void es9218_set_power_pin(int level)
{
    gpio_set_level(GPIO_ES9218_POWER, level ? 1 : 0);
}

void es9218_set_reset_pin(int level)
{
    gpio_set_level(GPIO_ES9218_RESET, level ? 1 : 0);
}

uint32_t es9218_get_mclk(void)
{
    /* Measured by running the DAC in asynchronous I2S slave mode,
     * and reading back the DPLL number from regs 0x42-0x45 while
     * playing back 44.1 KHz audio.
     *
     * CLK = (44_100 * 2**32) / 0x4b46e5
     *     = 38_393_403.29532737
     *     ~ 38.4 Mhz
     */
    return 38400000;
}
