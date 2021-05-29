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
#include "audio.h"
#include "system.h"
#include "pcm_sampr.h"
#include "aic-x1000.h"
#include "i2c-x1000.h"
#include "gpio-x1000.h"
#include "logf.h"

static int cur_audio_source = AUDIO_SRC_PLAYBACK;
static int cur_vol_r = INT_MIN;
static int cur_vol_l = INT_MIN;
static int cur_filter_roll_off = INT_MIN;
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
    if(cur_audio_source == AUDIO_SRC_PLAYBACK)
        ak4376_close();
    else if(cur_audio_source == AUDIO_SRC_MIC)
        x1000_icodec_close();
}

void audio_set_output_source(int source)
{
    /* this is a no-op */
    (void)source;
}

void audio_input_mux(int source, unsigned flags)
{
    (void)flags;
    if(source == cur_audio_source)
        return;

    /* close whatever codec is currently in use */
    audiohw_close();
    aic_enable_i2s_bit_clock(false);

    if(source == AUDIO_SRC_PLAYBACK) {
        /* power on DAC */
        aic_set_external_codec(true);
        aic_set_i2s_mode(AIC_I2S_MASTER_MODE);
        ak4376_open();

        /* apply the old settings */
        if(cur_vol_l != INT_MIN && cur_vol_r != INT_MIN)
            audiohw_set_volume(cur_vol_l, cur_vol_r);
        if(cur_filter_roll_off != INT_MIN)
            audiohw_set_filter_roll_off(cur_filter_roll_off);
        if(cur_fsel != INT_MIN && cur_power_mode != INT_MIN)
            set_ak_freqmode();

    } else if(source == AUDIO_SRC_MIC) {
        aic_set_external_codec(false);
        aic_set_i2s_mode(AIC_I2S_SLAVE_MODE);
        aic_set_i2s_clock(X1000_CLK_EXCLK, 12000000, 0);
        x1000_icodec_open();
        if(cur_fsel != INT_MIN)
            x1000_icodec_set_adc_frequency(cur_fsel);
        x1000_icodec_set_adc_highpass_filter(true);

        /* TODO: do icodec init */

    } else {
        logf("bad audio input source: %d (flags: %x)", source, flags);
    }

    cur_audio_source = source;
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    cur_vol_l = vol_l;
    cur_vol_r = vol_r;

    if(cur_audio_source == AUDIO_SRC_PLAYBACK)
        ak4376_set_volume(vol_l, vol_r);
}

void audiohw_set_filter_roll_off(int val)
{
    cur_filter_roll_off = val;

    if(cur_audio_source == AUDIO_SRC_PLAYBACK)
        ak4376_set_filter_roll_off(val);
}

void audiohw_set_frequency(int fsel)
{
    cur_fsel = fsel;

    if(cur_audio_source == AUDIO_SRC_PLAYBACK)
        set_ak_freqmode();
    else {
        /* TODO: handle x1000 icodec frequency */
    }
}

void audiohw_set_power_mode(int mode)
{
    cur_power_mode = mode;

    if(cur_audio_source == AUDIO_SRC_PLAYBACK)
        set_ak_freqmode();
}

void audiohw_set_recvol(int left, int right, int type)
{
    /* TODO: implement me */
    (void)left;
    (void)right;
    (void)type;
}

void ak4376_set_pdn_pin(int level)
{
    gpio_config(GPIO_A, 1 << 16, GPIO_OUTPUT(level ? 1 : 0));
}
