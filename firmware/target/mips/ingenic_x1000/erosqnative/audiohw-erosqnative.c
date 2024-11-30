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
#include "devicedata.h"

/*
 * Earlier devices audio path appears to be:
 * DAC \--> HP Amp --> Stereo Switch --> HP OUT
 *      \-> LO OUT
 *
 * Recent devices, the audio path seems to have changed to:
 * DAC --> HP Amp --> Stereo Switch \--> HP OUT
 *                                   \-> LO OUT
 */
#if !defined(BOOTLOADER)
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

    // devices hw2 and newer use es9018k2m i2c dac
    // hw1 devices use swvol
    // special case hw2 devices with bootloaders identifying as hw1 (pre-version field)
    if (device_data.hw_rev >= 2 || \
        (device_data.version == 0xff && eros_qn_discover_dac(true)))
    {
#if defined(LOGF_ENABLE)
        if (device_data.version == 0xff) {
            logf("OLD BOOTLOADER FOUND, UPDATE IT!");
        }
#endif
        es9018k2m_present_flag = true;

       /* Default is 32-bit data, and it works ok. Enabling the following
        * causes issue. Which is weird, I definitely thought AIC was configured
        * for 24-bit data... */
        // es9018k2m_write_reg(ES9018K2M_REG1_INPUT_CONFIG, 0b01001100); // 24-bit data

       /* Datasheet: Sets the number os FSR edges that must occur before    *
        * the DPLL and ASRC can lock on to the the incoming Signal.         *
        * When Samplerates >= 96khz could be used, STOP_DIV should be set   *
        * to 0 (= 16384 FSR Edges).                                         *
        *   Reg #10 [3:0] (0x05 default, 2730 FSR Edges)                    */
        es9018k2m_write_reg(ES9018K2M_REG10_MASTER_MODE_CTRL, 0x00);

       /* Datasheet: The ES90x8Q2M/K2M contains a Jitter Eliminator block,   *
        * which employs the use of a digital phase locked loop (DPLL) to     *
        * lock to the incoming audio clock rate. When in I2S or SPDIF mode,  *
        * the DPLL will lock to the frame clock (1 x fs). However, when in   *
        * DSD mode, the DPLL has no frame clock information, and must in-    *
        * stead lock to the bit clock rate (BCK). For this reason, there are *
        * two bandwidth settings for the DPLL.                               *
           Reg #12 [7:4] (0x05 default) bandwidth for I2S / SPDIF mode.
           Reg #12 [3:0] (0x0A default) bandwidth for DSD mode.
        * The DPLL bandwidth sets how quickly the DPLL can adjust its intern *
        * representation of the audio clock. The higher the jitter or        *
        * frequency drift on the audio clock, the higher the bandwidth must  *
        * be so that the DPLL can react.                                     *
        * ! If the bandwidth is “too low”, the DPLL will loose lock and you  *
        * ! will hear random dropouts. (Fixed my SurfansF20 v3.2 dropouts)   */
        es9018k2m_write_reg(ES9018K2M_REG12_DPLL_SETTINGS, 0xda);

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
    /* Due to the hardware's detection method, make the Line-Out
     * the default. The LO can only be detected if it is active
     * (assuming a high-impedance device is attached). HP takes priority
     * if both are present. */
    if (headphones_inserted())
    {
        eros_qn_switch_output(0);
    }
    else
    {
        eros_qn_switch_output(1);

        l = r = eros_qn_get_volume_limit();
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
        es9018k2m_set_volume_async(l, r);
    }
    else /* PCM5102A */
    {
        l = l <= PCM5102A_VOLUME_MIN ? PCM_MUTE_LEVEL : (l / 20);
        r = r <= PCM5102A_VOLUME_MIN ? PCM_MUTE_LEVEL : (r / 20);

        pcm_set_master_volume(l, r);
    }
}

void audiohw_set_filter_roll_off(int value)
{
    if (es9018k2m_present_flag)
    {
        es9018k2m_set_filter_roll_off(value);
    }
}
#endif /* !defined(BOOTLOADER) */