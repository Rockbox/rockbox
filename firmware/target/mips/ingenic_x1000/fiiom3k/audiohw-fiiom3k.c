/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021-2022 Aidan MacDonald
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
static int cur_vol_r = AK4376_MIN_VOLUME;
static int cur_vol_l = AK4376_MIN_VOLUME;
static int cur_recvol = 0;
static int cur_filter_roll_off = 0;
static int cur_fsel = HW_FREQ_48;
static int cur_power_mode = SOUND_HIGH_POWER;

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

    /* switch to new source */
    cur_audio_source = source;

    if(source == AUDIO_SRC_PLAYBACK) {
        /* power on DAC */
        aic_set_external_codec(true);
        aic_set_i2s_mode(AIC_I2S_MASTER_MODE);
        ak4376_open();

        /* apply the old settings */
        audiohw_set_volume(cur_vol_l, cur_vol_r);
        audiohw_set_filter_roll_off(cur_filter_roll_off);
        set_ak_freqmode();
    } else if(source == AUDIO_SRC_MIC) {
        aic_set_external_codec(false);
        aic_set_i2s_mode(AIC_I2S_SLAVE_MODE);
        /* Note: Sampling frequency is irrelevant here */
        aic_set_i2s_clock(X1000_CLK_EXCLK, 48000, 0);
        aic_enable_i2s_bit_clock(true);

        x1000_icodec_open();

        /* configure the mic */
        x1000_icodec_mic1_enable(true);
        x1000_icodec_mic1_bias_enable(true);
        x1000_icodec_mic1_configure(JZCODEC_MIC1_DIFFERENTIAL |
                                    JZCODEC_MIC1_BIAS_2_08V);

        /* configure the ADC */
        x1000_icodec_adc_mic_sel(JZCODEC_MIC_SEL_ANALOG);
        x1000_icodec_adc_highpass_filter(true);
        x1000_icodec_adc_frequency(cur_fsel);
        x1000_icodec_adc_enable(true);

        /* configure the mixer to put mic input in both channels */
        x1000_icodec_mixer_enable(true);
        x1000_icodec_write(JZCODEC_MIX2, 0x10);

        /* set gain and unmute */
        audiohw_set_recvol(cur_recvol, 0, AUDIO_GAIN_MIC);
        x1000_icodec_adc_mute(false);
    } else {
        logf("bad audio input source: %d (flags: %x)", source, flags);
    }
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
    else
        x1000_icodec_adc_frequency(fsel);
}

void audiohw_set_power_mode(int mode)
{
    cur_power_mode = mode;

    if(cur_audio_source == AUDIO_SRC_PLAYBACK)
        set_ak_freqmode();
}

static int round_step_up(int x, int step)
{
    int rem = x % step;
    if(rem > 0)
        rem -= step;
    return x - rem;
}

void audiohw_set_recvol(int left, int right, int type)
{
    (void)right;

    if(type == AUDIO_GAIN_MIC) {
        cur_recvol = left;

        if(cur_audio_source == AUDIO_SRC_MIC) {
            int mic_gain = round_step_up(left, X1000_ICODEC_MIC_GAIN_STEP);
            mic_gain = MIN(mic_gain, X1000_ICODEC_MIC_GAIN_MAX);
            mic_gain = MAX(mic_gain, X1000_ICODEC_MIC_GAIN_MIN);

            int adc_gain = left - mic_gain;
            adc_gain = MIN(adc_gain, X1000_ICODEC_ADC_GAIN_MAX);
            adc_gain = MAX(adc_gain, X1000_ICODEC_ADC_GAIN_MIN);

            x1000_icodec_adc_gain(adc_gain);
            x1000_icodec_mic1_gain(mic_gain);
        }
    }
}

void ak4376_set_pdn_pin(int level)
{
    gpio_set_level(GPIO_AK4376_POWER, level ? 1 : 0);
}
