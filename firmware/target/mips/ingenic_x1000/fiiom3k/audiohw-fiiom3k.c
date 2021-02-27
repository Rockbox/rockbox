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
#include "x1000/aic.h"
#include "x1000/cpm.h"

void audiohw_init(void)
{
    /* Configure AIC for I2S operation */
    jz_writef(CPM_CLKGR, AIC(0));
    gpio_config(GPIO_B, 0x1f, GPIO_DEVICE(1));
    jz_writef(AIC_I2SCR, STPBK(1));

    /* Operate as I2S master, use external codec */
    jz_writef(AIC_CFG, AUSEL(1), ICDC(0), BCKD(1), SYNCD(1), LSMP(1));
    jz_writef(AIC_I2SCR, ESCLK(1), AMSL(0));

    /* Stereo audio, packed 16 bit samples */
    jz_writef(AIC_CCR, PACK16(1), CHANNEL(1), OSS(1));

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
    /* Get the multiplier */
    int freq = hw_freq_sampr[hw_freq];
    int mult = freq >= SAMPR_176 ? 128 : 256;

    if(enabled) {
        /* Set the new frequency; clock is enabled afterward */
        if(aic_i2s_set_mclk(X1000_CLK_SCLK_A, freq, mult))
            logf("WARNING: unachievable audio rate %d x %d!?", freq, mult);
    } else {
        /* Shut off the clock */
        jz_writef(AIC_I2SCR, STPBK(1));
    }

    return mult;
}
