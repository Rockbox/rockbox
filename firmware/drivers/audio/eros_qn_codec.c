/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Andrew Ryabinin, Dana Conrad
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
#include "eros_qn_codec.h"
#include "config.h"
#include "audio.h"
#include "audiohw.h"
#ifndef BOOTLOADER
#include "settings.h"
#endif
#include "pcm_sw_volume.h"

// #define LOGF_ENABLE
#include "logf.h"

#include "gpio-x1000.h"
#include "i2c-x1000.h"

int es9018k2m_present_flag = 0;

#ifndef BOOTLOADER
static long int vol_l_hw = PCM5102A_VOLUME_MIN;
static long int vol_r_hw = PCM5102A_VOLUME_MIN;
void eros_qn_set_outputs(void)
{
    audiohw_set_volume(vol_l_hw, vol_r_hw);
}

void eros_qn_set_last_vol(long int vol_l, long int vol_r)
{
    vol_l_hw = vol_l;
    vol_r_hw = vol_r;
}

int eros_qn_get_volume_limit(void)
{
    return (global_settings.volume_limit * 10);
}

void eros_qn_switch_output(int select)
{
    /* normal operation 0 */
    if (select == 0)
    {
        gpio_set_level(GPIO_STEREOSW_SEL, 0);
    }
    /* normal operation 1 */
    else
    {
        gpio_set_level(GPIO_STEREOSW_SEL, 1);
    }
}
#endif /* !defined(BOOTLOADER) */

bool eros_qn_discover_dac(bool pwr_after_discovery)
{
    i2c_x1000_set_freq(ES9018K2M_BUS, I2C_FREQ_400K);
    gpio_set_level(GPIO_DAC_PWR, 1);
    gpio_set_level(GPIO_DAC_ANALOG_PWR, 1);
    mdelay(10);
    int ret = es9018k2m_read_reg(ES9018K2M_REG0_SYSTEM_SETTINGS);
    if (ret == 0)
    {
        es9018k2m_present_flag = 1;
        logf("ES9018K2M found! ret=%d", ret);
    }
    /* other options will go here if need be */
    else
    {
        es9018k2m_present_flag = 0;
        logf("Default to SWVOL: ret=%d", ret);
    }

    if (!pwr_after_discovery)
    {
        gpio_set_level(GPIO_DAC_PWR, 0);
        gpio_set_level(GPIO_DAC_ANALOG_PWR, 0);
    }

    return es9018k2m_present_flag;
}
