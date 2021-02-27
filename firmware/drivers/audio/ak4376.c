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

#include "ak4376.h"
#include "audiohw.h"
#include "pcm_sampr.h"
#include "system.h"
#include "kernel.h"

static int ak4376_regs[AK4376_NUM_REGS];

void ak4376_clear_reg_cache(void)
{
    for(int i = 0; i < AK4376_NUM_REGS; ++i)
        ak4376_regs[i] = -1;
}

void ak4376_write(int reg, int value)
{
    if(ak4376_regs[reg] != value) {
        ak4376_reg_write(reg, value);
        ak4376_regs[reg] = value;
    }
}

int ak4376_read(int reg)
{
    if(ak4376_regs[reg] == -1)
        ak4376_regs[reg] = ak4376_reg_read(reg);

    return ak4376_regs[reg];
}

static int dig_vol_to_hw(int vol)
{
    if(vol < AK4376_DIG_VOLUME_MIN) return 0;
    if(vol > AK4376_DIG_VOLUME_MAX) return 31;
    return (vol - AK4376_DIG_VOLUME_MIN) / AK4376_DIG_VOLUME_STEP + 1;
}

static int amp_vol_to_hw(int vol)
{
    if(vol < AK4376_AMP_VOLUME_MIN) return 0;
    if(vol > AK4376_AMP_VOLUME_MAX) return 14;
    return (vol - AK4376_AMP_VOLUME_MIN) / AK4376_AMP_VOLUME_STEP + 1;
}

void ak4376_set_dig_volume(int vol_l, int vol_r)
{
    ak4376_write(AK4376_REG_LCH_VOLUME, dig_vol_to_hw(vol_l) | (1 << 7));
    ak4376_write(AK4376_REG_RCH_VOLUME, dig_vol_to_hw(vol_r));
}

void ak4376_set_amp_volume(int vol)
{
    ak4376_write(AK4376_REG_HP_VOLUME, amp_vol_to_hw(vol));
}

void ak4376_set_mix(int lch, int rch)
{
    ak4376_write(AK4376_REG_MIXER, (lch & 0xf) | ((rch & 0xf) << 4));
}

void ak4376_set_filter_roll_off(int val)
{
    int reg = ak4376_read(AK4376_REG_FILTER);
    reg &= ~0xc0;
    reg |= (val & 3) << 6;
    ak4376_write(AK4376_REG_FILTER, reg);
}
