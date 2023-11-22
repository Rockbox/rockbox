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
#include "settings.h"
#include "pcm_sw_volume.h"

#include "gpio-x1000.h"

static long int vol_l_hw = PCM5102A_VOLUME_MIN;
static long int vol_r_hw = PCM5102A_VOLUME_MIN;
int es9018k2m_present_flag = 0;

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
    if (select == 0)
    {
        gpio_set_level(GPIO_STEREOSW_SEL, 0);
    }
    else
    {
        gpio_set_level(GPIO_STEREOSW_SEL, 1);
    }
}