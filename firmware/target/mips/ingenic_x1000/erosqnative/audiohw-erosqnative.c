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

#include "audiohw.h"
#include "system.h"
#include "pcm_sampr.h"
#include "aic-x1000.h"
#include "i2c-x1000.h"
#include "gpio-x1000.h"
#include "logf.h"

/* Audio path appears to be:
 * DAC --> HP Amp --> Stereo Switch --> HP OUT
 *     \--> LO OUT
 *
 * The real purpose of the Stereo Switch is not clear.
 * It appears to switch sources between the HP amp and something,
 * likely something unimplemented. */

void audiohw_init(void)
{
    /* explicitly mute everything */
    gpio_set_level(GPIO_MAX97220_SHDN, 0);
    gpio_set_level(GPIO_ISL54405_MUTE, 1);
    gpio_set_level(GPIO_PCM5102A_XMIT, 0);

    aic_set_play_last_sample(true);
    aic_set_external_codec(true);
    aic_set_i2s_mode(AIC_I2S_MASTER_MODE);
    audiohw_set_frequency(HW_FREQ_48);

    aic_enable_i2s_master_clock(true);
    aic_enable_i2s_bit_clock(true);

    mdelay(10);

    /* power on DAC and HP Amp */
    gpio_set_level(GPIO_PCM5102A_ANALOG_PWR, 1);
    gpio_set_level(GPIO_MAX97220_POWER, 1);
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
    gpio_set_level(GPIO_ISL54405_SEL, 0);
    gpio_set_level(GPIO_MAX97220_SHDN, 1);
    mdelay(10);
    gpio_set_level(GPIO_PCM5102A_XMIT, 1);
    mdelay(10);
    gpio_set_level(GPIO_ISL54405_MUTE, 0);
}

/* TODO: get shutdown just right according to dac datasheet */
void audiohw_close(void)
{
    /* mute - attempt to make power-off pop-free */
    gpio_set_level(GPIO_ISL54405_MUTE, 1);
    mdelay(10);
    gpio_set_level(GPIO_PCM5102A_XMIT, 0);
    mdelay(10);
    gpio_set_level(GPIO_MAX97220_SHDN, 0);
}

void audiohw_set_frequency(int fsel)
{
    int sampr = hw_freq_sampr[fsel];
    int mult = 256;

    aic_enable_i2s_bit_clock(false);
    aic_set_i2s_clock(X1000_CLK_SCLK_A, sampr, mult);
    aic_enable_i2s_bit_clock(true);
}

