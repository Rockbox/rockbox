/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald, Dana Conrad
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
#include "audiohw.h"
#include "pcm_sw_volume.h"
#include "pcm_sampr.h"
#include "i2c-target.h"
#include "button.h"

// #define LOGF_ENABLE
#include "logf.h"

#include "aic-x1000.h"
#include "i2c-x1000.h"
#include "gpio-x1000.h"

/*
 * Earlier devices audio path appears to be:
 * DAC \--> HP Amp --> Stereo Switch --> HP OUT
 *      \-> LO OUT
 *
 * Recent devices, the audio path seems to have changed to:
 * DAC --> HP Amp --> Stereo Switch \--> HP OUT
 *                                   \-> LO OUT
 */

void audiohw_init(void)
{
    /* explicitly mute everything */
    gpio_set_level(GPIO_HPAMP_SHDN, 0);
    gpio_set_level(GPIO_STEREOSW_MUTE, 1);
    gpio_set_level(GPIO_DAC_PWR, 0);

    aic_set_play_last_sample(true);
    aic_set_external_codec(true);
    aic_set_i2s_mode(AIC_I2S_MASTER_MODE);
    audiohw_set_frequency(HW_FREQ_48);

    aic_enable_i2s_master_clock(true);
    aic_enable_i2s_bit_clock(true);

    mdelay(10);

    /* power on DAC and HP Amp */
    gpio_set_level(GPIO_DAC_ANALOG_PWR, 1);
    gpio_set_level(GPIO_HPAMP_POWER, 1);
}

void audiohw_postinit(void)
{
    /*
     * enable playback, fill FIFO buffer with -1 to prevent
     * the DAC from auto-muting, wait, and then stop playback.
     * This seems to completely prevent power-on or first-track
     * clicking.
     */
    jz_writef(AIC_CCR, ERPL(1));
    for (int i = 0; i < 32; i++)
    {
        jz_write(AIC_DR, 0xFFFFFF);
    }
    /* Wait until all samples are through the FIFO. */
    while(jz_readf(AIC_SR, TFL) != 0);
    mdelay(20); /* This seems to silence the power-on click */
    jz_writef(AIC_CCR, ERPL(0));

    /* unmute - attempt to make power-on pop-free */
    gpio_set_level(GPIO_STEREOSW_SEL, 0);
    gpio_set_level(GPIO_HPAMP_SHDN, 1);
    mdelay(10);
    gpio_set_level(GPIO_DAC_PWR, 1);
    mdelay(10);
    gpio_set_level(GPIO_STEREOSW_MUTE, 0);

    i2c_x1000_set_freq(ES9018K2M_BUS, I2C_FREQ_400K);

    int ret = es9018k2m_read_reg(ES9018K2M_REG0_SYSTEM_SETTINGS);
    if (ret >= 0) /* Detected ES9018K2M DAC */
    {
        logf("ES9018K2M found: ret=%d", ret);
        es9018k2m_present_flag = 1;

       /* Default is 32-bit data, and it works ok. Enabling the following
        * causes issue. Which is weird, I definitely thought AIC was configured
        * for 24-bit data... */
        // es9018k2m_write_reg(ES9018K2M_REG1_INPUT_CONFIG, 0b01001100); // 24-bit data

    } else { /* Default to SWVOL for PCM5102A DAC */
        logf("Default to SWVOL: ret=%d", ret);
    }
}

void audiohw_close(void)
{
    /* mute - attempt to make power-off pop-free */
    gpio_set_level(GPIO_STEREOSW_MUTE, 1);
    mdelay(10);
    gpio_set_level(GPIO_DAC_PWR, 0);
    mdelay(10);
    gpio_set_level(GPIO_HPAMP_SHDN, 0);
}

void audiohw_set_frequency(int fsel)
{
    int sampr = hw_freq_sampr[fsel];
    int mult = 256;

    aic_enable_i2s_bit_clock(false);
    aic_set_i2s_clock(X1000_CLK_SCLK_A, sampr, mult);
    aic_enable_i2s_bit_clock(true);
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    int l, r;

    eros_qn_set_last_vol(vol_l, vol_r);

    l = vol_l;
    r = vol_r;

#if (defined(HAVE_HEADPHONE_DETECTION) && defined(HAVE_LINEOUT_DETECTION))
    /* make sure headphones aren't present - don't want to
     * blow out our eardrums cranking it to full */
    if (lineout_inserted() && !headphones_inserted())
    {
        eros_qn_switch_output(1);

        l = r = eros_qn_get_volume_limit();
    }
    else
    {
        eros_qn_switch_output(0);
    }
#endif

    if (es9018k2m_present_flag) /* ES9018K2M */
    {
        /* Same volume range and mute point for both DACs, so use PCM5102A_VOLUME_MIN */
        l = l <= PCM5102A_VOLUME_MIN ? PCM_MUTE_LEVEL : l;
        r = r <= PCM5102A_VOLUME_MIN ? PCM_MUTE_LEVEL : r;

        /* set software volume just below unity due to
         * DAC offset. We don't want to overflow the PCM system. */
        pcm_set_master_volume(-1, -1);
        es9018k2m_set_volume(l, r);
    }
    else /* PCM5102A */
    {
        l = l <= PCM5102A_VOLUME_MIN ? PCM_MUTE_LEVEL : (l / 20);
        r = r <= PCM5102A_VOLUME_MIN ? PCM_MUTE_LEVEL : (r / 20);

        pcm_set_master_volume(l, r);
    }
}