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
#include "logf.h"
#include "aic-x1000.h"
#include "i2c-x1000.h"
#include "gpio-x1000.h"

void audiohw_init(void)
{
    /* Configure AIC */
    aic_set_external_codec(true);
    aic_set_i2s_mode(AIC_I2S_MASTER_MODE);
    aic_enable_i2s_master_clock(true);

    /* Initialize DAC */
    i2c_x1000_set_freq(AK4376_BUS, I2C_FREQ_400K);
    ak4376_init();
}

void audiohw_postinit(void)
{
}

void audiohw_close(void)
{
    ak4376_close();
}

void ak4376_set_pdn_pin(int level)
{
    gpio_config(GPIO_A, 1 << 16, GPIO_OUTPUT(level ? 1 : 0));
}

int ak4376_set_mclk_freq(int hw_freq, bool enabled)
{
    int freq = hw_freq_sampr[hw_freq];
    int mult = freq >= SAMPR_176 ? 128 : 256;

    if(enabled) {
        if(aic_set_i2s_clock(X1000_CLK_SCLK_A, freq, mult)) {
            logf("WARNING: unachievable audio rate %d x %d!?", freq, mult);
        }
    }

    aic_enable_i2s_bit_clock(enabled);
    return mult;
}
