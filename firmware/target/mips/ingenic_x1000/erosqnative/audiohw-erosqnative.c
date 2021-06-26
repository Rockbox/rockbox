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
#include "pcm_sw_volume.h"

// static int cur_fsel = HW_FREQ_48;

void audiohw_init(void)
{
    aic_set_external_codec(true);
    aic_set_i2s_mode(AIC_I2S_MASTER_MODE);
    audiohw_set_frequency(HW_FREQ_44);

    aic_enable_i2s_master_clock(true);
    aic_enable_i2s_bit_clock(true);
    
    gpio_set_level(GPIO_PCM5102A_XMIT, 1);
    gpio_set_level(GPIO_MAX97220_SHDN, 1);
    gpio_set_level(GPIO_ISL54405_MUTE, 0);
    gpio_set_level(GPIO_ISL54405_SEL, 0);
}

void audiohw_postinit(void)
{

}

void audiohw_close(void)
{

}

void audiohw_set_volume(int vol_l, int vol_r)
{
    pcm_set_master_volume(vol_l, vol_r);
}

void audiohw_set_frequency(int fsel)
{
    int sampr = hw_freq_sampr[fsel];
    int mult = sampr >= SAMPR_176 ? 128 : 256;

    aic_enable_i2s_bit_clock(false);
    aic_set_i2s_clock(X1000_CLK_SCLK_A, sampr, mult);
    aic_enable_i2s_bit_clock(true);

    
}

